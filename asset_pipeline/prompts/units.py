"""
AI Prompt Templates for Unit (Vehicle) Sprites

Each prompt is designed to generate a consistent top-down isometric
sprite of a C&C vehicle unit. Prompts include the vehicle description,
visual characteristics, and style guidance.
"""

from .style_base import build_prompt, build_negative


# Unit prompt definitions: unit_id -> (description, faction, extra_style)
_UNIT_DEFS = {
    "unit_mammoth": (
        "Mammoth Tank, massive heavy battle tank with dual cannons and "
        "missile launchers on each side, very large heavily armored vehicle, "
        "thick angular armor plating, twin barrels extending forward",
        "gdi",
        "largest tank on battlefield, imposing silhouette, dust around treads"
    ),
    "unit_medium_tank": (
        "Medium Tank, standard military battle tank with single rotating turret, "
        "medium-sized armored vehicle, one main cannon barrel, "
        "conventional tank design with angled armor",
        "gdi",
        "workhorse tank, balanced proportions"
    ),
    "unit_light_tank": (
        "Light Tank, fast scout tank with small turret, "
        "compact lightly-armored tracked vehicle, single short cannon, "
        "sleek low-profile design",
        "nod",
        "fast and agile appearance, smaller than medium tank"
    ),
    "unit_stealth_tank": (
        "Stealth Tank, futuristic cloaking tank with angular stealth geometry, "
        "missile launchers on top, radar-absorbing angular surfaces, "
        "semi-transparent shimmering effect suggesting invisibility",
        "nod",
        "high-tech appearance, angular faceted armor, slightly translucent"
    ),
    "unit_apc": (
        "Armored Personnel Carrier, boxy troop transport vehicle, "
        "small machinegun turret on top, rear troop door visible, "
        "rugged wheeled/tracked military transport",
        "gdi",
        "transport vehicle, boxy utilitarian design"
    ),
    "unit_mlrs": (
        "Multiple Launch Rocket System, military truck with rocket pod "
        "mounted on back, multiple rocket tubes visible in rack formation, "
        "tracked chassis with elevated launcher",
        "gdi",
        "rocket artillery vehicle, launcher prominently visible"
    ),
    "unit_rocket_launcher": (
        "Mobile Rocket Launcher, tracked vehicle with twin rocket tubes, "
        "missile launcher mounted on tracked chassis, "
        "lighter than MLRS, dual launchers",
        "nod",
        "missile vehicle, angular Nod design"
    ),
    "unit_harvester": (
        "Tiberium Harvester, large industrial mining vehicle, "
        "front-mounted harvesting mechanism with rotating blades, "
        "large storage compartment on back, heavy-duty tracked vehicle, "
        "glowing green Tiberium residue on harvesting mechanism",
        "neutral",
        "industrial vehicle, heavy and bulky, Tiberium green glow"
    ),
    "unit_mcv": (
        "Mobile Construction Vehicle, very large deployment vehicle, "
        "folding construction equipment on top, massive tracked platform, "
        "complex mechanical systems visible, construction crane elements",
        "neutral",
        "largest ground vehicle, transforms into base, complex mechanical detail"
    ),
    "unit_humvee": (
        "Military Humvee, light reconnaissance vehicle with mounted "
        "machine gun turret on roof, four-wheeled military truck, "
        "open top weapon mount, fast scout vehicle",
        "gdi",
        "light fast vehicle, wheeled not tracked"
    ),
    "unit_buggy": (
        "Attack Buggy, lightweight dune buggy with mounted machine gun, "
        "open frame vehicle with roll cage, large off-road tires, "
        "stripped-down fast attack vehicle",
        "nod",
        "light raider vehicle, aggressive stance, minimal armor"
    ),
    "unit_recon_bike": (
        "Recon Bike, military motorcycle with dual rocket launchers, "
        "futuristic combat motorcycle, missile pods on sides, "
        "single rider with sleek profile",
        "nod",
        "fast motorcycle, futuristic design, rocket pods visible"
    ),
    "unit_artillery": (
        "Mobile Artillery, self-propelled artillery piece, "
        "long barrel cannon on tracked chassis, elevated gun barrel, "
        "artillery piece with stabilizing legs deployed",
        "nod",
        "long-range artillery, prominent barrel"
    ),
    "unit_ssm": (
        "Surface-to-Surface Missile Launcher, mobile ballistic missile "
        "vehicle, large single missile on elevating launch rail, "
        "tracked heavy vehicle with tall missile",
        "nod",
        "large missile clearly visible, raised launcher"
    ),
    "unit_flame_tank": (
        "Flame Tank, armored vehicle with dual flamethrower nozzles, "
        "heavily armored tracked vehicle, two flame projector barrels, "
        "heat-distorted air around nozzles, fire-resistant plating",
        "nod",
        "menacing appearance, flame effects near barrels"
    ),
    "unit_gunboat": (
        "Military Gunboat, small patrol boat with turret gun, "
        "naval vessel with mounted cannon, military watercraft, "
        "hull cutting through water, wake behind",
        "gdi",
        "naval vessel, water effects around hull"
    ),
    "unit_hovercraft": (
        "Military Hovercraft, armored landing craft on air cushion, "
        "fan-driven hover vehicle, troop bay visible, "
        "air skirt inflated underneath, large rear fan",
        "gdi",
        "hovering vehicle, air skirt visible"
    ),
}


# Pre-built full prompts
UNIT_PROMPTS = {}
for uid, (desc, faction, extra) in _UNIT_DEFS.items():
    UNIT_PROMPTS[uid] = {
        "prompt": build_prompt(desc, "vehicle", faction, extra),
        "negative": build_negative(),
        "faction": faction,
        "description": desc,
    }


def get_unit_prompt(unit_id: str) -> dict:
    """Get the full prompt data for a unit sprite."""
    return UNIT_PROMPTS.get(unit_id, {
        "prompt": build_prompt(
            f"military vehicle sprite for {unit_id}", "vehicle", "neutral"
        ),
        "negative": build_negative(),
        "faction": "neutral",
        "description": unit_id,
    })
