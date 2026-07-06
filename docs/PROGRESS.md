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
