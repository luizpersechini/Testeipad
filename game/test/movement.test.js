import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { idx } from '../src/sim/world.js';
import { spawn, get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { moveCommand, stopCommand } from '../src/sim/commands.js';

// Deterministic flat arena for movement scenarios.
function arena() {
  const game = createGame(1);
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  return game;
}

function spawnTank(game, x, y, facing = 8) {
  return spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI, x, y, hp: 400, facing,
  });
}

function runTicks(game, n, commandsAtTick = {}) {
  for (let i = 0; i < n; i++) {
    gameTick(game, commandsAtTick[game.tick] ?? []);
  }
}

test('unit ordered A to B arrives, frees origin, and goes idle', () => {
  const game = arena();
  const id = spawnTank(game, 2, 2, 8); // already facing east
  runTicks(game, 100, { 0: [moveCommand([id], 8, 2)] });
  const e = get(game.store, id);
  assert.equal(e.x, 8);
  assert.equal(e.y, 2);
  assert.equal(e.state, 'idle');
  assert.equal(e.path, null);
  assert.equal(game.world.occupancy[idx(game.world, 2, 2)], -1);
  assert.equal(game.world.occupancy[idx(game.world, 8, 2)], id);
});

test('arrival takes the expected tick count (speed math is stable)', () => {
  // Medium tank speed 5 -> 125 progress/tick -> 8 ticks per orthogonal cell.
  // 6 cells east, already facing east = 48 ticks.
  const game = arena();
  const id = spawnTank(game, 2, 2, 8);
  let arrivedAt = -1;
  gameTick(game, [moveCommand([id], 8, 2)]);
  for (let i = 0; i < 100 && arrivedAt === -1; i++) {
    gameTick(game);
    const e = get(game.store, id);
    if (e.x === 8 && e.state === 'idle') arrivedAt = game.tick;
  }
  // Movement starts on the command tick itself: 6 cells * 8 ticks = 48.
  assert.equal(arrivedAt, 48);
});

test('unit turns before driving when facing away', () => {
  const game = arena();
  const id = spawnTank(game, 2, 2, 24); // facing west, must turn around
  gameTick(game, [moveCommand([id], 8, 2)]);
  const e = get(game.store, id);
  runTicks(game, 2);
  assert.equal(e.x, 2, 'still turning in place');
  runTicks(game, 100);
  assert.equal(e.x, 8, 'arrives after turning');
});

test('stop command halts and clears the order', () => {
  const game = arena();
  const id = spawnTank(game, 2, 2, 8);
  runTicks(game, 10, { 0: [moveCommand([id], 20, 2)] });
  const e = get(game.store, id);
  const xAtStop = e.x;
  gameTick(game, [stopCommand([id])]);
  runTicks(game, 20);
  assert.equal(e.x, xAtStop);
  assert.equal(e.state, 'idle');
});

test('two units to the same spot: one takes it, the other settles nearby', () => {
  const game = arena();
  const a = spawnTank(game, 2, 2, 8);
  const b = spawnTank(game, 2, 6, 8);
  runTicks(game, 300, { 0: [moveCommand([a, b], 10, 4)] });
  const ea = get(game.store, a);
  const eb = get(game.store, b);
  assert.equal(ea.state, 'idle');
  assert.equal(eb.state, 'idle');
  const dist = (e) => Math.max(Math.abs(e.x - 10), Math.abs(e.y - 4));
  const [d1, d2] = [dist(ea), dist(eb)].sort();
  assert.equal(d1, 0, 'one unit reaches the exact cell');
  assert.ok(d2 <= 3, `other stops nearby, got distance ${d2}`);
  assert.ok(ea.x !== eb.x || ea.y !== eb.y, 'never stacked');
});

test('same seed + same command stream => identical state (iron rule 3)', () => {
  const mk = () => {
    const game = createGame(42);
    const s = game.world.startPositions[0];
    const id = spawn(game.store, game.world, {
      kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI, x: s.x, y: s.y, hp: 400,
    });
    runTicks(game, 120, { 0: [moveCommand([id], s.x + 10, s.y + 6)] });
    const e = get(game.store, id);
    return { x: e.x, y: e.y, facing: e.facing, state: e.state, rng: game.rng.getState() };
  };
  assert.deepEqual(mk(), mk());
});

test('infantry walk through trees on the way', () => {
  const game = arena();
  for (let y = 0; y < game.world.h; y++) {
    game.world.terrain[idx(game.world, 6, y)] = TerrainType.TREE;
  }
  const inf = spawn(game.store, game.world, {
    kind: EntityKind.INFANTRY, type: 'minigunner', owner: HouseType.GDI, x: 2, y: 2, hp: 50, facing: 8,
  });
  runTicks(game, 200, { 0: [moveCommand([inf], 10, 2)] });
  const e = get(game.store, inf);
  assert.equal(e.x, 10);
  assert.equal(e.state, 'idle');
});
