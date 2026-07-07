#!/usr/bin/env python3
"""
render_scene.py - Renders a complete C&C: Tiberian Dawn game scene composite
image from generated pixel-art assets using only Pillow (PIL).

Output: 1280x720 PNG with terrain grid, buildings, units, effects, sidebar, etc.
"""

import os
import math
import random
from PIL import Image, ImageDraw, ImageFont

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS_DIR = os.path.join(PROJECT_ROOT, "assets")
TILES_DIR = os.path.join(ASSETS_DIR, "tiles")
TERRAIN_DIR = os.path.join(TILES_DIR, "terrain")
OVERLAYS_DIR = os.path.join(TILES_DIR, "overlays")
SPRITES_DIR = os.path.join(ASSETS_DIR, "sprites")
BUILDINGS_DIR = os.path.join(SPRITES_DIR, "buildings")
UNITS_DIR = os.path.join(SPRITES_DIR, "units")
INFANTRY_DIR = os.path.join(SPRITES_DIR, "infantry")
EFFECTS_DIR = os.path.join(SPRITES_DIR, "effects")
UI_DIR = os.path.join(ASSETS_DIR, "ui")
OUTPUT_DIR = os.path.join(PROJECT_ROOT, "demo")
OUTPUT_PATH = os.path.join(OUTPUT_DIR, "game_scene.png")

# ---------------------------------------------------------------------------
# Scene constants
# ---------------------------------------------------------------------------
SCENE_W, SCENE_H = 1280, 720
TILE_SIZE = 48
GRID_COLS, GRID_ROWS = 20, 15       # 960 x 720  (left portion)
MAP_W = GRID_COLS * TILE_SIZE        # 960
SIDEBAR_W = SCENE_W - MAP_W         # 320

# Fallback colours used when a PNG cannot be loaded
FALLBACK_COLORS = {
    "clear":    (100, 130,  60),
    "sand":     (194, 168, 120),
    "water":    ( 40,  80, 160),
    "rock":     (110, 100,  90),
    "road_h":   (140, 130, 110),
    "road_v":   (140, 130, 110),
    "tiberium": ( 50, 200,  50),
    "building_gdi": (220, 180,  60),
    "building_nod": (200,  40,  40),
    "unit_gdi": (200, 200,  50),
    "unit_nod": (180,  30,  30),
    "infantry_gdi": (100, 200, 100),
    "infantry_nod": (200, 100, 100),
    "explosion": (255, 160,  30),
}

# ---------------------------------------------------------------------------
# Image loading helper
# ---------------------------------------------------------------------------
_image_cache: dict[str, Image.Image] = {}


def load_image(path: str, fallback_size: tuple[int, int] = (48, 48),
               fallback_color: tuple[int, int, int] = (200, 0, 200)) -> Image.Image:
    """Load a PNG with alpha from *path*. On failure return a coloured rectangle."""
    if path in _image_cache:
        return _image_cache[path].copy()
    try:
        img = Image.open(path).convert("RGBA")
        _image_cache[path] = img
        return img.copy()
    except Exception:
        img = Image.new("RGBA", fallback_size, fallback_color + (255,))
        _image_cache[path] = img
        return img.copy()


def load_terrain_tile(name: str, variant: int = 0) -> Image.Image:
    """Load a terrain tile such as 'clear_0.png'."""
    fname = f"{name}_{variant}.png"
    path = os.path.join(TERRAIN_DIR, fname)
    color = FALLBACK_COLORS.get(name, (128, 128, 128))
    return load_image(path, (TILE_SIZE, TILE_SIZE), color)


def load_overlay(name: str, variant: int = 0) -> Image.Image:
    fname = f"{name}_{variant}.png"
    path = os.path.join(OVERLAYS_DIR, fname)
    color = FALLBACK_COLORS.get(name, (128, 128, 128))
    return load_image(path, (TILE_SIZE, TILE_SIZE), color)


def load_building(name: str) -> Image.Image:
    fname = f"{name}.png"
    path = os.path.join(BUILDINGS_DIR, fname)
    # Determine expected size: construction_yard and refinery are 96x96
    big_buildings = ["construction_yard_gdi", "construction_yard_nod",
                     "refinery_gdi", "refinery_nod", "weapons_factory_gdi"]
    size = (96, 96) if name in big_buildings else (48, 48)
    faction = "building_gdi" if "gdi" in name else "building_nod"
    return load_image(path, size, FALLBACK_COLORS[faction])


