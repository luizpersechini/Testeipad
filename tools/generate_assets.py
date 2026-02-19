#!/usr/bin/env python3
"""
Command & Conquer: Tiberian Dawn - Asset Generator
===================================================
Generates placeholder pixel art sprites for all game entities.

Outputs to assets/ directory:
  assets/sprites/units/     - Vehicle sprites (8 facings each)
  assets/sprites/infantry/  - Infantry sprites
  assets/sprites/buildings/ - Building sprites (GDI + Nod variants)
  assets/tiles/terrain/     - Terrain tiles
  assets/tiles/overlays/    - Tiberium, walls, etc.
  assets/ui/                - Sidebar icons, cursors, health bars

Requires: pip install Pillow
Run: python3 tools/generate_assets.py
"""

from PIL import Image, ImageDraw
import os
import math

# ============================================================
# Configuration
# ============================================================

TILE_SIZE = 24       # Base tile size (original C&C)
TILE_RENDER = 48     # Rendered tile size (2x)
SPRITE_SIZE = 48     # Unit sprite size
BUILDING_CELL = 48   # Building cell size
ICON_SIZE = 64       # Sidebar icon size

OUTPUT_BASE = os.path.join(os.path.dirname(os.path.dirname(__file__)), "assets")

# Faction colors
GDI_PRIMARY = (218, 165, 32)
GDI_DARK = (140, 105, 20)
GDI_LIGHT = (245, 200, 60)
NOD_PRIMARY = (180, 0, 0)
NOD_DARK = (120, 0, 0)
NOD_LIGHT = (220, 40, 40)
NEUTRAL = (100, 100, 100)


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


# ============================================================
# Terrain Tiles
# ============================================================

