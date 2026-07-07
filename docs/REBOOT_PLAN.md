# Project Reboot Plan — C&C: Tiberian Dawn Modern Port

*Written 2026-07-06. This document is the single source of truth for what gets built next.
The overnight loop (see `LOOP_PROMPT.md`) works through the checkboxes below, top to bottom.*

---

## 1. What this project is actually trying to achieve

Play **Command & Conquer: Tiberian Dawn** on modern hardware — the owner's MacBook Air and
a web browser — rebuilt from the GPL v3 original source as a reference. Success looks like:
double-click / open a URL → main menu → pick GDI or Nod → play a skirmish against an AI →
build a base, harvest tiberium, produce units, win or lose. No toolchain installation required
to *play*.

## 2. Honest assessment of the current state

| Piece | Size | Reality check |
|---|---|---|
| `engine/` C++ engine | ~12,000 lines | Has **never run as a game**. The SDL2 path has never been compiled anywhere (no SDL2 in the sandbox, user has no dev toolchain). Only `visual_test` (software renderer → PPM images) ever built. It is unverified scaffolding. |
| `demo/index.html` | 1,847 lines | The **only playable graphical artifact** in the repo. Monolithic single file, shares no code with anything else. |
| `terminal_demo.py` | 941 lines | Fun ANSI toy. Dead end for the actual goal. |
| `original_source/` | ~290 files | GPL reference. **Cannot compile** — requires Watcom C++ 10.6, TASM/MASM, DirectX 5, and missing Westwood engine libs. It is a rulebook, not a base. |
| `assets/` | 800+ PNGs, 12 sprite sheets | Usable placeholder art with facings/walk frames. Good enough for now. |
| `asset_pipeline/` | ~10 files | Leonardo.ai pipeline, never wired to anything. Parked. |

**Core problem:** effort is split across three disconnected implementations, and the most
invested one (C++) is the one the owner can least run. The owner struggled to install cmake;
Emscripten would be worse. Meanwhile the browser demo — the thing that actually worked for
them — was treated as a throwaway.

## 3. The decision

**Pivot to a single web-first implementation.** One codebase, plain JavaScript ES modules,
**no build step**, playable by opening `game/index.html` (or via GitHub Pages later).

Why this wins:
- **The player's toolchain is a browser.** Zero-install matches the owner's environment.
- **Node 22 is available where the work happens**, so a headless, deterministic sim core can
  be tested automatically overnight (`node --test`) — the loop can verify its own work.
- **The original source stays what it really is: a reference** for stats, damage tables,
  build rules, AI behavior (`UDATA.CPP`, `IDATA.CPP`, `BDATA.CPP`, `COMBAT.CPP`,
  `HOUSE.CPP`, `FINDPATH.CPP`). Much of this was already transcribed into
  `engine/game/game_data.cpp` — transcribe from there into JS data modules.
- **Existing assets plug straight in**: `assets/sheets/*.png` (8-facing tanks, 8×3 infantry,
  buildings grid, effects strip) load directly as `<img>` sprite sheets.
- The 1,847-line HTML demo is a **donor**: its input handling, sidebar layout, and render
  loop get harvested into the modular codebase, then it's retired.

What happens to the C++: **frozen, not deleted.** It stays in `engine/` as reference for the
entity/combat design already worked out. No further work goes into it unless the owner asks.

## 4. Architecture

```
game/
  index.html            ← open this to play (no server needed; use file:// or python -m http.server)
  src/
    main.js             ← boot: canvas, asset load, game loop (render @ rAF, sim @ 15 ticks/s)
    sim/                ← DETERMINISTIC, HEADLESS, NO DOM. Runs in node for tests.
      constants.js        tick rate, tile size, costs
      rng.js              seeded LCG (determinism = replays + testability)
      data/
        units.js          stats ported from UDATA.CPP / game_data.cpp
        infantry.js       IDATA.CPP
        buildings.js      BDATA.CPP
        weapons.js        weapon + warhead-vs-armor tables from COMBAT.CPP
      world.js            map grid, terrain, tiberium, occupancy
      entity.js           units/infantry/buildings/projectiles (pooled, id-based)
      path.js             A* on cell grid
      combat.js           damage application, warhead tables
      economy.js          harvesting, refinery, credits, power
      production.js       build queues, tech tree prerequisites
      ai.js               opponent state machine (port design from engine/game/ai_player.cpp)
      fog.js              shroud/fog of war
      game.js             top-level: createGame(seed, scenario) / tick(game, commands)
      commands.js         serializable player commands (move/attack/build/…) — the ONLY way
                          input mutates the sim (this is what makes multiplayer possible later)
    render/             ← DOM/canvas only. Reads sim state, never writes it.
      assets.js           sprite sheet loader/registry (assets/sheets/*.png)
      camera.js           viewport scroll, edge pan, minimap coords
      draw_map.js         terrain + tiberium + fog
      draw_entities.js    facings, walk frames, health bars, selection
      draw_effects.js     explosions, muzzle flashes
      sidebar.js          build buttons, queue, credits, power, minimap
      menu.js             main menu / faction select / skirmish setup / victory-defeat
    input.js            ← mouse/keyboard → commands (drag select, right-click orders, hotkeys)
  test/                 ← node --test game/test/  (sim only, no DOM)
```