def load_unit(name: str, facing: int = 0) -> Image.Image:
    fname = f"{name}_{facing}.png"
    path = os.path.join(UNITS_DIR, fname)
    faction = "unit_gdi" if "gdi" in name else "unit_nod"
    return load_image(path, (48, 48), FALLBACK_COLORS[faction])


def load_infantry(name: str, facing: int = 0, frame: int = 0) -> Image.Image:
    fname = f"{name}_{facing}_{frame}.png"
    path = os.path.join(INFANTRY_DIR, fname)
    faction = "infantry_gdi" if "gdi" in name else "infantry_nod"
    return load_image(path, (24, 24), FALLBACK_COLORS[faction])


def load_effect(name: str, frame: int = 0) -> Image.Image:
    fname = f"{name}_{frame}.png"
    path = os.path.join(EFFECTS_DIR, fname)
    return load_image(path, (48, 48), FALLBACK_COLORS.get("explosion", (255, 160, 30)))


def load_ui(name: str) -> Image.Image:
    fname = f"{name}.png"
    path = os.path.join(UI_DIR, fname)
    return load_image(path, (32, 32), (180, 180, 180))


def paste_alpha(dest: Image.Image, src: Image.Image, pos: tuple[int, int]) -> None:
    """Paste *src* onto *dest* at *pos* respecting alpha transparency."""
    dest.paste(src, pos, src)


# ---------------------------------------------------------------------------
# Terrain map definition (20 cols x 15 rows)
# Each cell is (terrain_type, variant) with optional overlay
# ---------------------------------------------------------------------------
def build_terrain_map() -> list[list[tuple[str, int]]]:
    """Return a 2-D grid of (tile_name, variant) tuples."""
    random.seed(42)  # reproducible layout

    grid: list[list[tuple[str, int]]] = []
    for row in range(GRID_ROWS):
        row_data: list[tuple[str, int]] = []
        for col in range(GRID_COLS):
            # Default: mix of sand and clear
            if random.random() < 0.45:
                tile = ("sand", random.randint(0, 3))
            else:
                tile = ("clear", random.randint(0, 3))

            # Rock patches (top-right & bottom-left corners, plus scattered)
            if (row < 3 and col > 14) or (row > 11 and col < 5):
                if random.random() < 0.6:
                    tile = ("rock", random.randint(0, 3))

            # Water river running through the middle (columns 9-10, rows 0-14)
            if col in (9, 10) and not (row in (6, 7)):
                tile = ("water", random.randint(0, 3))
            # Bridge across river at rows 6-7
            if col in (9, 10) and row in (6, 7):
                tile = ("bridge_h", random.randint(0, 3))

            # Shore tiles adjacent to river
            if col in (8, 11) and row not in (6, 7):
                if random.random() < 0.4:
                    tile = ("water_shore_n", random.randint(0, 3))

            # Roads connecting bases (horizontal along row 7, vertical GDI col 4, Nod col 16)
            if row == 7 and col not in (9, 10):
                tile = ("road_h", random.randint(0, 3))
            if col == 4 and 2 <= row <= 7:
                tile = ("road_v", random.randint(0, 3))
            if col == 16 and 7 <= row <= 12:
                tile = ("road_v", random.randint(0, 3))
            # Crossroads
            if (col == 4 and row == 7) or (col == 16 and row == 7):
                tile = ("road_cross", random.randint(0, 3))

            row_data.append(tile)
        grid.append(row_data)
    return grid


# Tiberium field locations (row, col, variant)
TIBERIUM_FIELDS: list[tuple[int, int, int]] = [
    # Field 1 - near center-left
    (4, 6, 0), (4, 7, 1), (5, 6, 2), (5, 7, 3), (5, 8, 0),
    (6, 6, 1), (6, 7, 2), (6, 8, 3),
    # Field 2 - near center-right
    (9, 12, 0), (9, 13, 1), (10, 12, 2), (10, 13, 3), (10, 14, 0),
    (11, 12, 1), (11, 13, 2),
]


