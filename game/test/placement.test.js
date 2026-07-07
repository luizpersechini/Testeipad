import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { idx, NO_ENTITY, isOccupied } from '../src/sim/world.js';
import { spawn, get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { deployCommand, moveCommand } from '../src/sim/commands.js';
import {
  canPlaceBuilding, placeBuilding, footprintClear, deployMcv,
} from '../src/sim/placement.js';
import { findPath } from '../src/sim/path.js';

function arena() {
  const game = createGame(1);
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  return game;
}

test('footprint occupancy: a 3x3 yard claims and frees all nine cells', () => {
  const game = arena();
  const id = placeBuilding(game, HouseType.GDI, 'construction_yard', 10, 10, { ignoreAdjacency: true });
  assert.ok(id !== NO_ENTITY);
  for (let dy = 0; dy < 3; dy++) {
    for (let dx = 0; dx < 3; dx++) {
      assert.equal(game.world.occupancy[idx(game.world, 10 + dx, 10 + dy)], id);
    }
  }
  // Pathing routes around it.
  const path = findPath(game.world, 8, 11, 15, 11);
  for (const step of path) {
    assert.ok(!(step.x >= 10 && step.x <= 12 && step.y >= 10 && step.y <= 12),
      'path never crosses the yard');
  }
});

test('placement rejects blocked terrain, tiberium, overlap, and off-map', () => {
  const game = arena();
  placeBuilding(game, HouseType.GDI, 'construction_yard', 10, 10, { ignoreAdjacency: true });
  // Overlap.
  assert.equal(canPlaceBuilding(game, HouseType.GDI, 'power_plant', 11, 11), false);
  // Rock under one footprint cell.
  game.world.terrain[idx(game.world, 15, 10)] = TerrainType.ROCK;
  assert.equal(footprintClear(game.world, 'power_plant', 14, 10), false);
  // Tiberium underneath.
  game.world.tiberium[idx(game.world, 14, 14)] = 5;
  assert.equal(footprintClear(game.world, 'power_plant', 14, 14), false);
  // Off-map.
  assert.equal(footprintClear(game.world, 'power_plant', 63, 63), false);
});

test('adjacency: must build near the base, first building exempt', () => {
  const game = arena();
  assert.equal(canPlaceBuilding(game, HouseType.GDI, 'power_plant', 30, 30), false,
    'no base yet, normal placement fails');
  placeBuilding(game, HouseType.GDI, 'construction_yard', 10, 10, { ignoreAdjacency: true });
  assert.equal(canPlaceBuilding(game, HouseType.GDI, 'power_plant', 14, 10), true,
    'hugging the yard is fine');
  assert.equal(canPlaceBuilding(game, HouseType.GDI, 'power_plant', 30, 30), false,
    'far from the base is not');
  assert.equal(canPlaceBuilding(game, HouseType.NOD, 'power_plant', 14, 13), false,
    'enemy base does not grant adjacency');
});

test('MCV deploys into a construction yard centered on its cell', () => {
  const game = arena();
  const mcv = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'mcv', owner: HouseType.GDI, x: 20, y: 20, hp: 600,
  });
  gameTick(game, [deployCommand([mcv])]);
  assert.equal(get(game.store, mcv), undefined, 'MCV consumed');
  const yard = [...game.store.entities.values()].find((e) => e.type === 'construction_yard');
  assert.ok(yard);
  assert.equal(yard.x, 19);
  assert.equal(yard.y, 19);
  assert.equal(yard.owner, HouseType.GDI);
});

test('blocked deploy leaves the MCV intact', () => {
  const game = arena();
  game.world.terrain[idx(game.world, 21, 21)] = TerrainType.ROCK; // corner of would-be yard
  const mcv = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'mcv', owner: HouseType.GDI, x: 20, y: 20, hp: 600,
  });
  const result = deployMcv(game, mcv);
  assert.equal(result, NO_ENTITY);
  const survivors = [...game.store.entities.values()];
  assert.equal(survivors.length, 1);
  assert.equal(survivors[0].type, 'mcv');
  assert.deepEqual([survivors[0].x, survivors[0].y], [20, 20]);
  assert.equal(isOccupied(game.world, 20, 20), true);
});

test('drive the MCV somewhere open, then deploy: the classic opening', () => {
  const game = arena();
  const mcv = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'mcv', owner: HouseType.GDI, x: 5, y: 5, hp: 600, facing: 8,
  });
  gameTick(game, [moveCommand([mcv], 15, 5)]);
  for (let i = 0; i < 400; i++) gameTick(game);
  assert.equal(get(game.store, mcv).x, 15);
  gameTick(game, [deployCommand([mcv])]);
  const yard = [...game.store.entities.values()].find((e) => e.type === 'construction_yard');
  assert.ok(yard, 'base established');
});
