"""
Asset Factory - Main Orchestrator

The AssetFactory is the primary interface for generating all game assets.
It coordinates the API client, prompt templates, post-processor, and
manifest to provide a complete asset generation pipeline.

Usage:
    from asset_pipeline import AssetFactory, PipelineConfig
    from asset_pipeline.config import APIProvider

    config = PipelineConfig(
        api_provider=APIProvider.LEONARDO,
        api_key="your-api-key-here",
    )
    factory = AssetFactory(config)

    # Generate all assets
    factory.generate_all()

    # Or generate specific categories
    factory.generate_all_unit_sprites()
    factory.generate_all_building_sprites()
    factory.generate_terrain_tileset()
"""

import os
import logging
from typing import Optional

from .config import PipelineConfig, AssetCategory, ImageSpec, DEFAULT_SPECS
from .api_client import create_client, ImageGeneratorBase, GenerationResult
from .post_processor import PostProcessor
from .asset_manifest import AssetManifest, AssetEntry
from .prompts.units import UNIT_PROMPTS, get_unit_prompt
from .prompts.buildings import BUILDING_PROMPTS, get_building_prompt
from .prompts.terrain import TERRAIN_PROMPTS, get_terrain_prompt
from .prompts.effects import EFFECT_PROMPTS, get_effect_prompt
from .prompts.infantry import INFANTRY_PROMPTS, get_infantry_prompt

logger = logging.getLogger(__name__)


