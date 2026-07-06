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
