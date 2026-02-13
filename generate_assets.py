#!/usr/bin/env python3
"""
C&C Asset Generation Runner

Connects to Leonardo.ai API and generates all game assets.
Run from the project root directory.
"""

import os
import sys
import time
import json
import logging
import requests

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

# Leonardo API config
API_KEY = os.environ.get("LEONARDO_API_KEY", "")
API_BASE = "https://cloud.leonardo.ai/api/rest/v1"
HEADERS = {
    "Authorization": f"Bearer {API_KEY}",
    "Content-Type": "application/json",
    "Accept": "application/json",
}

# Output base
OUTPUT_BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "assets")


def verify_api_key():
    """Verify the API key works by checking user info."""
    logger.info("Verifying Leonardo API key...")
    try:
        resp = requests.get(f"{API_BASE}/me", headers=HEADERS, timeout=15)
        if resp.status_code == 200:
            data = resp.json()
            user = data.get("user_details", [{}])
            if isinstance(user, list) and user:
                user = user[0]
            username = user.get("username", "unknown")
            tokens = user.get("apiConcurrencySlots", "?")
            logger.info(f"API key valid. User: {username}, Slots: {tokens}")
            return True
        else:
            logger.error(f"API key verification failed: {resp.status_code} {resp.text[:200]}")
            return False
    except Exception as e:
        logger.error(f"API connection error: {e}")
        return False


def generate_image(prompt, negative_prompt, width=512, height=512,
                   transparent=True, model_id=None):
    """Submit a generation request to Leonardo.ai."""
    payload = {
        "prompt": prompt,
        "negative_prompt": negative_prompt,
        "modelId": model_id or "e71a1c2f-4f80-4800-934f-2c68979d8cc8",  # Leonardo Phoenix
        "width": width,
        "height": height,
        "num_images": 1,
        "presetStyle": "DYNAMIC",
        "alchemy": True,
        "highResolution": False,
    }

    if transparent:
        payload["transparency"] = "foreground_only"

    try:
        resp = requests.post(
            f"{API_BASE}/generations",
            headers=HEADERS,
            json=payload,
            timeout=30,
        )
        resp.raise_for_status()
        data = resp.json()
        gen_id = data.get("sdGenerationJob", {}).get("generationId")
        if gen_id:
            logger.info(f"  Generation submitted: {gen_id}")
            return gen_id
        else:
            logger.error(f"  No generation ID: {json.dumps(data)[:300]}")
            return None
    except requests.exceptions.HTTPError as e:
        logger.error(f"  HTTP error: {e.response.status_code} - {e.response.text[:300]}")
        return None
    except Exception as e:
        logger.error(f"  Request error: {e}")
        return None


def wait_for_generation(gen_id, max_wait=120):
    """Poll until generation completes. Returns list of image URLs."""
    poll_interval = 3
    elapsed = 0

    while elapsed < max_wait:
        time.sleep(poll_interval)
        elapsed += poll_interval

        try:
            resp = requests.get(
                f"{API_BASE}/generations/{gen_id}",
                headers=HEADERS,
                timeout=15,
            )
            resp.raise_for_status()
            data = resp.json()
            gen = data.get("generations_by_pk", {})
            status = gen.get("status")

            if status == "COMPLETE":
                images = gen.get("generated_images", [])
                urls = [img.get("url") for img in images if img.get("url")]
                logger.info(f"  Complete! {len(urls)} image(s)")
                return urls

            elif status == "FAILED":
                logger.error(f"  Generation failed on server")
                return []

            # Still processing
            logger.debug(f"  Status: {status} ({elapsed}s)")

        except Exception as e:
            logger.warning(f"  Poll error: {e}")

    logger.error(f"  Timed out after {max_wait}s")
    return []


def download_image(url, output_path):
    """Download an image from URL to local file."""
    try:
        resp = requests.get(url, timeout=30)
        resp.raise_for_status()
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        with open(output_path, 'wb') as f:
            f.write(resp.content)
        logger.info(f"  Saved: {output_path}")
        return True
    except Exception as e:
        logger.error(f"  Download failed: {e}")
        return False


# ===================================================================
# Asset definitions - prompt, category, output path
# ===================================================================

