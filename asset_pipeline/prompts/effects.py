"""
AI Prompt Templates for Visual Effect Sprites
"""

from .style_base import build_prompt, build_negative

_EFFECT_DEFS = {
    "effect_explosion_small": (
        "small explosion sprite, orange and yellow fireball, "
        "pixel art explosion effect, expanding blast",
        "effect", "neutral", "bright orange center, smoke edges"
    ),
    "effect_explosion_large": (
        "large explosion sprite, massive fireball with shockwave, "
        "pixel art explosion, billowing smoke and fire",
        "effect", "neutral", "intense bright center, debris particles"
    ),
    "effect_muzzle_flash": (
        "gun muzzle flash sprite, bright yellow-white flash, "
        "weapon firing effect, star-shaped flash",
        "effect", "neutral", "very bright, small and intense"
    ),
    "effect_laser_beam": (
        "red laser beam sprite, concentrated energy beam, "
        "Obelisk of Light laser, bright red beam with glow",
        "effect", "neutral", "bright red, energy beam effect"
    ),
    "effect_ion_cannon": (
        "Ion Cannon orbital strike effect, massive blue energy beam "
        "from sky, devastating orbital weapon blast, blue-white column of energy",
        "effect", "neutral", "massive blue beam, ground explosion"
    ),
    "effect_tiberium_glow": (
        "green Tiberium glow effect, bioluminescent green aura, "
        "alien crystal radiation, pulsing green light",
        "effect", "neutral", "green glow, alien radiation"
    ),
}

EFFECT_PROMPTS = {}
for eid, (desc, cat, faction, extra) in _EFFECT_DEFS.items():
    EFFECT_PROMPTS[eid] = {
        "prompt": build_prompt(desc, cat, faction, extra),
        "negative": build_negative(),
        "description": desc,
    }

def get_effect_prompt(effect_id: str) -> dict:
    return EFFECT_PROMPTS.get(effect_id, {
        "prompt": build_prompt(f"visual effect for {effect_id}", "effect", "neutral"),
        "negative": build_negative(),
        "description": effect_id,
    })