**Iron rules** (the loop must never violate these):
1. `game/src/sim/**` must never import from `render/`, `input.js`, or touch `document`/`window`.
2. All gameplay mutation flows through `commands.js` + `tick()`. Rendering is read-only.
3. Same seed + same command stream ⇒ identical state (there is a test for this).
4. `node --test game/test/*.test.js` must pass before every commit.

## 5. Milestone roadmap (the loop's work queue)

Work top to bottom. `[x]` = done and tested. One task per loop iteration.

### M0 — Foothold *(scaffolded 2026-07-06)*
- [x] M0.1 Create `game/` skeleton: index.html, main.js, sim/constants.js, sim/rng.js, one passing node test.

### M1 — World & rendering
- [x] M1.1 `sim/world.js`: 64×64 cell grid; terrain enum (clear/rock/tree/water); tiberium per cell (0–11 growth stages like original OVERLAY); `createWorld(seed)` generates a symmetric skirmish map. Tests: dimensions, determinism, passability.
- [x] M1.2 `render/assets.js`: load `../assets/sheets/*.png` + `../assets/tiles/**` into a registry with named frame lookups (sheet, frameW/H, index math for facing/frame). Handle load failure with colored-rect fallback so the game never white-screens.
- [x] M1.3 `render/camera.js` + `render/draw_map.js`: draw terrain + tiberium to canvas; arrow keys / edge pan scroll. `main.js` wires rAF render + fixed 15 t/s sim accumulator. **Playable check: opening index.html shows a scrollable map.**
- [x] M1.4 Minimap in sidebar area showing terrain + camera rectangle; click minimap to jump camera.

### M2 — Units & movement
- [x] M2.1 `sim/entity.js`: pooled entities `{id, kind, type, owner, cell, subOffset, facing(0-31 like original, render maps to 8), hp, state}`. Spawn/despawn. Tests.
- [x] M2.2 `sim/data/units.js` + `infantry.js`: port stats (cost, speed, hp, armor class, sight, weapon id) for MVP roster — GDI: minigunner, grenadier, rocket soldier, humvee, medium tank, mammoth, harvester, MCV; Nod: rifle, rocket, flamer, buggy, recon bike, light tank, stealth tank, harvester, MCV. Source: `engine/game/game_data.cpp` (already transcribed from originals). Tests: table sanity (every unit has weapon or is unarmed-by-design, costs > 0).
- [x] M2.3 `sim/path.js`: A* with terrain costs + occupancy; straight-line fast path; path caching per move order. Tests: reaches goal, routes around obstacles, no path ⇒ closest reachable.
- [x] M2.4 Movement in `tick()`: units follow paths with per-type speed, rotate facing toward heading, occupy/release cells. Test: unit ordered A→B arrives in expected tick count, deterministic.
- [x] M2.5 `input.js` + `render/draw_entities.js`: click select, drag-box select, right-click move; draw units from sheets with correct facing frame, selection brackets, health bars. **Playable check: order tanks around the map.**