STYLE_BASE = (
    "top-down isometric pixel art, military RTS game style, "
    "Command and Conquer Tiberian Dawn inspired, retro game sprite, "
    "clean pixel art with defined edges, "
    "muted military color palette, slight shadow underneath, "
    "centered on canvas, single object, transparent background"
)

NEGATIVE_BASE = (
    "blurry, low quality, text, watermark, signature, label, "
    "realistic photo, 3d render, gradient background, noisy, "
    "jpeg artifacts, cropped, out of frame, multiple objects, "
    "modern UI, cartoon, anime, chibi, busy background"
)


def make_prompt(desc, faction_style=""):
    parts = [desc, STYLE_BASE]
    if faction_style:
        parts.append(faction_style)
    return ", ".join(parts)


GDI_STYLE = "golden-tan and olive green military colors, NATO-inspired"
NOD_STYLE = "red and black color scheme, Brotherhood of Nod, angular militant"

# All assets to generate
ASSETS = [
    # === UNITS ===
    {
        "id": "unit_mammoth",
        "prompt": make_prompt(
            "Mammoth Tank, massive heavy battle tank with dual cannons and "
            "missile launchers, very large heavily armored vehicle, thick angular armor",
            GDI_STYLE
        ),
        "output": "sprites/units/unit_mammoth.png",
        "size": (512, 512),
    },
    {
        "id": "unit_medium_tank",
        "prompt": make_prompt(
            "Medium Tank, standard military battle tank with single rotating turret "
            "and one main cannon, medium-sized armored tracked vehicle",
            GDI_STYLE
        ),
        "output": "sprites/units/unit_medium_tank.png",
        "size": (512, 512),
    },
    {
        "id": "unit_light_tank",
        "prompt": make_prompt(
            "Light Tank, fast scout tank with small turret, compact lightly-armored "
            "tracked vehicle, single short cannon, sleek low-profile",
            NOD_STYLE
        ),
        "output": "sprites/units/unit_light_tank.png",
        "size": (512, 512),
    },
    {
        "id": "unit_humvee",
        "prompt": make_prompt(
            "Military Humvee, light reconnaissance vehicle with mounted machine gun "
            "turret on roof, four-wheeled military truck",
            GDI_STYLE
        ),
        "output": "sprites/units/unit_humvee.png",
        "size": (512, 512),
    },
    {
        "id": "unit_apc",
        "prompt": make_prompt(
            "Armored Personnel Carrier APC, boxy troop transport tracked vehicle, "
            "small machinegun turret, rear troop door visible",
            GDI_STYLE
        ),
        "output": "sprites/units/unit_apc.png",
        "size": (512, 512),
    },
    {
        "id": "unit_harvester",
        "prompt": make_prompt(
            "Tiberium Harvester, large industrial mining vehicle, front-mounted "
            "harvesting mechanism with rotating blades, large storage compartment, "
            "heavy tracked vehicle, glowing green Tiberium residue",
            ""
        ),
        "output": "sprites/units/unit_harvester.png",
        "size": (512, 512),
    },
    {
        "id": "unit_mcv",
        "prompt": make_prompt(
            "Mobile Construction Vehicle MCV, very large deployment vehicle, "
            "folding construction equipment, massive tracked platform, "
            "complex mechanical systems, largest ground vehicle",
            ""
        ),
        "output": "sprites/units/unit_mcv.png",
        "size": (512, 512),
    },
    {
        "id": "unit_buggy",
        "prompt": make_prompt(
            "Attack Buggy, lightweight dune buggy with mounted machine gun, "
            "open frame vehicle with roll cage, large off-road tires",
            NOD_STYLE
        ),
        "output": "sprites/units/unit_buggy.png",
        "size": (512, 512),
    },
    {
        "id": "unit_stealth_tank",
        "prompt": make_prompt(
            "Stealth Tank, futuristic cloaking tank with angular stealth geometry, "
            "missile launchers, radar-absorbing angular surfaces, high-tech",
            NOD_STYLE
        ),
        "output": "sprites/units/unit_stealth_tank.png",
        "size": (512, 512),
    },
    {
        "id": "unit_flame_tank",
        "prompt": make_prompt(
            "Flame Tank, armored vehicle with dual flamethrower nozzles, "
            "heavily armored tracked vehicle, fire-resistant plating, menacing",
            NOD_STYLE
        ),
        "output": "sprites/units/unit_flame_tank.png",
        "size": (512, 512),
    },
    {
        "id": "unit_artillery",
        "prompt": make_prompt(
            "Mobile Artillery, self-propelled artillery piece, long barrel cannon "
            "on tracked chassis, elevated gun barrel",
            NOD_STYLE
        ),
        "output": "sprites/units/unit_artillery.png",
        "size": (512, 512),
    },
    {
        "id": "unit_mlrs",
        "prompt": make_prompt(
            "Multiple Launch Rocket System MLRS, military vehicle with rocket pod "
            "mounted on back, multiple rocket tubes in rack",
            GDI_STYLE
        ),
        "output": "sprites/units/unit_mlrs.png",
        "size": (512, 512),
    },
    {
        "id": "unit_recon_bike",
        "prompt": make_prompt(
            "Recon Bike, military motorcycle with dual rocket launchers, "
            "futuristic combat motorcycle, missile pods on sides",
            NOD_STYLE
        ),
        "output": "sprites/units/unit_recon_bike.png",
        "size": (512, 512),
    },

    # === INFANTRY ===
    {
        "id": "inf_minigunner",
        "prompt": make_prompt(
            "Minigunner soldier, basic infantry with assault rifle, "
            "military uniform, helmet, small soldier pixel art sprite",
            GDI_STYLE
        ),
        "output": "sprites/infantry/inf_minigunner.png",
        "size": (384, 384),
    },
    {
        "id": "inf_grenadier",
        "prompt": make_prompt(
            "Grenadier soldier, infantry with grenade launcher, "
            "bulkier gear, grenade bandolier visible, military soldier sprite",
            GDI_STYLE
        ),
        "output": "sprites/infantry/inf_grenadier.png",
        "size": (384, 384),
    },
    {
        "id": "inf_rocket_soldier",
        "prompt": make_prompt(
            "Rocket Soldier, infantry carrying shoulder-mounted rocket launcher, "
            "anti-vehicle specialist, large launcher weapon clearly visible",
            GDI_STYLE
        ),
        "output": "sprites/infantry/inf_rocket_soldier.png",
        "size": (384, 384),
    },
    {
        "id": "inf_flamethrower",
        "prompt": make_prompt(
            "Flamethrower soldier, infantry with flamethrower weapon, "
            "fuel tank on back, flame nozzle, heat-resistant suit",
            NOD_STYLE
        ),
        "output": "sprites/infantry/inf_flamethrower.png",
        "size": (384, 384),
    },
    {
        "id": "inf_engineer",
        "prompt": make_prompt(
            "Combat Engineer soldier, repair tools and equipment, "
            "toolbox and wrench, no weapon, utility belt, support unit",
            ""
        ),
        "output": "sprites/infantry/inf_engineer.png",
        "size": (384, 384),
    },
    {
        "id": "inf_commando",
        "prompt": make_prompt(
            "Commando elite soldier, special forces with sniper rifle, "
            "C4 explosives on belt, beret, imposing muscular figure",
            GDI_STYLE
        ),
        "output": "sprites/infantry/inf_commando.png",
        "size": (384, 384),
    },

    # === BUILDINGS ===
    {
        "id": "bld_construction_yard",
        "prompt": make_prompt(
            "Construction Yard, large military base command facility with "
            "construction crane, deployment pads, heavy foundation, main base building",
            ""
        ),
        "output": "sprites/buildings/bld_construction_yard.png",
        "size": (512, 512),
    },
    {
        "id": "bld_power_plant",
        "prompt": make_prompt(
            "Military Power Plant, power generation facility with "
            "cooling towers and smokestacks, industrial electrical equipment",
            ""
        ),
        "output": "sprites/buildings/bld_power_plant.png",
        "size": (512, 512),
    },
    {
        "id": "bld_refinery",
        "prompt": make_prompt(
            "Tiberium Refinery, mineral processing plant with conveyor belts, "
            "green Tiberium crystals in hoppers, vehicle docking bay, "
            "industrial refining towers",
            ""
        ),
        "output": "sprites/buildings/bld_refinery.png",
        "size": (512, 512),
    },
    {
        "id": "bld_barracks",
        "prompt": make_prompt(
            "Military Barracks, troop training facility with training yard, "
            "rectangular military dormitory building, flagpole",
            GDI_STYLE
        ),
        "output": "sprites/buildings/bld_barracks.png",
        "size": (512, 512),
    },
    {
        "id": "bld_weapons_factory",
        "prompt": make_prompt(
            "Weapons Factory, vehicle manufacturing plant, large industrial "
            "building with vehicle assembly line, bay doors for vehicles",
            ""
        ),
        "output": "sprites/buildings/bld_weapons_factory.png",
        "size": (512, 512),
    },
    {
        "id": "bld_hand_of_nod",
        "prompt": make_prompt(
            "Hand of Nod, Brotherhood of Nod infantry training temple, "
            "distinctive angular dark building, red illumination, sinister design",
            NOD_STYLE
        ),
        "output": "sprites/buildings/bld_hand_of_nod.png",
        "size": (512, 512),
    },
    {
        "id": "bld_obelisk",
        "prompt": make_prompt(
            "Obelisk of Light, tall crystalline laser defense tower, "
            "narrow obelisk with red crystal at peak, energy focusing crystal, "
            "red glow, alien technology",
            NOD_STYLE
        ),
        "output": "sprites/buildings/bld_obelisk.png",
        "size": (512, 512),
    },
    {
        "id": "bld_guard_tower",
        "prompt": make_prompt(
            "Guard Tower, defensive watchtower with machine gun, "
            "tall narrow tower, gun emplacement at top, fortified",
            GDI_STYLE
        ),
        "output": "sprites/buildings/bld_guard_tower.png",
        "size": (512, 512),
    },
    {
        "id": "bld_temple_of_nod",
        "prompt": make_prompt(
            "Temple of Nod, massive pyramid-like dark temple, "
            "red glowing elements, imposing religious-military architecture, "
            "largest Brotherhood building, nuclear missile silo within",
            NOD_STYLE
        ),
        "output": "sprites/buildings/bld_temple_of_nod.png",
        "size": (512, 512),
    },
    {
        "id": "bld_comm_center",
        "prompt": make_prompt(
            "Advanced Communications Center, high-tech command with "
            "satellite dishes and antenna arrays, radar dome, Ion Cannon uplink",
            GDI_STYLE
        ),
        "output": "sprites/buildings/bld_comm_center.png",
        "size": (512, 512),
    },
    {
        "id": "bld_airstrip",
        "prompt": make_prompt(
            "Airstrip, aircraft landing strip and hangar, "
            "runway markings, aircraft hangar structure, landing lights",
            NOD_STYLE
        ),
        "output": "sprites/buildings/bld_airstrip.png",
        "size": (512, 512),
    },
    {
        "id": "bld_helipad",
        "prompt": make_prompt(
            "Helipad, helicopter landing pad with H marking, "
            "fuel storage, small maintenance structure, circular pad",
            GDI_STYLE
        ),
        "output": "sprites/buildings/bld_helipad.png",
        "size": (512, 512),
    },

    # === TERRAIN ===
    {
        "id": "terrain_clear",
        "prompt": (
            "flat dry earth ground texture, light brown sandy soil, "
            "sparse small pebbles, arid terrain surface, "
            "seamless tileable texture, top-down view, pixel art style, "
            "uniform ground coverage, no objects or structures"
        ),
        "output": "tiles/terrain/terrain_clear.png",
        "size": (256, 256),
        "transparent": False,
    },
    {
        "id": "terrain_sand",
        "prompt": (
            "desert sand terrain texture, golden tan sand, "
            "wind-swept dune texture, fine grain sand surface, "
            "seamless tileable, top-down pixel art, warm desert tones"
        ),
        "output": "tiles/terrain/terrain_sand.png",
        "size": (256, 256),
        "transparent": False,
    },
    {
        "id": "terrain_rock",
        "prompt": (
            "rocky terrain texture, gray stone surface, "
            "cracked rock ground, rough impassable stone terrain, "
            "seamless tileable, top-down pixel art"
        ),
        "output": "tiles/terrain/terrain_rock.png",
        "size": (256, 256),
        "transparent": False,
    },
    {
        "id": "terrain_water",
        "prompt": (
            "water surface texture, dark blue water, "
            "gentle ripple pattern, deep water surface, "
            "seamless tileable, top-down pixel art, blue tones"
        ),
        "output": "tiles/terrain/terrain_water.png",
        "size": (256, 256),
        "transparent": False,
    },
    {
        "id": "terrain_tiberium",
        "prompt": (
            "Tiberium crystal field, glowing green alien crystals "
            "growing from brown ground, bioluminescent green mineral deposits, "
            "toxic alien resource, jagged green crystal formations, "
            "top-down pixel art, seamless tileable"
        ),
        "output": "tiles/terrain/terrain_tiberium.png",
        "size": (256, 256),
        "transparent": False,
    },
    {
        "id": "terrain_road",
        "prompt": (
            "paved road texture, gray asphalt surface, "
            "cracked military road, worn pavement, "
            "seamless tileable, top-down pixel art"
        ),
        "output": "tiles/terrain/terrain_road.png",
        "size": (256, 256),
        "transparent": False,
    },

    # === EFFECTS ===
    {
        "id": "effect_explosion",
        "prompt": make_prompt(
            "explosion sprite, orange and yellow fireball, "
            "pixel art explosion effect, expanding blast, bright center, smoke"
        ),
        "output": "sprites/effects/effect_explosion.png",
        "size": (256, 256),
    },
    {
        "id": "effect_muzzle_flash",
        "prompt": make_prompt(
            "gun muzzle flash sprite, bright yellow-white star flash, "
            "weapon firing effect, very bright small and intense"
        ),
        "output": "sprites/effects/effect_muzzle_flash.png",
        "size": (256, 256),
    },
]


