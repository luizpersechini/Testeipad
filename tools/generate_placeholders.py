#!/usr/bin/env python3
"""
Generate Development Placeholder Sprites

Creates recognizable, color-coded placeholder sprites for all game assets
using Pillow. These allow the engine to run and be tested while waiting
for AI-generated art. Each sprite is visually distinct and faction-colored.
"""

import os
import math
import random
from PIL import Image, ImageDraw, ImageFont

BASE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets")

# Faction colors
GDI_PRIMARY = (218, 165, 32)
GDI_DARK = (130, 100, 20)
GDI_LIGHT = (240, 200, 80)

NOD_PRIMARY = (180, 0, 0)
NOD_DARK = (90, 0, 0)
NOD_LIGHT = (220, 50, 50)

NEUTRAL = (100, 110, 100)
NEUTRAL_DARK = (60, 65, 60)
NEUTRAL_LIGHT = (140, 150, 140)

TIBERIUM_GREEN = (0, 200, 0)
TIBERIUM_DARK = (0, 100, 0)


def draw_tank_shape(draw, cx, cy, size, body_color, turret_color, barrel_len=None):
    """Draw a top-down tank silhouette."""
    s = size // 2
    if barrel_len is None:
        barrel_len = s

    # Tracks
    track_w = s // 3
    draw.rectangle([cx - s, cy - s + 2, cx - s + track_w, cy + s - 2],
                   fill=(50, 50, 50))
    draw.rectangle([cx + s - track_w, cy - s + 2, cx + s, cy + s - 2],
                   fill=(50, 50, 50))
    # Track detail lines
    for ty in range(cy - s + 4, cy + s - 2, 4):
        draw.line([cx - s, ty, cx - s + track_w, ty], fill=(70, 70, 70))
        draw.line([cx + s - track_w, ty, cx + s, ty], fill=(70, 70, 70))

    # Body
    body_inset = track_w + 2
    draw.rectangle([cx - s + body_inset, cy - s + 4,
                    cx + s - body_inset, cy + s - 4],
                   fill=body_color, outline=(0, 0, 0))

    # Turret
    turret_r = s // 3
    draw.ellipse([cx - turret_r, cy - turret_r,
                  cx + turret_r, cy + turret_r],
                 fill=turret_color, outline=(0, 0, 0))

    # Barrel
    barrel_w = max(2, s // 8)
    draw.rectangle([cx - barrel_w, cy - barrel_len,
                    cx + barrel_w, cy - turret_r + 2],
                   fill=(60, 60, 60), outline=(0, 0, 0))


def draw_infantry_shape(draw, cx, cy, size, color, has_weapon=True):
    """Draw a simple top-down infantry figure."""
    s = size // 2

    # Body circle
    body_r = s // 2
    draw.ellipse([cx - body_r, cy - body_r + 2,
                  cx + body_r, cy + body_r + 2],
                 fill=color, outline=(0, 0, 0))

    # Head
    head_r = body_r // 2
    draw.ellipse([cx - head_r, cy - body_r - head_r,
                  cx + head_r, cy - body_r + head_r],
                 fill=(200, 170, 130), outline=(0, 0, 0))

    # Weapon line
    if has_weapon:
        draw.line([cx + body_r - 2, cy - 2,
                   cx + body_r + s // 2, cy - s // 3],
                  fill=(60, 60, 60), width=2)


def draw_building_shape(draw, x, y, w, h, color, dark_color, label=""):
    """Draw a top-down building footprint."""
    # Foundation/bib
    bib_margin = 3
    draw.rectangle([x - bib_margin, y - bib_margin,
                    x + w + bib_margin, y + h + bib_margin],
                   fill=(160, 150, 130))

    # Main structure
    draw.rectangle([x, y, x + w, y + h],
                   fill=color, outline=(0, 0, 0), width=2)

    # Roof detail
    margin = w // 6
    draw.rectangle([x + margin, y + margin,
                    x + w - margin, y + h - margin],
                   fill=dark_color, outline=(0, 0, 0))

    # Label
    if label:
        text_x = x + w // 2 - len(label) * 3
        text_y = y + h // 2 - 5
        draw.text((text_x, text_y), label, fill=(255, 255, 255))


def create_unit_sprite(name, size, body_color, turret_color,
                       special=None, barrel_len=None):
    """Create a unit sprite image."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    cx, cy = size // 2, size // 2

    if special == "harvester":
        # Harvester: large boxy vehicle with front scoop
        s = size // 3
        draw.rectangle([cx - s, cy - s + 5, cx + s, cy + s],
                       fill=body_color, outline=(0, 0, 0), width=2)
        # Scoop
        draw.polygon([(cx - s + 5, cy - s + 5),
                      (cx + s - 5, cy - s + 5),
                      (cx + s - 15, cy - s - 10),
                      (cx - s + 15, cy - s - 10)],
                     fill=(80, 80, 80), outline=(0, 0, 0))
        # Tiberium glow
        draw.rectangle([cx - s + 8, cy - 5, cx + s - 8, cy + s - 8],
                       fill=(0, 180, 0, 150))

    elif special == "mcv":
        s = size // 3
        draw.rectangle([cx - s, cy - s, cx + s, cy + s],
                       fill=body_color, outline=(0, 0, 0), width=2)
        # Construction crane
        draw.line([cx, cy - s, cx + s // 2, cy - s - s // 2],
                  fill=(200, 200, 0), width=3)
        draw.line([cx + s // 2, cy - s - s // 2, cx + s // 2, cy - 5],
                  fill=(200, 200, 0), width=2)
        draw.rectangle([cx - s + 5, cy - s + 5, cx + s - 5, cy + s - 5],
                       fill=turret_color)

    elif special == "buggy":
        s = size // 3
        # Wheels
        wr = s // 4
        for wx, wy in [(-s + 3, -s + 5), (s - 3, -s + 5),
                        (-s + 3, s - 5), (s - 3, s - 5)]:
            draw.ellipse([cx + wx - wr, cy + wy - wr,
                          cx + wx + wr, cy + wy + wr],
                         fill=(30, 30, 30))
        # Body
        draw.polygon([(cx - s + 8, cy - s + 2),
                      (cx + s - 8, cy - s + 2),
                      (cx + s - 3, cy + s - 2),
                      (cx - s + 3, cy + s - 2)],
                     fill=body_color, outline=(0, 0, 0))
        # Gun mount
        draw.line([cx, cy - 3, cx, cy - s - 5], fill=(60, 60, 60), width=2)

    elif special == "bike":
        s = size // 4
        # Motorcycle body
        draw.ellipse([cx - s // 2, cy - s, cx + s // 2, cy + s],
                     fill=body_color, outline=(0, 0, 0))
        # Missile pods
        draw.rectangle([cx - s - 3, cy - 3, cx - s + 3, cy + 3],
                       fill=(80, 80, 80))
        draw.rectangle([cx + s - 3, cy - 3, cx + s + 3, cy + 3],
                       fill=(80, 80, 80))

    elif special == "artillery":
        s = size // 3
        draw.rectangle([cx - s, cy - s // 2, cx + s, cy + s // 2],
                       fill=body_color, outline=(0, 0, 0), width=2)
        # Long barrel
        draw.rectangle([cx - 2, cy - s * 2, cx + 2, cy - s // 2],
                       fill=(60, 60, 60), outline=(0, 0, 0))

    elif special == "mlrs":
        s = size // 3
        draw.rectangle([cx - s, cy - s // 2, cx + s, cy + s // 2 + 3],
                       fill=body_color, outline=(0, 0, 0), width=2)
        # Rocket rack
        rw = s - 5
        draw.rectangle([cx - rw, cy - s + 2, cx + rw, cy - 3],
                       fill=(80, 80, 80), outline=(0, 0, 0))
        # Rocket tubes
        for rx in range(cx - rw + 3, cx + rw, 5):
            draw.ellipse([rx - 1, cy - s + 4, rx + 1, cy - s + 6],
                         fill=(40, 40, 40))

    elif special == "stealth":
        s = size // 3
        # Angular stealth shape
        draw.polygon([
            (cx, cy - s - 5),
            (cx + s, cy),
            (cx + s - 5, cy + s),
            (cx - s + 5, cy + s),
            (cx - s, cy),
        ], fill=(*body_color, 180), outline=(0, 0, 0))
        # Shimmer effect
        for _ in range(10):
            sx = random.randint(cx - s, cx + s)
            sy = random.randint(cy - s, cy + s)
            draw.point((sx, sy), fill=(255, 255, 255, 100))

    elif special == "flame_tank":
        s = size // 3
        draw.rectangle([cx - s, cy - s + 5, cx + s, cy + s - 2],
                       fill=body_color, outline=(0, 0, 0), width=2)
        # Dual flame nozzles
        for dx in [-s // 3, s // 3]:
            draw.rectangle([cx + dx - 2, cy - s - 3, cx + dx + 2, cy - s + 8],
                           fill=(100, 100, 100))
            # Flame effect
            draw.ellipse([cx + dx - 4, cy - s - 8, cx + dx + 4, cy - s - 2],
                         fill=(255, 150, 0, 200))

    else:
        # Default tank
        draw_tank_shape(draw, cx, cy, size // 3, body_color, turret_color,
                        barrel_len)

    # Shadow
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    sd = ImageDraw.Draw(shadow)
    sd.ellipse([cx - size // 3, cy + size // 4,
                cx + size // 3, cy + size // 3 + 5],
               fill=(0, 0, 0, 40))

    result = Image.alpha_composite(shadow, img)
    return result


def create_infantry_sprite(name, size, color, special=None):
    """Create an infantry sprite."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    cx, cy = size // 2, size // 2

    has_weapon = special != "engineer"
    draw_infantry_shape(draw, cx, cy, size // 2, color, has_weapon)

    if special == "commando":
        # Beret
        draw.arc([cx - 6, cy - size // 4 - 8, cx + 6, cy - size // 4],
                 0, 180, fill=(139, 69, 19), width=2)

    if special == "engineer":
        # Wrench symbol
        draw.rectangle([cx + 5, cy - 5, cx + 12, cy + 5],
                       fill=(200, 200, 0))

    return img


def create_building_sprite(name, size, color, dark_color, label="",
                           special=None):
    """Create a building sprite."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    margin = size // 8
    bw = size - margin * 2
    bh = size - margin * 2

    if special == "obelisk":
        # Tall narrow obelisk
        cx = size // 2
        draw.polygon([
            (cx, margin),
            (cx + size // 6, size - margin),
            (cx - size // 6, size - margin),
        ], fill=color, outline=(0, 0, 0), width=2)
        # Red crystal tip
        draw.ellipse([cx - 5, margin - 2, cx + 5, margin + 10],
                     fill=(255, 0, 0), outline=(200, 0, 0))
        # Glow
        draw.ellipse([cx - 8, margin - 5, cx + 8, margin + 13],
                     fill=(255, 0, 0, 60))

    elif special == "guard_tower":
        cx, cy = size // 2, size // 2
        # Base
        draw.rectangle([cx - size // 6, cy - size // 6,
                        cx + size // 6, cy + size // 4],
                       fill=color, outline=(0, 0, 0), width=2)
        # Tower top
        draw.rectangle([cx - size // 5, cy - size // 4,
                        cx + size // 5, cy - size // 8],
                       fill=dark_color, outline=(0, 0, 0))
        # Gun
        draw.rectangle([cx - 1, cy - size // 3, cx + 1, cy - size // 5],
                       fill=(60, 60, 60))

    elif special == "temple":
        # Pyramid shape
        cx = size // 2
        draw.polygon([
            (cx, margin + 10),
            (size - margin - 10, size - margin),
            (margin + 10, size - margin),
        ], fill=color, outline=(0, 0, 0), width=2)
        # Red glow
        draw.ellipse([cx - 15, margin + 20, cx + 15, margin + 50],
                     fill=(255, 0, 0, 100))
        # Steps
        for i in range(3):
            y = size - margin - (i * 15) - 5
            w = size // 2 - i * 20
            draw.line([cx - w, y, cx + w, y], fill=(0, 0, 0), width=1)

    elif special == "refinery":
        draw_building_shape(draw, margin, margin, bw, bh, color, dark_color, label)
        # Tiberium hopper (green)
        draw.rectangle([margin + 5, margin + bh // 2,
                        margin + bw // 3, margin + bh - 5],
                       fill=(0, 150, 0, 180), outline=(0, 100, 0))
        # Conveyor
        for i in range(3):
            y = margin + bh // 2 + i * 8
            draw.line([margin + bw // 3, y, margin + bw * 2 // 3, y],
                      fill=(100, 100, 100), width=2)

    elif special == "power_plant":
        draw_building_shape(draw, margin, margin, bw, bh, color, dark_color, label)
        # Cooling towers
        for dx in [bw // 4, bw * 3 // 4]:
            cx = margin + dx
            cy_t = margin + bh // 4
            r = bw // 8
            draw.ellipse([cx - r, cy_t - r, cx + r, cy_t + r],
                         fill=(180, 180, 180), outline=(0, 0, 0))

    else:
        draw_building_shape(draw, margin, margin, bw, bh, color, dark_color, label)

    return img


def create_terrain_tile(name, size, base_color, detail_color, special=None):
    """Create a seamless terrain tile."""
    img = Image.new("RGB", (size, size), base_color)
    draw = ImageDraw.Draw(img)

    random.seed(hash(name))

    if special == "water":
        # Water with wave lines
        for y in range(0, size, 8):
            for x in range(0, size, 2):
                offset = int(math.sin((x + y * 0.5) * 0.1) * 3)
                py = y + offset
                if 0 <= py < size:
                    r = base_color[0] + random.randint(-10, 10)
                    g = base_color[1] + random.randint(-10, 10)
                    b = base_color[2] + random.randint(-10, 10)
                    draw.point((x, py), fill=(max(0, min(255, r)),
                                               max(0, min(255, g)),
                                               max(0, min(255, b))))

    elif special == "tiberium":
        # Brown ground with green crystals
        for _ in range(40):
            x = random.randint(0, size - 1)
            y = random.randint(0, size - 1)
            h = random.randint(3, 10)
            w = random.randint(1, 3)
            green = random.randint(150, 255)
            draw.polygon([(x, y), (x + w, y - h), (x + w * 2, y)],
                         fill=(0, green, 0))
        # Glow
        for _ in range(20):
            x = random.randint(0, size - 1)
            y = random.randint(0, size - 1)
            r = random.randint(2, 5)
            draw.ellipse([x - r, y - r, x + r, y + r],
                         fill=(0, random.randint(100, 200), 0))

    elif special == "road":
        # Asphalt with cracks
        for _ in range(200):
            x = random.randint(0, size - 1)
            y = random.randint(0, size - 1)
            v = random.randint(-15, 15)
            c = tuple(max(0, min(255, base_color[i] + v)) for i in range(3))
            draw.point((x, y), fill=c)
        # Cracks
        for _ in range(3):
            x = random.randint(0, size)
            y = random.randint(0, size)
            for _ in range(10):
                nx = x + random.randint(-3, 3)
                ny = y + random.randint(1, 4)
                draw.line([(x, y), (nx, ny)], fill=detail_color, width=1)
                x, y = nx, min(ny, size - 1)

    else:
        # Generic terrain noise
        for _ in range(300):
            x = random.randint(0, size - 1)
            y = random.randint(0, size - 1)
            v = random.randint(-20, 20)
            c = tuple(max(0, min(255, base_color[i] + v)) for i in range(3))
            draw.point((x, y), fill=c)
        # Pebbles/rocks
        for _ in range(15):
            x = random.randint(2, size - 3)
            y = random.randint(2, size - 3)
            r = random.randint(1, 2)
            draw.ellipse([x - r, y - r, x + r, y + r], fill=detail_color)

    return img


def create_effect_sprite(name, size, special=None):
    """Create an effect sprite."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    cx, cy = size // 2, size // 2

    if special == "explosion":
        # Multi-layered explosion
        layers = [
            (size // 3, (255, 200, 50, 200)),
            (size // 4, (255, 150, 0, 220)),
            (size // 6, (255, 255, 200, 255)),
        ]
        for r, color in layers:
            draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=color)
        # Debris particles
        for _ in range(12):
            angle = random.random() * math.pi * 2
            dist = random.randint(size // 6, size // 3)
            px = cx + int(math.cos(angle) * dist)
            py = cy + int(math.sin(angle) * dist)
            draw.ellipse([px - 2, py - 2, px + 2, py + 2],
                         fill=(255, random.randint(100, 200), 0, 200))

    elif special == "muzzle_flash":
        # Star-shaped flash
        for i in range(8):
            angle = i * math.pi / 4
            ex = cx + int(math.cos(angle) * size // 4)
            ey = cy + int(math.sin(angle) * size // 4)
            draw.line([(cx, cy), (ex, ey)],
                      fill=(255, 255, 200, 220), width=2)
        draw.ellipse([cx - 5, cy - 5, cx + 5, cy + 5],
                     fill=(255, 255, 255, 255))

    return img


def main():
    os.makedirs(BASE_DIR, exist_ok=True)
    generated = 0

    # ===== UNITS =====
    units_dir = os.path.join(BASE_DIR, "sprites", "units")
    os.makedirs(units_dir, exist_ok=True)

    units = [
        ("unit_mammoth", 128, GDI_PRIMARY, GDI_DARK, None, 50),
        ("unit_medium_tank", 128, GDI_PRIMARY, GDI_DARK, None, 35),
        ("unit_light_tank", 128, NOD_PRIMARY, NOD_DARK, None, 28),
        ("unit_humvee", 128, GDI_PRIMARY, GDI_DARK, "buggy", None),
        ("unit_apc", 128, GDI_PRIMARY, GDI_DARK, None, 20),
        ("unit_harvester", 128, NEUTRAL, NEUTRAL_DARK, "harvester", None),
        ("unit_mcv", 128, NEUTRAL, NEUTRAL_DARK, "mcv", None),
        ("unit_buggy", 128, NOD_PRIMARY, NOD_DARK, "buggy", None),
        ("unit_stealth_tank", 128, NOD_DARK, NOD_PRIMARY, "stealth", None),
        ("unit_flame_tank", 128, NOD_PRIMARY, NOD_DARK, "flame_tank", None),
        ("unit_artillery", 128, NOD_PRIMARY, NOD_DARK, "artillery", None),
        ("unit_mlrs", 128, GDI_PRIMARY, GDI_DARK, "mlrs", None),
        ("unit_recon_bike", 128, NOD_PRIMARY, NOD_DARK, "bike", None),
    ]

    for name, size, body, turret, special, barrel in units:
        img = create_unit_sprite(name, size, body, turret, special, barrel)
        path = os.path.join(units_dir, f"{name}.png")
        img.save(path)
        generated += 1
        print(f"  [UNIT] {name}.png")

    # ===== INFANTRY =====
    inf_dir = os.path.join(BASE_DIR, "sprites", "infantry")
    os.makedirs(inf_dir, exist_ok=True)

    infantry = [
        ("inf_minigunner", 64, GDI_PRIMARY, None),
        ("inf_grenadier", 64, GDI_PRIMARY, None),
        ("inf_rocket_soldier", 64, GDI_PRIMARY, None),
        ("inf_flamethrower", 64, NOD_PRIMARY, None),
        ("inf_engineer", 64, NEUTRAL, "engineer"),
        ("inf_commando", 64, GDI_LIGHT, "commando"),
    ]

    for name, size, color, special in infantry:
        img = create_infantry_sprite(name, size, color, special)
        path = os.path.join(inf_dir, f"{name}.png")
        img.save(path)
        generated += 1
        print(f"  [INF]  {name}.png")

    # ===== BUILDINGS =====
    bld_dir = os.path.join(BASE_DIR, "sprites", "buildings")
    os.makedirs(bld_dir, exist_ok=True)

    buildings = [
        ("bld_construction_yard", 192, NEUTRAL, NEUTRAL_DARK, "CY", None),
        ("bld_power_plant", 128, NEUTRAL, NEUTRAL_DARK, "PWR", "power_plant"),
        ("bld_refinery", 160, NEUTRAL, NEUTRAL_DARK, "REF", "refinery"),
        ("bld_barracks", 128, GDI_PRIMARY, GDI_DARK, "BAR", None),
        ("bld_weapons_factory", 160, NEUTRAL, NEUTRAL_DARK, "WF", None),
        ("bld_hand_of_nod", 128, NOD_PRIMARY, NOD_DARK, "HON", None),
        ("bld_obelisk", 128, NOD_DARK, NOD_PRIMARY, "", "obelisk"),
        ("bld_guard_tower", 96, GDI_PRIMARY, GDI_DARK, "", "guard_tower"),
        ("bld_temple_of_nod", 192, NOD_DARK, NOD_PRIMARY, "", "temple"),
        ("bld_comm_center", 128, GDI_PRIMARY, GDI_DARK, "COM", None),
        ("bld_airstrip", 160, NOD_PRIMARY, NOD_DARK, "AIR", None),
        ("bld_helipad", 128, GDI_PRIMARY, GDI_DARK, "HELI", None),
    ]

    for name, size, color, dark, label, special in buildings:
        img = create_building_sprite(name, size, color, dark, label, special)
        path = os.path.join(bld_dir, f"{name}.png")
        img.save(path)
        generated += 1
        print(f"  [BLD]  {name}.png")

    # ===== TERRAIN =====
    tile_dir = os.path.join(BASE_DIR, "tiles", "terrain")
    os.makedirs(tile_dir, exist_ok=True)

    terrains = [
        ("terrain_clear", 128, (180, 160, 120), (140, 130, 100), None),
        ("terrain_sand", 128, (210, 190, 140), (190, 170, 120), None),
        ("terrain_rock", 128, (100, 100, 100), (70, 70, 70), None),
        ("terrain_water", 128, (30, 60, 120), (20, 45, 100), "water"),
        ("terrain_tiberium", 128, (140, 120, 80), (0, 180, 0), "tiberium"),
        ("terrain_road", 128, (90, 90, 90), (60, 60, 60), "road"),
    ]

    for name, size, base, detail, special in terrains:
        img = create_terrain_tile(name, size, base, detail, special)
        path = os.path.join(tile_dir, f"{name}.png")
        img.save(path)
        generated += 1
        print(f"  [TILE] {name}.png")

    # ===== EFFECTS =====
    fx_dir = os.path.join(BASE_DIR, "sprites", "effects")
    os.makedirs(fx_dir, exist_ok=True)

    effects = [
        ("effect_explosion", 96, "explosion"),
        ("effect_muzzle_flash", 64, "muzzle_flash"),
    ]

    for name, size, special in effects:
        img = create_effect_sprite(name, size, special)
        path = os.path.join(fx_dir, f"{name}.png")
        img.save(path)
        generated += 1
        print(f"  [FX]   {name}.png")

    print(f"\nGenerated {generated} placeholder sprites in {BASE_DIR}/")


if __name__ == "__main__":
    main()
