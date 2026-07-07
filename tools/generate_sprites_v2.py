#!/usr/bin/env python3
"""
generate_sprites_v2.py - Enhanced C&C Tiberian Dawn sprite generator
Produces high-quality pixel art sprites with:
- Detailed tank treads, hull rotation, turret rendering
- Building interiors (crane, turbine, windows, scaffolding)
- Infantry walk animations
- Sprite sheet assembly
"""

import os
import math
from PIL import Image, ImageDraw

# Output directories
OUT_UNITS    = "assets/sprites/units"
OUT_INF      = "assets/sprites/infantry"
OUT_BLDG     = "assets/sprites/buildings"
OUT_EFFECTS  = "assets/sprites/effects"
OUT_SHEETS   = "assets/sheets"

for d in [OUT_UNITS, OUT_INF, OUT_BLDG, OUT_EFFECTS, OUT_SHEETS]:
    os.makedirs(d, exist_ok=True)

# ── Palette ──────────────────────────────────────────────────────────────────
PAL = {
    # GDI
    "gdi_hull"      : (180, 160, 100),
    "gdi_hull_hi"   : (210, 190, 130),
    "gdi_hull_sh"   : (130, 110,  60),
    "gdi_tread"     : ( 60,  55,  50),
    "gdi_tread_hi"  : ( 80,  75,  70),
    "gdi_gun"       : (120, 110,  80),
    # Nod
    "nod_hull"      : ( 80, 100,  60),
    "nod_hull_hi"   : (110, 130,  85),
    "nod_hull_sh"   : ( 50,  65,  35),
    "nod_tread"     : ( 45,  45,  40),
    "nod_tread_hi"  : ( 65,  65,  55),
    "nod_gun"       : ( 70,  80,  50),
    # Infantry
    "gdi_uniform"   : (180, 160, 100),
    "gdi_armor"     : (200, 180, 120),
    "nod_uniform"   : ( 40,  40,  40),
    "nod_armor"     : ( 60,  60,  60),
    "skin"          : (210, 170, 130),
    "gun_metal"     : ( 80,  80,  80),
    # Terrain
    "dirt"          : (140, 120,  80),
    "dirt_hi"       : (160, 140, 100),
    "grass"         : ( 80, 110,  60),
    "tib_green"     : ( 50, 180,  50),
    "tib_hi"        : ( 80, 220,  80),
    # Effects
    "explosion_1"   : (255, 200,  50),
    "explosion_2"   : (255, 120,  20),
    "explosion_3"   : (200,  60,  10),
    "smoke_1"       : (150, 150, 150),
    "smoke_2"       : (100, 100, 100),
    # UI / misc
    "black"         : (  0,   0,   0),
    "white"         : (255, 255, 255),
    "shadow"        : (  0,   0,   0, 80),
}

TRANSPARENT = (0, 0, 0, 0)

def new_img(w, h):
    return Image.new("RGBA", (w, h), TRANSPARENT)

def save(img, path):
    img.save(path, "PNG")

# ── Math helpers ──────────────────────────────────────────────────────────────
def rot_pt(cx, cy, x, y, angle_deg):
    r = math.radians(angle_deg)
    dx, dy = x - cx, y - cy
    nx = dx * math.cos(r) - dy * math.sin(r)
    ny = dx * math.sin(r) + dy * math.cos(r)
    return cx + nx, cy + ny

def angle_for_facing(facing, num_facings=8):
    """0=N, 1=NE, 2=E, ... clockwise"""
    return facing * (360.0 / num_facings)

# ─────────────────────────────────────────────────────────────────────────────
# TANKS
# ─────────────────────────────────────────────────────────────────────────────

