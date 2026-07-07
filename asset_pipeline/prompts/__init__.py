"""
AI Prompt Templates for C&C Asset Generation

This module contains carefully crafted prompt templates for generating
game assets that match the Command & Conquer visual style.

The prompts are designed to produce consistent results across different
AI models by including detailed style instructions and visual references.
"""

from .units import UNIT_PROMPTS, get_unit_prompt
from .buildings import BUILDING_PROMPTS, get_building_prompt
from .terrain import TERRAIN_PROMPTS, get_terrain_prompt
from .effects import EFFECT_PROMPTS, get_effect_prompt
from .infantry import INFANTRY_PROMPTS, get_infantry_prompt

__all__ = [
    "UNIT_PROMPTS", "get_unit_prompt",
    "BUILDING_PROMPTS", "get_building_prompt",
    "TERRAIN_PROMPTS", "get_terrain_prompt",
    "EFFECT_PROMPTS", "get_effect_prompt",
    "INFANTRY_PROMPTS", "get_infantry_prompt",
]
