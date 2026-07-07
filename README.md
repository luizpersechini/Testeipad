# Command & Conquer: Tiberian Dawn — Web Port

A from-scratch remake of the classic **Command & Conquer: Tiberian Dawn** (1995) by
Westwood Studios that runs directly in your browser. Game rules, stats, and damage
tables are transcribed from the [original GPL v3 source](https://github.com/electronicarts/CnC_Tiberian_Dawn)
released by Electronic Arts.

![In-game screenshot](docs/shot_game.png)

## ▶ How to play

**Online:** once this branch is merged to `main` and GitHub Pages is enabled for the
repository (Settings → Pages → Source: *GitHub Actions*), every push deploys the game
to `https://<user>.github.io/<repo>/` automatically — tests run first, then it ships.

**Locally:** no build step, no dependencies — you just need a browser and any static
file server:

```bash
cd Testeipad
python3 -m http.server 8000
```

Then open **http://localhost:8000/game/** — main menu → SKIRMISH → pick your faction,
enemy AI difficulty, starting credits, and map seed → START GAME.

> ES modules can't load over `file://`, which is why the one-line server is needed.
> macOS ships `python3`, so the command above works out of the box on a Mac.

### Playing on iPad

Open the same URL in Safari on the iPad, then **Share → Add to Home Screen** — the game
installs as a fullscreen landscape app. Touch controls:

| Gesture | Action |
|---|---|
| Tap own unit/building | Select |
| Tap ground / enemy (with selection) | Move / attack |
| Drag from empty ground | Box-select |
| Two-finger drag | Pan the camera |
| Long-press (with selection) | Attack-move |
| Bottom command bar | Stop, attack-move, deploy, harvest, repair, sell, pause |
| Tap minimap / sidebar | Jump camera / build & place |

### Mouse & keyboard controls

| Input | Action |
|---|---|
| Left-click / drag | Select unit / box-select |
| Right-click | Move, or attack the enemy under the cursor |
| Right-drag | Pan the camera (arrows & screen edge also scroll) |
| A + click | Attack-move |
| Ctrl + click | Force-attack the ground |
| S / D / H | Stop / Deploy MCV / send harvester to work |
| R / X | Repair / sell the selected building |
| Ctrl+1–9, 1–9 | Assign / recall control groups |
| P / M | Pause / mute |
| F2–F4, F6–F8 | Save / load (3 slots, localStorage) |
| Sidebar | Click to build; click READY item to place it (ghost shows legality) |

## Using the original graphics, sounds and videos

EA released Tiberian Dawn as **official freeware in 2007**, so the original assets are
legally downloadable — they just aren't committed to this repo. To upgrade the
placeholder art to the real 1995 sprites on your machine:

1. **Download the freeware game** (any one of these official community mirrors):
   - CnCNet: https://cncnet.org/command-and-conquer → *Download* (installer with game files)
   - C&C Community: https://cnc.community/tiberian-dawn/how-to-play (installer + optional campaign-video pack)
   - CNCNZ: https://cncnz.com/downloads/tiberian-dawn-downloads/ (original GDI/Nod freeware ISOs)
2. **Locate the game files** — you need the `.MIX` archives (`CONQUER.MIX`,
   `TEMPERAT.MIX`, ideally also the sound MIXes; mount an ISO or unzip the installer
   to find them).
3. **Import them:**
   ```bash
   pip install pillow          # once
   brew install ffmpeg         # optional: converts original sounds and videos
   python3 tools/cnc/import_assets.py /path/to/the/game/files
   ```

The importer decodes the Westwood formats (MIX archives, SHP sprites with LCW/XOR
compression, 6-bit palettes, and — via ffmpeg — AUD sounds and VQA videos) and writes
web-ready sprite sheets to `assets/original/`. The game automatically prefers those
over the placeholders on the next reload. `assets/original/` is git-ignored: the
freeware assets are for local play and are not redistributed through this repo or
the GitHub Pages deployment.

## What's implemented

- Deterministic sim core at the original's 15 ticks/sec: same seed + same commands
  ⇒ identical game (save/load resumes bit-identically; tested)
- Original stats and combat math: UDATA/IDATA/BDATA unit tables, CONST.CPP weapon
  table, warhead-vs-armor percentages, `damage * modifier >> 8`
- A\* pathfinding (octile, no corner cutting, infantry pass through trees)
- Full economy: tiberium growth/spread, 28-bail harvester cycle, power grid with
  low-power penalties
- Base building: tech tree, pay-as-you-go build queues, placement rules, MCV deploy,
  sell/repair, defense towers
- Skirmish AI with faction build orders, attack waves, three difficulty tiers
- Fog of war (shroud + fog, per house), victory/defeat with match stats
- Synthesized WebAudio SFX (no copyrighted samples)

## Project layout

```
game/            ← the playable web game (plain ES modules, zero dependencies)
  src/sim/       ← deterministic, headless game logic (runs in node for tests)
  src/render/    ← canvas rendering (reads sim state, never writes it)
  test/          ← node --test game/test/*.test.js  (150+ tests)
assets/          ← generated placeholder sprites & sheets
original_source/ ← EA's GPL v3 C&C source (reference only; not compilable)
engine/          ← earlier C++/SDL2 port attempt (frozen, kept as reference)
tools/           ← asset generators (Pillow) and legacy build scripts
legacy/          ← retired demos (single-file HTML prototype, terminal demo)
docs/            ← REBOOT_PLAN.md (roadmap), PROGRESS.md (dev log), screenshots
```

## Development

```bash
node --test game/test/*.test.js   # the whole sim test suite
```

The sim (`game/src/sim/`) never touches the DOM, all mutation flows through
`commands.js` + `gameTick()`, and determinism is enforced by tests — see
`docs/REBOOT_PLAN.md` for the architecture and roadmap, and `docs/PROGRESS.md`
for the iteration-by-iteration development log.

## License

Game rule data derives from the C&C Tiberian Dawn source released under **GPL v3**;
this project follows the same license. Command & Conquer is a trademark of
Electronic Arts. This is a non-commercial fan remake; you should own the original
game (available in the C&C Ultimate Collection).
