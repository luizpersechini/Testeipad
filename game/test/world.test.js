import { test } from 'node:test';
import assert from 'node:assert/strict';
import { MAP_W, MAP_H, TerrainType, TIBERIUM_MAX_STAGE } from '../src/sim/constants.js';
import {
  createWorld, idx, inBounds, isPassable, isOccupied, NO_ENTITY,
} from '../src/sim/world.js';

test('world has expected dimensions and array sizes', () => {
  const world = createWorld(42);
  assert.equal(world.w, MAP_W);
  assert.equal(world.h, MAP_H);
  assert.equal(world.terrain.length, MAP_W * MAP_H);
  assert.equal(world.tiberium.length, MAP_W * MAP_H);
  assert.equal(world.occupancy.length, MAP_W * MAP_H);
});

test('world generation is deterministic for equal seeds', () => {
  const a = createWorld(1234);
  const b = createWorld(1234);
  assert.deepEqual(a.terrain, b.terrain);
  assert.deepEqual(a.tiberium, b.tiberium);
});

test('different seeds produce different maps', () => {
  const a = createWorld(1);
  const b = createWorld(2);
  let diff = 0;
  for (let i = 0; i < a.terrain.length; i++) {
    if (a.terrain[i] !== b.terrain[i]) diff++;
  }
  assert.ok(diff > 0, 'terrain should differ between seeds');
});

test('start zones are clear and passable', () => {
  const world = createWorld(42);
  assert.equal(world.startPositions.length, 2);
  for (const start of world.startPositions) {
    for (let dy = -4; dy <= 4; dy++) {
      for (let dx = -4; dx <= 4; dx++) {
        assert.ok(isPassable(world, start.x + dx, start.y + dy),
          `cell (${start.x + dx},${start.y + dy}) near start should be passable`);
      }
    }
  }
});

test('passability: out of bounds and non-clear terrain block', () => {
  const world = createWorld(42);
  assert.equal(isPassable(world, -1, 0), false);
  assert.equal(isPassable(world, 0, -1), false);
  assert.equal(isPassable(world, world.w, 0), false);
  assert.equal(isPassable(world, 0, world.h), false);
  world.terrain[idx(world, 20, 20)] = TerrainType.ROCK;
  assert.equal(isPassable(world, 20, 20), false);
  world.terrain[idx(world, 20, 20)] = TerrainType.WATER;
  assert.equal(isPassable(world, 20, 20), false);
  world.terrain[idx(world, 20, 20)] = TerrainType.CLEAR;
  assert.equal(isPassable(world, 20, 20), true);
});

test('map terrain is 180-degree rotation symmetric outside start zones', () => {
  const world = createWorld(777);
  const starts = world.startPositions;
  const nearStart = (x, y) => starts.some(
    (s) => Math.abs(x - s.x) <= 6 && Math.abs(y - s.y) <= 6,
  );
  for (let y = 0; y < world.h >> 1; y++) {
    for (let x = 0; x < world.w; x++) {
      const mx = world.w - 1 - x;
      const my = world.h - 1 - y;
      if (nearStart(x, y) || nearStart(mx, my)) continue;
      assert.equal(
        world.terrain[idx(world, x, y)],
        world.terrain[idx(world, mx, my)],
        `terrain mismatch between (${x},${y}) and mirror (${mx},${my})`,
      );
    }
  }
});

test('tiberium exists, stays in stage range, and never sits on blocked terrain', () => {
  const world = createWorld(42);
  let count = 0;
  for (let i = 0; i < world.tiberium.length; i++) {
    const t = world.tiberium[i];
    assert.ok(t >= 0 && t <= TIBERIUM_MAX_STAGE);
    if (t > 0) {
      count++;
      assert.equal(world.terrain[i], TerrainType.CLEAR,
        'tiberium must only grow on clear terrain');
    }
  }
  assert.ok(count >= 30, `expected a meaningful tiberium economy, got ${count} cells`);
});

test('occupancy starts empty and inBounds behaves', () => {
  const world = createWorld(42);
  assert.equal(isOccupied(world, 5, 5), false);
  assert.equal(world.occupancy[0], NO_ENTITY);
  assert.equal(inBounds(world, 0, 0), true);
  assert.equal(inBounds(world, world.w - 1, world.h - 1), true);
  assert.equal(inBounds(world, world.w, world.h), false);
});
