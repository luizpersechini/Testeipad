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