# ---------------------------------------------------------------------------
# 1. Render terrain base layer
# ---------------------------------------------------------------------------
def render_terrain(scene: Image.Image) -> None:
    """Tile the 20x15 terrain grid onto the left portion of the scene."""
    grid = build_terrain_map()
    for row in range(GRID_ROWS):
        for col in range(GRID_COLS):
            name, var = grid[row][col]
            tile = load_terrain_tile(name, var)
            tile = tile.resize((TILE_SIZE, TILE_SIZE), Image.NEAREST)
            paste_alpha(scene, tile, (col * TILE_SIZE, row * TILE_SIZE))


# ---------------------------------------------------------------------------
# 2. Render tiberium overlays
# ---------------------------------------------------------------------------
def render_tiberium(scene: Image.Image) -> None:
    for row, col, var in TIBERIUM_FIELDS:
        overlay = load_overlay("tiberium", var)
        overlay = overlay.resize((TILE_SIZE, TILE_SIZE), Image.NEAREST)
        paste_alpha(scene, overlay, (col * TILE_SIZE, row * TILE_SIZE))


# ---------------------------------------------------------------------------
# 3 & 4. Place buildings
# ---------------------------------------------------------------------------
# (name, grid_col, grid_row)  -- position is in tile coords; large buildings
# occupy 2x2 tiles.
GDI_BUILDINGS = [
    ("construction_yard_gdi", 1, 1),   # 96x96 -> 2x2 tiles
    ("power_plant_gdi",       3, 1),   # 48x48
    ("refinery_gdi",          1, 3),   # 96x96 -> 2x2
    ("barracks_gdi",          3, 3),   # 48x48
]

NOD_BUILDINGS = [
    ("construction_yard_nod", 15, 10),
    ("power_plant_nod",       17, 10),
    ("hand_of_nod_nod",       15, 12),
    ("obelisk_nod",           17, 12),
]


def render_buildings(scene: Image.Image) -> None:
    for name, col, row in GDI_BUILDINGS + NOD_BUILDINGS:
        img = load_building(name)
        x = col * TILE_SIZE
        y = row * TILE_SIZE
        paste_alpha(scene, img, (x, y))


# ---------------------------------------------------------------------------
# 5. Place units
# ---------------------------------------------------------------------------
# (loader_name, facing, pixel_x, pixel_y, faction_label)
UNITS = [
    # 3 GDI medium tanks heading south (facing 4)
    ("medium_tank_gdi", 4, 5 * 48 + 0,  6 * 48 + 0),
    ("medium_tank_gdi", 4, 6 * 48 + 0,  5 * 48 + 24),
    ("medium_tank_gdi", 4, 5 * 48 + 24, 5 * 48 + 0),
    # 2 Nod light tanks heading north (facing 0)
    ("light_tank_nod",  0, 13 * 48 + 0,  9 * 48 + 0),
    ("light_tank_nod",  0, 14 * 48 + 0,  9 * 48 + 24),
    # 1 GDI harvester near tiberium field 1
    ("harvester_gdi",   2, 8 * 48 + 0,   5 * 48 + 0),
]

# Infantry squads: (name, facing, frame, pixel_x, pixel_y)
INFANTRY = [
    # GDI infantry near GDI base
    ("minigunner_gdi", 4, 0, 4 * 48 + 4,  4 * 48 + 4),
    ("minigunner_gdi", 4, 1, 4 * 48 + 20, 4 * 48 + 4),
    ("minigunner_gdi", 4, 0, 4 * 48 + 4,  4 * 48 + 20),
    ("rocket_soldier_gdi", 4, 0, 4 * 48 + 20, 4 * 48 + 20),
    ("minigunner_gdi", 4, 2, 5 * 48 + 4,  4 * 48 + 4),
    # Nod infantry near Nod base
    ("minigunner_nod", 0, 0, 14 * 48 + 4,  12 * 48 + 4),
    ("minigunner_nod", 0, 1, 14 * 48 + 20, 12 * 48 + 4),
    ("minigunner_nod", 0, 0, 14 * 48 + 4,  12 * 48 + 20),
    ("flamethrower_nod", 0, 0, 14 * 48 + 20, 12 * 48 + 20),
    ("rocket_soldier_nod", 0, 0, 13 * 48 + 4, 12 * 48 + 4),
]


