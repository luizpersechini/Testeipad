"""
Asset Pipeline Configuration

Central configuration for the AI image generation pipeline.
Defines output paths, image specifications, and API settings.
"""

from dataclasses import dataclass, field
from typing import Optional
from enum import Enum
import os
import json


class APIProvider(Enum):
    """Supported AI image generation API providers."""
    LEONARDO = "leonardo"        # leonardo.ai - Recommended for game art
    OPENAI = "openai"            # DALL-E / GPT Image
    STABILITY = "stability"      # Stable Diffusion API
    LOCAL = "local"              # Local Stable Diffusion (ComfyUI/A1111)


class AssetCategory(Enum):
    """Categories of game assets to generate."""
    UNIT_SPRITE = "unit_sprite"
    INFANTRY_SPRITE = "infantry_sprite"
    BUILDING_SPRITE = "building_sprite"
    TERRAIN_TILE = "terrain_tile"
    OVERLAY_TILE = "overlay_tile"
    EFFECT_SPRITE = "effect_sprite"
    UI_ELEMENT = "ui_element"
    PORTRAIT = "portrait"
    ICON = "icon"


@dataclass
class ImageSpec:
    """Specification for a generated image."""
    width: int = 512
    height: int = 512
    format: str = "png"
    transparent_background: bool = True
    num_variants: int = 1
    seed: Optional[int] = None


@dataclass
class SpriteSheetSpec:
    """Specification for a generated sprite sheet."""
    frame_width: int = 64
    frame_height: int = 64
    columns: int = 8
    rows: int = 8
    facings: int = 8             # 8-directional sprites
    frames_per_facing: int = 1
    animation_frames: int = 4    # Frames per animation


# Default image specifications for each asset category
DEFAULT_SPECS = {
    AssetCategory.UNIT_SPRITE: ImageSpec(
        width=512, height=512, transparent_background=True, num_variants=1
    ),
    AssetCategory.INFANTRY_SPRITE: ImageSpec(
        width=384, height=384, transparent_background=True, num_variants=1
    ),
    AssetCategory.BUILDING_SPRITE: ImageSpec(
        width=512, height=512, transparent_background=True, num_variants=1
    ),
    AssetCategory.TERRAIN_TILE: ImageSpec(
        width=256, height=256, transparent_background=False, num_variants=4
    ),
    AssetCategory.OVERLAY_TILE: ImageSpec(
        width=256, height=256, transparent_background=True, num_variants=2
    ),
    AssetCategory.EFFECT_SPRITE: ImageSpec(
        width=256, height=256, transparent_background=True, num_variants=1
    ),
    AssetCategory.UI_ELEMENT: ImageSpec(
        width=512, height=128, transparent_background=True, num_variants=1
    ),
    AssetCategory.PORTRAIT: ImageSpec(
        width=256, height=256, transparent_background=False, num_variants=1
    ),
    AssetCategory.ICON: ImageSpec(
        width=64, height=64, transparent_background=True, num_variants=1
    ),
}


@dataclass
class PipelineConfig:
    """Main configuration for the asset generation pipeline."""

    # API configuration
    api_provider: APIProvider = APIProvider.LEONARDO
    api_key: str = ""
    api_base_url: str = ""
    model_id: str = ""           # Specific model to use

    # Leonardo.ai specific
    leonardo_model_id: str = "e71a1c2f-4f80-4800-934f-2c68979d8cc8"  # Leonardo Phoenix
    leonardo_preset: str = "DYNAMIC"

    # OpenAI specific
    openai_model: str = "gpt-image-1"
    openai_quality: str = "high"

    # Stability AI specific
    stability_engine: str = "stable-diffusion-v3"
    stability_style_preset: str = "pixel-art"

    # Local (ComfyUI) specific
    local_api_url: str = "http://localhost:8188"

    # Output paths
    output_base_path: str = "assets"
    sprite_output_path: str = "assets/sprites"
    tile_output_path: str = "assets/tiles"
    ui_output_path: str = "assets/ui"

    # Generation settings
    batch_size: int = 4          # Number of concurrent generations
    retry_attempts: int = 3
    retry_delay_seconds: float = 2.0
    rate_limit_delay: float = 0.5   # Delay between API calls

    # Style settings
    art_style: str = "pixel art"
    color_palette: str = "military"
    perspective: str = "top-down isometric"
    resolution_scale: int = 2    # Upscale factor

    # Post-processing
    auto_remove_background: bool = True
    auto_create_sprite_sheet: bool = True
    auto_generate_faction_variants: bool = True
    auto_generate_shadow: bool = True

    # Manifest
    manifest_path: str = "assets/manifest.json"

    def load_from_file(self, path: str) -> None:
        """Load configuration from a JSON file."""
        with open(path, 'r') as f:
            data = json.load(f)
        for key, value in data.items():
            if hasattr(self, key):
                if key == 'api_provider':
                    setattr(self, key, APIProvider(value))
                else:
                    setattr(self, key, value)

    def save_to_file(self, path: str) -> None:
        """Save configuration to a JSON file."""
        data = {}
        for key, value in self.__dict__.items():
            if isinstance(value, Enum):
                data[key] = value.value
            else:
                data[key] = value
        with open(path, 'w') as f:
            json.dump(data, f, indent=2)

    def load_api_key_from_env(self) -> None:
        """Load API key from environment variables."""
        env_vars = {
            APIProvider.LEONARDO: "LEONARDO_API_KEY",
            APIProvider.OPENAI: "OPENAI_API_KEY",
            APIProvider.STABILITY: "STABILITY_API_KEY",
        }
        env_var = env_vars.get(self.api_provider)
        if env_var:
            self.api_key = os.environ.get(env_var, self.api_key)

    def ensure_directories(self) -> None:
        """Create output directories if they don't exist."""
        dirs = [
            self.output_base_path,
            self.sprite_output_path,
            os.path.join(self.sprite_output_path, "units"),
            os.path.join(self.sprite_output_path, "buildings"),
            os.path.join(self.sprite_output_path, "infantry"),
            os.path.join(self.sprite_output_path, "effects"),
            self.tile_output_path,
            os.path.join(self.tile_output_path, "terrain"),
            os.path.join(self.tile_output_path, "overlays"),
            self.ui_output_path,
        ]
        for d in dirs:
            os.makedirs(d, exist_ok=True)
