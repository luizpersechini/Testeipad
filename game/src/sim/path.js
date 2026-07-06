// A* pathfinding on the cell grid (original reference: FINDPATH.CPP).
// 8-directional, diagonal cost 141/100, no cutting corners past blocked cells.
// Deterministic: binary heap ties break on insertion order.

import { TerrainType } from './constants.js';
import { idx, inBounds, NO_ENTITY } from './world.js';

const ORTHO_COST = 100;
const DIAG_COST = 141;
const MAX_EXPANSIONS = 4096;

const DIRS = [
  { dx: 0, dy: -1, cost: ORTHO_COST }, { dx: 1, dy: -1, cost: DIAG_COST },
  { dx: 1, dy: 0, cost: ORTHO_COST }, { dx: 1, dy: 1, cost: DIAG_COST },
  { dx: 0, dy: 1, cost: ORTHO_COST }, { dx: -1, dy: 1, cost: DIAG_COST },
  { dx: -1, dy: 0, cost: ORTHO_COST }, { dx: -1, dy: -1, cost: DIAG_COST },
];

// Can `moverId` plan through cell (x,y)? Trees admit infantry only.
// Other entities block unless the cell is the goal (approach handling is the
// mover's problem at execution time).
export function cellEnterable(world, x, y, { infantry = false, moverId = NO_ENTITY } = {}) {
  if (!inBounds(world, x, y)) return false;
  const t = world.terrain[idx(world, x, y)];
  if (t === TerrainType.ROCK || t === TerrainType.WATER) return false;
  if (t === TerrainType.TREE && !infantry) return false;
  const occ = world.occupancy[idx(world, x, y)];
  if (occ !== NO_ENTITY && occ !== moverId) return false;
  return true;
}

function heuristic(x0, y0, x1, y1) {
  // Octile distance in the same cost units as edges.
  const dx = Math.abs(x1 - x0);
  const dy = Math.abs(y1 - y0);
  return ORTHO_COST * Math.max(dx, dy) + (DIAG_COST - ORTHO_COST) * Math.min(dx, dy);
}

// Min-heap keyed by f, ties by insertion sequence for determinism.
function heapPush(heap, node) {
  heap.push(node);
  let i = heap.length - 1;
  while (i > 0) {
    const p = (i - 1) >> 1;
    if (heap[p].f < node.f || (heap[p].f === node.f && heap[p].seq < node.seq)) break;
    heap[i] = heap[p];
    i = p;
  }
  heap[i] = node;
}

function heapPop(heap) {
  const top = heap[0];
  const last = heap.pop();
  if (heap.length === 0) return top;
  let i = 0;
  for (;;) {
    const l = i * 2 + 1;
    const r = l + 1;
    let best = i;
    const better = (a, b) => a.f < b.f || (a.f === b.f && a.seq < b.seq);
    if (l < heap.length && better(heap[l], best === i ? last : heap[best])) best = l;
    if (r < heap.length && better(heap[r], best === i ? last : heap[best])) best = r;
    if (best === i) break;
    heap[i] = heap[best];
    i = best;
  }
  heap[i] = last;
  return top;
}

// Returns an array of {x, y} steps from start (exclusive) to the goal, or to
// the closest reachable cell if the goal itself can't be reached. Returns []
// when already at the goal, null when no progress is possible at all.
export function findPath(world, sx, sy, tx, ty, opts = {}) {
  if (sx === tx && sy === ty) return [];
  if (!inBounds(world, sx, sy) || !inBounds(world, tx, ty)) return null;

  const w = world.w;
  const size = w * world.h;
  const gScore = new Int32Array(size).fill(-1);
  const cameFrom = new Int32Array(size).fill(-1);
  const closed = new Uint8Array(size);

  const startI = sy * w + sx;
  const goalI = ty * w + tx;
  gScore[startI] = 0;

  let seq = 0;
  const heap = [];
  heapPush(heap, { i: startI, f: heuristic(sx, sy, tx, ty), seq: seq++ });

  // Track the expanded node closest to the goal for partial paths.
  let bestI = startI;
  let bestH = heuristic(sx, sy, tx, ty);

  let expansions = 0;
  while (heap.length > 0 && expansions < MAX_EXPANSIONS) {
    const { i } = heapPop(heap);
    if (closed[i]) continue;
    closed[i] = 1;
    expansions++;

    if (i === goalI) {
      bestI = goalI;
      break;
    }
    const cx = i % w;
    const cy = (i / w) | 0;
    const h = heuristic(cx, cy, tx, ty);
    if (h < bestH) {
      bestH = h;
      bestI = i;
    }

    for (const d of DIRS) {
      const nx = cx + d.dx;
      const ny = cy + d.dy;
      const isGoal = nx === tx && ny === ty;
      if (!isGoal && !cellEnterable(world, nx, ny, opts)) continue;
      if (isGoal && !inBounds(world, nx, ny)) continue;
      // Goal cell is allowed even when occupied (approach), but not when the
      // terrain itself is impassable.
      if (isGoal && !cellEnterable(world, nx, ny, { ...opts, moverId: world.occupancy[ny * w + nx] })) continue;
      // No diagonal corner cutting: both orthogonal neighbors must be open.
      if (d.dx !== 0 && d.dy !== 0) {
        if (!cellEnterable(world, cx + d.dx, cy, opts)) continue;
        if (!cellEnterable(world, cx, cy + d.dy, opts)) continue;
      }
      const ni = ny * w + nx;
      if (closed[ni]) continue;
      const tentative = gScore[i] + d.cost;
      if (gScore[ni] !== -1 && tentative >= gScore[ni]) continue;
      gScore[ni] = tentative;
      cameFrom[ni] = i;
      heapPush(heap, { i: ni, f: tentative + heuristic(nx, ny, tx, ty), seq: seq++ });
    }
  }

  if (bestI === startI) return null;

  const path = [];
  for (let i = bestI; i !== startI; i = cameFrom[i]) {
    path.push({ x: i % w, y: (i / w) | 0 });
  }
  path.reverse();
  return path;
}
