# Command & Conquer: Tiberian Dawn - Modern Port

A modern port of the classic **Command & Conquer: Tiberian Dawn** (1995) by Westwood Studios, targeting PC (Windows/macOS/Linux) and web browsers.

Based on the [original source code](https://github.com/electronicarts/CnC_Tiberian_Dawn) released under GPL v3 by Electronic Arts.

## Project Structure

```
├── original_source/         # Original C&C Tiberian Dawn source (GPL v3)
├── engine/                  # Modern game engine
│   ├── core/                # Platform abstraction, renderer, audio
│   │   ├── platform.h       # Cross-platform interface
│   │   ├── platform_sdl.cpp # Desktop backend (SDL2)
│   │   ├── platform_web.cpp # Browser backend (Emscripten)
│   │   ├── renderer_sdl.cpp # SDL2+OpenGL renderer
│   │   ├── audio_sdl.cpp    # SDL2_mixer audio
│   │   └── game_engine.*    # Main engine loop & state machine
│   ├── graphics/            # Sprite & tilemap systems
│   │   ├── sprite.h         # Sprite sheets, animation, faction colors
│   │   └── tilemap.h        # Tile-based map with fog of war
│   ├── game/                # Game logic (ported from original)
│   │   ├── game_types.h     # All unit/building/infantry type definitions
│   │   ├── game_data.cpp    # Complete data registry (stats from original)
│   │   └── unit.h           # Entity system, combat, pathfinding
│   └── network/             # Multiplayer (future)
├── asset_pipeline/          # AI-powered asset generation (Python)
│   ├── asset_factory.py     # Main orchestrator
│   ├── api_client.py        # Leonardo.ai / OpenAI / Stability clients
│   ├── post_processor.py    # Background removal, sprite sheets, shadows
│   ├── asset_manifest.py    # Asset tracking & caching
│   └── prompts/             # AI prompt templates for all asset types
│       ├── units.py         # Vehicle sprite prompts
│       ├── buildings.py     # Building sprite prompts
│       ├── infantry.py      # Infantry sprite prompts
│       ├── terrain.py       # Terrain tile prompts
│       └── effects.py       # Visual effects prompts
├── assets/                  # Generated game assets
├── web/                     # Web build shell & styles
├── tools/                   # Build & setup scripts
├── CMakeLists.txt           # CMake build system
├── main.cpp                 # Entry point
└── requirements.txt         # Python dependencies
```

## Architecture

### Engine Design

The engine uses a **platform abstraction layer** that enables the same game code to run on:
- **Desktop** (Windows/macOS/Linux) via SDL2 + OpenGL
- **Web browsers** via Emscripten + WebGL

The original C&C class hierarchy (`AbstractClass → ObjectClass → MissionClass → TechnoClass → FootClass → DriveClass → UnitClass`) has been flattened into a simpler entity-component style while preserving all gameplay mechanics.

Key systems:
- **Game loop**: Fixed 15-tick/second game logic (matching original) with variable render rate
- **Entity manager**: Pooled management of units, infantry, buildings, projectiles
- **Combat system**: Weapon-vs-armor damage tables ported from original `COMBAT.CPP`
- **Pathfinding**: A* on the cell grid with terrain cost awareness
- **Fog of war**: Per-cell visibility with exploration tracking

### AI Asset Pipeline

The asset pipeline uses AI image generation to create all game sprites and terrain. Three API providers are supported:

| Provider | Best For | Cost |
|----------|----------|------|
| **Leonardo.ai** (Recommended) | Consistent game art style, batch processing | ~$10/mo |
| OpenAI (DALL-E / GPT Image) | Complex prompt understanding | ~$0.04-0.08/image |
| Stability AI | Fine-grained control, inpainting | ~$0.03/image |

The pipeline includes:
- **Prompt templates** for every game asset (17 vehicles, 7 infantry, 23 buildings, 8 terrain types, 6 effects)
- **Post-processing**: Background removal, sprite sheet assembly, faction color remapping, shadow generation
- **Asset manifest**: Tracks all generated assets to avoid regeneration
- **Style guide**: JSON-based visual consistency rules

## Quick Start

### Prerequisites
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- SDL2, SDL2_image, SDL2_mixer
- Python 3.10+ (for asset pipeline)

### Setup & Build

```bash
# Install dependencies
./tools/setup.sh

# Build (desktop)
./tools/build.sh

# Run
./build/release/cnc_td
```

### Web Build

```bash
# Requires Emscripten SDK
./tools/build.sh web

# Serve locally
cd build/web && python3 -m http.server 8080
```

### Generate Game Assets

```bash
# Install Python dependencies
pip install -r requirements.txt

# Set your API key
export LEONARDO_API_KEY=your-key-here

# Generate all assets
python3 -c "
from asset_pipeline import AssetFactory
factory = AssetFactory()
print(factory.list_required_assets())
factory.generate_all()
"
```

## Game Data

All unit and building statistics are faithfully ported from the original source:

### GDI Units
| Unit | HP | Cost | Speed | Weapon |
|------|-----|------|-------|--------|
| Mammoth Tank | 600 | $1500 | 3 | Dual Missiles + Cannon |
| Medium Tank | 400 | $800 | 5 | 120mm Cannon |
| Humvee | 150 | $400 | 8 | Machine Gun |
| APC | 200 | $700 | 6 | Machine Gun |
| MLRS | 150 | $800 | 5 | Rockets |

### Nod Units
| Unit | HP | Cost | Speed | Weapon |
|------|-----|------|-------|--------|
| Light Tank | 300 | $600 | 6 | Cannon |
| Stealth Tank | 180 | $900 | 7 | Missiles (cloaked) |
| Flame Tank | 300 | $800 | 5 | Flamethrower |
| Artillery | 100 | $450 | 3 | Long-range shells |
| Recon Bike | 120 | $500 | 10 | Missiles |

## License

- **Original C&C source code** (`original_source/`): GPL v3 - Copyright Electronic Arts / Westwood Studios
- **Modern port code** (`engine/`, `asset_pipeline/`, etc.): GPL v3
- **AI-generated assets**: Created specifically for this project
