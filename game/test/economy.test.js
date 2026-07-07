import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { idx } from '../src/sim/world.js';
import { spawn, get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { harvestCommand } from '../src/sim/commands.js';
import {
  findNearestTiberium, HARVESTER_CAPACITY, CREDITS_PER_BAIL,
} from '../src/sim/economy.js';

function totalTiberium(world) {
  let sum = 0;
  for (let i = 0; i < world.tiberium.length; i++) sum += world.tiberium[i];
  return sum;
}

function harvestArena() {
  const game = createGame(1, { startingCredits: 0 });
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  // Rich patch: 5 cells x stage 8 = 40 bails available.
  for (let x = 10; x <= 14; x++) {
    game.world.tiberium[idx(game.world, x, 10)] = 8;
  }
  const refinery = spawn(game.store, game.world, {
    kind: EntityKind.BUILDING, type: 'refinery', owner: HouseType.GDI, x: 3, y: 3, hp: 450,
  });
  const harv = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'harvester', owner: HouseType.GDI, x: 5, y: 5, hp: 600,
  });
  return { game, refinery, harv };
}

test('tiberium grows and spreads over time, deterministically', () => {
  const a = createGame(42);
  const b = createGame(42);
  const before = totalTiberium(a.world);
  for (let i = 0; i < 3000; i++) {
    gameTick(a);
    gameTick(b);
  }
  const after = totalTiberium(a.world);
  assert.ok(after > before, `tiberium economy expands (${before} -> ${after})`);
  assert.equal(after, totalTiberium(b.world), 'growth is deterministic');
});

test('tiberium never grows on rock or water', () => {
  const game = createGame(42);
  for (let i = 0; i < 5000; i++) gameTick(game);
  const { world } = game;
  for (let i = 0; i < world.tiberium.length; i++) {
    if (world.tiberium[i] > 0) {
      assert.equal(world.terrain[i], TerrainType.CLEAR);
    }
  }
});

test('findNearestTiberium picks the closest cell', () => {
  const { game } = harvestArena();
  const hit = findNearestTiberium(game.world, 5, 10);
  assert.deepEqual(hit, { x: 10, y: 10 });
  assert.equal(findNearestTiberium(game.world, 50, 50, 5), null);
});

test('full harvest cycle: seek, load 28 bails, deliver 700 credits', () => {
  const { game, harv } = harvestArena();
  gameTick(game, [harvestCommand([harv])]);
  let deliveredAt = -1;
  for (let i = 0; i < 3000 && deliveredAt === -1; i++) {
    gameTick(game);
    if (game.houses[HouseType.GDI].credits > 0) deliveredAt = game.tick;
  }
  assert.ok(deliveredAt > 0, 'delivery happened');
  assert.equal(
    game.houses[HouseType.GDI].credits,
    HARVESTER_CAPACITY * CREDITS_PER_BAIL,
    'exactly one full load of credits',
  );
  // Field lost exactly the harvested bails.
  let remaining = 0;
  for (let x = 10; x <= 14; x++) remaining += game.world.tiberium[idx(game.world, x, 10)];
  assert.equal(remaining, 40 - HARVESTER_CAPACITY);
  // And the harvester heads straight back out.
  const e = get(game.store, harv);
  assert.ok(['harvest_seek', 'harvest_load'].includes(e.state), `back to work, state=${e.state}`);
});

test('harvester keeps cycling until the field is stripped', () => {
  const { game, harv } = harvestArena();
  gameTick(game, [harvestCommand([harv])]);
  for (let i = 0; i < 9000; i++) gameTick(game);
  // 40 bails total in the patch (growth may add a little nearby, so >=).
  assert.ok(
    game.houses[HouseType.GDI].credits >= 40 * CREDITS_PER_BAIL,
    `stripped the whole field, got ${game.houses[HouseType.GDI].credits}`,
  );
  const e = get(game.store, harv);
  assert.ok(e.hp === 600, 'harvester unharmed');
});

test('harvest cycle is deterministic (iron rule 3)', () => {
  const play = () => {
    const { game, harv } = harvestArena();
    gameTick(game, [harvestCommand([harv])]);
    for (let i = 0; i < 2000; i++) gameTick(game);
    const e = get(game.store, harv);
    return `${game.houses[HouseType.GDI].credits}|${e.x},${e.y}|${e.state}|${e.bails}`;
  };
  assert.equal(play(), play());
});
