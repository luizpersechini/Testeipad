import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { createWorld, idx } from '../src/sim/world.js';
import { createStore, spawn, EntityKind } from '../src/sim/entity.js';
import { findPath, cellEnterable } from '../src/sim/path.js';

// Empty flat world for controlled scenarios.
function flatWorld() {
  const world = createWorld(1, 32, 32);
  world.terrain.fill(TerrainType.CLEAR);
  world.tiberium.fill(0);
  return world;
}

function wallColumn(world, x, y0, y1) {
  for (let y = y0; y <= y1; y++) {
    world.terrain[idx(world, x, y)] = TerrainType.ROCK;
  }
}

test('straight-line path has the expected length and endpoint', () => {
  const world = flatWorld();
  const path = findPath(world, 2, 2, 10, 2);
  assert.equal(path.length, 8);
  assert.deepEqual(path.at(-1), { x: 10, y: 2 });
});

test('diagonal path uses diagonal steps', () => {
  const world = flatWorld();
  const path = findPath(world, 2, 2, 7, 7);
  assert.equal(path.length, 5, 'pure diagonal should be 5 steps');
});

test('path routes around a wall', () => {
  const world = flatWorld();
  wallColumn(world, 8, 0, 20);
  const path = findPath(world, 2, 10, 14, 10);
  assert.ok(path, 'path exists around the wall');
  assert.deepEqual(path.at(-1), { x: 14, y: 10 });
  for (const step of path) {
    assert.notEqual(world.terrain[idx(world, step.x, step.y)], TerrainType.ROCK);
  }
  assert.ok(path.length > 12, 'detour must be longer than the straight line');
});

test('fully enclosed goal yields closest reachable cell', () => {
  const world = flatWorld();
  // Box in the goal at (20, 10).
  for (const [x, y] of [[19, 9], [20, 9], [21, 9], [19, 10], [21, 10], [19, 11], [20, 11], [21, 11]]) {
    world.terrain[idx(world, x, y)] = TerrainType.ROCK;
  }
  const path = findPath(world, 2, 10, 20, 10);
  assert.ok(path && path.length > 0, 'partial path toward the box');
  const end = path.at(-1);
  const d = Math.max(Math.abs(end.x - 20), Math.abs(end.y - 10));
  assert.ok(d <= 2, `should stop adjacent to the box, got distance ${d}`);
});

test('no diagonal corner cutting through blocked cells', () => {
  const world = flatWorld();
  // Blocked cells forming a corner at (5,5)/(6,6) gap.
  world.terrain[idx(world, 6, 5)] = TerrainType.ROCK;
  world.terrain[idx(world, 5, 6)] = TerrainType.ROCK;
  const path = findPath(world, 5, 5, 6, 6);
  // Direct diagonal is illegal; path must go around.
  assert.ok(path.length > 1, 'must not squeeze through the corner');
});

test('trees block vehicles but not infantry', () => {
  const world = flatWorld();
  wallColumn(world, 8, 0, 31);
  for (let y = 0; y < 32; y++) {
    world.terrain[idx(world, 8, y)] = TerrainType.TREE;
  }
  const vehPath = findPath(world, 2, 10, 14, 10);
  assert.ok(vehPath, 'vehicles get a partial path up to the tree line');
  for (const step of vehPath) {
    assert.ok(step.x < 8, 'vehicles never enter or cross the tree line');
  }
  const infPath = findPath(world, 2, 10, 14, 10, { infantry: true });
  assert.ok(infPath, 'infantry pass through trees');
  assert.deepEqual(infPath.at(-1), { x: 14, y: 10 });
});

test('occupied cells are avoided but an occupied goal is approachable', () => {
  const world = flatWorld();
  const store = createStore();
  const blocker = spawn(store, world, {
    kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.NOD, x: 6, y: 2, hp: 400,
  });
  assert.ok(blocker);
  assert.equal(cellEnterable(world, 6, 2), false);
  const path = findPath(world, 2, 2, 6, 2);
  assert.ok(path, 'can path toward an occupied goal');
  assert.deepEqual(path.at(-1), { x: 6, y: 2 }, 'ends on the goal; mover resolves approach');
});

test('pathfinding is deterministic', () => {
  const world = createWorld(42);
  const [s, t] = world.startPositions;
  const a = findPath(world, s.x, s.y, t.x, t.y);
  const b = findPath(world, s.x, s.y, t.x, t.y);
  assert.ok(a && a.length > 0, 'cross-map path exists');
  assert.deepEqual(a, b);
});
