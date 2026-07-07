"""
AI Prompt Templates for Infantry Sprites
"""

from .style_base import build_prompt, build_negative

_INFANTRY_DEFS = {
    "inf_minigunner": (
        "Minigunner soldier, basic infantry with assault rifle, "
        "military uniform, helmet, small soldier sprite",
        "gdi", "basic foot soldier, standard military gear"
    ),
    "inf_grenadier": (
        "Grenadier soldier, infantry with grenade launcher, "
        "bulkier gear, grenade bandolier visible",
        "gdi", "heavier infantry, grenade equipment"
    ),
    "inf_rocket_soldier": (
        "Rocket Soldier, infantry carrying shoulder-mounted rocket launcher, "
        "anti-vehicle specialist, large launcher weapon",
        "gdi", "rocket launcher clearly visible"
    ),
    "inf_flamethrower": (
        "Flamethrower soldier, infantry with flamethrower weapon, "
        "fuel tank on back, flame nozzle extended, heat-resistant suit",
        "nod", "fire equipment visible, protective suit"
    ),
    "inf_chem_warrior": (
        "Chemical Warrior, soldier in hazmat suit with chem sprayer, "
        "full protective suit, chemical weapon nozzle, toxic green accents",
        "nod", "hazmat suit, green chemical effects"
    ),
    "inf_engineer": (
        "Combat Engineer, soldier with repair tools and equipment, "
        "toolbox and wrench, no weapon, lighter gear, utility belt",
        "neutral", "tools instead of weapons, support unit"
    ),
    "inf_commando": (
        "Commando elite soldier, special forces operative with sniper rifle, "
        "C4 explosives on belt, beret, muscular imposing figure",
        "gdi", "elite soldier, imposing presence, sniper rifle"
    ),
}

INFANTRY_PROMPTS = {}
for iid, (desc, faction, extra) in _INFANTRY_DEFS.items():
    INFANTRY_PROMPTS[iid] = {
        "prompt": build_prompt(desc, "infantry", faction, extra),
        "negative": build_negative(),
        "faction": faction,
        "description": desc,
    }

def get_infantry_prompt(infantry_id: str) -> dict:
    return INFANTRY_PROMPTS.get(infantry_id, {
        "prompt": build_prompt(f"infantry soldier for {infantry_id}", "infantry", "neutral"),
        "negative": build_negative(),
        "faction": "neutral",
        "description": infantry_id,
    })
