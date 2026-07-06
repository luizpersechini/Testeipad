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