def render_units(scene: Image.Image) -> None:
    for name, facing, px, py in UNITS:
        img = load_unit(name, facing)
        img = img.resize((TILE_SIZE, TILE_SIZE), Image.NEAREST)
        paste_alpha(scene, img, (px, py))

    for name, facing, frame, px, py in INFANTRY:
        img = load_infantry(name, facing, frame)
        img = img.resize((24, 24), Image.NEAREST)
        paste_alpha(scene, img, (px, py))


# ---------------------------------------------------------------------------
# 6. Explosion effect composite
# ---------------------------------------------------------------------------
def render_explosion(scene: Image.Image) -> None:
    """Composite several explosion frames with decreasing opacity to create
    an animated-look explosion near the battle area."""
    # Place explosion at the midpoint between GDI tanks and Nod tanks
    base_x, base_y = 11 * 48, 8 * 48
    frames_to_draw = [0, 1, 2, 3, 4]
    for i, fr in enumerate(frames_to_draw):
        img = load_effect("explosion", fr)
        img = img.resize((TILE_SIZE, TILE_SIZE), Image.NEAREST)
        # Decrease opacity for later frames to give expansion feel
        alpha_factor = max(0.35, 1.0 - i * 0.15)
        # Apply alpha
        r, g, b, a = img.split()
        a = a.point(lambda p: int(p * alpha_factor))
        img = Image.merge("RGBA", (r, g, b, a))
        # Slight offset per frame to give blast radius
        offset_x = int(math.cos(i * 1.3) * i * 6)
        offset_y = int(math.sin(i * 1.3) * i * 6)
        paste_alpha(scene, img, (base_x + offset_x, base_y + offset_y))


# ---------------------------------------------------------------------------
# 7. Projectile trail between fighting units
# ---------------------------------------------------------------------------
def render_projectile_trail(scene: Image.Image) -> None:
    """Draw a projectile trail from a GDI tank toward a Nod tank."""
    draw = ImageDraw.Draw(scene)

    # Source: GDI tank at (5*48+24, 6*48+24), Target: Nod light tank at (13*48+24, 9*48+24)
    sx, sy = 6 * 48 + 24, 6 * 48
    tx, ty = 13 * 48 + 24, 9 * 48 + 24

    # Try to load the projectile_shell sprite
    proj_path = os.path.join(EFFECTS_DIR, "projectile_shell.png")
    proj_img = load_image(proj_path, (8, 8), (255, 255, 100))
    proj_img = proj_img.resize((12, 12), Image.NEAREST)

    # Draw a dashed trail line
    steps = 16
    for i in range(steps):
        t = i / steps
        x = int(sx + (tx - sx) * t)
        y = int(sy + (ty - sy) * t)
        # Thin yellow-orange trail dots
        if i % 2 == 0:
            draw.ellipse([x - 2, y - 2, x + 2, y + 2], fill=(255, 200, 50, 180))
        else:
            draw.ellipse([x - 1, y - 1, x + 1, y + 1], fill=(255, 160, 30, 120))

    # Place the projectile sprite near the midpoint of the trail
    mid_x = int((sx + tx) / 2)
    mid_y = int((sy + ty) / 2)
    paste_alpha(scene, proj_img, (mid_x - 6, mid_y - 6))

    # Muzzle flash at source
    muzzle = load_effect("muzzle", 0)
    muzzle = muzzle.resize((24, 24), Image.NEAREST)
    paste_alpha(scene, muzzle, (sx - 12, sy - 12))


