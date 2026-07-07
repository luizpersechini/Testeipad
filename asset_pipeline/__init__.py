"""
Command & Conquer: Tiberian Dawn - Modern Port
AI-Powered Asset Generation Pipeline

This package provides a complete framework for generating game sprites,
terrain tiles, building artwork, and UI elements using AI image generation APIs.

Supported APIs:
  - Leonardo.ai (recommended for game art - consistent style, good API)
  - OpenAI DALL-E / GPT Image (good prompt understanding)
  - Stability AI (Stable Diffusion - fine control, inpainting)

Usage:
    from asset_pipeline import AssetFactory
    factory = AssetFactory(api_provider="leonardo", api_key="...")
    factory.generate_all_unit_sprites()
    factory.generate_terrain_tileset()
"""

from .config import PipelineConfig
from .asset_factory import AssetFactory

__version__ = "0.1.0"
__all__ = ["AssetFactory", "PipelineConfig"]
