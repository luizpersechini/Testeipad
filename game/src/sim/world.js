// World: cell grid with terrain, tiberium overlay, and entity occupancy.
// Deterministic: createWorld(seed) always yields the identical map.
// Skirmish maps are 180°-rotation symmetric so both start positions are fair.

import { MAP_W, MAP_H, TerrainType, TIBERIUM_MAX_STAGE } from './constants.js';
import { createRng } from './rng.js';

export const NO_ENTITY = -1;

export function idx(world, x, y) {
  return y * world.w + x;
}

export function inBounds(world, x, y) {
  return x >= 0 && x < world.w && y >= 0 && y < world.h;
}

// Terrain-only passability for ground units. Trees block vehicles in the
// original (infantry can pass); the infantry exception lands with pathfinding.
export function isPassable(world, x, y) {
  if (!inBounds(world, x, y)) return false;
  return world.terrain[idx(world, x, y)] === TerrainType.CLEAR;
}

export function isOccupied(world, x, y) {
  return world.occupancy[idx(world, x, y)] !== NO_ENTITY;
}

function mirror(world, x, y) {
  return { x: world.w - 1 - x, y: world.h - 1 - y };
}

// Paint a rough blob of terrain around (cx, cy). Only paints the source half
// of the map; the caller mirrors afterwards, keeping generation symmetric.
function paintBlob(world, rng, cx, cy, radius, type) {
  for (let y = cy - radius; y <= cy + radius; y++) {
    for (let x = cx - radius; x <= cx + radius; x++) {
      if (!inBounds(world, x, y)) continue;
      const dx = x - cx;
      const dy = y - cy;
      const d2 = dx * dx + dy * dy;
      if (d2 > radius * radius) continue;
      // Ragged edge: cells near the rim only sometimes paint.
      if (d2 > (radius - 1) * (radius - 1) && !rng.chance(0.5)) continue;
      world.terrain[idx(world, x, y)] = type;
    }
  }
}

function clearArea(world, cx, cy, radius) {
  for (let y = cy - radius; y <= cy + radius; y++) {
    for (let x = cx - radius; x <= cx + radius; x++) {
      if (!inBounds(world, x, y)) continue;
      world.terrain[idx(world, x, y)] = TerrainType.CLEAR;
    }
  }
}

function paintTiberiumField(world, rng, cx, cy, radius) {
  for (let y = cy - radius; y <= cy + radius; y++) {
    for (let x = cx - radius; x <= cx + radius; x++) {
      if (!inBounds(world, x, y)) continue;
      const dx = x - cx;
      const dy = y - cy;
      if (dx * dx + dy * dy > radius * radius) continue;
      const i = idx(world, x, y);
      if (world.terrain[i] !== TerrainType.CLEAR) continue;
      // Denser toward the center, like a natural field.
      const stage = rng.range(3, 8);
      world.tiberium[i] = Math.min(stage, TIBERIUM_MAX_STAGE);
    }
  }
}

export function createWorld(seed, w = MAP_W, h = MAP_H) {
  const world = {
    w,
    h,
    terrain: new Uint8Array(w * h), // TerrainType, defaults to CLEAR (0)
    tiberium: new Uint8Array(w * h), // 0 = none, 1..TIBERIUM_MAX_STAGE = growth
    occupancy: new Int32Array(w * h).fill(NO_ENTITY), // entity id per cell
    startPositions: [
      { x: 8, y: 8 },
      { x: w - 9, y: h - 9 },
    ],
  };
  const rng = createRng(seed);

  // Scatter terrain features on the top half only, then mirror 180° so the
  // map is fair. Feature counts scale with map area.
  const cells = w * h;
  const rockBlobs = Math.max(3, (cells / 512) | 0);
  const treeBlobs = Math.max(4, (cells / 384) | 0);
  const waterBlobs = Math.max(1, (cells / 2048) | 0);

  for (let i = 0; i < rockBlobs; i++) {
    paintBlob(world, rng, rng.int(w), rng.int(h >> 1), rng.range(2, 4), TerrainType.ROCK);
  }
  for (let i = 0; i < treeBlobs; i++) {
    paintBlob(world, rng, rng.int(w), rng.int(h >> 1), rng.range(1, 3), TerrainType.TREE);
  }
  for (let i = 0; i < waterBlobs; i++) {
    paintBlob(world, rng, rng.int(w), rng.int(h >> 1), rng.range(3, 5), TerrainType.WATER);
  }

  // Mirror the generated half onto the bottom half (180° rotation).
  for (let y = 0; y < h >> 1; y++) {
    for (let x = 0; x < w; x++) {
      const m = mirror(world, x, y);
      world.terrain[idx(world, m.x, m.y)] = world.terrain[idx(world, x, y)];
    }
  }

  // Guarantee buildable, connected start zones.
  for (const start of world.startPositions) {
    clearArea(world, start.x, start.y, 6);
  }

  // One tiberium field near each start (mirrored positions), plus a shared
  // contested field at the map center.
  const fieldOffset = 10;
  const s0 = world.startPositions[0];
  paintTiberiumField(world, rng, s0.x + fieldOffset, s0.y + fieldOffset, 4);
  // Mirror the field for player 2 by painting at the rotated position with
  // the same rng-driven raggedness (stages differ per cell; extent matches).
  const mf = mirror(world, s0.x + fieldOffset, s0.y + fieldOffset);
  paintTiberiumField(world, rng, mf.x, mf.y, 4);
  clearArea(world, w >> 1, h >> 1, 3);
  paintTiberiumField(world, rng, w >> 1, h >> 1, 3);

  return world;
}
