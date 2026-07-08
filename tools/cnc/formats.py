"""Readers for the classic Westwood file formats used by C&C: Tiberian Dawn.

Implements:
  - MIX archives (TD flavor: unencrypted, 6-byte header) + the filename hash
  - Format80 "LCW" decompression (and a simple literal-only compressor for tests)
  - Format40 XOR-delta decompression
  - SHP (TD flavor) sprite decoding
  - PAL 6-bit palettes

References: the GPL'd Tiberian Dawn source (WWLIB), XCC Mixer's documentation,
and the ModEnc/Shikadi format wikis.
"""

import struct


# ── Filename hash (TD/RA1 MIX id) ────────────────────────────────────────────

def mix_id(name: str) -> int:
    """Westwood's rolling hash over the uppercased name, 4 bytes at a time."""
    name = name.upper()
    data = name.encode("ascii")
    i = 0
    h = 0
    while i < len(data):
        a = 0
        for j in range(4):
            a >>= 8
            if i < len(data):
                a += data[i] << 24
            i += 1
        h = ((h << 1 | h >> 31) + a) & 0xFFFFFFFF
    return h


# ── MIX archives ─────────────────────────────────────────────────────────────

class MixFile:
    """TD-format .MIX: uint16 count, uint32 body size, then count index
    entries of (uint32 id, uint32 offset, uint32 size), then the body."""

    def __init__(self, data: bytes):
        self.data = data
        count, body_size = struct.unpack_from("<HI", data, 0)
        self.entries = {}
        pos = 6
        for _ in range(count):
            fid, off, size = struct.unpack_from("<III", data, pos)
            pos += 12
            self.entries[fid] = (off, size)
        self.body_start = pos
        self.body_size = body_size

    @classmethod
    def open(cls, path):
        with open(path, "rb") as f:
            return cls(f.read())

    def has(self, name: str) -> bool:
        return mix_id(name) in self.entries

    def read(self, name: str) -> bytes:
        off, size = self.entries[mix_id(name)]
        start = self.body_start + off
        return self.data[start:start + size]

    def names_present(self, candidates):
        return [n for n in candidates if self.has(n)]


# ── Format80 (LCW) ───────────────────────────────────────────────────────────

def decode_format80(src, dest_size=None):
    """LCW decompression, as used by SHP/TMP/WSA."""
    out = bytearray()
    i = 0
    n = len(src)
    while i < n:
        cmd = src[i]
        i += 1
        if cmd == 0x80:  # end marker
            break
        if cmd & 0x80 == 0:
            # Short relative copy: count = (cmd >> 4) + 3,
            # offset = ((cmd & 0x0F) << 8) | next, back from current position.
            count = (cmd >> 4) + 3
            rel = ((cmd & 0x0F) << 8) | src[i]
            i += 1
            pos = len(out) - rel
            for _ in range(count):
                out.append(out[pos])
                pos += 1
        elif cmd & 0x40 == 0:
            # 0x81..0xBF: literal run of (cmd & 0x3F) bytes.
            count = cmd & 0x3F
            out += src[i:i + count]
            i += count
        elif cmd == 0xFE:
            # Fill: uint16 count, byte value.
            count = struct.unpack_from("<H", src, i)[0]
            value = src[i + 2]
            i += 3
            out += bytes([value]) * count
        elif cmd == 0xFF:
            # Long absolute copy: uint16 count, uint16 offset (from dest start).
            count, off = struct.unpack_from("<HH", src, i)
            i += 4
            for k in range(count):
                out.append(out[off + k])
        else:
            # 0xC0..0xFD: absolute copy, count = (cmd & 0x3F) + 3, uint16 offset.
            count = (cmd & 0x3F) + 3
            off = struct.unpack_from("<H", src, i)[0]
            i += 2
            for k in range(count):
                out.append(out[off + k])
        if dest_size is not None and len(out) >= dest_size:
            break
    return bytes(out)


def encode_format80_literal(data: bytes) -> bytes:
    """Minimal valid LCW stream using only literal runs (for tests)."""
    out = bytearray()
    for start in range(0, len(data), 63):
        chunk = data[start:start + 63]
        out.append(0x80 | len(chunk))
        out += chunk
    out.append(0x80)  # end
    return bytes(out)