# ---------------------------------------------------------------------------
# 8. Sidebar (right 320 px)
# ---------------------------------------------------------------------------
def render_sidebar(scene: Image.Image) -> None:
    draw = ImageDraw.Draw(scene)
    sx = MAP_W  # sidebar starts at x=960

    # Try to load a font; fall back to Pillow default
    try:
        font_large = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 16)
        font_med = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 13)
        font_sm = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 11)
    except Exception:
        font_large = ImageFont.load_default()
        font_med = ImageFont.load_default()
        font_sm = ImageFont.load_default()

    # Dark sidebar background
    draw.rectangle([sx, 0, SCENE_W, SCENE_H], fill=(20, 22, 28))

    # Thin gold border on left edge of sidebar
    draw.line([(sx, 0), (sx, SCENE_H)], fill=(180, 160, 60), width=2)

    # --- Title ---
    y = 12
    title = "C&C: TIBERIAN DAWN"
    draw.text((sx + 20, y), title, fill=(220, 190, 50), font=font_large)
    y += 28
    draw.line([(sx + 10, y), (SCENE_W - 10, y)], fill=(100, 90, 40), width=1)
    y += 12

    # --- Credits ---
    draw.text((sx + 20, y), "CREDITS", fill=(160, 160, 160), font=font_sm)
    y += 16
    draw.text((sx + 20, y), "$5,000", fill=(80, 220, 80), font=font_large)
    y += 28
    draw.line([(sx + 10, y), (SCENE_W - 10, y)], fill=(60, 60, 60), width=1)
    y += 12

    # --- Power bar ---
    draw.text((sx + 20, y), "POWER", fill=(160, 160, 160), font=font_sm)
    y += 16
    bar_x = sx + 20
    bar_w = 260
    bar_h = 16
    # Background
    draw.rectangle([bar_x, y, bar_x + bar_w, y + bar_h], fill=(40, 40, 40))
    # Fill (green = good power)
    fill_pct = 0.72
    draw.rectangle([bar_x, y, bar_x + int(bar_w * fill_pct), y + bar_h],
                    fill=(30, 180, 30))
    # Border
    draw.rectangle([bar_x, y, bar_x + bar_w, y + bar_h], outline=(120, 120, 120))
    # Label
    draw.text((bar_x + bar_w + 6, y), "72%", fill=(30, 180, 30), font=font_sm)
    y += bar_h + 20
    draw.line([(sx + 10, y), (SCENE_W - 10, y)], fill=(60, 60, 60), width=1)
    y += 12

    # --- Minimap ---
    draw.text((sx + 20, y), "RADAR MAP", fill=(160, 160, 160), font=font_sm)
    y += 18
    mm_x, mm_y = sx + 30, y
    mm_w, mm_h = 240, 160
    draw.rectangle([mm_x, mm_y, mm_x + mm_w, mm_y + mm_h], fill=(15, 30, 15))
    draw.rectangle([mm_x, mm_y, mm_x + mm_w, mm_y + mm_h], outline=(80, 80, 80))

    # Draw minimap terrain representation
    cell_w = mm_w / GRID_COLS
    cell_h = mm_h / GRID_ROWS
    terrain_grid = build_terrain_map()
    for row in range(GRID_ROWS):
        for col in range(GRID_COLS):
            name, _ = terrain_grid[row][col]
            color_map = {
                "clear": (60, 90, 40), "sand": (140, 120, 80),
                "water": (30, 60, 130), "rock": (80, 75, 65),
                "road_h": (110, 100, 85), "road_v": (110, 100, 85),
                "road_cross": (110, 100, 85),
                "bridge_h": (130, 110, 70),
                "water_shore_n": (50, 80, 120),
                "rough": (90, 85, 60),
            }
            c = color_map.get(name, (60, 90, 40))
            rx = mm_x + int(col * cell_w)
            ry = mm_y + int(row * cell_h)
            draw.rectangle([rx, ry, rx + int(cell_w), ry + int(cell_h)], fill=c)

    # Tiberium on minimap
    for row, col, _ in TIBERIUM_FIELDS:
        rx = mm_x + int(col * cell_w)
        ry = mm_y + int(row * cell_h)
        draw.rectangle([rx, ry, rx + int(cell_w), ry + int(cell_h)],
                        fill=(40, 200, 40))

    # GDI base marker
    draw.rectangle([mm_x + int(1 * cell_w), mm_y + int(1 * cell_h),
                     mm_x + int(4 * cell_w), mm_y + int(5 * cell_h)],
                    outline=(220, 200, 50), width=1)
    # Nod base marker
    draw.rectangle([mm_x + int(15 * cell_w), mm_y + int(10 * cell_h),
                     mm_x + int(19 * cell_w), mm_y + int(14 * cell_h)],
                    outline=(220, 40, 40), width=1)

    # Viewport rectangle on minimap
    draw.rectangle([mm_x + 1, mm_y + 1, mm_x + mm_w - 1, mm_y + mm_h - 1],
                    outline=(200, 200, 200), width=1)

    y = mm_y + mm_h + 16
    draw.line([(sx + 10, y), (SCENE_W - 10, y)], fill=(60, 60, 60), width=1)
    y += 12

    # --- Build queue ---
    draw.text((sx + 20, y), "BUILD QUEUE", fill=(160, 160, 160), font=font_sm)
    y += 18

    # Build item: Guard Tower with progress bar
    build_icon = load_building("guard_tower_gdi")
    build_icon = build_icon.resize((48, 48), Image.NEAREST)
    icon_x, icon_y = sx + 25, y
    paste_alpha(scene, build_icon, (icon_x, icon_y))
    # Label next to icon
    draw.text((icon_x + 56, icon_y + 4), "Guard Tower", fill=(200, 200, 200), font=font_med)
    draw.text((icon_x + 56, icon_y + 22), "Building...", fill=(180, 180, 80), font=font_sm)
    # Progress bar below
    pb_x = icon_x
    pb_y = icon_y + 54
    pb_w = 250
    pb_h = 12
    draw.rectangle([pb_x, pb_y, pb_x + pb_w, pb_y + pb_h], fill=(40, 40, 40))
    progress = 0.65
    draw.rectangle([pb_x, pb_y, pb_x + int(pb_w * progress), pb_y + pb_h],
                    fill=(50, 170, 220))
    draw.rectangle([pb_x, pb_y, pb_x + pb_w, pb_y + pb_h], outline=(100, 100, 100))
    draw.text((pb_x + pb_w + 6, pb_y - 1), "65%", fill=(50, 170, 220), font=font_sm)

    y = pb_y + pb_h + 20
    draw.line([(sx + 10, y), (SCENE_W - 10, y)], fill=(60, 60, 60), width=1)
    y += 12

    # --- Second build slot (empty) ---
    draw.text((sx + 20, y), "UNIT QUEUE", fill=(160, 160, 160), font=font_sm)
    y += 18
    unit_icon = load_unit("medium_tank_gdi", 0)
    unit_icon = unit_icon.resize((48, 48), Image.NEAREST)
    paste_alpha(scene, unit_icon, (sx + 25, y))
    draw.text((sx + 81, y + 4), "Medium Tank", fill=(200, 200, 200), font=font_med)
    draw.text((sx + 81, y + 22), "Ready", fill=(80, 220, 80), font=font_sm)

    y += 60
    draw.line([(sx + 10, y), (SCENE_W - 10, y)], fill=(60, 60, 60), width=1)
    y += 12

    # --- Status line ---
    draw.text((sx + 20, y), "STATUS: Engaging enemy", fill=(200, 80, 80), font=font_sm)
    y += 16
    draw.text((sx + 20, y), "Units: 12 / Buildings: 4", fill=(160, 160, 160), font=font_sm)


