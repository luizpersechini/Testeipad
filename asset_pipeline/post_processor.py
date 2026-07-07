"""
Image Post-Processing Pipeline

Handles all post-processing of AI-generated images:
  - Background removal
  - Sprite sheet assembly
  - Faction color remapping
  - Shadow generation
  - Image scaling and format conversion
  - Tiling validation for terrain
"""

import os
import io
import json
import logging
from typing import List, Tuple, Optional
from dataclasses import dataclass

logger = logging.getLogger(__name__)


@dataclass
class ProcessedImage:
    """Result of post-processing an image."""
    data: bytes
    width: int
    height: int
    format: str = "png"
    path: Optional[str] = None


class PostProcessor:
    """Image post-processing for game asset preparation."""

    # C&C faction color palettes for remapping
    FACTION_COLORS = {
        "gdi": {
            "primary": (218, 165, 32),     # Gold
            "secondary": (107, 142, 35),   # Olive
            "accent": (210, 180, 80),      # Light gold
        },
        "nod": {
            "primary": (180, 0, 0),        # Red
            "secondary": (40, 0, 0),       # Dark red
            "accent": (220, 50, 50),       # Bright red
        },
        "neutral": {
            "primary": (128, 128, 128),    # Gray
            "secondary": (96, 96, 96),     # Dark gray
            "accent": (160, 160, 160),     # Light gray
        },
    }

    def __init__(self, output_base: str = "assets"):
        self.output_base = output_base

    def remove_background(self, image_data: bytes) -> bytes:
        """Remove background from an image, making it transparent."""
        try:
            from PIL import Image
            img = Image.open(io.BytesIO(image_data)).convert("RGBA")

            pixels = img.load()
            w, h = img.size

            # Sample corners to determine background color
            corners = [
                pixels[0, 0], pixels[w-1, 0],
                pixels[0, h-1], pixels[w-1, h-1]
            ]
            bg_r = sum(c[0] for c in corners) // 4
            bg_g = sum(c[1] for c in corners) // 4
            bg_b = sum(c[2] for c in corners) // 4

            threshold = 30
            for y in range(h):
                for x in range(w):
                    r, g, b, a = pixels[x, y]
                    if (abs(r - bg_r) < threshold and
                        abs(g - bg_g) < threshold and
                        abs(b - bg_b) < threshold):
                        pixels[x, y] = (r, g, b, 0)

            buf = io.BytesIO()
            img.save(buf, format="PNG")
            return buf.getvalue()

        except ImportError:
            logger.warning("Pillow not installed, skipping background removal")
            return image_data

    def create_sprite_sheet(self, frames: List[bytes],
                            frame_width: int, frame_height: int,
                            columns: int) -> bytes:
        """Assemble multiple frame images into a sprite sheet."""
        try:
            from PIL import Image

            rows = (len(frames) + columns - 1) // columns
            sheet_w = columns * frame_width
            sheet_h = rows * frame_height
            sheet = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))

            for i, frame_data in enumerate(frames):
                frame_img = Image.open(io.BytesIO(frame_data)).convert("RGBA")
                frame_img = frame_img.resize((frame_width, frame_height),
                                             Image.Resampling.NEAREST)
                col = i % columns
                row = i // columns
                sheet.paste(frame_img, (col * frame_width, row * frame_height))

            buf = io.BytesIO()
            sheet.save(buf, format="PNG")
            return buf.getvalue()

        except ImportError:
            logger.error("Pillow required for sprite sheet assembly")
            return b""

    def generate_faction_variant(self, image_data: bytes,
                                  source_faction: str,
                                  target_faction: str) -> bytes:
        """Recolor an image from one faction color to another."""
        try:
            from PIL import Image
            img = Image.open(io.BytesIO(image_data)).convert("RGBA")

            src = self.FACTION_COLORS.get(source_faction, self.FACTION_COLORS["neutral"])
            tgt = self.FACTION_COLORS.get(target_faction, self.FACTION_COLORS["neutral"])

            pixels = img.load()
            w, h = img.size
            threshold = 50

            for y in range(h):
                for x in range(w):
                    r, g, b, a = pixels[x, y]
                    if a == 0:
                        continue
                    for color_key in ["primary", "secondary", "accent"]:
                        sr, sg, sb = src[color_key]
                        if (abs(r - sr) < threshold and
                            abs(g - sg) < threshold and
                            abs(b - sb) < threshold):
                            tr, tg, tb = tgt[color_key]
                            dr = r - sr
                            dg = g - sg
                            db = b - sb
                            pixels[x, y] = (
                                max(0, min(255, tr + dr)),
                                max(0, min(255, tg + dg)),
                                max(0, min(255, tb + db)),
                                a
                            )
                            break

            buf = io.BytesIO()
            img.save(buf, format="PNG")
            return buf.getvalue()

        except ImportError:
            logger.warning("Pillow not installed")
            return image_data

    def generate_shadow(self, image_data: bytes,
                        offset_x: int = 3, offset_y: int = 3,
                        opacity: int = 80) -> bytes:
        """Generate a drop shadow for a sprite."""
        try:
            from PIL import Image, ImageFilter
            img = Image.open(io.BytesIO(image_data)).convert("RGBA")
            w, h = img.size

            shadow = Image.new("RGBA", (w + abs(offset_x), h + abs(offset_y)),
                              (0, 0, 0, 0))

            # Create shadow from alpha channel
            for y in range(h):
                for x in range(w):
                    _, _, _, a = img.getpixel((x, y))
                    if a > 0:
                        sx = x + max(0, offset_x)
                        sy = y + max(0, offset_y)
                        if 0 <= sx < shadow.size[0] and 0 <= sy < shadow.size[1]:
                            shadow.putpixel((sx, sy),
                                           (0, 0, 0, min(a, opacity)))

            shadow = shadow.filter(ImageFilter.GaussianBlur(1))
            shadow.paste(img, (0, 0), img)

            buf = io.BytesIO()
            shadow.save(buf, format="PNG")
            return buf.getvalue()

        except ImportError:
            return image_data

    def scale_image(self, image_data: bytes, scale: int = 2,
                    nearest: bool = True) -> bytes:
        """Scale an image by an integer factor."""
        try:
            from PIL import Image
            img = Image.open(io.BytesIO(image_data))
            new_size = (img.width * scale, img.height * scale)
            method = Image.Resampling.NEAREST if nearest else Image.Resampling.LANCZOS
            img = img.resize(new_size, method)

            buf = io.BytesIO()
            img.save(buf, format="PNG")
            return buf.getvalue()

        except ImportError:
            return image_data

    def validate_tileable(self, image_data: bytes) -> bool:
        """Check if a terrain tile is properly seamless."""
        try:
            from PIL import Image
            img = Image.open(io.BytesIO(image_data)).convert("RGB")
            pixels = img.load()
            w, h = img.size
            threshold = 40
            edge_mismatch = 0
            total_edge = 0

            # Check horizontal edges
            for x in range(w):
                r1, g1, b1 = pixels[x, 0]
                r2, g2, b2 = pixels[x, h-1]
                total_edge += 1
                if (abs(r1-r2) > threshold or abs(g1-g2) > threshold or
                    abs(b1-b2) > threshold):
                    edge_mismatch += 1

            # Check vertical edges
            for y in range(h):
                r1, g1, b1 = pixels[0, y]
                r2, g2, b2 = pixels[w-1, y]
                total_edge += 1
                if (abs(r1-r2) > threshold or abs(g1-g2) > threshold or
                    abs(b1-b2) > threshold):
                    edge_mismatch += 1

            mismatch_ratio = edge_mismatch / max(total_edge, 1)
            return mismatch_ratio < 0.3

        except ImportError:
            return True

    def save_image(self, image_data: bytes, relative_path: str) -> str:
        """Save image data to the output directory."""
        full_path = os.path.join(self.output_base, relative_path)
        os.makedirs(os.path.dirname(full_path), exist_ok=True)
        with open(full_path, 'wb') as f:
            f.write(image_data)
        return full_path