def draw_tread_strip(draw, cx, side, angle_deg, color, hi_color, size):
    """Draw a single tread strip on left (side=-1) or right (side=+1)."""
    tw = max(4, size // 6)
    tlen = int(size * 0.72)
    tx = cx + side * (size // 4)
    ty = size // 2

    # Bounding polygon for the tread (rotated rectangle)
    corners = [
        rot_pt(cx, ty, tx - tw // 2, ty - tlen // 2, angle_deg),
        rot_pt(cx, ty, tx + tw // 2, ty - tlen // 2, angle_deg),
        rot_pt(cx, ty, tx + tw // 2, ty + tlen // 2, angle_deg),
        rot_pt(cx, ty, tx - tw // 2, ty + tlen // 2, angle_deg),
    ]
    draw.polygon(corners, fill=color)
    # Tread link lines
    num_links = 6
    for i in range(num_links + 1):
        t = i / num_links
        lx = tx
        ly = (ty - tlen // 2) + t * tlen
        p0 = rot_pt(cx, ty, tx - tw // 2, ly, angle_deg)
        p1 = rot_pt(cx, ty, tx + tw // 2, ly, angle_deg)
        draw.line([p0, p1], fill=hi_color, width=1)
    # End caps
    for sign in (-1, 1):
        end_y = ty + sign * tlen // 2
        cap = [
            rot_pt(cx, ty, tx - tw // 2, end_y, angle_deg),
            rot_pt(cx, ty, tx + tw // 2, end_y, angle_deg),
            rot_pt(cx, ty, tx + tw // 2, end_y + sign * tw // 2, angle_deg),
            rot_pt(cx, ty, tx - tw // 2, end_y + sign * tw // 2, angle_deg),
        ]
        draw.polygon(cap, fill=hi_color)


def draw_tank_hull(draw, cx, angle_deg, hull_c, hull_hi, hull_sh, size):
    hw = int(size * 0.5)
    hh = int(size * 0.4)
    ty = size // 2
    corners = [
        rot_pt(cx, ty, cx - hw // 2, ty - hh // 2, angle_deg),
        rot_pt(cx, ty, cx + hw // 2, ty - hh // 2, angle_deg),
        rot_pt(cx, ty, cx + hw // 2, ty + hh // 2, angle_deg),
        rot_pt(cx, ty, cx - hw // 2, ty + hh // 2, angle_deg),
    ]
    draw.polygon(corners, fill=hull_c)
    # Top highlight
    hi_edge = [corners[0], corners[1],
               rot_pt(cx, ty, cx + hw // 2, ty - hh // 2 + 3, angle_deg),
               rot_pt(cx, ty, cx - hw // 2, ty - hh // 2 + 3, angle_deg)]
    draw.polygon(hi_edge, fill=hull_hi)
    # Bottom shadow
    sh_edge = [corners[2], corners[3],
               rot_pt(cx, ty, cx - hw // 2, ty + hh // 2 - 3, angle_deg),
               rot_pt(cx, ty, cx + hw // 2, ty + hh // 2 - 3, angle_deg)]
    draw.polygon(sh_edge, fill=hull_sh)


def draw_turret(draw, cx, turret_angle, gun_c, hull_c, size):
    ty = size // 2
    tr = max(5, size // 5)
    # Turret body (circle approximated as octagon)
    pts = []
    for i in range(8):
        a = math.radians(i * 45 + turret_angle)
        pts.append((cx + tr * math.cos(a), ty + tr * math.sin(a)))
    draw.polygon(pts, fill=hull_c)
    # Gun barrel
    gun_len = int(size * 0.45)
    gun_w = max(2, size // 14)
    bx = cx + gun_len * math.sin(math.radians(turret_angle))
    by = ty - gun_len * math.cos(math.radians(turret_angle))
    # Draw barrel as thin rectangle
    perp = math.radians(turret_angle + 90)
    px, py = gun_w * math.cos(perp), gun_w * math.sin(perp)
    barrel = [
        (cx + px, ty + py),
        (cx - px, ty - py),
        (bx - px, by - py),
        (bx + px, by + py),
    ]
    draw.polygon(barrel, fill=gun_c)


def make_tank(faction, unit_name, size=32):
    """Generate all 8 facing sprites for a tank unit."""
    if faction == "gdi":
        hull_c  = PAL["gdi_hull"]
        hull_hi = PAL["gdi_hull_hi"]
        hull_sh = PAL["gdi_hull_sh"]
        tread_c = PAL["gdi_tread"]
        tread_hi= PAL["gdi_tread_hi"]
        gun_c   = PAL["gdi_gun"]
    else:
        hull_c  = PAL["nod_hull"]
        hull_hi = PAL["nod_hull_hi"]
        hull_sh = PAL["nod_hull_sh"]
        tread_c = PAL["nod_tread"]
        tread_hi= PAL["nod_tread_hi"]
        gun_c   = PAL["nod_gun"]

    cx = size // 2
    sprites = []
    for facing in range(8):
        body_angle = angle_for_facing(facing)
        img = new_img(size, size)
        draw = ImageDraw.Draw(img, "RGBA")

        # Drop shadow
        shadow_offset = 2
        draw.ellipse([cx - size//3 + shadow_offset, size//2 - size//6 + shadow_offset,
                      cx + size//3 + shadow_offset, size//2 + size//6 + shadow_offset],
                     fill=(0, 0, 0, 60))

        draw_tread_strip(draw, cx, -1, body_angle, tread_c, tread_hi, size)
        draw_tread_strip(draw, cx,  1, body_angle, tread_c, tread_hi, size)
        draw_tank_hull(draw, cx, body_angle, hull_c, hull_hi, hull_sh, size)
        # Turret always faces North for base sprite (game rotates it)
        draw_turret(draw, cx, body_angle, gun_c, hull_hi, size)

        path = os.path.join(OUT_UNITS, f"{faction}_{unit_name}_f{facing}.png")
        save(img, path)
        sprites.append(img)
    return sprites


# ─────────────────────────────────────────────────────────────────────────────
# INFANTRY
# ─────────────────────────────────────────────────────────────────────────────

def draw_infantry_frame(draw, cx, facing, frame, uniform_c, armor_c, size):
    """Draw a single infantry frame (facing 0-7, frame 0-2 walk cycle)."""
    angle = angle_for_facing(facing)
    cy = size // 2

    # Walk offsets
    leg_offsets = [(0, 3), (-2, 1), (2, 1)]
    lox, loy = leg_offsets[frame % 3]

    # Rotate all points around center
    def rp(x, y):
        return rot_pt(cx, cy, x, y, angle)

    # Body
    body_pts = [rp(cx-3, cy-6), rp(cx+3, cy-6), rp(cx+4, cy+2), rp(cx-4, cy+2)]
    draw.polygon(body_pts, fill=uniform_c)
    # Helmet/head
    head_pts = [rp(cx-3, cy-11), rp(cx+3, cy-11), rp(cx+3, cy-7), rp(cx-3, cy-7)]
    draw.polygon(head_pts, fill=armor_c)
    # Legs
    draw.polygon([rp(cx-3+lox, cy+2), rp(cx+lox, cy+2), rp(cx+lox, cy+7), rp(cx-3+lox, cy+7)],
                 fill=uniform_c)
    draw.polygon([rp(cx+lox, cy+2), rp(cx+3-lox, cy+2), rp(cx+3-lox, cy+7), rp(cx+lox, cy+7)],
                 fill=uniform_c)
    # Gun
    gun_x = cx + 4
    gun_pts = [rp(gun_x, cy-5), rp(gun_x+2, cy-5), rp(gun_x+2, cy+3), rp(gun_x, cy+3)]
    draw.polygon(gun_pts, fill=PAL["gun_metal"])


def make_infantry(faction, unit_name, size=24):
    """Generate 8 facings x 3 walk frames."""
    if faction == "gdi":
        uniform_c = PAL["gdi_uniform"]
        armor_c   = PAL["gdi_armor"]
    else:
        uniform_c = PAL["nod_uniform"]
        armor_c   = PAL["nod_armor"]

    for facing in range(8):
        for frame in range(3):
            img = new_img(size, size)
            draw = ImageDraw.Draw(img, "RGBA")
            draw_infantry_frame(draw, size//2, facing, frame, uniform_c, armor_c, size)
            path = os.path.join(OUT_INF, f"{faction}_{unit_name}_f{facing}_w{frame}.png")
            save(img, path)


# ─────────────────────────────────────────────────────────────────────────────
# BUILDINGS
# ─────────────────────────────────────────────────────────────────────────────

def make_construction_yard(faction, size=64):
    """Construction Yard with crane arm."""
    img = new_img(size, size)
    draw = ImageDraw.Draw(img, "RGBA")

    if faction == "gdi":
        wall_c  = PAL["gdi_hull"]
        roof_c  = PAL["gdi_hull_hi"]
        shadow_c= PAL["gdi_hull_sh"]
    else:
        wall_c  = PAL["nod_hull"]
        roof_c  = PAL["nod_hull_hi"]
        shadow_c= PAL["nod_hull_sh"]

    # Main structure
    draw.rectangle([4, 16, 60, 60], fill=wall_c)
    draw.rectangle([4, 16, 60, 24], fill=roof_c)
    draw.rectangle([56, 16, 60, 60], fill=shadow_c)

    # Windows
    for wx in [10, 22, 34, 46]:
        draw.rectangle([wx, 28, wx+8, 38], fill=(50, 80, 120, 200))
        draw.line([wx+4, 28, wx+4, 38], fill=roof_c, width=1)

    # Crane base
    draw.rectangle([26, 4, 38, 18], fill=shadow_c)
    # Crane arm
    draw.line([32, 6, 55, 6], fill=shadow_c, width=3)
    draw.line([55, 6, 55, 14], fill=shadow_c, width=2)
    # Crane cable
    draw.line([50, 6, 50, 16], fill=PAL["gun_metal"], width=1)

    # Door
    draw.rectangle([24, 44, 40, 60], fill=shadow_c)
    draw.rectangle([25, 45, 31, 59], fill=(40, 60, 80, 200))
    draw.rectangle([33, 45, 39, 59], fill=(40, 60, 80, 200))

    # Outline
    draw.rectangle([4, 16, 60, 60], outline=PAL["black"], width=1)

    path = os.path.join(OUT_BLDG, f"{faction}_construction_yard.png")
    save(img, path)


def make_power_plant(faction, size=48):
    """Power Plant with cooling towers."""
    img = new_img(size, size)
    draw = ImageDraw.Draw(img, "RGBA")

    if faction == "gdi":
        wall_c = PAL["gdi_hull"]
        roof_c = PAL["gdi_hull_hi"]
        sh_c   = PAL["gdi_hull_sh"]
    else:
        wall_c = PAL["nod_hull"]
        roof_c = PAL["nod_hull_hi"]
        sh_c   = PAL["nod_hull_sh"]

    # Main building
    draw.rectangle([4, 20, 44, 46], fill=wall_c)
    draw.rectangle([4, 20, 44, 26], fill=roof_c)

    # Cooling towers (two trapezoids)
    for tx in [6, 28]:
        tower = [(tx, 6), (tx+14, 6), (tx+12, 22), (tx+2, 22)]
        draw.polygon(tower, fill=sh_c)
        draw.polygon(tower, outline=PAL["black"], width=1)
        # Steam effect (lighter top)
        draw.ellipse([tx+2, 2, tx+12, 10], fill=(200, 200, 200, 100))

    # Energy indicator (glowing stripe)
    draw.rectangle([8, 30, 40, 34], fill=(80, 200, 80, 220))
    draw.rectangle([10, 31, 38, 33], fill=(120, 255, 120, 180))

    # Outline
    draw.rectangle([4, 20, 44, 46], outline=PAL["black"], width=1)

    path = os.path.join(OUT_BLDG, f"{faction}_power_plant.png")
    save(img, path)


def make_refinery(faction, size=64):
    """Tiberium Refinery with silo."""
    img = new_img(size, size)
    draw = ImageDraw.Draw(img, "RGBA")

    if faction == "gdi":
        wall_c = PAL["gdi_hull"]
        roof_c = PAL["gdi_hull_hi"]
        sh_c   = PAL["gdi_hull_sh"]
    else:
        wall_c = PAL["nod_hull"]
        roof_c = PAL["nod_hull_hi"]
        sh_c   = PAL["nod_hull_sh"]

    # Main refinery building
    draw.rectangle([4, 18, 50, 60], fill=wall_c)
    draw.rectangle([4, 18, 50, 26], fill=roof_c)
    draw.rectangle([46, 18, 50, 60], fill=sh_c)

    # Storage silos
    for sy in [10, 24, 38]:
        draw.rectangle([52, sy, 62, sy+12], fill=sh_c)
        draw.rectangle([52, sy, 62, sy+4], fill=roof_c)
        # Tiberium color indicator
        draw.rectangle([53, sy+5, 61, sy+11], fill=(50, 180, 50, 200))

    # Processing pipes
    draw.rectangle([30, 28, 34, 50], fill=sh_c)
    draw.rectangle([28, 46, 38, 50], fill=sh_c)

    # Entry ramp
    draw.polygon([(4, 60), (20, 60), (20, 54), (4, 60)], fill=sh_c)

    # Tiberium glow
    draw.rectangle([8, 32, 26, 44], fill=(30, 140, 30, 180))

    draw.rectangle([4, 18, 50, 60], outline=PAL["black"], width=1)

    path = os.path.join(OUT_BLDG, f"{faction}_refinery.png")
    save(img, path)


def make_barracks(faction, size=48):
    """Barracks building."""
    img = new_img(size, size)
    draw = ImageDraw.Draw(img, "RGBA")

    if faction == "gdi":
        wall_c = PAL["gdi_hull"]
        roof_c = PAL["gdi_hull_hi"]
        sh_c   = PAL["gdi_hull_sh"]
    else:
        wall_c = PAL["nod_hull"]
        roof_c = PAL["nod_hull_hi"]
        sh_c   = PAL["nod_hull_sh"]

    # Main structure
    draw.rectangle([4, 12, 44, 46], fill=wall_c)
    # Sloped roof
    draw.polygon([(4, 12), (44, 12), (40, 4), (8, 4)], fill=roof_c)
    draw.rectangle([40, 12, 44, 46], fill=sh_c)

    # Windows (barracks row)
    for wx in [8, 18, 28]:
        draw.rectangle([wx, 18, wx+7, 28], fill=(50, 80, 120, 200))

    # Door
    draw.rectangle([18, 34, 30, 46], fill=sh_c)
    draw.rectangle([19, 35, 24, 45], fill=(40, 60, 80, 180))
    draw.rectangle([25, 35, 29, 45], fill=(40, 60, 80, 180))

    # Faction symbol above door
    if faction == "gdi":
        draw.ellipse([21, 14, 27, 20], fill=(200, 180, 100))
    else:
        draw.polygon([(24, 14), (28, 20), (20, 20)], fill=(180, 20, 20))

    draw.rectangle([4, 12, 44, 46], outline=PAL["black"], width=1)

    path = os.path.join(OUT_BLDG, f"{faction}_barracks.png")
    save(img, path)


def make_war_factory(faction, size=64):
    """War Factory with garage doors."""
    img = new_img(size, size)
    draw = ImageDraw.Draw(img, "RGBA")

    if faction == "gdi":
        wall_c = PAL["gdi_hull"]
        roof_c = PAL["gdi_hull_hi"]
        sh_c   = PAL["gdi_hull_sh"]
    else:
        wall_c = PAL["nod_hull"]
        roof_c = PAL["nod_hull_hi"]
        sh_c   = PAL["nod_hull_sh"]

    # Large factory building
    draw.rectangle([2, 10, 62, 62], fill=wall_c)
    draw.rectangle([2, 10, 62, 18], fill=roof_c)
    draw.rectangle([58, 10, 62, 62], fill=sh_c)

    # Garage doors (main feature)
    for gx in [6, 34]:
        # Door frame
        draw.rectangle([gx, 32, gx+24, 62], fill=sh_c)
        # Door panels (horizontal slats)
        for gy in range(34, 62, 5):
            draw.rectangle([gx+1, gy, gx+23, gy+3], fill=(wall_c[0]-20, wall_c[1]-20, wall_c[2]-20))

    # Windows / vents above garage
    for wx in [8, 20, 32, 44]:
        draw.rectangle([wx, 14, wx+8, 28], fill=(50, 80, 120, 200))

    # Crane track on roof
    draw.rectangle([2, 10, 62, 12], fill=sh_c)
    draw.rectangle([28, 6, 36, 12], fill=sh_c)

    draw.rectangle([2, 10, 62, 62], outline=PAL["black"], width=1)

    path = os.path.join(OUT_BLDG, f"{faction}_war_factory.png")
    save(img, path)


def make_radar(faction, size=40):
    """Communications Center / Radar Dome."""
    img = new_img(size, size)
    draw = ImageDraw.Draw(img, "RGBA")

    if faction == "gdi":
        wall_c = PAL["gdi_hull"]
        roof_c = PAL["gdi_hull_hi"]
        sh_c   = PAL["gdi_hull_sh"]
        dome_c = (100, 160, 200)
    else:
        wall_c = PAL["nod_hull"]
        roof_c = PAL["nod_hull_hi"]
        sh_c   = PAL["nod_hull_sh"]
        dome_c = (80, 120, 60)

    # Base
    draw.rectangle([6, 24, 34, 38], fill=wall_c)
    draw.rectangle([6, 24, 34, 28], fill=roof_c)

    # Radar dome
    draw.ellipse([4, 8, 36, 28], fill=dome_c)
    draw.ellipse([4, 8, 36, 28], outline=sh_c, width=1)
    # Dome shading
    draw.ellipse([6, 10, 20, 20], fill=(min(255, dome_c[0]+40), min(255, dome_c[1]+40), min(255, dome_c[2]+40), 150))

    # Antenna
    draw.line([20, 2, 20, 10], fill=sh_c, width=2)
    draw.line([16, 4, 24, 4], fill=sh_c, width=1)

    draw.rectangle([6, 24, 34, 38], outline=PAL["black"], width=1)

    path = os.path.join(OUT_BLDG, f"{faction}_radar.png")
    save(img, path)


def make_guard_tower(faction, size=32):
    """Guard Tower / Turret."""
    img = new_img(size, size)
    draw = ImageDraw.Draw(img, "RGBA")

    if faction == "gdi":
        wall_c = PAL["gdi_hull"]
        roof_c = PAL["gdi_hull_hi"]
        sh_c   = PAL["gdi_hull_sh"]
        gun_c  = PAL["gdi_gun"]
    else:
        wall_c = PAL["nod_hull"]
        roof_c = PAL["nod_hull_hi"]
        sh_c   = PAL["nod_hull_sh"]
        gun_c  = PAL["nod_gun"]

    # Tower base
    draw.polygon([(8, 30), (24, 30), (22, 20), (10, 20)], fill=sh_c)
    # Tower body
    draw.rectangle([8, 10, 24, 22], fill=wall_c)
    draw.rectangle([8, 10, 24, 14], fill=roof_c)
    draw.rectangle([22, 10, 24, 22], fill=sh_c)

    # Turret on top
    draw.rectangle([10, 6, 22, 12], fill=gun_c)
    # Gun barrel pointing north
    draw.rectangle([14, 0, 18, 8], fill=sh_c)

    draw.rectangle([8, 10, 24, 22], outline=PAL["black"], width=1)

    path = os.path.join(OUT_BLDG, f"{faction}_guard_tower.png")
    save(img, path)


# ─────────────────────────────────────────────────────────────────────────────
# EFFECTS
# ─────────────────────────────────────────────────────────────────────────────

def make_explosion_frames(size=32, num_frames=6):
    """Generate explosion animation frames."""
    for frame in range(num_frames):
        img = new_img(size, size)
        draw = ImageDraw.Draw(img, "RGBA")
        cx, cy = size // 2, size // 2
        t = frame / (num_frames - 1)
        outer_r = int(4 + t * (size // 2 - 2))
        inner_r = max(1, outer_r - 4)

        # Outer glow
        alpha_outer = int(200 * (1.0 - t))
        color_outer = (int(255 * (1 - t * 0.5)), int(80 * (1 - t)), 0, alpha_outer)
        draw.ellipse([cx - outer_r, cy - outer_r, cx + outer_r, cy + outer_r], fill=color_outer)

        # Inner bright core
        if inner_r > 0:
            alpha_inner = int(255 * (1.0 - t * 1.2))
            alpha_inner = max(0, alpha_inner)
            core_c = (255, int(200 * (1 - t * 0.5)), int(50 * (1 - t)), alpha_inner)
            draw.ellipse([cx - inner_r, cy - inner_r, cx + inner_r, cy + inner_r], fill=core_c)

        # Sparks
        num_sparks = 8
        for i in range(num_sparks):
            a = math.radians(i * (360 / num_sparks) + frame * 15)
            sr = outer_r + 2
            sx = int(cx + sr * math.cos(a))
            sy = int(cy + sr * math.sin(a))
            spark_alpha = int(200 * (1.0 - t))
            draw.ellipse([sx-1, sy-1, sx+2, sy+2], fill=(255, 200, 50, spark_alpha))

        save(img, os.path.join(OUT_EFFECTS, f"explosion_f{frame}.png"))


def make_smoke_frames(size=24, num_frames=5):
    """Generate smoke puff frames."""
    for frame in range(num_frames):
        img = new_img(size, size)
        draw = ImageDraw.Draw(img, "RGBA")
        cx = size // 2
        t = frame / (num_frames - 1)
        r = int(4 + t * (size // 2 - 2))
        alpha = int(180 * (1.0 - t * 0.7))
        gray = int(100 + t * 80)
        draw.ellipse([cx - r, size - 2 - 2*r, cx + r, size - 2], fill=(gray, gray, gray, alpha))
        draw.ellipse([cx - r + 3, size - 4 - 2*r, cx + r - 2, size - 4], fill=(gray+20, gray+20, gray+20, alpha // 2))
        save(img, os.path.join(OUT_EFFECTS, f"smoke_f{frame}.png"))


def make_muzzle_flash(size=16):
    """Muzzle flash sprite."""
    img = new_img(size, size)
    draw = ImageDraw.Draw(img, "RGBA")
    cx, cy = size // 2, size // 2
    # Star burst
    for i in range(8):
        a = math.radians(i * 45)
        ex = int(cx + (size // 2 - 1) * math.cos(a))
        ey = int(cy + (size // 2 - 1) * math.sin(a))
        draw.line([cx, cy, ex, ey], fill=(255, 220, 80, 200), width=2)
    draw.ellipse([cx-3, cy-3, cx+3, cy+3], fill=(255, 255, 200, 240))
    save(img, os.path.join(OUT_EFFECTS, "muzzle_flash.png"))


def make_bullet(size=8):
    """Bullet/projectile sprite."""
    img = new_img(size, size)
    draw = ImageDraw.Draw(img, "RGBA")
    draw.ellipse([1, 1, size-2, size-2], fill=(255, 220, 80, 220))
    draw.ellipse([2, 2, size-3, size-3], fill=(255, 255, 200, 255))
    save(img, os.path.join(OUT_EFFECTS, "bullet.png"))


# ─────────────────────────────────────────────────────────────────────────────
# SPRITE SHEET ASSEMBLY
# ─────────────────────────────────────────────────────────────────────────────

def assemble_unit_sheet(faction, unit_name, sprite_size=32, facings=8):
    """Pack all facings of a unit into a single horizontal sprite sheet."""
    sheet_w = sprite_size * facings
    sheet_h = sprite_size
    sheet = Image.new("RGBA", (sheet_w, sheet_h), TRANSPARENT)

    for facing in range(facings):
        path = os.path.join(OUT_UNITS, f"{faction}_{unit_name}_f{facing}.png")
        if os.path.exists(path):
            sprite = Image.open(path).convert("RGBA")
            sheet.paste(sprite, (facing * sprite_size, 0))

    out_path = os.path.join(OUT_SHEETS, f"{faction}_{unit_name}_sheet.png")
    save(sheet, out_path)
    return out_path


def assemble_infantry_sheet(faction, unit_name, sprite_size=24, facings=8, frames=3):
    """Pack infantry facings x walk frames into a sprite sheet (facings wide, frames tall)."""
    sheet_w = sprite_size * facings
    sheet_h = sprite_size * frames
    sheet = Image.new("RGBA", (sheet_w, sheet_h), TRANSPARENT)

    for facing in range(facings):
        for frame in range(frames):
            path = os.path.join(OUT_INF, f"{faction}_{unit_name}_f{facing}_w{frame}.png")
            if os.path.exists(path):
                sprite = Image.open(path).convert("RGBA")
                sheet.paste(sprite, (facing * sprite_size, frame * sprite_size))

    out_path = os.path.join(OUT_SHEETS, f"{faction}_{unit_name}_sheet.png")
    save(sheet, out_path)
    return out_path


def assemble_building_sheet(buildings, cols=4):
    """Pack building sprites into a grid sheet."""
    if not buildings:
        return
    imgs = []
    max_w, max_h = 0, 0
    for path in buildings:
        if os.path.exists(path):
            img = Image.open(path).convert("RGBA")
            imgs.append(img)
            max_w = max(max_w, img.width)
            max_h = max(max_h, img.height)

    if not imgs:
        return

    rows = math.ceil(len(imgs) / cols)
    sheet = Image.new("RGBA", (cols * max_w, rows * max_h), TRANSPARENT)
    for i, img in enumerate(imgs):
        col = i % cols
        row = i // cols
        sheet.paste(img, (col * max_w, row * max_h))

    out_path = os.path.join(OUT_SHEETS, "buildings_sheet.png")
    save(sheet, out_path)
    return out_path


def assemble_effects_sheet(size=32):
    """Pack explosion and smoke frames into a sheet."""
    all_effects = []
    for f in range(6):
        p = os.path.join(OUT_EFFECTS, f"explosion_f{f}.png")
        if os.path.exists(p):
            all_effects.append(p)
    for f in range(5):
        p = os.path.join(OUT_EFFECTS, f"smoke_f{f}.png")
        if os.path.exists(p):
            all_effects.append(p)

    if not all_effects:
        return

    sheet = Image.new("RGBA", (size * len(all_effects), size), TRANSPARENT)
    for i, path in enumerate(all_effects):
        img = Image.open(path).convert("RGBA").resize((size, size), Image.NEAREST)
        sheet.paste(img, (i * size, 0))

    out_path = os.path.join(OUT_SHEETS, "effects_sheet.png")
    save(sheet, out_path)
    return out_path


# ─────────────────────────────────────────────────────────────────────────────
# MAIN
# ─────────────────────────────────────────────────────────────────────────────

def main():
    print("=== C&C Tiberian Dawn Enhanced Sprite Generator v2 ===\n")
    total = 0

    # --- Tanks ---
    tank_defs = [
        ("gdi", "medium_tank"),
        ("gdi", "mammoth_tank"),
        ("nod", "light_tank"),
        ("nod", "stealth_tank"),
    ]
    print("Generating tanks...")
    for faction, name in tank_defs:
        make_tank(faction, name, size=32 if "mammoth" not in name else 40)
        assemble_unit_sheet(faction, name)
        total += 9  # 8 sprites + 1 sheet
        print(f"  {faction} {name}: 8 facings + sheet")

    # --- Infantry ---
    infantry_defs = [
        ("gdi", "minigunner"),
        ("gdi", "grenadier"),
        ("gdi", "rocket_soldier"),
        ("nod", "rifle_infantry"),
        ("nod", "chemical_warrior"),
        ("nod", "flamethrower"),
    ]
    print("\nGenerating infantry...")
    for faction, name in infantry_defs:
        make_infantry(faction, name, size=24)
        assemble_infantry_sheet(faction, name)
        total += 25  # 24 sprites + 1 sheet
        print(f"  {faction} {name}: 8x3 frames + sheet")

    # --- Buildings ---
    print("\nGenerating buildings...")
    bldg_paths = []
    for faction in ["gdi", "nod"]:
        make_construction_yard(faction)
        make_power_plant(faction)
        make_refinery(faction)
        make_barracks(faction)
        make_war_factory(faction)
        make_radar(faction)
        make_guard_tower(faction)
        for bname in ["construction_yard", "power_plant", "refinery",
                      "barracks", "war_factory", "radar", "guard_tower"]:
            p = os.path.join(OUT_BLDG, f"{faction}_{bname}.png")
            bldg_paths.append(p)
        total += 7
        print(f"  {faction}: 7 buildings")

    assemble_building_sheet(bldg_paths, cols=7)
    total += 1

    # --- Effects ---
    print("\nGenerating effects...")
    make_explosion_frames(size=32, num_frames=6)
    make_smoke_frames(size=24, num_frames=5)
    make_muzzle_flash(size=16)
    make_bullet(size=8)
    assemble_effects_sheet(size=32)
    total += 13
    print("  6 explosion + 5 smoke + muzzle flash + bullet + effects sheet")

    print(f"\nDone! Generated ~{total} new high-quality sprite files.")
    print(f"Sprite sheets saved to: {OUT_SHEETS}/")

    # Print summary of sheets
    sheets = [f for f in os.listdir(OUT_SHEETS) if f.endswith(".png")]
    print(f"\nSprite sheets ({len(sheets)}):")
    for s in sorted(sheets):
        p = os.path.join(OUT_SHEETS, s)
        img = Image.open(p)
        print(f"  {s}: {img.width}x{img.height}")


if __name__ == "__main__":
    main()
