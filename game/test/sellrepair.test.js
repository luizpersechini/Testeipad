import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType, TILE } from '../src/sim/constants.js';
import { get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { placeBuilding } from '../src/sim/placement.js';
import { sellCommand, repairCommand } from '../src/sim/commands.js';
import { BUILDING_TYPES } from '../src/sim/data/buildings.js';
import { isOccupied } from '../src/sim/world.js';
import { pickEntityAt } from '../src/input.js';

function base(credits = 1000) {
  const game = createGame(1, { startingCredits: credits });
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  const yard = placeBuilding(game, HouseType.GDI, 'construction_yard', 10, 10, { ignoreAdjacency: true });
  return { game, yard };
}

test('selling a healthy building refunds half its cost and frees the cells', () => {
  const { game } = base(0);
  const pp = placeBuilding(game, HouseType.GDI, 'power_plant', 14, 10);
  gameTick(game, [sellCommand([pp])]);
  assert.equal(get(game.store, pp), undefined);
  assert.equal(game.houses[HouseType.GDI].credits, BUILDING_TYPES.power_plant.cost / 2);
  assert.equal(isOccupied(game.world, 14, 10), false);
});

test('selling a damaged building refunds proportionally less', () => {
  const { game } = base(0);
  const pp = placeBuilding(game, HouseType.GDI, 'power_plant', 14, 10);
  get(game.store, pp).hp = 200; // half health
  gameTick(game, [sellCommand([pp])]);
  assert.equal(game.houses[HouseType.GDI].credits, Math.floor(300 / 2 / 2));
});

test('repair heals to full, drains credits, then switches itself off', () => {
  const { game } = base(1000);
  const pp = placeBuilding(game, HouseType.GDI, 'power_plant', 14, 10);
  const e = get(game.store, pp);
  e.hp = 100; // 300 hp missing
  const startCredits = game.houses[HouseType.GDI].credits;
  gameTick(game, [repairCommand([pp])]);
  for (let i = 0; i < 400; i++) gameTick(game);
  assert.equal(e.hp, e.maxHp, 'fully repaired');
  assert.equal(e.repairing, false, 'auto-off at full');
  // 300hp of a 400hp/300-cost plant: (300/2)*(300/400) = 112.5 credits.
  const spent = startCredits - game.houses[HouseType.GDI].credits;
  assert.ok(Math.abs(spent - 112.5) < 1, `repair bill ~112.5, got ${spent}`);
});

test('repair pauses when broke and resumes with money', () => {
  const { game } = base(0);
  const pp = placeBuilding(game, HouseType.GDI, 'power_plant', 14, 10);
  const e = get(game.store, pp);
  e.hp = 100;
  gameTick(game, [repairCommand([pp])]);
  for (let i = 0; i < 100; i++) gameTick(game);
  assert.equal(e.hp, 100, 'no free repairs');
  game.houses[HouseType.GDI].credits = 500;
  for (let i = 0; i < 400; i++) gameTick(game);
  assert.equal(e.hp, e.maxHp);
});

test('buildings are pickable across their whole footprint', () => {
  const { game, yard } = base();
  // Yard is 3x3 at (10,10); click its far corner cell.
  const hit = pickEntityAt(game.store, 12 * TILE + 5, 12 * TILE + 5);
  assert.equal(hit.id, yard);
});