# ---------------------------------------------------------------------------
# 9. Health bars above damaged units
# ---------------------------------------------------------------------------
def render_health_bars(scene: Image.Image) -> None:
    """Draw health bars above selected units. Some units shown as damaged."""
    draw = ImageDraw.Draw(scene)

    # (center_x, top_y, hp_fraction, bar_width)
    health_bar_data = [
        # GDI tanks
        (5 * 48 + 24,  6 * 48 - 6,  0.85, 36),
        (6 * 48 + 24,  5 * 48 + 24 - 6, 0.60, 36),
        (5 * 48 + 48,  5 * 48 - 6,  1.0,  36),
        # Nod light tanks (damaged)
        (13 * 48 + 24, 9 * 48 - 6,  0.40, 36),
        (14 * 48 + 24, 9 * 48 + 18, 0.25, 36),
        # Harvester
        (8 * 48 + 24,  5 * 48 - 6,  0.95, 36),
    ]

    for cx, top_y, hp, bw in health_bar_data:
        bx = cx - bw // 2
        bh = 4
        # Background (dark)
        draw.rectangle([bx, top_y, bx + bw, top_y + bh], fill=(40, 0, 0))
        # HP fill - colour depends on HP fraction
        if hp > 0.65:
            color = (30, 200, 30)
        elif hp > 0.35:
            color = (220, 200, 30)
        else:
            color = (220, 40, 30)
        draw.rectangle([bx, top_y, bx + int(bw * hp), top_y + bh], fill=color)
        # Border
        draw.rectangle([bx, top_y, bx + bw, top_y + bh], outline=(180, 180, 180))

    # Also try loading the health bar UI assets and place one near a building
    health_img = load_ui("health_100")
    health_img = health_img.resize((40, 5), Image.NEAREST)
    # Above GDI construction yard
    paste_alpha(scene, health_img, (1 * 48 + 28, 1 * 48 - 8))

    health_75 = load_ui("health_75")
    health_75 = health_75.resize((40, 5), Image.NEAREST)
    # Above Nod construction yard (slightly damaged)
    paste_alpha(scene, health_75, (15 * 48 + 28, 10 * 48 - 8))