# ── Format40 (XOR delta) ─────────────────────────────────────────────────────

def apply_format40(src: bytes, base: bytearray) -> bytearray:
    """Apply an XOR-delta stream onto a copy of `base` and return it."""
    out = bytearray(base)
    i = 0
    pos = 0
    n = len(src)
    while i < n:
        cmd = src[i]
        i += 1
        if cmd & 0x80:
            if cmd == 0x80:
                word = struct.unpack_from("<H", src, i)[0]
                i += 2
                if word == 0:
                    break
                if word & 0x8000:
                    if word & 0x4000:
                        # Large XOR-fill.
                        count = word & 0x3FFF
                        value = src[i]
                        i += 1
                        for _ in range(count):
                            out[pos] ^= value
                            pos += 1
                    else:
                        # Large XOR from source.
                        count = word & 0x3FFF
                        for _ in range(count):
                            out[pos] ^= src[i]
                            i += 1
                            pos += 1
                else:
                    pos += word  # large skip
            else:
                pos += cmd & 0x7F  # small skip
        elif cmd == 0:
            # Small XOR-fill: byte count, byte value.
            count = src[i]
            value = src[i + 1]
            i += 2
            for _ in range(count):
                out[pos] ^= value
                pos += 1
        else:
            # Small XOR from source: cmd bytes.
            for _ in range(cmd):
                out[pos] ^= src[i]
                i += 1
                pos += 1
    return out


# ── SHP (TD) ─────────────────────────────────────────────────────────────────

class ShpFile:
    """TD SHP: fixed-size frames, each stored as LCW (0x80), XOR vs a keyframe
    (0x40), or XOR vs the previous frame (0x20)."""

    def __init__(self, data: bytes):
        (self.count, _xpos, _ypos, self.width, self.height,
         _delta, _flags) = struct.unpack_from("<7H", data, 0)
        self.frames = []
        entries = []
        pos = 14
        for _ in range(self.count + 2):
            off_fmt, ref_fmt = struct.unpack_from("<II", data, pos)
            pos += 8
            entries.append((
                off_fmt & 0xFFFFFF, off_fmt >> 24,
                ref_fmt & 0xFFFFFF, ref_fmt >> 24,
            ))

        frame_size = self.width * self.height
        decoded_by_offset = {}
        for idx in range(self.count):
            offset, fmt, ref_off, ref_fmt = entries[idx]
            if fmt == 0x80:
                frame = bytearray(decode_format80(data[offset:], frame_size))
            elif fmt == 0x40:
                # XOR against the frame stored at ref_off (a keyframe).
                base = decoded_by_offset[ref_off]
                frame = apply_format40(data[offset:], base)
            elif fmt == 0x20:
                # XOR chain against the previous frame.
                base = self.frames[idx - 1]
                frame = apply_format40(data[offset:], base)
            else:
                frame = bytearray(frame_size)
            frame = bytearray(frame[:frame_size].ljust(frame_size, b"\0"))
            decoded_by_offset[offset] = frame
            self.frames.append(frame)

    @classmethod
    def open(cls, path):
        with open(path, "rb") as f:
            return cls(f.read())


# ── PAL ──────────────────────────────────────────────────────────────────────

def load_palette(data: bytes):
    """256 RGB triples, 6-bit per channel (VGA), scaled to 8-bit."""
    pal = []
    for i in range(256):
        r, g, b = data[i * 3:i * 3 + 3]
        pal.append((min(255, r << 2), min(255, g << 2), min(255, b << 2)))
    return pal


def frame_to_rgba(frame: bytes, width: int, height: int, palette,
                  remap=None):
    """Palette-indexed frame -> flat RGBA byte list. Index 0 is transparent."""
    out = bytearray(width * height * 4)
    for i, ci in enumerate(frame[:width * height]):
        if remap and ci in remap:
            ci = remap[ci]
        if ci == 0:
            continue  # transparent
        r, g, b = palette[ci]
        out[i * 4:i * 4 + 4] = bytes((r, g, b, 255))
    return bytes(out)