def generate_terrain():
    """Generate all terrain tile variants."""
    out = os.path.join(OUTPUT_BASE, "tiles", "terrain")
    ensure_dir(out)

    tiles = {
        "clear": {"base": (120, 100, 60), "noise": 15},
        "sand": {"base": (180, 160, 90), "noise": 12},
        "rock": {"base": (90, 85, 75), "noise": 8},
        "rough": {"base": (85, 70, 45), "noise": 10},
        "road_h": {"base": (95, 90, 80), "noise": 5},
        "road_v": {"base": (95, 90, 80), "noise": 5},
        "road_cross": {"base": (95, 90, 80), "noise": 5},
        "water": {"base": (30, 60, 140), "noise": 10},
        "water_shore_n": {"base": (30, 60, 140), "noise": 10},
        "cliff_n": {"base": (65, 60, 50), "noise": 6},
        "cliff_s": {"base": (55, 50, 42), "noise": 6},
        "bridge_h": {"base": (100, 80, 50), "noise": 8},
    }

    import random
    random.seed(42)

    for name, props in tiles.items():
        # Generate 4 variants for each tile
        for variant in range(4):
            img = Image.new('RGBA', (TILE_RENDER, TILE_RENDER), (0, 0, 0, 255))
            draw = ImageDraw.Draw(img)
            br, bg, bb = props["base"]
            noise = props["noise"]

            # Fill with noisy base color
            for py in range(TILE_RENDER):
                for px in range(TILE_RENDER):
                    nr = max(0, min(255, br + random.randint(-noise, noise)))
                    ng = max(0, min(255, bg + random.randint(-noise, noise)))
                    nb = max(0, min(255, bb + random.randint(-noise, noise)))
                    img.putpixel((px, py), (nr, ng, nb, 255))

            # Special features
            if name == 'water' or name.startswith('water_'):
                # Wave lines
                for wy in range(0, TILE_RENDER, 8):
                    for wx in range(TILE_RENDER):
                        offset = int(3 * math.sin(wx * 0.3 + variant))
                        y = wy + offset
                        if 0 <= y < TILE_RENDER:
                            img.putpixel((wx, y), (60, 90, 170, 255))

            if 'road' in name:
                # Road markings
                if name == 'road_h':
                    draw.rectangle([0, TILE_RENDER//2-6, TILE_RENDER, TILE_RENDER//2+6],
                                   fill=(85, 80, 72))
                    for x in range(4, TILE_RENDER, 12):
                        draw.rectangle([x, TILE_RENDER//2-1, x+6, TILE_RENDER//2+1],
                                       fill=(120, 115, 100))
                elif name == 'road_v':
                    draw.rectangle([TILE_RENDER//2-6, 0, TILE_RENDER//2+6, TILE_RENDER],
                                   fill=(85, 80, 72))

            if 'cliff' in name:
                # Cliff edge
                if name == 'cliff_n':
                    for x in range(TILE_RENDER):
                        h = random.randint(6, 14)
                        draw.rectangle([x, 0, x, h], fill=(45, 40, 35))
                        draw.point((x, h+1), fill=(80, 75, 65))

            if name == 'rock':
                # Rock bumps
                for _ in range(3 + variant):
                    rx = random.randint(4, TILE_RENDER-8)
                    ry = random.randint(4, TILE_RENDER-8)
                    rs = random.randint(4, 10)
                    draw.ellipse([rx, ry, rx+rs, ry+rs],
                                fill=(75, 70, 60), outline=(100, 95, 85))

            img.save(os.path.join(out, f"{name}_{variant}.png"))

    print(f"  Generated {len(tiles) * 4} terrain tiles")


# ============================================================
# Tiberium & Overlays
# ============================================================

def generate_overlays():
    """Generate tiberium crystals and other overlays."""
    out = os.path.join(OUTPUT_BASE, "tiles", "overlays")
    ensure_dir(out)

    import random
    random.seed(123)

    # Tiberium stages (growth levels 0-3)
    for stage in range(4):
        img = Image.new('RGBA', (TILE_RENDER, TILE_RENDER), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)

        num_crystals = 2 + stage * 2
        for _ in range(num_crystals):
            cx = random.randint(8, TILE_RENDER - 12)
            cy = random.randint(8, TILE_RENDER - 8)
            height = 6 + stage * 3 + random.randint(0, 4)
            width = 2 + random.randint(0, 2)

            # Crystal shaft
            green = random.randint(160, 230)
            crystal_color = (0, green, 0, 255)
            highlight = (40, min(255, green + 40), 40, 255)
            shadow = (0, green // 2, 0, 255)

            # Draw crystal polygon (pointed top)
            points = [
                (cx, cy - height),           # Top point
                (cx + width, cy - height//3), # Right
                (cx + width, cy),             # Bottom right
                (cx - width, cy),             # Bottom left
                (cx - width, cy - height//3), # Left
            ]
            draw.polygon(points, fill=crystal_color, outline=highlight)

            # Glow at base
            draw.ellipse([cx-width-1, cy-1, cx+width+1, cy+2],
                        fill=(0, green//2, 0, 100))

        img.save(os.path.join(out, f"tiberium_{stage}.png"))

    # Scorch mark
    img = Image.new('RGBA', (TILE_RENDER, TILE_RENDER), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    cx, cy = TILE_RENDER // 2, TILE_RENDER // 2
    for r in range(12, 0, -1):
        alpha = int(180 * (r / 12))
        draw.ellipse([cx-r, cy-r, cx+r, cy+r], fill=(30, 25, 20, alpha))
    img.save(os.path.join(out, "scorch.png"))

    # Wall segments
    for wall_type in ["concrete", "sandbag", "chain"]:
        img = Image.new('RGBA', (TILE_RENDER, TILE_RENDER), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        if wall_type == "concrete":
            draw.rectangle([16, 16, 32, 32], fill=(140, 140, 140), outline=(160, 160, 160))
        elif wall_type == "sandbag":
            draw.ellipse([14, 18, 34, 30], fill=(150, 130, 80), outline=(120, 100, 60))
        elif wall_type == "chain":
            for y in range(16, 33, 4):
                draw.line([(16, y), (32, y)], fill=(120, 120, 120), width=1)
        img.save(os.path.join(out, f"wall_{wall_type}.png"))

    print(f"  Generated tiberium (4 stages) + overlays")


# ============================================================
# Unit Sprites
# ============================================================

def draw_tank_body(draw, cx, cy, size, color, dark_color, facing):
    """Draw a tank body at given facing (0=N, 1=NE, ... 7=NW)."""
    half = size // 2
    # Body rectangle rotated by facing
    angle = facing * 45
    rad = math.radians(angle)

    # Simple oriented rectangle
    cos_a = math.cos(rad)
    sin_a = math.sin(rad)

    # Body corners (before rotation)
    bw = half * 0.7
    bh = half * 0.9
    corners = [(-bw, -bh), (bw, -bh), (bw, bh), (-bw, bh)]

    # Rotate
    rotated = [(cx + x*cos_a - y*sin_a, cy + x*sin_a + y*cos_a) for x, y in corners]
    draw.polygon(rotated, fill=color, outline=dark_color)


def draw_turret(draw, cx, cy, size, color, highlight, facing):
    """Draw a turret with gun barrel."""
    ts = size // 4
    draw.ellipse([cx-ts, cy-ts, cx+ts, cy+ts], fill=color, outline=highlight)

    # Gun barrel
    angle = facing * 45
    rad = math.radians(angle)
    blen = size * 0.5
    ex = cx + blen * math.sin(rad)  # Note: 0=North=up
    ey = cy - blen * math.cos(rad)
    draw.line([(cx, cy), (ex, ey)], fill=highlight, width=2)


def generate_units():
    """Generate all unit sprites with 8 facings."""
    out = os.path.join(OUTPUT_BASE, "sprites", "units")
    ensure_dir(out)

    units = {
        "mammoth_tank": {"size": 22, "has_turret": True, "turret_size": 8,
                         "faction": "gdi", "double_barrel": True},
        "medium_tank": {"size": 18, "has_turret": True, "turret_size": 6,
                        "faction": "gdi"},
        "light_tank": {"size": 15, "has_turret": True, "turret_size": 5,
                       "faction": "nod"},
        "stealth_tank": {"size": 14, "has_turret": True, "turret_size": 5,
                         "faction": "nod", "stealth": True},
        "apc": {"size": 17, "has_turret": False, "faction": "both"},
        "humvee": {"size": 13, "has_turret": True, "turret_size": 4,
                   "faction": "gdi"},
        "buggy": {"size": 12, "has_turret": True, "turret_size": 3,
                  "faction": "nod"},
        "bike": {"size": 10, "has_turret": False, "faction": "nod"},
        "harvester": {"size": 18, "has_turret": False, "faction": "both",
                      "harvester": True},
        "mcv": {"size": 22, "has_turret": False, "faction": "both",
                "mcv": True},
        "mlrs": {"size": 16, "has_turret": True, "turret_size": 6,
                 "faction": "gdi", "rockets": True},
        "artillery": {"size": 16, "has_turret": True, "turret_size": 5,
                      "faction": "nod"},
        "flame_tank": {"size": 16, "has_turret": False, "faction": "nod"},
        "gunboat": {"size": 20, "has_turret": True, "turret_size": 5,
                    "faction": "gdi"},
    }

    for faction_variant in ['gdi', 'nod']:
        primary = GDI_PRIMARY if faction_variant == 'gdi' else NOD_PRIMARY
        dark = GDI_DARK if faction_variant == 'gdi' else NOD_DARK
        light = GDI_LIGHT if faction_variant == 'gdi' else NOD_LIGHT

        for name, props in units.items():
            uf = props.get("faction", "both")
            if uf != "both" and uf != faction_variant:
                continue

            for facing in range(8):
                img = Image.new('RGBA', (SPRITE_SIZE, SPRITE_SIZE), (0, 0, 0, 0))
                draw = ImageDraw.Draw(img)
                cx, cy = SPRITE_SIZE // 2, SPRITE_SIZE // 2
                sz = props["size"]

                # Tracks / wheels
                track_color = (60, 55, 45)
                angle = facing * 45
                rad = math.radians(angle)
                cos_a, sin_a = math.cos(rad), math.sin(rad)

                # Draw track marks
                for side in [-1, 1]:
                    tx = cx + side * sz * 0.35 * cos_a
                    ty = cy + side * sz * 0.35 * sin_a
                    tw = sz * 0.15
                    th = sz * 0.8
                    corners = [(-tw, -th/2), (tw, -th/2), (tw, th/2), (-tw, th/2)]
                    rot = [(tx + x*cos_a - y*sin_a, ty + x*sin_a + y*cos_a) for x, y in corners]
                    draw.polygon(rot, fill=track_color)

                # Body
                draw_tank_body(draw, cx, cy, sz, primary, dark, facing)

                # Special markings
                if props.get("mcv"):
                    # MCV has a large boxy body
                    draw.rectangle([cx-sz//2+2, cy-sz//2+2, cx+sz//2-2, cy+sz//2-2],
                                   outline=light)
                    draw.text((cx-3, cy-3), "M", fill=light)

                if props.get("harvester"):
                    # Harvester scoop
                    draw.rectangle([cx-sz//3, cy+sz//4, cx+sz//3, cy+sz//3+4],
                                   fill=(80, 80, 80), outline=(100, 100, 100))

                # Turret
                if props.get("has_turret"):
                    draw_turret(draw, cx, cy, sz, dark, light, facing)

                    if props.get("double_barrel"):
                        # Second barrel offset
                        offset = 2
                        blen = sz * 0.5
                        for side in [-1, 1]:
                            sx = cx + side * offset * cos_a
                            sy = cy + side * offset * sin_a
                            ex = sx + blen * math.sin(rad)
                            ey = sy - blen * math.cos(rad)
                            draw.line([(sx, sy), (ex, ey)], fill=light, width=1)

                # Stealth shimmer
                if props.get("stealth"):
                    # Make semi-transparent
                    pixels = list(img.getdata())
                    new_data = [(r, g, b, a//2) for r, g, b, a in pixels]
                    img.putdata(new_data)

                img.save(os.path.join(out, f"{name}_{faction_variant}_{facing}.png"))

    print(f"  Generated {sum(1 for _ in units.items()) * 8 * 2} unit sprites")


# ============================================================
# Infantry Sprites
# ============================================================

def generate_infantry():
    """Generate infantry sprites."""
    out = os.path.join(OUTPUT_BASE, "sprites", "infantry")
    ensure_dir(out)

    infantry = {
        "minigunner": {"weapon": "rifle"},
        "grenadier": {"weapon": "grenade"},
        "rocket_soldier": {"weapon": "rocket"},
        "flamethrower": {"weapon": "flame"},
        "engineer": {"weapon": "none", "special": True},
        "commando": {"weapon": "sniper", "special": True},
    }

    body_size = 6
    head_size = 4

    for faction in ['gdi', 'nod']:
        primary = GDI_PRIMARY if faction == 'gdi' else NOD_PRIMARY
        dark = GDI_DARK if faction == 'gdi' else NOD_DARK

        for name, props in infantry.items():
            for facing in range(8):
                # 3 animation frames (stand, walk1, walk2)
                for frame in range(3):
                    img = Image.new('RGBA', (24, 24), (0, 0, 0, 0))
                    draw = ImageDraw.Draw(img)
                    cx, cy = 12, 14

                    angle = facing * 45
                    rad = math.radians(angle)

                    # Legs (animated)
                    leg_color = (60, 50, 30)
                    if frame == 0:
                        draw.line([(cx-2, cy+2), (cx-3, cy+5)], fill=leg_color, width=1)
                        draw.line([(cx+2, cy+2), (cx+3, cy+5)], fill=leg_color, width=1)
                    elif frame == 1:
                        draw.line([(cx-1, cy+2), (cx-4, cy+5)], fill=leg_color, width=1)
                        draw.line([(cx+1, cy+2), (cx+2, cy+5)], fill=leg_color, width=1)
                    else:
                        draw.line([(cx-1, cy+2), (cx, cy+5)], fill=leg_color, width=1)
                        draw.line([(cx+1, cy+2), (cx+4, cy+5)], fill=leg_color, width=1)

                    # Body
                    draw.rectangle([cx-body_size//2, cy-body_size//2,
                                    cx+body_size//2, cy+body_size//2],
                                   fill=primary, outline=dark)

                    # Head
                    draw.ellipse([cx-head_size//2, cy-body_size//2-head_size,
                                  cx+head_size//2, cy-body_size//2],
                                 fill=(200, 170, 120), outline=(150, 120, 80))

                    # Weapon
                    wx = cx + int(4 * math.sin(rad))
                    wy = cy - int(4 * math.cos(rad))
                    weapon_color = (80, 80, 80)

                    if props["weapon"] == "rifle":
                        draw.line([(cx+2, cy-1), (wx+3, wy-2)], fill=weapon_color, width=1)
                    elif props["weapon"] == "rocket":
                        draw.line([(cx+2, cy-1), (wx+4, wy-2)], fill=weapon_color, width=2)
                    elif props["weapon"] == "flame":
                        draw.line([(cx+2, cy), (wx+3, wy)], fill=(200, 100, 0), width=2)
                    elif props["weapon"] == "sniper":
                        draw.line([(cx+2, cy-2), (wx+5, wy-3)], fill=weapon_color, width=1)
                        draw.point((wx+5, wy-3), fill=(255, 0, 0))  # Laser dot

                    # Engineer wrench
                    if props.get("special") and name == "engineer":
                        draw.rectangle([cx+3, cy-2, cx+5, cy+1], fill=(200, 200, 0))

                    img.save(os.path.join(out, f"{name}_{faction}_{facing}_{frame}.png"))

    print(f"  Generated {len(infantry) * 8 * 3 * 2} infantry sprites")


# ============================================================
# Building Sprites
# ============================================================

def generate_buildings():
    """Generate building sprites."""
    out = os.path.join(OUTPUT_BASE, "sprites", "buildings")
    ensure_dir(out)

    buildings = {
        "construction_yard": {"w": 2, "h": 2, "faction": "both",
                               "features": ["crane"]},
        "power_plant": {"w": 2, "h": 2, "faction": "both",
                         "features": ["turbine"]},
        "adv_power_plant": {"w": 2, "h": 2, "faction": "both",
                             "features": ["turbine", "glow"]},
        "refinery": {"w": 2, "h": 2, "faction": "both",
                      "features": ["silo", "dock"]},
        "silo": {"w": 1, "h": 1, "faction": "both",
                  "features": ["tank"]},
        "barracks": {"w": 2, "h": 2, "faction": "gdi",
                      "features": ["door"]},
        "hand_of_nod": {"w": 2, "h": 2, "faction": "nod",
                         "features": ["hand"]},
        "weapons_factory": {"w": 2, "h": 2, "faction": "gdi",
                             "features": ["door", "smokestack"]},
        "airstrip": {"w": 2, "h": 2, "faction": "nod",
                      "features": ["runway"]},
        "guard_tower": {"w": 1, "h": 1, "faction": "gdi",
                         "features": ["gun"]},
        "adv_guard_tower": {"w": 1, "h": 1, "faction": "gdi",
                             "features": ["rocket"]},
        "obelisk": {"w": 1, "h": 1, "faction": "nod",
                     "features": ["laser"]},
        "turret": {"w": 1, "h": 1, "faction": "gdi",
                    "features": ["turret"]},
        "sam_site": {"w": 2, "h": 1, "faction": "nod",
                      "features": ["missile"]},
        "helipad": {"w": 2, "h": 2, "faction": "both",
                     "features": ["pad"]},
        "temple_of_nod": {"w": 2, "h": 2, "faction": "nod",
                           "features": ["dome"]},
        "adv_comm_center": {"w": 2, "h": 2, "faction": "gdi",
                             "features": ["dish"]},
        "repair_bay": {"w": 2, "h": 2, "faction": "both",
                        "features": ["bay"]},
    }

    for faction in ['gdi', 'nod']:
        primary = GDI_PRIMARY if faction == 'gdi' else NOD_PRIMARY
        dark = GDI_DARK if faction == 'gdi' else NOD_DARK
        light = GDI_LIGHT if faction == 'gdi' else NOD_LIGHT

        for name, props in buildings.items():
            bf = props.get("faction", "both")
            if bf != "both" and bf != faction:
                continue

            pw = props["w"] * BUILDING_CELL
            ph = props["h"] * BUILDING_CELL

            img = Image.new('RGBA', (pw, ph), (0, 0, 0, 0))
            draw = ImageDraw.Draw(img)

            # Building foundation
            draw.rectangle([2, 2, pw-3, ph-3], fill=dark, outline=primary)

            # Roof / main structure
            margin = 6
            draw.rectangle([margin, margin, pw-margin-1, ph-margin-1],
                           fill=(dark[0]+20, dark[1]+20, dark[2]+20))

            # Top-left highlight
            draw.line([(margin, margin), (pw-margin, margin)], fill=light)
            draw.line([(margin, margin), (margin, ph-margin)], fill=light)

            # Bottom-right shadow
            shadow = (dark[0]//2, dark[1]//2, dark[2]//2)
            draw.line([(pw-margin, margin), (pw-margin, ph-margin)], fill=shadow)
            draw.line([(margin, ph-margin), (pw-margin, ph-margin)], fill=shadow)

            # Features
            features = props.get("features", [])
            cx, cy = pw // 2, ph // 2

            if "crane" in features:
                draw.line([(cx, margin+4), (cx+15, margin+4)], fill=(160,160,160), width=2)
                draw.line([(cx+15, margin+4), (cx+15, cy)], fill=(160,160,160), width=2)
                draw.rectangle([cx-4, cy-4, cx+4, cy+4], fill=primary, outline=light)

            if "turbine" in features:
                draw.ellipse([cx-8, cy-8, cx+8, cy+8], fill=(80,80,80), outline=(120,120,120))
                for a in range(0, 360, 60):
                    ex = cx + int(6 * math.cos(math.radians(a)))
                    ey = cy + int(6 * math.sin(math.radians(a)))
                    draw.line([(cx, cy), (ex, ey)], fill=(140,140,140))

            if "glow" in features:
                draw.ellipse([cx-3, cy-3, cx+3, cy+3], fill=(100, 200, 255, 150))

            if "door" in features:
                draw.rectangle([cx-6, ph-margin-12, cx+6, ph-margin],
                               fill=(50, 50, 50), outline=(80, 80, 80))

            if "smokestack" in features:
                draw.rectangle([pw-margin-8, margin+2, pw-margin-2, margin+16],
                               fill=(100, 100, 100), outline=(120, 120, 120))

            if "silo" in features or "tank" in features:
                draw.ellipse([cx-10, cy-10, cx+10, cy+10],
                             fill=(0, 120, 0, 200), outline=(0, 80, 0))

            if "dock" in features:
                draw.rectangle([2, ph-12, 14, ph-3], fill=(80, 80, 80))

            if "gun" in features:
                draw.line([(cx, cy-4), (cx, margin)], fill=(120,120,120), width=2)
                draw.ellipse([cx-4, cy-4, cx+4, cy+4], fill=primary)

            if "rocket" in features:
                for dx in [-4, 4]:
                    draw.rectangle([cx+dx-1, margin+2, cx+dx+1, cy-2],
                                   fill=(150,150,150), outline=(180,180,180))

            if "laser" in features:
                # Obelisk spire
                points = [(cx, margin+2), (cx-8, ph-margin), (cx+8, ph-margin)]
                draw.polygon(points, fill=NOD_PRIMARY, outline=NOD_LIGHT)
                draw.ellipse([cx-2, margin+4, cx+2, margin+8], fill=(255, 50, 50))

            if "turret" in features:
                draw.ellipse([cx-6, cy-6, cx+6, cy+6], fill=(100,100,100))
                draw.line([(cx, cy), (cx, margin+2)], fill=(140,140,140), width=2)

            if "missile" in features:
                for dx in [-8, 0, 8]:
                    draw.polygon([(cx+dx, margin+4), (cx+dx-3, cy), (cx+dx+3, cy)],
                                 fill=(150,150,150))

            if "pad" in features:
                draw.ellipse([margin+4, margin+4, pw-margin-4, ph-margin-4],
                             outline=(200, 200, 200))
                draw.line([(cx-10, cy), (cx+10, cy)], fill=(200, 200, 0))
                draw.line([(cx, cy-10), (cx, cy+10)], fill=(200, 200, 0))

            if "hand" in features:
                # Stylized hand shape
                draw.polygon([(cx, margin+4), (cx-12, cy+8), (cx+12, cy+8)],
                             fill=NOD_PRIMARY, outline=NOD_LIGHT)

            if "dome" in features:
                draw.ellipse([cx-14, cy-14, cx+14, cy+14],
                             fill=NOD_DARK, outline=NOD_PRIMARY)
                draw.ellipse([cx-4, cy-4, cx+4, cy+4], fill=(200, 0, 0))

            if "dish" in features:
                draw.arc([cx-10, cy-10, cx+10, cy+10], 200, 340, fill=light, width=2)
                draw.line([(cx, cy), (cx+8, cy-8)], fill=(200,200,200), width=1)

            if "runway" in features:
                draw.rectangle([4, cy-2, pw-4, cy+2], fill=(60, 60, 60))
                for x in range(10, pw-10, 12):
                    draw.rectangle([x, cy-1, x+6, cy+1], fill=(200, 200, 0))

            if "bay" in features:
                draw.rectangle([margin+4, margin+4, pw-margin-4, ph-margin-4],
                               fill=(50, 50, 55))
                draw.rectangle([cx-8, ph-margin-8, cx+8, ph-margin],
                               fill=(60, 60, 60), outline=(100, 100, 100))

            # Bib (ground apron)
            bib_color = (dark[0]-10, dark[1]-10, dark[2]-10)
            draw.rectangle([0, ph-6, pw, ph], fill=bib_color)

            img.save(os.path.join(out, f"{name}_{faction}.png"))

            # Also generate "under construction" frame
            uc_img = img.copy()
            uc_draw = ImageDraw.Draw(uc_img)
            # Scaffolding overlay
            for x in range(0, pw, 8):
                uc_draw.line([(x, 0), (x, ph)], fill=(150, 150, 100, 100), width=1)
            for y in range(0, ph, 8):
                uc_draw.line([(0, y), (pw, y)], fill=(150, 150, 100, 100), width=1)
            uc_img.save(os.path.join(out, f"{name}_{faction}_uc.png"))

    print(f"  Generated {sum(1 for _ in buildings.items()) * 2 * 2} building sprites")


# ============================================================
# UI Elements
# ============================================================

def generate_ui():
    """Generate UI sprites: icons, cursors, bars."""
    out = os.path.join(OUTPUT_BASE, "ui")
    ensure_dir(out)

    # Sidebar build icons for each building
    icon_buildings = [
        ("Power Plant", "PP", (0, 180, 0)),
        ("Refinery", "REF", (0, 200, 200)),
        ("Barracks", "BAR", GDI_PRIMARY),
        ("Weapons Factory", "WF", GDI_PRIMARY),
        ("Guard Tower", "GT", (150, 150, 150)),
        ("Adv Guard Tower", "AGT", (180, 180, 180)),
        ("Silo", "SILO", (0, 140, 0)),
        ("Helipad", "HPAD", (180, 180, 180)),
        ("Repair Bay", "REP", (180, 180, 0)),
    ]

    for name, abbr, color in icon_buildings:
        img = Image.new('RGBA', (ICON_SIZE, ICON_SIZE), (40, 40, 45, 255))
        draw = ImageDraw.Draw(img)
        draw.rectangle([2, 2, ICON_SIZE-3, ICON_SIZE-3], outline=color)
        draw.rectangle([4, 4, ICON_SIZE-5, ICON_SIZE//2], fill=(60, 60, 65))
        # Text would go here - just put a color block for the icon
        draw.rectangle([ICON_SIZE//4, ICON_SIZE//4, ICON_SIZE*3//4, ICON_SIZE*3//4],
                       fill=color)
        img.save(os.path.join(out, f"icon_{abbr.lower()}.png"))

    # Cursors
    cursors = {
        "normal": (255, 255, 255),
        "move": (0, 200, 0),
        "attack": (255, 0, 0),
        "select": (0, 255, 0),
        "no_go": (200, 0, 0),
        "deploy": (0, 100, 255),
        "repair": (200, 200, 0),
    }

    for name, color in cursors.items():
        img = Image.new('RGBA', (24, 24), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        if name == "normal":
            draw.polygon([(1,1), (1,16), (6,12), (10,18), (12,16), (8,10), (14,8)],
                         fill=color, outline=(0,0,0))
        elif name == "attack":
            draw.line([(12, 2), (12, 22)], fill=color, width=1)
            draw.line([(2, 12), (22, 12)], fill=color, width=1)
            draw.ellipse([4, 4, 20, 20], outline=color)
        elif name == "move":
            draw.polygon([(12, 2), (20, 12), (15, 12), (15, 22), (9, 22), (9, 12), (4, 12)],
                         fill=color, outline=(0,0,0))
        else:
            draw.ellipse([2, 2, 22, 22], outline=color, width=2)
        img.save(os.path.join(out, f"cursor_{name}.png"))

    # Health bar segments
    for pct in [100, 75, 50, 25]:
        img = Image.new('RGBA', (32, 4), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        filled = int(32 * pct / 100)
        color = (0, 200, 0) if pct > 50 else (200, 200, 0) if pct > 25 else (200, 0, 0)
        if filled > 0:
            draw.rectangle([0, 0, filled - 1, 3], fill=color)
        if filled < 32:
            draw.rectangle([filled, 0, 31, 3], fill=(40, 0, 0))
        img.save(os.path.join(out, f"health_{pct}.png"))

    print(f"  Generated {len(icon_buildings)} icons, {len(cursors)} cursors, 4 health bars")


# ============================================================
# Effects
# ============================================================

def generate_effects():
    """Generate explosion and projectile sprites."""
    out = os.path.join(OUTPUT_BASE, "sprites", "effects")
    ensure_dir(out)

    # Explosion animation (8 frames)
    for frame in range(8):
        img = Image.new('RGBA', (48, 48), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        cx, cy = 24, 24

        progress = frame / 7
        if progress < 0.3:
            # Initial flash
            r = int(20 * (progress / 0.3))
            draw.ellipse([cx-r, cy-r, cx+r, cy+r], fill=(255, 255, 200, 255))
            draw.ellipse([cx-r//2, cy-r//2, cx+r//2, cy+r//2], fill=(255, 255, 255, 255))
        elif progress < 0.7:
            # Fireball
            r = int(20 * ((progress - 0.3) / 0.4) + 10)
            alpha = int(255 * (1.0 - (progress - 0.3) / 0.4))
            draw.ellipse([cx-r, cy-r, cx+r, cy+r], fill=(255, 120, 20, alpha))
            draw.ellipse([cx-r//2, cy-r//2, cx+r//2, cy+r//2], fill=(255, 200, 50, alpha))
        else:
            # Smoke
            r = int(22 * ((progress - 0.7) / 0.3) + 15)
            alpha = int(180 * (1.0 - (progress - 0.7) / 0.3))
            draw.ellipse([cx-r, cy-r, cx+r, cy+r], fill=(80, 80, 80, alpha))

        img.save(os.path.join(out, f"explosion_{frame}.png"))

    # Muzzle flash
    for frame in range(3):
        img = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        r = 3 + frame * 2
        alpha = 255 - frame * 80
        draw.ellipse([8-r, 8-r, 8+r, 8+r], fill=(255, 255, 150, alpha))
        img.save(os.path.join(out, f"muzzle_{frame}.png"))

    # Bullet / projectile types
    projectiles = {
        "bullet": ((255, 255, 100), 2),
        "rocket": ((255, 200, 50), 4),
        "shell": ((200, 200, 200), 3),
        "flame": ((255, 100, 0), 5),
        "laser": ((255, 0, 0), 1),
    }

    for name, (color, size) in projectiles.items():
        img = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        draw.ellipse([8-size, 8-size, 8+size, 8+size], fill=color)
        # Trail
        draw.line([(8, 8), (8-size*2, 8)], fill=(*color[:3], 128), width=max(1, size-1))
        img.save(os.path.join(out, f"projectile_{name}.png"))

    print(f"  Generated 8 explosion frames, 3 muzzle, {len(projectiles)} projectiles")


# ============================================================
# Main
# ============================================================

def main():
    print("C&C Tiberian Dawn - Asset Generator")
    print("=" * 40)
    print(f"Output: {OUTPUT_BASE}/\n")

    generate_terrain()
    generate_overlays()
    generate_units()
    generate_infantry()
    generate_buildings()
    generate_ui()
    generate_effects()

    # Count total files
    total = 0
    for root, dirs, files in os.walk(OUTPUT_BASE):
        total += sum(1 for f in files if f.endswith('.png'))

    print(f"\nTotal: {total} asset files generated")
    print(f"Location: {OUTPUT_BASE}/")


if __name__ == "__main__":
    main()