# ---------------------------------------------------------------------------
# 10. Selection box around a selected unit
# ---------------------------------------------------------------------------
def render_selection_box(scene: Image.Image) -> None:
    """Draw a bright selection box around one of the GDI medium tanks."""
    draw = ImageDraw.Draw(scene)
    # Select the first medium tank at (5*48, 6*48)
    sel_x, sel_y = 5 * 48, 6 * 48
    sel_w, sel_h = 48, 48
    padding = 3
    x0 = sel_x - padding
    y0 = sel_y - padding
    x1 = sel_x + sel_w + padding
    y1 = sel_y + sel_h + padding

    # Draw corner brackets (C&C style selection)
    corner_len = 10
    color = (255, 255, 255)
    lw = 2

    # Top-left corner
    draw.line([(x0, y0), (x0 + corner_len, y0)], fill=color, width=lw)
    draw.line([(x0, y0), (x0, y0 + corner_len)], fill=color, width=lw)
    # Top-right corner
    draw.line([(x1, y0), (x1 - corner_len, y0)], fill=color, width=lw)
    draw.line([(x1, y0), (x1, y0 + corner_len)], fill=color, width=lw)
    # Bottom-left corner
    draw.line([(x0, y1), (x0 + corner_len, y1)], fill=color, width=lw)
    draw.line([(x0, y1), (x0, y1 - corner_len)], fill=color, width=lw)
    # Bottom-right corner
    draw.line([(x1, y1), (x1 - corner_len, y1)], fill=color, width=lw)
    draw.line([(x1, y1), (x1, y1 - corner_len)], fill=color, width=lw)

    # Also draw the cursor_attack near the Nod tanks
    cursor = load_ui("cursor_attack")
    cursor = cursor.resize((24, 24), Image.NEAREST)
    paste_alpha(scene, cursor, (13 * 48 + 12, 9 * 48 - 14))


# ---------------------------------------------------------------------------
# 11. Fog of war - darken edges
# ---------------------------------------------------------------------------
def render_fog_of_war(scene: Image.Image) -> None:
    """Apply semi-transparent dark overlay to the map edges to simulate fog."""
    fog = Image.new("RGBA", (MAP_W, SCENE_H), (0, 0, 0, 0))
    fog_draw = ImageDraw.Draw(fog)

    # Fog thickness in pixels for each edge
    fog_depth = 72  # 1.5 tiles

    # Top edge
    for i in range(fog_depth):
        alpha = int(160 * (1.0 - i / fog_depth))
        fog_draw.line([(0, i), (MAP_W, i)], fill=(0, 0, 0, alpha))

    # Bottom edge
    for i in range(fog_depth):
        y = SCENE_H - 1 - i
        alpha = int(160 * (1.0 - i / fog_depth))
        fog_draw.line([(0, y), (MAP_W, y)], fill=(0, 0, 0, alpha))

    # Left edge
    for i in range(fog_depth):
        alpha = int(140 * (1.0 - i / fog_depth))
        fog_draw.line([(i, 0), (i, SCENE_H)], fill=(0, 0, 0, alpha))

    # Right edge (near sidebar)
    for i in range(fog_depth):
        x = MAP_W - 1 - i
        alpha = int(100 * (1.0 - i / fog_depth))
        fog_draw.line([(x, 0), (x, SCENE_H)], fill=(0, 0, 0, alpha))

    # Corner intensification (darkest in corners)
    corner_size = 96
    for cy in range(corner_size):
        for cx in range(corner_size):
            dist = math.sqrt(cx * cx + cy * cy) / math.sqrt(
                corner_size * corner_size * 2)
            alpha = int(120 * max(0, 1.0 - dist * 1.5))
            if alpha > 0:
                # Top-left
                existing = fog.getpixel((cx, cy))
                new_a = min(255, existing[3] + alpha)
                fog.putpixel((cx, cy), (0, 0, 0, new_a))
                # Top-right
                rx = MAP_W - 1 - cx
                if 0 <= rx < MAP_W:
                    existing = fog.getpixel((rx, cy))
                    new_a = min(255, existing[3] + alpha)
                    fog.putpixel((rx, cy), (0, 0, 0, new_a))
                # Bottom-left
                by = SCENE_H - 1 - cy
                existing = fog.getpixel((cx, by))
                new_a = min(255, existing[3] + alpha)
                fog.putpixel((cx, by), (0, 0, 0, new_a))
                # Bottom-right
                if 0 <= rx < MAP_W:
                    existing = fog.getpixel((rx, by))
                    new_a = min(255, existing[3] + alpha)
                    fog.putpixel((rx, by), (0, 0, 0, new_a))

    paste_alpha(scene, fog, (0, 0))


