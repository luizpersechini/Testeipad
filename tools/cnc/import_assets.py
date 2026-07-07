#!/usr/bin/env python3
"""Import original C&C: Tiberian Dawn assets (from the official freeware
release, which you must download yourself) into the web game.

Usage:
    python3 tools/cnc/import_assets.py /path/to/game/files

Where /path/to/game/files contains the game's .MIX archives (at minimum
CONQUER.MIX and TEMPERAT.MIX; optionally GENERAL.MIX, SOUNDS.MIX/SPEECH.MIX,
MOVIES.MIX or loose .AUD/.VQA files). Output goes to assets/original/, which
the game loads in preference to the placeholder art. That directory is
git-ignored: the freeware assets are for local play, not for committing.

Requires: Pillow (pip install pillow). Audio/video conversion additionally
uses ffmpeg if it is on PATH (brew install ffmpeg) - ffmpeg natively decodes
Westwood AUD and VQA.
"""

import json
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from formats import MixFile, ShpFile, load_palette, frame_to_rgba  # noqa: E402

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required: pip install pillow")

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "original")

# TD sprite frames face north at 0 and rotate counter-clockwise through 32
# frames; the web game's sheets are 8 columns rotating clockwise from north.
FACING8_TO_FRAME32 = [(32 - 4 * k) % 32 for k in range(8)]

# Vehicles: sheet name in the web game -> SHP filename inside CONQUER.MIX.
VEHICLES = {
    "gdi_medium_tank": "MTNK.SHP",
    "gdi_mammoth_tank": "HTNK.SHP",
    "gdi_humvee": "JEEP.SHP",
    "gdi_apc": "APC.SHP",
    "gdi_mlrs": "MLRS.SHP",
    "gdi_harvester": "HARV.SHP",
    "gdi_mcv": "MCV.SHP",
    "nod_light_tank": "LTNK.SHP",
    "nod_stealth_tank": "STNK.SHP",
    "nod_buggy": "BGGY.SHP",
    "nod_recon_bike": "BIKE.SHP",
    "nod_artillery": "ARTY.SHP",
    "nod_flame_tank": "FTNK.SHP",
    "nod_harvester": "HARV.SHP",
    "nod_mcv": "MCV.SHP",
}

# Infantry: (SHP, stand frame base, walk cycle base, frames per facing).
# TD infantry animation layout: 8 standing frames (one per facing), then the
# walk cycle as 6 frames per facing. If the first import looks scrambled,
# tune WALK_BASE/WALK_STRIDE here - it is the only knob.
INFANTRY = {
    "gdi_minigunner": "E1.SHP",
    "gdi_grenadier": "E2.SHP",
    "gdi_rocket_soldier": "E3.SHP",
    "nod_rifle_infantry": "E1.SHP",
    "nod_rocket_soldier": "E3.SHP",
    "nod_flamethrower": "E4.SHP",
    "nod_chemical_warrior": "E5.SHP",
}
WALK_BASE = 8
WALK_STRIDE = 6

# Buildings: web building sprite id -> (SHP, faction rows wanted).
BUILDINGS = {
    "construction_yard": "FACT.SHP",
    "power_plant": "NUKE.SHP",
    "adv_power_plant": "NUK2.SHP",
    "refinery": "PROC.SHP",
    "silo": "SILO.SHP",
    "barracks": "PYLE.SHP",
    "hand_of_nod": "HAND.SHP",
    "war_factory": "WEAP.SHP",
    "comm_center": "HQ.SHP",
    "adv_comm_center": "EYE.SHP",
    "guard_tower": "GTWR.SHP",
    "adv_guard_tower": "ATWR.SHP",
    "gun_turret": "GUN.SHP",
    "obelisk": "OBLI.SHP",
    "temple": "TMPL.SHP",
}

# House-color remap: TD reserves palette indices 176..191 for the owner color
# (gold by default). Nod's classic scheme remaps them to the red ramp 32..47.
NOD_REMAP = {176 + i: 32 + i for i in range(16)}

AUDIO_NAMES = [
    "AWAIT1.AUD", "ACKNO.AUD", "YESSIR1.AUD", "REPORT1.AUD", "RITAWAY.AUD",
    "UNITRDY.AUD", "CONSTRU2.AUD", "XPLOBIG4.AUD", "XPLOS.AUD", "GUN18.AUD",
    "TNKFIRE4.AUD", "ROCKET1.AUD", "FLAMER2.AUD", "MGUN11.AUD", "BASEATK1.AUD",
    "NUYELL1.AUD", "CASHTURN.AUD",
]


def find_mixes(src_dir):
    mixes = {}
    for root, _dirs, files in os.walk(src_dir):
        for f in files:
            if f.lower().endswith(".mix"):
                try:
                    mixes[f.upper()] = MixFile.open(os.path.join(root, f))
                except Exception as e:  # noqa: BLE001
                    print(f"  ! could not parse {f}: {e}")
    return mixes


def read_from_any(mixes, name):
    for mix in mixes.values():
        if mix.has(name):
            return mix.read(name)
    return None


