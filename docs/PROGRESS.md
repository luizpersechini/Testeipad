# Progress Log

One entry per loop iteration, newest at the bottom. See `LOOP_PROMPT.md` for format.

## 2026-07-06 — M0.1 (manual bootstrap)
Did: project reboot — wrote REBOOT_PLAN.md, LOOP_PROMPT.md, scaffolded game/ skeleton
(index.html, main.js, sim/constants.js, sim/rng.js) with passing node tests.
Notes: sim/render split is the load-bearing decision; keep sim DOM-free at all costs.

## 2026-07-06 — M1.1
Did: sim/world.js — terrain/tiberium/occupancy grids as typed arrays, seeded generation
with rock/tree/water blobs mirrored 180° for fairness, cleared start zones, three
tiberium fields (one per start + contested center). Tests: 14 pass (8 new).
Notes: trees currently block all ground units; original lets infantry through — handle in
M2.3 pathfinding. Tiberium field *stages* aren't mirror-identical (extent is); acceptable
since harvest value differences are small — revisit if fairness complaints.

## 2026-07-06 — M1.2
Did: render/assets.js — sheet metadata + pure frame math (vehicle/infantry/building/
effect/tiberium lookups) with DOM loading isolated in loadAssets(); every image can be
null and callers must rect-fallback. Tests: 20 pass (6 new).
Notes: mammoth sheet frames are clipped (authored 40px, packed at 32px stride) —
regenerate sheet with 40px stride later; harmless placeholder for now.

## 2026-07-06 — M1.3
Did: render/camera.js (pure: clamp/move/center/coord conversions/visibleCells),
render/draw_map.js (terrain tiles w/ deterministic per-cell variants, tiberium overlays,
rect fallbacks), main.js rewired: real world render, arrows/WASD + edge pan scrolling.
Tests: 26 pass (6 new).
Notes: no dedicated tree tile art — using tinted 'rough' tile as stand-in.

## 2026-07-06 — M1.4
Did: render/minimap.js (pure layout + hit-test + coordinate mapping, drawn terrain/
tiberium/camera-rect), sidebar strip reserved in main.js (camera view now 1080px wide),
click minimap jumps camera. Tests: 30 pass (4 new).
Notes: minimap redraws all 4096 cells per frame; cache to offscreen canvas if profiling
flags it (M9.3).

## 2026-07-06 — M2.1
Did: sim/entity.js — insertion-ordered Map store (deterministic iteration), monotonic
never-reused ids, spawn/despawn/relocate with occupancy bookkeeping (projectiles don't
occupy), facing helpers (32-facing sim -> 8-column sheets, facingToward compass math).
Tests: 39 pass (9 new).
Notes: dropped the free-list pool from the plan wording — Map is deterministic and
simpler; revisit only if M9.3 profiling says allocation matters.

## 2026-07-06 — M2.2
Did: sim/data/units.js (13 vehicles) + infantry.js (7 types), stats transcribed from
engine/game/game_data.cpp (itself from UDATA/IDATA). Weapon fields are string ids
resolved by M3.1 weapons.js. Tests: 44 pass (5 new: table sanity + roster + original
cost relationships).
Notes: E1/E3 marked faction 'both' per original TD rules (plan said 'nod rifle' — same
unit). Added APC/MLRS/artillery/flame_tank beyond MVP since data was at hand.

## 2026-07-06 — M2.3
Did: sim/path.js — A* with binary heap (deterministic tie-break by insertion seq),
octile heuristic, 141/100 diagonal cost, no corner cutting, trees admit infantry only,
occupied cells blocked at plan time except the goal (approach handled by mover),
unreachable goal -> partial path to closest reachable cell, 4096-expansion guard.
Tests: 52 pass (8 new).
Notes: plan mentioned "path caching per move order" — deferred to M2.4 movement where
re-path triggers live; caching without movement is untestable.

