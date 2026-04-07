#!/usr/bin/env python3
"""
render_scene_v2.py - Compose a game scene from v2 sprite sheets
Output: demo/game_scene_v2.png (1280x720 full game view)
"""
import os, math
from PIL import Image, ImageDraw, ImageFont

W, H = 1280, 720
TILE = 32

# Palette
DIRT      = (140, 120, 80)
DIRT2     = (150, 130, 90)
GRASS     = (80, 110, 60)
TIBERIUM  = (50, 180, 50)
FOG       = (0, 0, 0, 160)
SIDEBAR_C = (40, 40, 40)
SIDEBAR_W = 160
GAME_W    = W - SIDEBAR_W

os.makedirs("demo", exist_ok=True)

scene = Image.new("RGB", (W, H), DIRT)
draw = ImageDraw.Draw(scene, "RGBA")

# ── Terrain ──────────────────────────────────────────────────────────────────
import random
rng = random.Random(42)

for ty in range(H // TILE + 1):
    for tx in range(GAME_W // TILE + 1):
        r = rng.random()
        if r < 0.08:
            c = GRASS
        elif r < 0.12:
            c = TIBERIUM
        else:
            noise = rng.randint(-10, 10)
            c = (DIRT[0]+noise, DIRT[1]+noise, DIRT[2]+noise)
        draw.rectangle([tx*TILE, ty*TILE, tx*TILE+TILE-1, ty*TILE+TILE-1], fill=c)

# Tiberium patch
for i in range(18):
    tx = 8 + rng.randint(0,3)
    ty = 3 + rng.randint(0,3)
    draw.rectangle([tx*TILE, ty*TILE, tx*TILE+TILE-1, ty*TILE+TILE-1],
                   fill=(50+rng.randint(0,30), 180+rng.randint(0,40), 50+rng.randint(0,20)))

# ── Grid lines (subtle) ──────────────────────────────────────────────────────
for ty in range(H // TILE + 1):
    draw.line([(0, ty*TILE), (GAME_W, ty*TILE)], fill=(0,0,0,20))
for tx in range(GAME_W // TILE + 1):
    draw.line([(tx*TILE, 0), (tx*TILE, H)], fill=(0,0,0,20))

# ── Helper: paste sprite with optional scale ─────────────────────────────────
def paste_sprite(scene, path, x, y, scale=1):
    if not os.path.exists(path):
        return
    img = Image.open(path).convert("RGBA")
    if scale != 1:
        img = img.resize((int(img.width*scale), int(img.height*scale)), Image.NEAREST)
    # Drop shadow
    shadow = Image.new("RGBA", scene.size, (0,0,0,0))
    sh_draw = ImageDraw.Draw(shadow, "RGBA")
    sh_draw.ellipse([x+2, y+img.height-4, x+img.width+2, y+img.height+4], fill=(0,0,0,60))
    scene.paste(shadow, mask=shadow)
    scene.paste(img, (x, y), img)

def paste_from_sheet(scene, sheet_path, frame_w, frame_h, frame_idx, x, y, scale=1):
    """Paste a single frame from a sprite sheet."""
    if not os.path.exists(sheet_path):
        return
    sheet = Image.open(sheet_path).convert("RGBA")
    cols = sheet.width // frame_w
    col = frame_idx % cols
    row = frame_idx // cols
    frame = sheet.crop((col*frame_w, row*frame_h, (col+1)*frame_w, (row+1)*frame_h))
    if scale != 1:
        frame = frame.resize((int(frame_w*scale), int(frame_h*scale)), Image.NEAREST)
    scene.paste(frame, (x, y), frame)

# ── GDI Base ─────────────────────────────────────────────────────────────────
# Construction yard
paste_sprite(scene, "assets/sprites/buildings/gdi_construction_yard.png", 64, 64, scale=1.5)
# Power plants
paste_sprite(scene, "assets/sprites/buildings/gdi_power_plant.png", 200, 72)
paste_sprite(scene, "assets/sprites/buildings/gdi_power_plant.png", 256, 72)
# Barracks
paste_sprite(scene, "assets/sprites/buildings/gdi_barracks.png", 200, 130)
# War Factory
paste_sprite(scene, "assets/sprites/buildings/gdi_war_factory.png", 64, 160)
# Radar
paste_sprite(scene, "assets/sprites/buildings/gdi_radar.png", 310, 130)

# ── Nod Base (far side) ──────────────────────────────────────────────────────
paste_sprite(scene, "assets/sprites/buildings/nod_construction_yard.png", 780, 380, scale=1.2)
paste_sprite(scene, "assets/sprites/buildings/nod_power_plant.png", 700, 390)
paste_sprite(scene, "assets/sprites/buildings/nod_barracks.png", 700, 450)
paste_sprite(scene, "assets/sprites/buildings/nod_war_factory.png", 780, 450)
paste_sprite(scene, "assets/sprites/buildings/nod_guard_tower.png", 660, 340)
paste_sprite(scene, "assets/sprites/buildings/nod_guard_tower.png", 900, 340)

# ── GDI Units (patrol line) ──────────────────────────────────────────────────
tank_sheet = "assets/sheets/gdi_medium_tank_sheet.png"
mammoth_sheet = "assets/sheets/gdi_mammoth_tank_sheet.png"
inf_sheet = "assets/sheets/gdi_minigunner_sheet.png"

# Medium tanks facing East (facing 2)
for i in range(3):
    paste_from_sheet(scene, tank_sheet, 32, 32, 2, 360 + i*48, 260)

# Mammoth tank facing SE (facing 3)
paste_from_sheet(scene, mammoth_sheet, 40, 32, 3, 480, 300)

# Infantry line
for i in range(4):
    paste_from_sheet(scene, inf_sheet, 24, 24, i % 8, 340 + i*28, 320)

# ── Nod Units ────────────────────────────────────────────────────────────────
nod_tank_sheet = "assets/sheets/nod_light_tank_sheet.png"
stealth_sheet  = "assets/sheets/nod_stealth_tank_sheet.png"
nod_inf_sheet  = "assets/sheets/nod_rifle_infantry_sheet.png"

# Light tanks facing West (facing 6)
for i in range(2):
    paste_from_sheet(scene, nod_tank_sheet, 32, 32, 6, 640 + i*44, 300)

# Stealth tank
paste_from_sheet(scene, stealth_sheet, 32, 32, 7, 620, 350)

# Nod infantry
for i in range(3):
    paste_from_sheet(scene, nod_inf_sheet, 24, 24, (i+4) % 8, 630 + i*26, 380)

# ── Explosion effects (mid-field) ────────────────────────────────────────────
exp_sheet = "assets/sheets/effects_sheet.png"
# Frame 2 = mid-explosion
paste_from_sheet(scene, exp_sheet, 32, 32, 2, 560, 280)
paste_from_sheet(scene, exp_sheet, 32, 32, 3, 596, 268)

# ── Fog of War (right edge) ──────────────────────────────────────────────────
fog_layer = Image.new("RGBA", (W, H), (0,0,0,0))
fog_draw = ImageDraw.Draw(fog_layer, "RGBA")
# Gradient fog on right portion
for x in range(200):
    alpha = int(x / 200 * 180)
    fog_draw.line([(GAME_W - 200 + x, 0), (GAME_W - 200 + x, H)], fill=(0,0,0,alpha))
scene.paste(fog_layer, mask=fog_layer)

# ── Selection circle under selected unit ─────────────────────────────────────
sel_draw = ImageDraw.Draw(scene, "RGBA")
sel_draw.ellipse([360, 268, 392, 284], outline=(80, 255, 80, 200), width=2)

# Health bars
for i in range(3):
    bx = 360 + i*48
    sel_draw.rectangle([bx, 258, bx+30, 261], fill=(20, 20, 20, 180))
    sel_draw.rectangle([bx, 258, bx+28, 261], fill=(80, 220, 80, 220))

# ── Sidebar ───────────────────────────────────────────────────────────────────
sb = ImageDraw.Draw(scene)
# Background
scene.paste(Image.new("RGB", (SIDEBAR_W, H), (30, 30, 30)), (GAME_W, 0))
sb = ImageDraw.Draw(scene)

# Credits
sb.rectangle([GAME_W+4, 4, W-4, 24], fill=(50, 50, 50))
sb.text((GAME_W+10, 7), "$ 5,000", fill=(80, 220, 80))

# Minimap
mm_x, mm_y = GAME_W + 4, 30
mm_w, mm_h = SIDEBAR_W - 8, 90
sb.rectangle([mm_x, mm_y, mm_x+mm_w, mm_y+mm_h], fill=(20, 40, 20))
sb.rectangle([mm_x, mm_y, mm_x+mm_w, mm_y+mm_h], outline=(60, 80, 60))

# Minimap dots
scale_x = mm_w / GAME_W
scale_y = mm_h / H
# GDI structures
for bx, by in [(64,64),(200,72),(256,72),(200,130),(64,160)]:
    sb.rectangle([mm_x+int(bx*scale_x), mm_y+int(by*scale_y),
                  mm_x+int(bx*scale_x)+3, mm_y+int(by*scale_y)+3], fill=(180,180,80))
# GDI units
for ux, uy in [(360,260),(408,260),(456,260),(480,300)]:
    sb.rectangle([mm_x+int(ux*scale_x), mm_y+int(uy*scale_y),
                  mm_x+int(ux*scale_x)+2, mm_y+int(uy*scale_y)+2], fill=(80,200,80))
# Nod
for bx, by in [(780,380),(700,390),(780,450)]:
    sb.rectangle([mm_x+int(bx*scale_x), mm_y+int(by*scale_y),
                  mm_x+int(bx*scale_x)+3, mm_y+int(by*scale_y)+3], fill=(180,80,80))

# Build queue
sb.text((GAME_W+6, 128), "BUILD:", fill=(160, 160, 160))
items = [
    ("assets/ui/icon_bar.png", "BARRACKS", 60),
    ("assets/ui/icon_pp.png",  "POWER",    35),
    ("assets/ui/icon_ref.png", "REFINERY", 12),
]
for idx, (icon_path, label, pct) in enumerate(items):
    iy = 144 + idx * 54
    sb.rectangle([GAME_W+4, iy, W-4, iy+50], fill=(50,50,50))
    if os.path.exists(icon_path):
        icon = Image.open(icon_path).convert("RGBA").resize((40, 40), Image.NEAREST)
        scene.paste(icon, (GAME_W+6, iy+5), icon)
    sb.text((GAME_W+50, iy+6), label, fill=(200, 200, 200))
    # Progress bar
    sb.rectangle([GAME_W+50, iy+26, W-8, iy+34], fill=(30,30,30))
    bar_w = int((W-8 - GAME_W-50) * pct / 100)
    if bar_w > 0:
        sb.rectangle([GAME_W+50, iy+26, GAME_W+50+bar_w, iy+34], fill=(80,180,80))
    sb.text((GAME_W+50, iy+36), f"{pct}%", fill=(120,200,120))

# Selected unit info
sel_y = 320
sb.rectangle([GAME_W+4, sel_y, W-4, sel_y+90], fill=(45,45,45))
sb.text((GAME_W+8, sel_y+4), "MEDIUM TANK", fill=(220, 200, 120))
sb.text((GAME_W+8, sel_y+18), "HP: 400/400", fill=(80, 220, 80))
sb.text((GAME_W+8, sel_y+32), "ARMOR: Heavy", fill=(160, 160, 160))
sb.text((GAME_W+8, sel_y+46), "WPN: 75mm", fill=(160, 160, 160))
sb.text((GAME_W+8, sel_y+60), "STATUS: Moving", fill=(120, 180, 220))

# Command buttons
sb.text((GAME_W+8, sel_y+76), "[M]ove [A]ttack [S]top", fill=(120,120,120))

# Divider
sb.line([(GAME_W, sel_y), (W, sel_y)], fill=(60,60,60), width=1)

# Game title bar
sb.rectangle([0, 0, GAME_W, 18], fill=(20, 20, 20))
sb.text((4, 2), "C&C: TIBERIAN DAWN  |  GDI CAMPAIGN  |  TICK 1247  |  [P]ause", fill=(140, 140, 140))

print(f"Scene size: {scene.size}")
out = "demo/game_scene_v2.png"
scene.save(out, "PNG")
print(f"Saved: {out}")
