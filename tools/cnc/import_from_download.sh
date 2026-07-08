#!/bin/bash
# One-command original-asset import for macOS.
#
#   ./tools/cnc/import_from_download.sh [path-to-installer-or-folder]
#
# With no argument, it looks for the most recent C&C download (.exe/.zip/.iso)
# in ~/Downloads. It unpacks it, finds the .MIX game files, installs the two
# things it needs (7-zip via Homebrew, Pillow via pip), runs the importer,
# and tells you how to play.

set -e
cd "$(dirname "$0")/../.."   # repo root

say() { printf "\n\033[1;33m%s\033[0m\n" "$*"; }

# ── 1. Locate the download ───────────────────────────────────────────────────
TARGET="$1"
if [ -z "$TARGET" ]; then
  TARGET=$(ls -t ~/Downloads/*.exe ~/Downloads/*.zip ~/Downloads/*.iso 2>/dev/null \
    | grep -iE 'c(n)?c|command|conquer|tiberian|td' | head -1 || true)
  if [ -z "$TARGET" ]; then
    TARGET=$(ls -t ~/Downloads/*.exe ~/Downloads/*.zip ~/Downloads/*.iso 2>/dev/null | head -1 || true)
  fi
fi
if [ -z "$TARGET" ] || [ ! -e "$TARGET" ]; then
  echo "Could not find a downloaded installer. Run:"
  echo "  $0 /path/to/the/download"
  exit 1
fi
say "Using: $TARGET"

# ── 2. Tools ─────────────────────────────────────────────────────────────────
if ! command -v 7z >/dev/null && ! command -v 7zz >/dev/null; then
  if command -v brew >/dev/null; then
    say "Installing 7-zip (needed to unpack the installer)..."
    brew install p7zip
  else
    echo "Homebrew not found. Install it from https://brew.sh then re-run."
    exit 1
  fi
fi
SEVENZ=$(command -v 7z || command -v 7zz)

python3 -c "import PIL" 2>/dev/null || {
  say "Installing Pillow (image library)..."
  pip3 install --user pillow || python3 -m pip install --user pillow
}

# ── 3. Unpack: try 7-zip, then innoextract, then unar ────────────────────────
# Different community installers use different packers; try them all.
try_extract() {
  local target="$1" dest="$2"
  "$SEVENZ" x -y -o"$dest" "$target" >/dev/null 2>&1 \
    && [ -n "$(find "$dest" -type f 2>/dev/null | head -1)" ] && return 0
  if ! command -v innoextract >/dev/null; then
    say "7-zip couldn't open it; trying innoextract (Inno Setup installers)..."
    brew install innoextract >/dev/null 2>&1 || true
  fi
  command -v innoextract >/dev/null \
    && innoextract -s -d "$dest" "$target" >/dev/null 2>&1 \
    && [ -n "$(find "$dest" -type f 2>/dev/null | head -1)" ] && return 0
  if ! command -v unar >/dev/null; then
    say "Trying unar (self-extracting archives)..."
    brew install unar >/dev/null 2>&1 || true
  fi
  command -v unar >/dev/null \
    && unar -quiet -force-overwrite -o "$dest" "$target" >/dev/null 2>&1 \
    && [ -n "$(find "$dest" -type f 2>/dev/null | head -1)" ] && return 0
  return 1
}

WORK=$(mktemp -d /tmp/cnc_import.XXXX)
if [ -d "$TARGET" ]; then
  WORK="$TARGET"
else
  say "Unpacking (this can take a minute)..."
  if ! try_extract "$TARGET" "$WORK"; then
    echo
    echo "None of the extractors could open this installer:"
    file "$TARGET" 2>/dev/null || true
    echo
    echo "Easiest fix: use the freeware ISO instead (no extraction needed on a Mac):"
    echo "  1. Download the GDI disc from https://cncnz.com/downloads/tiberian-dawn-downloads/"
    echo "  2. Double-click the .iso in Finder to mount it"
    echo "  3. Re-run:  $0 /Volumes/<the mounted disc>   (check the name with: ls /Volumes)"
    exit 1
  fi
  # Some releases nest an ISO or a second archive inside; unpack one level deeper.
  find "$WORK" -maxdepth 2 \( -iname "*.iso" -o -iname "*.zip" -o -iname "*.exe" \) 2>/dev/null \
    | head -3 | while read -r inner; do
      try_extract "$inner" "$WORK/inner_$(basename "$inner" | tr -c 'A-Za-z0-9' _)" || true
    done
fi

# ── 4. Find the game data ────────────────────────────────────────────────────
MIX_COUNT=$(find "$WORK" -iname "*.mix" 2>/dev/null | wc -l | tr -d ' ')
if [ "$MIX_COUNT" -eq 0 ]; then
  echo
  echo "No .MIX game files found inside. Contents were:"
  find "$WORK" -maxdepth 3 -type f 2>/dev/null | head -25
  echo
  echo "If you see an .iso above, double-click it in Finder, then re-run:"
  echo "  $0 /Volumes/<the mounted disc>"
  exit 1
fi
say "Found $MIX_COUNT .MIX archives. Importing..."

# ── 5. Import ────────────────────────────────────────────────────────────────
python3 tools/cnc/import_assets.py "$WORK"

say "All done! Start the game with:"
echo "  python3 -m http.server 8000"
echo "then open http://localhost:8000/game/ - the original sprites load automatically."
