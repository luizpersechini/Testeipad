import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { spawn, get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { placeBuilding } from '../src/sim/placement.js';
import { attackCommand, harvestCommand } from '../src/sim/commands.js';
import { enableAI } from '../src/sim/ai.js';
import { idx } from '../src/sim/world.js';

function arena() {
  const game = createGame(1, { startingCredits: 0 });
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  return game;
}

function run(game, n) {
  for (let i = 0; i < n; i++) gameTick(game);
}

test('no winner while both sides can produce; loser declared when razed', () => {
  const game = arena();
  placeBuilding(game, HouseType.GDI, 'construction_yard', 5, 5, { ignoreAdjacency: true });
  const nodYard = placeBuilding(game, HouseType.NOD, 'barracks', 30, 30, { ignoreAdjacency: true });
  run(game, 40);
  assert.equal(game.winner, null, 'both alive');
  // Raze the Nod barracks with an overwhelming force.
  const tanks = [];
  for (let i = 0; i < 4; i++) {
    tanks.push(spawn(game.store, game.world, {
      kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI,
      x: 26, y: 29 + i, hp: 400, facing: 8,
    }));
  }
  gameTick(game, [attackCommand(tanks, nodYard)]);
  for (let i = 0; i < 3000 && game.winner === null; i++) gameTick(game);
  assert.equal(game.winner, HouseType.GDI, 'GDI wins when Nod cannot produce');
});

test('an MCV keeps a razed house in the game', () => {
  const game = arena();
  placeBuilding(game, HouseType.GDI, 'construction_yard', 5, 5, { ignoreAdjacency: true });
  placeBuilding(game, HouseType.NOD, 'war_factory', 30, 30, { ignoreAdjacency: true });
  run(game, 20);
  // Nod loses its factory but still holds an MCV.
  spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'mcv', owner: HouseType.NOD, x: 40, y: 40, hp: 600,
  });
  const wf = [...game.store.entities.values()].find((e) => e.type === 'war_factory');
  wf.hp = 0;
  game.store.entities.delete(wf.id); // simulate destruction
  run(game, 40);
  assert.equal(game.winner, null, 'MCV means the fight is not over');
});

test('stats accumulate: built, lost, kills, harvested', () => {
  const game = arena();
  placeBuilding(game, HouseType.GDI, 'construction_yard', 3, 3, { ignoreAdjacency: true });
  // Harvest a bit.
  for (let x = 12; x <= 13; x++) game.world.tiberium[idx(game.world, x, 10)] = 6;
  const refinery = placeBuilding(game, HouseType.GDI, 'refinery', 3, 8);
  assert.ok(refinery, 'refinery placed');
  const harv = [...game.store.entities.values()].find((e) => e.type === 'harvester');
  assert.ok(harv, 'free harvester arrived with the refinery');
  gameTick(game, [harvestCommand([harv.id])]);
  run(game, 2500);
  const stats = game.houses[HouseType.GDI].stats;
  assert.ok(stats.creditsHarvested > 0, `harvest counted (${stats.creditsHarvested})`);
  assert.equal(stats.buildingsBuilt, 2);

  // A kill.
  const tank = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI, x: 20, y: 20, hp: 400, facing: 8,
  });
  const victim = spawn(game.store, game.world, {
    kind: EntityKind.INFANTRY, type: 'minigunner', owner: HouseType.NOD, x: 23, y: 20, hp: 50,
  });
  gameTick(game, [attackCommand([tank], victim)]);
  for (let i = 0; i < 600 && get(game.store, victim); i++) gameTick(game);
  assert.equal(stats.kills, 1);
  assert.equal(game.houses[HouseType.NOD].stats.unitsLost, 1);
});

test('full AI-vs-AI war runs to a decisive winner', () => {
  const game = createGame(3, { startingCredits: 6000 });
  const [s0, s1] = game.world.startPositions;
  spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'mcv', owner: HouseType.GDI, x: s0.x, y: s0.y, hp: 600,
  });
  spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'mcv', owner: HouseType.NOD, x: s1.x, y: s1.y, hp: 600,
  });
  enableAI(game, HouseType.GDI, 'hard');
  enableAI(game, HouseType.NOD, 'easy');
  let ticks = 0;
  for (; ticks < 30000 && game.winner === null; ticks++) gameTick(game);
  assert.notEqual(game.winner, null, `war concluded by tick ${ticks}`);
  assert.equal(game.winner, HouseType.GDI, 'hard AI beats easy AI');
});