## 2026-07-06 — M2.4
Did: sim/game.js (createGame/gameTick) + sim/commands.js (move/stop). Integer-math
movement: speed*25 progress/tick vs step cost 1000/1410, turn-in-place (4 facings/tick,
drives within 4 of desired), blocked -> cooldown re-path (max 3) -> idle, sub-cell
offsets exposed for smooth rendering. Tests: 59 pass (7 new incl. iron-rule-3
determinism test and exact 48-tick arrival).
Notes: collision resolution is plan-time avoid + re-path; no push/yield mechanics yet.
Two-units-same-spot settles within 3 cells — fine until formations (post-MVP).

## 2026-07-06 — M2.5
Did: input.js (click/drag-box select with pure pick/rect helpers, right-click move,
S stop, minimap jump moved here), render/draw_entities.js (sheet sprites by facing +
walk frame, faction-colored fallback with facing tick, selection brackets, health
bars, drag box), main.js spawns starter forces for both factions and drains the
command queue into gameTick. Tests: 62 pass (3 new incl. headless playable check).
Notes: WASD scroll removed (S = stop hotkey per original); arrows + edge pan remain.
Game is now genuinely interactive in the browser: select and order units around.

## 2026-07-06 — M3.1
Did: sim/data/weapons.js — 16 weapons + 6 warheads transcribed verbatim from
original_source/CONST.CPP Weapons[]/Warheads[] with bullet->warhead mapping from
BBDATA.CPP (better source than the C++ port's approximations). modifyDamage()
replicates COMBAT.CPP integer math (damage * mod >> 8, min 1). Tests: 68 pass (6 new).
Notes: earlier unit tables (M2.2) carried approximated damage in game_data.cpp;
weapons.js now holds the true original numbers and unit weapon ids all resolve.
Projectile speeds are invented (original uses MPH per bullet class) — tune in M3.3.

## 2026-07-07 — M3.2
Did: sim/combat.js — attack orders, guard auto-acquire (idle armed units scan sight
range every 8 ticks, staggered by id), chase with stale-goal re-path, homing projectile
entities, damage via original modifyDamage, death events + kill credit; game.js emits
per-tick events[] and runs movement+combat; attackCommand added; stats.js extracted
(import-cycle break); movement now also drives chasing attackers. Fixed: reload field
was undefined at spawn (nobody could ever fire). Tests: 75 pass (7 new incl. combat
determinism).
Notes: turret facing snaps instantly (gradual turret turn = polish). Two test
expectations were corrected, not code: authentic AP-vs-infantry (0x40) makes tanks
slow infantry-killers, and guard return fire is legitimate chip damage.

## 2026-07-07 — M3.3
Did: render/draw_effects.js — event-driven effect system (explosion 6-frame anim from
effects sheet, muzzle flash at barrel tip, hit sparks; pure spawn/prune/frame logic
node-tested), projectile rendering by type color; return fire in combat.js via
lastAttacker (works beyond sight range — artillery can't shell with impunity);
right-click on an enemy now issues attack instead of move. Tests: 79 pass (4 new).
Notes: effects list is unbounded between prunes in huge battles — prune runs per tick
so it's fine; muzzle uses a dot, not the muzzle_flash.png star (asset hookup = polish).

## 2026-07-07 — M3.4
Did: attack-move (A+click: engage what you sight, resume advance on kill), force-attack
ground (Ctrl+click) with warhead splash (linear falloff, radius spread/4 cells,
friendlies hit, shooter excluded), control groups Ctrl+1-9/1-9 in input.js, Escape
clears selection/mode; move orders now cancel combat state. Tests: 84 pass (5 new).
Notes: splash radius spread/4 is my scaling of the original SpreadFactor (original
uses it as a falloff divisor, not radius) — feels right, revisit vs COMBAT.CPP
Explosion_Damage if balance is off. M3 milestone complete.

## 2026-07-07 — M4.1
Did: sim/economy.js — tiberium growth/spread (every 32 ticks, 24 rng samples: stage++
or seed a clear neighbor at stage>=6) and the full harvester cycle (seek nearest field,
1 bail/8 ticks to 28-bail capacity, return to refinery, 20-tick unload -> 700 credits,
repeat until stripped). game.houses[] with credits added to createGame. harvestCommand.
Tests: 90 pass (6 new).
Fixed two real bugs the new tests exposed: (1) rng.int used modulo — an LCG's low bits
have degenerate periods, so coordinate sampling hit a lattice and growth NEVER fired;
switched to high-bits scaling. (2) map-gen center clearing broke 180° symmetry (rotation
center is 31.5, not 32) — made the cleared region a symmetric union.
Notes: refineries are placeholder 1-cell buildings until M5.1 footprints.

## 2026-07-07 — M4.2
Did: sim/data/buildings.js (15 buildings from BDATA via game_data.cpp: hp/cost/power/
footprints/armor/tech/weapons — towers, obelisk, temple, comm centers), tickPower()
(damaged plants generate proportionally less, lowPower flag), statsFor covers
buildings so defensive towers auto-fire via existing combat, tower_rocket weapon
(WEAPON_TOW_TWO), sidebar: rolling credits ticker + power bar with drain marker.
Tests: 96 pass (6 new).
Notes: M4 milestone complete. Obelisk uses guard_tower art placeholder. comm_center
values partially reconstructed (grep missed the HQ block) — verify against BDATA later.

## 2026-07-07 — M5.1
Did: multi-cell footprints in entity spawn/despawn (atomic claim of the whole
rectangle), sim/placement.js (footprintClear: bounds/terrain/occupancy/tiberium;
adjacency within 3 cells of a friendly building, first building exempt;
placeBuilding; deployMcv with exact-restore on blocked deploy), deployCommand +
D hotkey, H hotkey for harvest, buildings render from the buildings sheet scaled
to footprint. Tests: 102 pass (6 new).
Notes: building x,y is top-left; combat centerDist treats it as a point — big
buildings are slightly "closer" on their top-left. Cosmetic for now.

## 2026-07-07 — M5.2
Did: sim/production.js — availableToBuild (faction + factory + prereq gating), one
queue per category (buildings/infantry/units), pay-as-you-go cost drain (stalls when
broke, full refund on cancel), low-power half speed, finished units spawn at a free
ring cell around their factory, finished buildings wait as 'ready' for placeCommand.
build/cancelbuild/placebuilding commands wired. Tests: 110 pass (8 new).
Notes: rally points deferred (plan mentions them in M5.2 — moved to M5.3/M9 polish;
factory exit search is deterministic ring scan). All 8 first-time-through: zero fails.

## 2026-07-07 — M5.3
Did: render/sidebar.js — build menu grouped by category (structures/infantry/vehicles)
with icons, cost labels, progress sweep + %, flashing READY, blocked-sibling dimming;
pure layout + hit-test tested in node. Input: click to build, click in-progress to
cancel, click READY -> placement mode with green/red ghost (right-click/Esc cancels).
main.js starter scene upgraded to real bases (yard + power plant each side).
Tests: 113 pass (3 new). **The full base-building loop is now playable in the browser:
harvest -> credits -> build -> place -> produce units -> fight.**
Notes: single shared 'GDI is the human' assumption baked into input/main — parametrize
when menus (M8.1) add faction select.

## 2026-07-07 — M5.4
Did: sell (50% of cost scaled by health, frees footprint) + repair toggle (heals
4hp/3 ticks, full repair from zero costs half price, pauses when broke, auto-off at
full) in economy.js; sell/repair commands + X/R hotkeys; buildings click-selectable
across their whole footprint (pickEntityAt now footprint-aware). Tower auto-fire was
already live via combat + building weapons (proven in power.test). Tests: 118 pass
(5 new). M5 milestone complete — the game loop matches original C&C base play.
Notes: no sell animation / crew ejection (original spawns survivors) — polish.

## 2026-07-07 — M6.1
Did: sim/ai.js — deterministic skirmish AI (all randomness via game.rng): deploys MCV,
sends idle harvesters to work, faction build orders (GDI/Nod distinct incl. defenses),
places ready buildings on a spiral around the yard, keeps infantry+vehicle queues fed,
launches attack waves at the enemy yard when army >= wave size. Refineries now ship
with a free working harvester like the original (this un-deadlocked the AI economy).
Tests: 123 pass (5 new: AI-vs-AI 5000-tick war, economy end-to-end, determinism).
Notes: seed-42 run has GDI razing Nod by tick ~5000 — the AI actually plays. Peak-based
assertions because a losing AI's final base can legitimately be rubble.

## 2026-07-07 — M6.2
Did: DIFFICULTY tiers exported (wave size/cooldown + incomeMult 0.8/1.0/1.4 as the
classic AI handicap), income multiplier applied at harvester unload for AI houses only;
browser game now runs a Nod AI by default (?ai=easy|normal|hard|off). Tests: 126 pass
(3 new). M6 milestone complete — the browser game is now a real fight.
Notes: ai.js<->economy.js import cycle resolves cleanly (function bindings, no
top-level use). Reaction-time knob folded into waveCooldown rather than a separate
timer.

## 2026-07-07 — M7.1
Did: sim/fog.js — per-house shroud/fogged/visible maps recomputed every 5 ticks
(visible decays to fogged, live sight sources re-reveal from footprint centers);
entityVisibleTo (enemy units need VISIBLE, enemy buildings linger on FOGGED ground);
renderer: black shroud + dim fog veil in draw_map, hidden enemies in draw_entities,
fogged minimap. Tests: 131 pass (5 new).
Notes: fog full-recompute is O(entities * sight²) every 5 ticks — cheap. Buildings
on fogged ground show live hp; original froze last-seen state. Minor fidelity gap.

## 2026-07-07 — M7.2
Did: sim/victory.js — participants join once they can produce; defeated = no
production structure and no MCV; last house standing wins (draw = -1); per-house
stats (built/lost/kills/harvested) hooked into combat/production/placement/economy;
main.js end screen (MISSION ACCOMPLISHED/FAILED + stats) freezes the sim.
Fixed a real AI flaw the war test exposed: a broke AI below full wave size would
stalemate forever — added the impatience rule (attack with >=3 units after 2x wave
cooldown). Hard-vs-easy AI war now concludes decisively. Tests: 135 pass (4 new).
M7 milestone complete.
Notes: AI can still deadlock economically if its harvester dies while credits < 1400
(cannot rebuild); impatience masks it strategically. Consider sell-to-rebuild later.

## 2026-07-07 — M8.1
Did: render/menu.js (main -> skirmish setup with faction/AI difficulty/credits/map-seed
cyclers -> start; pure model/layout/hit-test), main.js restructured into a shell state
machine (menu <-> session), sessions built from menu settings (play as GDI *or Nod*,
enemy AI gets the other side), P pauses, end screen click returns to menu, input.js
rewritten session-aware (player faction no longer hardcoded; listeners attach once and
read the live session). Tests: 139 pass (4 new).
Notes: right-click cycles menu options backward. ?ai= URL param removed — difficulty
now comes from the menu. Playing as Nod is fully supported end to end.

## 2026-07-07 — M8.2
Did: sim/save.js — full game serialization (typed arrays/Map/Set/rng state <-> JSON)
with version gate; main.js localStorage slots (F2/F3/F4 save, F6/F7/F8 load) with
toast feedback; loading rebuilds the whole session mid-match. Tests: 143 pass (4 new)
including the gold test: a save/loaded game continues bit-identically for 800 ticks
alongside the original, and survives a JSON round trip. M8 milestone complete.
Notes: saves store the player's house; menu-based load UI (list slots with
timestamps) would be nicer than F-keys — polish item.

## 2026-07-07 — M9.1
Did: render/audio.js — WebAudio-synthesized SFX (filtered noise bursts for guns/
explosions, oscillator blips for laser/EVA-style ready beeps/sell/unload), pure
planSounds() with per-tick dedupe + global cap of 5, lazy AudioContext (first
gesture), M mute toggle persisted to localStorage. Every weapon maps to a recipe
(test-enforced). Tests: 148 pass (5 new).
Notes: all synthesized — zero copyrighted audio. Victory fanfare is a single square
tone; could be a small melody later.

## 2026-07-07 — M9.2
Did: render/feedback.js — order markers (green move / red attack rings that expand and
fade), screen shake (buildings shake harder, exponential decay, render-only jitter),
"BASE UNDER ATTACK" toast + blinking minimap pings when player buildings take hits
(throttled to one alert per 10s); right-drag now pans the camera (a clean right-click
still issues orders — decided on mouseup by drag distance). Tests: 152 pass (4 new).
Notes: unit acknowledgment flashes folded into order markers (rings at the order
target) rather than per-unit flashes — reads better at RTS zoom.

## 2026-07-07 — M9.3
Did: perf budget codified as a test — 300 units + 60 buildings converging in battle:
avg tick 1.5ms (budget 8ms), worst ~9ms only on the 300-simultaneous-pathfind command
tick. Render side: minimap terrain+fog now cached to an offscreen canvas (redrawn
every 10 ticks instead of 4096 fillRects/frame) and gained live entity dots (fog-
filtered, faction-colored). Tests: 153 pass (1 new).
Notes: sim was already comfortably inside budget — no sim changes needed. If worst-
case matters later, stagger bulk move-order pathfinding across ticks.

## 2026-07-07 — M9.4
Did: retired the prototype demos (demo/ + terminal_demo.py -> legacy/), rewrote
README as a player-first doc (how to run, controls table, feature list, layout,
GPL note). Verified the game end-to-end in a REAL browser via the sandbox's
Playwright/Chromium: menu -> skirmish setup -> in-game with fog, sprites, sidebar,
minimap all rendering — live screenshots committed to docs/ (shot_menu/setup/game).
Tests: 153 pass. M9.1-M9.4 done.
Notes: important correction — ES modules do NOT load over file://; README now says
to serve with python3 -m http.server. One 404 during browser run (missing optional
asset; harmless rect fallback) — track down in a polish pass.

## 2026-07-07 — M10.1
Did: sim/scenarios.js — two scripted missions ported from the frozen C++ designs
(GDI 1: The Beachhead — full base battle vs Nod outpost with river/rock terrain;
Nod 1: Silencing Dissent — base-less strike team assault), declarative layouts on
flat worlds (fully deterministic, no rng), MISSIONS menu screen wired into the shell.
Fixed a real victory-logic hole the mission test exposed: houses that never owned
production (strike teams) never became participants, so such games could never end —
victory now tracks everHadProduction (base owners lose with their base; base-less
forces fight while units live), serialized in saves. Tests: 158 pass (5 new).
Notes: mission 2's "destroy the comm center" objective is currently "destroy all GDI
production" — bespoke objectives (destroy-specific-target) would need a triggers
system; noted as future work.

## 2026-07-07 — M10.2
Did: sim/setup.js (match construction moved into the sim: skirmish + scenario behind
one buildFromSetup door — main.js startSession now delegates) and sim/replay.js
(recording = setup descriptor + sparse {tick, commands} log; playback rebuilds and
re-feeds). Shell: every live match records; on game end the recording persists to
localStorage; press W on the end screen to watch the whole match replay as a
spectator. Tests: 163 pass (5 new) incl. bit-identical 1200-tick replay and JSON
round trip.
Notes: replay watches from the recorder's fog perspective; a "reveal all" spectator
toggle would be nice. Recording of loaded-from-save games is off (no recording field)
— replays cover full matches only.

## 2026-07-07 — M10.3
Did: .github/workflows/pages.yml — on push to main: run the full sim test suite,
then assemble a static site (root redirect + game/ + assets/) and deploy to GitHub
Pages; root index.html redirects to game/. README documents enabling Pages
(Settings -> Pages -> Source: GitHub Actions). Tests: 163 pass.
Notes: deploy is gated on the test job — a red suite never ships. The owner needs
to merge to main and flip the Pages setting once; after that every merge deploys.
M10 COMPLETE — every checkbox in the reboot plan is done.

## 2026-07-07 — verification pass
Did: fresh Playwright/Chromium end-to-end run against the finished game: main menu ->
MISSIONS -> GDI 1 (scripted base renders with scenario credits/build tree, fog,
minimap entity dots) and menu -> skirmish -> in-game. Zero page exceptions.
Screenshots refreshed in docs/.

## 2026-07-07 — quality pass (loop wrap-up)
Did: game/test/architecture.test.js — mechanical enforcement of the iron rules:
sim/ can never import render//input.js or touch DOM globals, sim/ can never use
Math.random/Date.now/performance.now, render/ can never import commands.js. All
pass on first run — the codebase honors its own architecture. Scanned for unused
exports (all hits were internal-use false positives; no churn). Final suite:
166 tests pass.
STATUS: every checkbox in REBOOT_PLAN.md (M0-M10) is complete. The loop's work
queue is empty. Future direction ideas: bespoke mission objectives/triggers,
spectator fog toggle for replays, sprite art upgrades, multiplayer over the
command stream.

## 2026-07-07 — M11 (iPad port)
Did: responsive fullscreen shell — fixed 1280x720 internal canvas CSS-scaled to any
screen (16:9 letterboxed), all pointer input mapped through canvasPos(); PWA
(manifest.webmanifest, generated icons, iOS metas: Add to Home Screen = fullscreen
landscape app, no rubber-banding/zoom); touch controls in input.js wireTouch — tap
select/order (C&C left-click style via shared primaryMapAction), one-finger drag from
empty ground = box-select, two-finger drag = camera pan, long-press = attack-move,
render/touchbar.js on-screen command bar (stop/attack/deploy/harvest/repair/sell/
pause, 44px targets) shared with mouse clicks; WebAudio resume on first gesture
(iOS suspends contexts). Verified with Playwright touch emulation at iPad 1180x820:
menu -> skirmish via taps, tap-select shows brackets + full command bar, zero page
exceptions; screenshots in docs/shot_ipad_*.png. Tests: 170 pass (4 new).
Notes: pinch-zoom not implemented (fixed TILE renderer); marquee requires starting
on empty ground — starting on a unit selects it instead, matching finger accuracy.

## 2026-07-07 — M12.1 + M12.2 (original terrain + sounds)
Did: TmpFile decoder in formats.py (TD template format verified against OpenRA's
TmpTDLoader: magic 0x0D1AFFFF, byte index table, raw 24x24 tiles); importer now
emits terrain tile variants (clear/water/rock/rough), tiberium overlay stages
(TI*.TEM as SHP), a tree object, and ~45 original sound effects with a manifest
audio list; game loads tile/tiberium/tree overrides and plays original OGGs via
lazy WebAudio buffers (synth fallback preserved). First REAL-file contact succeeded
on the owner's Mac: innoextract cracked the v1.06c installer, 39 MIXes found,
37 sprite sheets imported and rendering. Python-3.9 compat fixed (no X|None).
Tests: 170 pass + 22 format self-tests.
Notes: owner reports "some errors" during import — awaiting the '-' lines to fix
per-file mappings. Missions (M12.3) and videos (M12.4) queued; videos need the ISO
or the separate video pack (the 1.06c installer has no MOVIES.MIX).
