import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { idx } from '../src/sim/world.js';
import { spawn, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { harvestCommand } from '../src/sim/commands.js';
import { enableAI, DIFFICULTY, incomeMultiplier } from '../src/sim/ai.js';
import { HARVESTER_CAPACITY, CREDITS_PER_BAIL } from '../src/sim/economy.js';

test('difficulty tiers scale monotonically', () => {
  assert.ok(DIFFICULTY.easy.waveSize < DIFFICULTY.normal.waveSize);
  assert.ok(DIFFICULTY.normal.waveSize < DIFFICULTY.hard.waveSize);
  assert.ok(DIFFICULTY.easy.waveCooldown > DIFFICULTY.hard.waveCooldown);
  assert.ok(DIFFICULTY.easy.incomeMult < 1 && DIFFICULTY.hard.incomeMult > 1);
});

function harvestGame(difficulty) {
  const game = createGame(1, { startingCredits: 0 });
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  for (let x = 10; x <= 14; x++) game.world.tiberium[idx(game.world, x, 10)] = 8;
  spawn(game.store, game.world, {
    kind: EntityKind.BUILDING, type: 'refinery', owner: HouseType.NOD, x: 3, y: 3, hp: 450,
  });
  const harv = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'harvester', owner: HouseType.NOD, x: 5, y: 5, hp: 600,
  });
  if (difficulty) enableAI(game, HouseType.NOD, difficulty);
  gameTick(game, [harvestCommand([harv])]);
  for (let i = 0; i < 2500 && game.houses[HouseType.NOD].credits === 0; i++) gameTick(game);
  return game.houses[HouseType.NOD].credits;
}

test('hard AI earns more per load, easy AI less, humans exactly face value', () => {
  const full = HARVESTER_CAPACITY * CREDITS_PER_BAIL;
  assert.equal(harvestGame(null), full, 'no AI: face value');
  assert.equal(harvestGame('hard'), Math.round(full * 1.4));
  assert.equal(harvestGame('easy'), Math.round(full * 0.8));
});

test('incomeMultiplier only applies to AI-controlled houses', () => {
  const game = createGame(1);
  enableAI(game, HouseType.NOD, 'hard');
  assert.equal(incomeMultiplier(game, HouseType.GDI), 1.0);
  assert.equal(incomeMultiplier(game, HouseType.NOD), 1.4);
});
