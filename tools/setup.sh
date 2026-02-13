#!/bin/bash
# C&C: Tiberian Dawn Modern Port - Setup Script
#
# Installs dependencies for building and running the project.

set -e

echo "=== C&C: Tiberian Dawn Modern Port - Setup ==="
echo ""

# Detect OS
OS="unknown"
if [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
elif [[ "$OSTYPE" == "linux"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    OS="windows"
fi

echo "Detected OS: ${OS}"
echo ""

# Install C++ build dependencies
echo "--- Installing C++ build dependencies ---"
case "$OS" in
    macos)
        if ! command -v brew &> /dev/null; then
            echo "Homebrew not found. Install from https://brew.sh"
            exit 1
        fi
        brew install cmake sdl2 sdl2_image sdl2_mixer
        ;;
    linux)
        if command -v apt-get &> /dev/null; then
            sudo apt-get update
            sudo apt-get install -y cmake build-essential \
                libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y cmake gcc-c++ \
                SDL2-devel SDL2_image-devel SDL2_mixer-devel
        elif command -v pacman &> /dev/null; then
            sudo pacman -S --noconfirm cmake sdl2 sdl2_image sdl2_mixer
        else
            echo "Package manager not recognized. Install manually:"
            echo "  cmake, SDL2, SDL2_image, SDL2_mixer"
        fi
        ;;
    windows)
        echo "On Windows, install via vcpkg:"
        echo "  vcpkg install sdl2 sdl2-image sdl2-mixer"
        ;;
esac

echo ""

# Install Python dependencies for asset pipeline
echo "--- Installing Python asset pipeline dependencies ---"
if command -v python3 &> /dev/null; then
    python3 -m pip install -r requirements.txt
else
    echo "Python 3 not found. Install Python 3.10+ for the asset pipeline."
fi

echo ""

# Create asset directories
echo "--- Creating asset directories ---"
mkdir -p assets/sprites/{units,buildings,infantry,effects}
mkdir -p assets/tiles/{terrain,overlays}
mkdir -p assets/ui
mkdir -p assets/sounds
mkdir -p assets/music

echo ""
echo "=== Setup complete! ==="
echo ""
echo "Next steps:"
echo "  1. Build:  ./tools/build.sh"
echo "  2. Run:    ./build/release/cnc_td"
echo ""
echo "To generate game assets with AI:"
echo "  export LEONARDO_API_KEY=your-key-here"
echo "  python3 -c 'from asset_pipeline import AssetFactory; AssetFactory().generate_all()'"