### M3 — Combat
- [x] M3.1 `sim/data/weapons.js`: weapons (damage, ROF, range, projectile speed) + warhead-vs-armor % table from `COMBAT.CPP` / `game_data.cpp`. Tests: table complete for all referenced weapons.
- [x] M3.2 `sim/combat.js`: attack orders, target acquisition (guard range), projectiles as entities, damage on impact, death + removal. Turret facing separate from hull for tanks that have one. Tests: DPS math, armor modifiers, kill credit.
- [x] M3.3 `render/draw_effects.js`: muzzle flash, projectile, explosion anim from effects sheet on death/impact. Auto-return-fire when attacked (guard mission).
- [x] M3.4 Stances/missions: guard (default), attack-move (A+click), force-attack (Ctrl+click), stop (S). Group hotkeys Ctrl+1-9 / 1-9.

### M4 — Economy
- [x] M4.1 Tiberium growth + spread per original logic (slow tick); harvester behavior: seek nearest tiberium, load (bail count), return to refinery, unload → credits. Tests: full harvest cycle yields expected credits deterministically.
- [x] M4.2 Credits + power model on the player house (`sim/economy.js`): power output/drain, low-power effects (slower production, radar off). Sidebar shows credits ticker + power bar.

### M5 — Buildings & production
- [ ] M5.1 Buildings as entities with footprints (multi-cell occupancy from BDATA), placement validity (adjacency to existing base, terrain clear), MCV deploy → Construction Yard.
- [ ] M5.2 `sim/production.js`: tech-tree prerequisites (from `engine/game/game_data.cpp` tables), build queue per category (building/infantry/vehicle), cost drain over time (credits tick down during build like original), ready → place building or spawn unit at factory exit w/ rally point.
- [ ] M5.3 `render/sidebar.js`: icon grid from `assets/ui/icon_*.png`, progress overlay clock, click to build, click-ready building → placement ghost (green/red cells). **Playable check: full base-building loop.**
- [ ] M5.4 Defense structures (guard tower, obelisk-lite, AGT) with turret auto-fire; sell (50% refund) + repair toggle.

### M6 — AI opponent
- [ ] M6.1 `sim/ai.js`: port the state machine design from `engine/game/ai_player.cpp` — build-order phase, economy phase, army phase, attack waves scaling with difficulty; target selection (nearest/weakest). Tests: headless AI-vs-AI game for 5,000 ticks completes without error and both AIs build bases (assert entity counts).
- [ ] M6.2 Difficulty settings (easy/normal/hard: income multiplier, wave size, reaction time).

### M7 — Fog of war & win/lose
- [ ] M7.1 `sim/fog.js`: shroud (never seen, black) + fog (seen, dimmed, remembers buildings) per house; sight radii reveal. Render pass + minimap respect it. Tests: reveal math.
- [ ] M7.2 Victory/defeat: destroy all enemy production structures ⇒ win; lose all yours ⇒ lose. End screen with stats (built/lost/harvested), return to menu.

### M8 — Menus & game shell
- [ ] M8.1 `render/menu.js`: main menu → skirmish setup (faction, color, credits, difficulty, map seed) → game → end screen → menu. Pause (P/Esc). Port look from the old C++ menu design.
- [ ] M8.2 Save/load to localStorage (serialize sim state — it's already plain data), 3 slots.

### M9 — Feel & polish
- [ ] M9.1 Audio via WebAudio: synthesize placeholder SFX (shots, explosions, "unit ready", EVA-style beeps) — no copyrighted audio. Mute toggle.
- [ ] M9.2 Right-drag scroll, screen-shake on big explosions, unit acknowledgment flashes, attack-notify minimap ping ("base under attack").
- [ ] M9.3 Performance pass: 300 units + 60 buildings at 60fps render / stable 15 t/s sim on a MacBook Air (measure with a headless perf test in node; budget: tick < 8ms at 300 entities).
- [ ] M9.4 Retire `demo/index.html` (move to `legacy/`), update README with "open game/index.html to play" + screenshot generated from the new game.

### M10 — Stretch (only if all above done)
- [ ] M10.1 Campaign-style scripted scenarios (port 2–3 mission layouts from the old `engine/game/scenario.cpp` designs).
- [ ] M10.2 Replay system (record command stream + seed — determinism makes this nearly free).
- [ ] M10.3 GitHub Pages workflow so the owner gets a playable URL.

## 6. Progress log

The loop appends one entry per iteration to `docs/PROGRESS.md`. Format:

```
## 2026-07-06 03:14 — M1.1
Did: implemented world grid + terrain gen. Tests: 14 pass.
Notes: tiberium spread rate guessed at 1/32 chance per slow-tick; revisit vs OVERLAY.CPP.
```