def frames_to_sheet(shp, palette, frame_indices, cols, rows, remap=None):
    w, h = shp.width, shp.height
    sheet = Image.new("RGBA", (cols * w, rows * h), (0, 0, 0, 0))
    for i, fi in enumerate(frame_indices):
        if fi >= shp.count:
            continue
        rgba = frame_to_rgba(shp.frames[fi], w, h, palette, remap)
        img = Image.frombytes("RGBA", (w, h), rgba)
        sheet.paste(img, ((i % cols) * w, (i // cols) * h))
    return sheet


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    src_dir = sys.argv[1]
    os.makedirs(OUT_DIR, exist_ok=True)

    print(f"Scanning {src_dir} for .MIX archives...")
    mixes = find_mixes(src_dir)
    if not mixes:
        sys.exit("No .MIX files found. Point this at your freeware game folder.")
    print(f"  found: {', '.join(sorted(mixes))}")

    pal_data = read_from_any(mixes, "TEMPERAT.PAL") or read_from_any(mixes, "PALETTE.PAL")
    if not pal_data:
        sys.exit("No palette (TEMPERAT.PAL) found - is TEMPERAT.MIX present?")
    palette = load_palette(pal_data)

    manifest = {"sheets": {}}

    def emit(sheet_name, image, kind, frame_w, frame_h, cols, rows):
        path = os.path.join(OUT_DIR, f"{sheet_name}.png")
        image.save(path)
        manifest["sheets"][sheet_name] = {
            "file": f"{sheet_name}.png", "kind": kind,
            "frameW": frame_w, "frameH": frame_h, "cols": cols, "rows": rows,
        }
        print(f"  + {sheet_name}.png ({cols}x{rows} of {frame_w}x{frame_h})")

    print("Vehicles:")
    for sheet_name, shp_name in VEHICLES.items():
        data = read_from_any(mixes, shp_name)
        if not data:
            print(f"  - {shp_name} not found, skipped")
            continue
        shp = ShpFile(data)
        remap = NOD_REMAP if sheet_name.startswith("nod_") else None
        img = frames_to_sheet(shp, palette, FACING8_TO_FRAME32, 8, 1, remap)
        emit(sheet_name, img, "vehicle", shp.width, shp.height, 8, 1)

    print("Infantry:")
    for sheet_name, shp_name in INFANTRY.items():
        data = read_from_any(mixes, shp_name)
        if not data:
            print(f"  - {shp_name} not found, skipped")
            continue
        shp = ShpFile(data)
        remap = NOD_REMAP if sheet_name.startswith("nod_") else None
        # Rows: 3 walk frames; columns: 8 facings (clockwise from north).
        indices = []
        for row in range(3):
            for facing in range(8):
                indices.append(WALK_BASE + facing * WALK_STRIDE + row * 2)
        img = frames_to_sheet(shp, palette, indices, 8, 3, remap)
        emit(sheet_name, img, "infantry", shp.width, shp.height, 8, 3)

    print("Buildings:")
    for sprite_id, shp_name in BUILDINGS.items():
        data = read_from_any(mixes, shp_name)
        if not data:
            print(f"  - {shp_name} not found, skipped")
            continue
        shp = ShpFile(data)
        # Row 0: GDI colors, row 1: Nod remap - same layout the game already uses.
        img = Image.new("RGBA", (shp.width, shp.height * 2), (0, 0, 0, 0))
        img.paste(frames_to_sheet(shp, palette, [0], 1, 1), (0, 0))
        img.paste(frames_to_sheet(shp, palette, [0], 1, 1, NOD_REMAP), (0, shp.height))
        emit(f"building_{sprite_id}", img, "building_pair", shp.width, shp.height, 1, 2)

    # Audio / video via ffmpeg (optional).
    ffmpeg = shutil.which("ffmpeg")
    audio_out = os.path.join(OUT_DIR, "audio")
    if ffmpeg:
        os.makedirs(audio_out, exist_ok=True)
        print("Audio (via ffmpeg):")
        for name in AUDIO_NAMES:
            data = read_from_any(mixes, name)
            if not data:
                continue
            raw = os.path.join(audio_out, name)
            with open(raw, "wb") as f:
                f.write(data)
            ogg = raw.rsplit(".", 1)[0].lower() + ".ogg"
            r = subprocess.run([ffmpeg, "-y", "-loglevel", "error", "-i", raw, ogg],
                               check=False)
            os.remove(raw)
            if r.returncode == 0:
                print(f"  + audio/{os.path.basename(ogg)}")
        print("Video: extracting .VQA (convert with: ffmpeg -i file.vqa file.mp4)")
        video_out = os.path.join(OUT_DIR, "video")
        for mix in mixes.values():
            pass  # VQA names vary per disc; loose .VQA handling below
        for root, _dirs, files in os.walk(src_dir):
            for f in files:
                if f.lower().endswith(".vqa"):
                    os.makedirs(video_out, exist_ok=True)
                    mp4 = os.path.join(video_out, f.rsplit(".", 1)[0].lower() + ".mp4")
                    subprocess.run([ffmpeg, "-y", "-loglevel", "error",
                                    "-i", os.path.join(root, f), mp4], check=False)
                    print(f"  + video/{os.path.basename(mp4)}")
    else:
        print("ffmpeg not found - skipping audio/video (brew install ffmpeg)")

    with open(os.path.join(OUT_DIR, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=2)
    print(f"\nDone. Manifest with {len(manifest['sheets'])} sheets written to "
          f"assets/original/. Reload the game - it picks these up automatically.")


if __name__ == "__main__":
    main()
