"""
AI Prompt Templates for Building Sprites

Each prompt generates a top-down isometric view of a C&C base building.
Buildings have specific visual characteristics tied to their function
and faction.
"""

from .style_base import build_prompt, build_negative


_BUILDING_DEFS = {
    "bld_construction_yard": (
        "Construction Yard, large military base construction facility, "
        "central command building with construction crane, "
        "deployment pads and scaffolding visible, main base structure, "
        "heavy foundation with vehicle access ramps",
        "neutral",
        "largest base building, 3x3 cell footprint, industrial"
    ),
    "bld_power_plant": (
        "Power Plant, military power generation facility, "
        "cooling towers or smokestacks, electrical equipment visible, "
        "industrial power generation building, power lines connecting",
        "neutral",
        "2x2 cell footprint, industrial energy facility"
    ),
    "bld_adv_power_plant": (
        "Advanced Power Plant, high-tech power generation facility, "
        "larger cooling towers, advanced electrical systems, "
        "glowing energy conduits, more advanced than basic plant",
        "neutral",
        "2x2 cell footprint, advanced technology visible"
    ),
    "bld_refinery": (
        "Tiberium Refinery, mineral processing plant, "
        "conveyor belts and processing equipment, "
        "green Tiberium crystals in hoppers, vehicle docking bay "
        "for harvester, industrial refining towers",
        "neutral",
        "3x2 cell footprint, green Tiberium glow, industrial processing"
    ),
    "bld_silo": (
        "Tiberium Silo, storage container for refined Tiberium, "
        "cylindrical storage tank, green glowing contents visible, "
        "small single-cell storage structure",
        "neutral",
        "1x1 cell footprint, small storage container"
    ),
    "bld_barracks": (
        "Military Barracks, troop training facility, "
        "rectangular building with training yard, military dormitory, "
        "soldiers drilling area visible, flagpole with banner",
        "gdi",
        "2x2 cell footprint, military training facility"
    ),
    "bld_hand_of_nod": (
        "Hand of Nod, Brotherhood of Nod infantry training temple, "
        "distinctive hand-shaped or angular dark building, "
        "red illumination, cult-like military training facility, "
        "sinister architectural design with sharp angles",
        "nod",
        "2x2 cell footprint, dark temple aesthetic"
    ),
    "bld_weapons_factory": (
        "Weapons Factory, vehicle manufacturing plant, "
        "large industrial building with vehicle assembly line, "
        "bay doors for vehicle exit, manufacturing equipment visible, "
        "heavy industrial structure",
        "neutral",
        "3x2 cell footprint, large industrial building, vehicle bay doors"
    ),
    "bld_guard_tower": (
        "Guard Tower, defensive watchtower with machine gun, "
        "tall narrow tower structure, gun emplacement at top, "
        "observation post, small fortified structure",
        "gdi",
        "1x1 cell footprint, tall defensive tower"
    ),
    "bld_adv_guard_tower": (
        "Advanced Guard Tower, upgraded defensive tower with rocket launcher, "
        "reinforced tall tower, missile launcher at top, "
        "advanced targeting systems visible, heavily fortified",
        "gdi",
        "1x2 cell footprint, taller and more fortified"
    ),
    "bld_turret": (
        "Gun Turret, automated defensive turret emplacement, "
        "rotating cannon turret on concrete base, "
        "small fortified gun position, barrel extends outward",
        "nod",
        "1x1 cell footprint, rotating turret visible"
    ),
    "bld_obelisk": (
        "Obelisk of Light, tall crystalline laser defense tower, "
        "tall narrow obelisk structure with red crystal at peak, "
        "energy focusing crystal emitting red glow, "
        "Nod's most powerful defense, alien-looking technology",
        "nod",
        "1x2 cell footprint, tall glowing structure, red energy beam"
    ),
    "bld_sam_site": (
        "SAM Site, anti-aircraft missile emplacement, "
        "quad missile launcher pointing skyward, "
        "concrete bunker base with missile rack, "
        "anti-air defense position",
        "nod",
        "2x1 cell footprint, missiles pointing upward"
    ),
    "bld_temple_of_nod": (
        "Temple of Nod, massive Brotherhood headquarters, "
        "pyramid-like dark temple structure, "
        "red glowing elements, nuclear missile silo hidden within, "
        "imposing religious-military architecture, largest Nod building",
        "nod",
        "3x3 cell footprint, grand imposing structure, dark and red"
    ),
    "bld_comm_center": (
        "Advanced Communications Center, high-tech command facility, "
        "satellite dishes and antenna arrays on roof, "
        "radar dome, communication equipment, Ion Cannon uplink, "
        "advanced military command building",
        "gdi",
        "2x2 cell footprint, satellite dishes and antennas"
    ),
    "bld_airstrip": (
        "Airstrip, aircraft landing strip and hangar, "
        "runway markings visible, aircraft hangar structure, "
        "landing lights, vehicle delivery platform",
        "nod",
        "3x2 cell footprint, runway and hangar"
    ),
    "bld_helipad": (
        "Helipad, helicopter landing pad and maintenance bay, "
        "circular landing pad with H marking, "
        "fuel and ammunition storage, small maintenance structure",
        "gdi",
        "2x2 cell footprint, landing pad marking visible"
    ),
    "bld_repair_bay": (
        "Repair Bay, vehicle repair and maintenance facility, "
        "open bay with repair equipment, hydraulic lifts, "
        "tool stations, vehicle maintenance area",
        "neutral",
        "3x2 cell footprint, open vehicle bay"
    ),
    "bld_concrete_wall": (
        "Concrete Wall segment, heavy defensive wall section, "
        "top-down view of thick concrete barrier, "
        "reinforced military wall, gray concrete",
        "neutral",
        "1x1 cell footprint, wall segment, connects to adjacent walls"
    ),
    "bld_sandbag": (
        "Sandbag Wall, military sandbag barrier, "
        "stacked sandbag fortification viewed from above, "
        "tan sandbag wall segment",
        "neutral",
        "1x1 cell footprint, simple defensive wall"
    ),
}


BUILDING_PROMPTS = {}
for bid, (desc, faction, extra) in _BUILDING_DEFS.items():
    BUILDING_PROMPTS[bid] = {
        "prompt": build_prompt(desc, "building", faction, extra),
        "negative": build_negative(),
        "faction": faction,
        "description": desc,
    }


def get_building_prompt(building_id: str) -> dict:
    """Get the full prompt data for a building sprite."""
    return BUILDING_PROMPTS.get(building_id, {
        "prompt": build_prompt(
            f"military base building for {building_id}", "building", "neutral"
        ),
        "negative": build_negative(),
        "faction": "neutral",
        "description": building_id,
    })
