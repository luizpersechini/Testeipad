"""
AI Prompt Templates for Terrain Tiles
"""

from .style_base import build_prompt, build_negative


_TERRAIN_DEFS = {
    "terrain_clear": (
        "flat dry earth ground texture, light brown sandy soil, "
        "sparse small pebbles, arid terrain surface",
        "terrain", "neutral",
        "seamless tileable, uniform ground coverage"
    ),
    "terrain_sand": (
        "desert sand terrain texture, golden tan sand, "
        "wind-swept dune texture, fine grain sand surface",
        "terrain", "neutral",
        "seamless tileable, warm desert tones"
    ),
    "terrain_rock": (
        "rocky terrain texture, gray stone surface, "
        "cracked rock ground, rough stone terrain",
        "terrain", "neutral",
        "seamless tileable, impassable rocky ground"
    ),
    "terrain_water": (
        "water surface texture, dark blue water, "
        "gentle ripple pattern, deep water surface",
        "terrain", "neutral",
        "seamless tileable, animated water look"
    ),
    "terrain_road": (
        "paved road texture, gray asphalt surface, "
        "cracked military road, worn pavement",
        "terrain", "neutral",
        "seamless tileable, road surface"
    ),
    "terrain_tiberium": (
        "Tiberium crystal field, glowing green alien crystals "
        "growing from ground, bioluminescent green mineral deposits, "
        "toxic alien resource, jagged green crystal formations",
        "terrain", "neutral",
        "green glow, alien crystals, hazardous ground"
    ),
    "terrain_rough": (
        "rough broken terrain, uneven ground with debris, "
        "damaged earth surface, cratered rough ground",
        "terrain", "neutral",
        "seamless tileable, difficult terrain"
    ),
    "terrain_cliff": (
        "cliff edge terrain, rocky cliff face viewed from above, "
        "elevation change, steep rocky edge",
        "terrain", "neutral",
        "impassable cliff edge"
    ),
}


TERRAIN_PROMPTS = {}
for tid, (desc, cat, faction, extra) in _TERRAIN_DEFS.items():
    TERRAIN_PROMPTS[tid] = {
        "prompt": build_prompt(desc, cat, faction, extra),
        "negative": build_negative("objects, structures, vehicles, people"),
        "description": desc,
    }


def get_terrain_prompt(terrain_id: str) -> dict:
    return TERRAIN_PROMPTS.get(terrain_id, {
        "prompt": build_prompt(f"terrain tile for {terrain_id}", "terrain", "neutral"),
        "negative": build_negative(),
        "description": terrain_id,
    })