def main():
    if not API_KEY:
        logger.error("No API key set. Set LEONARDO_API_KEY environment variable.")
        sys.exit(1)

    if not verify_api_key():
        sys.exit(1)

    # Track results
    results = {"generated": 0, "failed": 0, "skipped": 0}
    total = len(ASSETS)

    logger.info(f"Starting generation of {total} assets...")
    logger.info(f"Output directory: {OUTPUT_BASE}")
    print()

    # Track pending generations for parallel polling
    pending = []  # (asset_info, gen_id)

    for i, asset in enumerate(ASSETS, 1):
        asset_id = asset["id"]
        output_path = os.path.join(OUTPUT_BASE, asset["output"])

        # Skip if already exists
        if os.path.exists(output_path):
            logger.info(f"[{i}/{total}] Skipping {asset_id} (already exists)")
            results["skipped"] += 1
            continue

        logger.info(f"[{i}/{total}] Generating: {asset_id}")

        w, h = asset.get("size", (512, 512))
        transparent = asset.get("transparent", True)

        gen_id = generate_image(
            prompt=asset["prompt"],
            negative_prompt=NEGATIVE_BASE,
            width=w,
            height=h,
            transparent=transparent,
        )

        if not gen_id:
            results["failed"] += 1
            continue

        # Wait for this generation
        urls = wait_for_generation(gen_id)

        if urls:
            if download_image(urls[0], output_path):
                results["generated"] += 1
            else:
                results["failed"] += 1
        else:
            results["failed"] += 1

        # Rate limit between generations
        time.sleep(1.0)

    print()
    logger.info("=" * 50)
    logger.info(f"GENERATION COMPLETE")
    logger.info(f"  Generated: {results['generated']}")
    logger.info(f"  Skipped:   {results['skipped']}")
    logger.info(f"  Failed:    {results['failed']}")
    logger.info(f"  Total:     {total}")
    logger.info("=" * 50)

    # Save manifest
    manifest_path = os.path.join(OUTPUT_BASE, "manifest.json")
    manifest = {
        "version": "1.0",
        "provider": "leonardo.ai",
        "total_assets": results["generated"] + results["skipped"],
        "assets": {}
    }
    for asset in ASSETS:
        path = os.path.join(OUTPUT_BASE, asset["output"])
        manifest["assets"][asset["id"]] = {
            "path": asset["output"],
            "exists": os.path.exists(path),
            "size": asset.get("size", [512, 512]),
        }
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)
    logger.info(f"Manifest saved: {manifest_path}")


if __name__ == "__main__":
    main()
