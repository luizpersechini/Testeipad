"""
Base style definitions for all C&C asset prompts.

These style strings are appended to every generation prompt to ensure
visual consistency across all generated assets.
"""

# Core visual style applied to ALL assets
STYLE_BASE = (
    "top-down isometric pixel art, military RTS game style, "
    "Command and Conquer inspired, retro game sprite, "
    "clean pixel art with defined edges, "
    "muted military color palette with tactical feel, "
    "slight shadow underneath, centered on canvas"
)

# Negative prompt applied to all generations
NEGATIVE_BASE = (
    "blurry, low quality, text, watermark, signature, label, "
    "realistic photo, 3d render, gradient background, noisy, "
    "jpeg artifacts, cropped, out of frame, multiple objects, "
    "modern UI elements, cartoon style, anime, chibi, "
    "excessive detail, busy background"
)

# Style variants by faction
FACTION_STYLES = {
    "gdi": (
        "golden-tan and olive green military colors, "
        "modern western military aesthetic, NATO-inspired, "
        "angular utilitarian design, GDI eagle emblem"
    ),
    "nod": (
        "red and black color scheme, sleek angular design, "
        "Brotherhood of Nod aesthetic, scorpion/serpent motifs, "
        "futuristic militant appearance, crimson accents"
    ),
    "neutral": (
        "civilian colors, muted earth tones, "
        "non-military appearance, neutral gray"
    ),
}

# Style for different asset categories
CATEGORY_STYLES = {
    "vehicle": (
        "military vehicle sprite, mechanical detail, "
        "treads or wheels visible, turret if applicable, "
        "approximately 48x48 pixel apparent size"
    ),
    "infantry": (
        "small military soldier sprite, human figure, "
        "weapon visible, approximately 24x24 pixel apparent size, "
        "simple but recognizable silhouette"
    ),
    "building": (
        "military base building sprite, architectural structure, "
        "roof and walls visible from above, "
        "approximately 72x72 pixel apparent size for large buildings"
    ),
    "terrain": (
        "tileable terrain texture, seamless edges, "
        "top-down view, natural ground texture, "
        "no objects or structures, uniform coverage"
    ),
    "effect": (
        "visual effect sprite, bright and dynamic, "
        "transparent background, particle-like quality"
    ),
}


def build_prompt(description: str,
                 category: str = "vehicle",
                 faction: str = "neutral",
                 extra_style: str = "") -> str:
    """Build a complete generation prompt from components."""
    parts = [description]
    parts.append(STYLE_BASE)

    if category in CATEGORY_STYLES:
        parts.append(CATEGORY_STYLES[category])

    if faction in FACTION_STYLES:
        parts.append(FACTION_STYLES[faction])

    if extra_style:
        parts.append(extra_style)

    return ", ".join(parts)


def build_negative(extra: str = "") -> str:
    """Build a complete negative prompt."""
    if extra:
        return f"{NEGATIVE_BASE}, {extra}"
    return NEGATIVE_BASE