class AssetFactory:
    """Main factory for generating all game assets using AI."""

    def __init__(self, config: Optional[PipelineConfig] = None):
        self.config = config or PipelineConfig()
        self.config.load_api_key_from_env()
        self.config.ensure_directories()

        self.client: ImageGeneratorBase = create_client(self.config)
        self.processor = PostProcessor(self.config.output_base_path)
        self.manifest = AssetManifest(self.config.manifest_path)

        self._stats = {"generated": 0, "skipped": 0, "failed": 0}

    # ===========================================================
    # High-level generation methods
    # ===========================================================

    def generate_all(self, force: bool = False) -> dict:
        """Generate all game assets. Returns statistics."""
        self._stats = {"generated": 0, "skipped": 0, "failed": 0}

        logger.info("Starting full asset generation pipeline...")
        self.generate_all_unit_sprites(force=force)
        self.generate_all_infantry_sprites(force=force)
        self.generate_all_building_sprites(force=force)
        self.generate_terrain_tileset(force=force)
        self.generate_all_effects(force=force)

        logger.info(
            f"Generation complete: {self._stats['generated']} generated, "
            f"{self._stats['skipped']} skipped, "
            f"{self._stats['failed']} failed"
        )
        return self._stats

    def generate_all_unit_sprites(self, force: bool = False) -> None:
        """Generate sprites for all vehicle units."""
        logger.info("Generating unit sprites...")
        for unit_id, prompt_data in UNIT_PROMPTS.items():
            self._generate_asset(
                asset_id=unit_id,
                category=AssetCategory.UNIT_SPRITE,
                prompt_data=prompt_data,
                output_subdir="sprites/units",
                force=force,
            )

    def generate_all_infantry_sprites(self, force: bool = False) -> None:
        """Generate sprites for all infantry types."""
        logger.info("Generating infantry sprites...")
        for inf_id, prompt_data in INFANTRY_PROMPTS.items():
            self._generate_asset(
                asset_id=inf_id,
                category=AssetCategory.INFANTRY_SPRITE,
                prompt_data=prompt_data,
                output_subdir="sprites/infantry",
                force=force,
            )

    def generate_all_building_sprites(self, force: bool = False) -> None:
        """Generate sprites for all buildings."""
        logger.info("Generating building sprites...")
        for bld_id, prompt_data in BUILDING_PROMPTS.items():
            self._generate_asset(
                asset_id=bld_id,
                category=AssetCategory.BUILDING_SPRITE,
                prompt_data=prompt_data,
                output_subdir="sprites/buildings",
                force=force,
            )

    def generate_terrain_tileset(self, force: bool = False) -> None:
        """Generate all terrain tiles."""
        logger.info("Generating terrain tiles...")
        for terrain_id, prompt_data in TERRAIN_PROMPTS.items():
            spec = DEFAULT_SPECS[AssetCategory.TERRAIN_TILE]
            self._generate_asset(
                asset_id=terrain_id,
                category=AssetCategory.TERRAIN_TILE,
                prompt_data=prompt_data,
                output_subdir="tiles/terrain",
                force=force,
                spec_override=spec,
            )

    def generate_all_effects(self, force: bool = False) -> None:
        """Generate all visual effect sprites."""
        logger.info("Generating effect sprites...")
        for fx_id, prompt_data in EFFECT_PROMPTS.items():
            self._generate_asset(
                asset_id=fx_id,
                category=AssetCategory.EFFECT_SPRITE,
                prompt_data=prompt_data,
                output_subdir="sprites/effects",
                force=force,
            )

    def generate_single(self, asset_id: str, category: str,
                        prompt: str, negative: str = "",
                        force: bool = False) -> Optional[str]:
        """Generate a single custom asset. Returns file path or None."""
        prompt_data = {
            "prompt": prompt,
            "negative": negative,
            "faction": "neutral",
            "description": asset_id,
        }
        cat = AssetCategory(category) if isinstance(category, str) else category
        return self._generate_asset(
            asset_id=asset_id,
            category=cat,
            prompt_data=prompt_data,
            output_subdir=f"sprites/custom",
            force=force,
        )

    # ===========================================================
    # Core generation pipeline
    # ===========================================================

    def _generate_asset(self, asset_id: str, category: AssetCategory,
                        prompt_data: dict, output_subdir: str,
                        force: bool = False,
                        spec_override: Optional[ImageSpec] = None) -> Optional[str]:
        """Core method: generate a single asset through the full pipeline."""

        prompt_text = prompt_data["prompt"]
        negative_text = prompt_data.get("negative", "")
        faction = prompt_data.get("faction", "neutral")

        # Check if already generated
        if not force and not self.manifest.needs_regeneration(asset_id, prompt_text):
            logger.debug(f"Skipping {asset_id} (already generated)")
            self._stats["skipped"] += 1
            return self.manifest.get(asset_id).file_path

        logger.info(f"Generating: {asset_id}")

        # Get image spec
        spec = spec_override or DEFAULT_SPECS.get(category, ImageSpec())

        # Call AI API
        result = self._generate_with_retry(prompt_text, negative_text, spec)
        if not result.success:
            logger.error(f"Failed to generate {asset_id}: {result.error_message}")
            self._stats["failed"] += 1
            return None

        # Download image if URL-based result
        image_data = result.image_data
        if not image_data and result.image_url:
            try:
                image_data = self.client.download_image(result.image_url)
            except Exception as e:
                logger.error(f"Failed to download {asset_id}: {e}")
                self._stats["failed"] += 1
                return None

        if not image_data:
            logger.error(f"No image data for {asset_id}")
            self._stats["failed"] += 1
            return None

        # Post-processing pipeline
        image_data = self._post_process(image_data, category, faction)

        # Save the image
        filename = f"{asset_id}.png"
        relative_path = os.path.join(output_subdir, filename)
        full_path = self.processor.save_image(image_data, relative_path)

        # Generate faction variants if applicable
        variants = []
        if (self.config.auto_generate_faction_variants and
                faction != "neutral" and
                category in (AssetCategory.UNIT_SPRITE,
                             AssetCategory.INFANTRY_SPRITE)):
            variants = self._generate_faction_variants(
                image_data, asset_id, faction, output_subdir
            )

        # Update manifest
        entry = AssetEntry(
            asset_id=asset_id,
            category=category.value,
            file_path=full_path,
            prompt_hash=AssetManifest._hash_prompt(prompt_text),
            prompt_text=prompt_text,
            api_provider=self.config.api_provider.value,
            generation_id=result.generation_id or "",
            width=spec.width,
            height=spec.height,
            faction=faction,
            variants=variants,
        )
        self.manifest.add(entry)

        self._stats["generated"] += 1
        logger.info(f"Generated: {asset_id} -> {full_path}")
        return full_path

    def _generate_with_retry(self, prompt: str, negative: str,
                              spec: ImageSpec) -> GenerationResult:
        """Generate with retry logic."""
        import time

        for attempt in range(self.config.retry_attempts):
            result = self.client.generate(prompt, negative, spec)
            if result.success:
                return result

            if attempt < self.config.retry_attempts - 1:
                delay = self.config.retry_delay_seconds * (2 ** attempt)
                logger.warning(
                    f"Attempt {attempt + 1} failed, retrying in {delay}s: "
                    f"{result.error_message}"
                )
                time.sleep(delay)

        return result

    def _post_process(self, image_data: bytes, category: AssetCategory,
                       faction: str) -> bytes:
        """Apply post-processing pipeline to generated image."""

        # Background removal for sprites
        if (self.config.auto_remove_background and
                category != AssetCategory.TERRAIN_TILE):
            image_data = self.processor.remove_background(image_data)

        # Shadow generation
        if self.config.auto_generate_shadow and category in (
                AssetCategory.UNIT_SPRITE, AssetCategory.BUILDING_SPRITE):
            image_data = self.processor.generate_shadow(image_data)

        # Scale up if needed
        if self.config.resolution_scale > 1:
            image_data = self.processor.scale_image(
                image_data, self.config.resolution_scale
            )

        # Validate tileability for terrain
        if category == AssetCategory.TERRAIN_TILE:
            if not self.processor.validate_tileable(image_data):
                logger.warning("Terrain tile may not be seamlessly tileable")

        return image_data

    def _generate_faction_variants(self, image_data: bytes,
                                    asset_id: str, source_faction: str,
                                    output_subdir: str) -> list[str]:
        """Generate faction color variants of a sprite."""
        variants = []
        target_factions = ["gdi", "nod"]

        for target in target_factions:
            if target == source_faction:
                continue

            variant_data = self.processor.generate_faction_variant(
                image_data, source_faction, target
            )
            variant_id = f"{asset_id}_{target}"
            variant_filename = f"{variant_id}.png"
            variant_path = os.path.join(output_subdir, variant_filename)
            full_path = self.processor.save_image(variant_data, variant_path)
            variants.append(full_path)
            logger.debug(f"Generated faction variant: {variant_id}")

        return variants

    # ===========================================================
    # Utility methods
    # ===========================================================

    def get_status(self) -> dict:
        """Get current pipeline status and manifest summary."""
        return {
            "config": {
                "provider": self.config.api_provider.value,
                "art_style": self.config.art_style,
            },
            "manifest": self.manifest.summary(),
            "last_run": self._stats,
        }

    def list_required_assets(self) -> dict:
        """List all assets that need to be generated."""
        required = {
            "units": list(UNIT_PROMPTS.keys()),
            "infantry": list(INFANTRY_PROMPTS.keys()),
            "buildings": list(BUILDING_PROMPTS.keys()),
            "terrain": list(TERRAIN_PROMPTS.keys()),
            "effects": list(EFFECT_PROMPTS.keys()),
        }
        total = sum(len(v) for v in required.values())
        existing = sum(1 for ids in required.values()
                       for aid in ids if self.manifest.exists(aid))
        return {
            "required": required,
            "total_required": total,
            "total_existing": existing,
            "total_missing": total - existing,
        }