# ---------------------------------------------------------------------------
# 12. Text labels
# ---------------------------------------------------------------------------
def render_labels(scene: Image.Image) -> None:
    """Add text labels using Pillow's built-in or system font."""
    draw = ImageDraw.Draw(scene)

    try:
        font = ImageFont.truetype(
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 11)
        font_tiny = ImageFont.truetype(
            "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 9)
    except Exception:
        font = ImageFont.load_default()
        font_tiny = font

    labels = [
        # (text, x, y, color)
        ("GDI Base", 1 * 48, 0 * 48 + 6, (220, 200, 50)),
        ("Nod Base", 15 * 48, 9 * 48 + 18, (220, 50, 50)),
        ("Tiberium", 6 * 48 + 6, 3 * 48 + 18, (50, 220, 50)),
        ("Tiberium", 12 * 48 + 2, 8 * 48 + 18, (50, 220, 50)),
        ("River", 9 * 48 + 8, 3 * 48, (100, 160, 255)),
    ]

    for text, x, y, color in labels:
        # Draw shadow
        draw.text((x + 1, y + 1), text, fill=(0, 0, 0), font=font)
        draw.text((x, y), text, fill=color, font=font)

    # Unit labels
    unit_labels = [
        ("MedTank", 5 * 48 + 2, 7 * 48 + 2, (200, 200, 150)),
        ("Harvester", 8 * 48, 6 * 48 + 2, (200, 200, 150)),
        ("Lt.Tank", 13 * 48, 10 * 48 + 2, (200, 150, 150)),
    ]
    for text, x, y, color in unit_labels:
        draw.text((x + 1, y + 1), text, fill=(0, 0, 0), font=font_tiny)
        draw.text((x, y), text, fill=color, font=font_tiny)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> None:
    print("Rendering C&C: Tiberian Dawn game scene...")

    # Create RGBA scene
    scene = Image.new("RGBA", (SCENE_W, SCENE_H), (0, 0, 0, 255))

    print("  [1/12] Terrain grid...")
    render_terrain(scene)

    print("  [2/12] Tiberium overlays...")
    render_tiberium(scene)

    print("  [3/12] GDI buildings...")
    print("  [4/12] Nod buildings...")
    render_buildings(scene)

    print("  [5/12] Units and infantry...")
    render_units(scene)

    print("  [6/12] Explosion effects...")
    render_explosion(scene)

    print("  [7/12] Projectile trail...")
    render_projectile_trail(scene)

    print("  [8/12] Sidebar UI...")
    render_sidebar(scene)

    print("  [9/12] Health bars...")
    render_health_bars(scene)

    print(" [10/12] Selection box...")
    render_selection_box(scene)

    print(" [11/12] Fog of war...")
    render_fog_of_war(scene)

    print(" [12/12] Text labels...")
    render_labels(scene)

    # Ensure output directory exists
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # Convert to RGB for final PNG (no alpha channel needed in output)
    final = scene.convert("RGB")
    final.save(OUTPUT_PATH, "PNG")
    print(f"\nScene saved to: {OUTPUT_PATH}")
    print(f"Image size: {final.size[0]}x{final.size[1]}")


if __name__ == "__main__":
    main()
