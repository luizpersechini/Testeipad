#!/usr/bin/env python3
"""Self-tests for the Westwood format readers, using synthetic files
(round-trips through our own encoders). Run: python3 tools/cnc/test_formats.py"""

import struct
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from formats import (  # noqa: E402
    mix_id, MixFile, decode_format80, encode_format80_literal,
    apply_format40, ShpFile, load_palette, frame_to_rgba,
)

failures = []


def check(name, cond, detail=""):
    if cond:
        print(f"  ok  {name}")
    else:
        failures.append(name)
        print(f" FAIL {name} {detail}")


# ── mix_id: verify against the algorithm's known fixed points ────────────────
# For a 1-char name the block is padded by three >>8 shifts, so 'A' (0x41)
# ends in the low byte and one rotl of seed 0 leaves it there: id == 0x41.
check("mix_id('A') matches hand computation", mix_id("A") == 0x41)
# A full 4-char block 'ABCD' packs little-endian: 0x44434241.
check("mix_id('ABCD') packs little-endian", mix_id("ABCD") == 0x44434241)
check("mix_id is case-insensitive", mix_id("conquer.mix") == mix_id("CONQUER.MIX"))
check("mix_id differs across names", mix_id("MTNK.SHP") != mix_id("HTNK.SHP"))

# ── MIX round trip ───────────────────────────────────────────────────────────
payloads = {"MTNK.SHP": b"tank-bytes", "TEMPERAT.PAL": bytes(range(256)) * 3}
body = b""
index = b""
for name, blob in payloads.items():
    index += struct.pack("<III", mix_id(name), len(body), len(blob))
    body += blob
mix_data = struct.pack("<HI", len(payloads), len(body)) + index + body
mix = MixFile(mix_data)
check("MIX finds present names", mix.has("mtnk.shp") and mix.has("TEMPERAT.PAL"))
check("MIX rejects absent names", not mix.has("HTNK.SHP"))
check("MIX reads correct bytes", mix.read("MTNK.SHP") == b"tank-bytes")
check("MIX second entry offset", mix.read("TEMPERAT.PAL")[:4] == bytes([0, 1, 2, 3]))

# ── Format80 ─────────────────────────────────────────────────────────────────
raw = bytes((i * 7 + 3) % 251 for i in range(1000))
check("LCW literal round trip", decode_format80(encode_format80_literal(raw)) == raw)
# Fill command: 0xFE count value.
fill = bytes([0xFE]) + struct.pack("<H", 500) + bytes([0xAB]) + bytes([0x80])
check("LCW fill command", decode_format80(fill) == bytes([0xAB]) * 500)
# Absolute copy: literal 5 bytes then copy 3 from start.
stream = bytes([0x85]) + b"HELLO" + bytes([0xC0]) + struct.pack("<H", 0) + bytes([0x80])
check("LCW absolute copy", decode_format80(stream) == b"HELLOHEL")
# Short relative copy: literal 4, then copy 3 from 4 back (cmd 0x00, rel 4).
stream = bytes([0x84]) + b"ABCD" + bytes([0x00, 0x04]) + bytes([0x80])
check("LCW relative copy", decode_format80(stream) == b"ABCDABC")

# ── Format40 ─────────────────────────────────────────────────────────────────
base = bytearray(b"\x10" * 32)
# Skip 4, XOR 3 source bytes (0x03 a b c), small XOR-fill 5 of 0xFF (0x00 05 FF), end.
delta = bytes([0x84, 0x03, 0x01, 0x02, 0x03, 0x00, 0x05, 0xFF, 0x80]) + struct.pack("<H", 0)
out = apply_format40(delta, base)
check("F40 skip preserved", out[:4] == b"\x10" * 4)
check("F40 xor from source", bytes(out[4:7]) == bytes([0x11, 0x12, 0x13]))
check("F40 xor fill", bytes(out[7:12]) == bytes([0xEF] * 5))
check("F40 tail untouched", out[12:] == b"\x10" * 20)

# ── SHP synthetic (LCW frames + one XOR-chain frame) ─────────────────────────
W, H = 8, 4
frame0 = bytes((i % 5) for i in range(W * H))
frame1 = bytes(((i + 2) % 5) for i in range(W * H))
f0 = encode_format80_literal(frame0)
# Frame 1 as Format20 (XOR vs previous): xor stream = full XOR from source.
xor_bytes = bytes(a ^ b for a, b in zip(frame0, frame1))
f1 = bytes([0x80]) + struct.pack("<H", 0x8000 | len(xor_bytes)) + xor_bytes \
    + bytes([0x80]) + struct.pack("<H", 0)
header = struct.pack("<7H", 2, 0, 0, W, H, max(len(f0), len(f1)), 0)
off0 = 14 + 8 * 4
off1 = off0 + len(f0)
entries = struct.pack("<II", off0 | (0x80 << 24), 0)
entries += struct.pack("<II", off1 | (0x20 << 24), 0)
entries += struct.pack("<II", (off1 + len(f1)) | (0x80 << 24), 0)  # end sentinel
entries += struct.pack("<II", 0, 0)
shp_data = header + entries + f0 + f1
shp = ShpFile(shp_data)
check("SHP header", (shp.count, shp.width, shp.height) == (2, W, H))
check("SHP LCW frame", bytes(shp.frames[0]) == frame0)
check("SHP XOR-chain frame", bytes(shp.frames[1]) == frame1)

# ── Palette + RGBA ───────────────────────────────────────────────────────────
pal = load_palette(bytes([63, 0, 0] * 256))
check("PAL 6-bit scaling", pal[0] == (252, 0, 0))
rgba = frame_to_rgba(bytes([0, 1]), 2, 1, pal)
check("index 0 transparent", rgba[3] == 0 and rgba[7] == 255)
rgba = frame_to_rgba(bytes([176]), 1, 1, pal, remap={176: 1})
check("remap applied", rgba[:3] == bytes(pal[1]))

print()
if failures:
    sys.exit(f"{len(failures)} failure(s): {failures}")
print("All format self-tests passed.")
