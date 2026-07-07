"""
Asset Manifest Manager

Tracks all generated assets, their metadata, generation parameters,
and file locations. Used to avoid regenerating existing assets and
to maintain consistency across the project.
"""

import os
import json
import hashlib
from datetime import datetime
from typing import Optional
from dataclasses import dataclass, field, asdict


@dataclass
class AssetEntry:
    """Single entry in the asset manifest."""
    asset_id: str
    category: str                # unit_sprite, building_sprite, terrain_tile, etc.
    file_path: str
    prompt_hash: str             # Hash of the prompt used to generate
    prompt_text: str
    api_provider: str
    generation_id: str = ""
    width: int = 0
    height: int = 0
    created_at: str = ""
    faction: str = "neutral"
    variants: list = field(default_factory=list)
    sprite_sheet_path: str = ""
    metadata: dict = field(default_factory=dict)

    def __post_init__(self):
        if not self.created_at:
            self.created_at = datetime.utcnow().isoformat()


class AssetManifest:
    """Manages the complete asset manifest for the project."""

    def __init__(self, manifest_path: str = "assets/manifest.json"):
        self.manifest_path = manifest_path
        self.entries: dict[str, AssetEntry] = {}
        self.load()

    def load(self) -> None:
        """Load manifest from disk."""
        if os.path.exists(self.manifest_path):
            with open(self.manifest_path, 'r') as f:
                data = json.load(f)
            for entry_data in data.get("assets", []):
                entry = AssetEntry(**entry_data)
                self.entries[entry.asset_id] = entry

    def save(self) -> None:
        """Save manifest to disk."""
        os.makedirs(os.path.dirname(self.manifest_path), exist_ok=True)
        data = {
            "version": "1.0",
            "updated_at": datetime.utcnow().isoformat(),
            "total_assets": len(self.entries),
            "assets": [asdict(e) for e in self.entries.values()]
        }
        with open(self.manifest_path, 'w') as f:
            json.dump(data, f, indent=2)

    def add(self, entry: AssetEntry) -> None:
        """Add or update an asset entry."""
        self.entries[entry.asset_id] = entry
        self.save()

    def get(self, asset_id: str) -> Optional[AssetEntry]:
        """Get an asset entry by ID."""
        return self.entries.get(asset_id)

    def exists(self, asset_id: str) -> bool:
        """Check if an asset has been generated."""
        entry = self.entries.get(asset_id)
        if not entry:
            return False
        return os.path.exists(entry.file_path)

    def needs_regeneration(self, asset_id: str, prompt: str) -> bool:
        """Check if an asset needs to be regenerated (prompt changed)."""
        entry = self.entries.get(asset_id)
        if not entry:
            return True
        if not os.path.exists(entry.file_path):
            return True
        new_hash = self._hash_prompt(prompt)
        return entry.prompt_hash != new_hash

    def get_by_category(self, category: str) -> list[AssetEntry]:
        """Get all assets of a specific category."""
        return [e for e in self.entries.values() if e.category == category]

    def get_missing(self, required_ids: list[str]) -> list[str]:
        """Get list of required asset IDs that are missing."""
        return [aid for aid in required_ids if not self.exists(aid)]

    def summary(self) -> dict:
        """Get a summary of manifest contents."""
        categories = {}
        for entry in self.entries.values():
            cat = entry.category
            categories[cat] = categories.get(cat, 0) + 1
        return {
            "total": len(self.entries),
            "by_category": categories,
        }

    @staticmethod
    def _hash_prompt(prompt: str) -> str:
        return hashlib.sha256(prompt.encode()).hexdigest()[:16]
