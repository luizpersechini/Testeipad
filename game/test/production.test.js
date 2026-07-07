import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { buildCommand, cancelBuildCommand, placeCommand } from '../src/sim/commands.js';
import { placeBuilding } from '../src/sim/placement.js';
import { availableToBuild, findExitCell } from '../src/sim/production.js';
import { BUILDING_TYPES } from '../src/sim/data/buildings.js';
import { INFANTRY_TYPES } from '../src/sim/data/infantry.js';

// Base with yard + power + barracks + refinery + war factory, rich house.
function base(owner = HouseType.GDI, credits = 10000) {
  const game = createGame(1, { startingCredits: credits });
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  placeBuilding(game, owner, 'construction_yard', 10, 10, { ignoreAdjacency: true });
  placeBuilding(game, owner, 'power_plant', 14, 10);
  placeBuilding(game, owner, 'barracks', 14, 13);
  placeBuilding(game, owner, 'refinery', 10, 14);
  placeBuilding(game, owner, 'war_factory', 14, 16);
  return game;
}

function run(game, n) {
  for (let i = 0; i < n; i++) gameTick(game);
}

test('tech tree gates what can be built', () => {
  const game = createGame(1, { startingCredits: 10000 });
  game.world.terrain.fill(TerrainType.CLEAR);
  assert.deepEqual(availableToBuild(game, HouseType.GDI, 'infantry'), [],
    'no barracks, no infantry');
  placeBuilding(game, HouseType.GDI, 'construction_yard', 10, 10, { ignoreAdjacency: true });
  const buildable = availableToBuild(game, HouseType.GDI, 'buildings');
  assert.ok(buildable.includes('power_plant'));
  assert.ok(!buildable.includes('war_factory'), 'war factory needs power + refinery');
  assert.ok(!buildable.includes('hand_of_nod'), 'GDI cannot build Nod structures');
});

test('full base unlocks the GDI roster; Nod differs', () => {
  const gdi = base(HouseType.GDI);
  const inf = availableToBuild(gdi, HouseType.GDI, 'infantry');
  assert.ok(inf.includes('minigunner') && inf.includes('grenadier'));
  assert.ok(!inf.includes('flamethrower'), 'flamer is Nod-only');
  const units = availableToBuild(gdi, HouseType.GDI, 'units');
  assert.ok(units.includes('medium_tank') && units.includes('harvester'));
  assert.ok(!units.includes('mammoth_tank'), 'mammoth needs comm center tech');
  assert.ok(!units.includes('light_tank'), 'light tank is Nod-only');
});

test('infantry build drains exact cost over time and spawns at the barracks', () => {
  const game = base();
  const startCredits = game.houses[HouseType.GDI].credits;
  const before = game.store.entities.size;
  gameTick(game, [buildCommand(HouseType.GDI, 'infantry', 'minigunner')]);
  const dur = INFANTRY_TYPES.minigunner.buildTime * 2;
  run(game, dur + 4);
  assert.equal(game.store.entities.size, before + 1, 'one new entity');
  const trooper = [...game.store.entities.values()].at(-1);
  assert.equal(trooper.type, 'minigunner');
  assert.equal(trooper.kind, EntityKind.INFANTRY);
  // Rolled out adjacent to the barracks footprint (2x2 at 14,13).
  assert.ok(trooper.x >= 12 && trooper.x <= 17 && trooper.y >= 11 && trooper.y <= 16);
  assert.ok(
    Math.abs(startCredits - 100 - game.houses[HouseType.GDI].credits) < 0.01,
    'exactly the unit cost was drained',
  );
});

test('no credits means the queue stalls, then resumes', () => {
  const game = base(HouseType.GDI, 3); // almost broke
  gameTick(game, [buildCommand(HouseType.GDI, 'infantry', 'minigunner')]);
  run(game, 200);
  const q = game.houses[HouseType.GDI].queues.infantry;
  assert.ok(q, 'still queued, not finished');
  assert.ok(q.ticksLeft > 0);
  game.houses[HouseType.GDI].credits += 500;
  run(game, INFANTRY_TYPES.minigunner.buildTime * 2 + 4);
  assert.equal(game.houses[HouseType.GDI].queues.infantry, null, 'finished after refunding');
});

test('cancel refunds everything paid so far', () => {
  const game = base();
  const start = game.houses[HouseType.GDI].credits;
  gameTick(game, [buildCommand(HouseType.GDI, 'units', 'medium_tank')]);
  run(game, 50);
  assert.ok(game.houses[HouseType.GDI].credits < start, 'money sunk');
  gameTick(game, [cancelBuildCommand(HouseType.GDI, 'units')]);
  assert.ok(Math.abs(game.houses[HouseType.GDI].credits - start) < 0.01, 'fully refunded');
});

test('low power halves production speed', () => {
  const fast = base();
  const slow = base();
  // Kill the power plant in the slow base (drain > output).
  const plant = [...slow.store.entities.values()].find((e) => e.type === 'power_plant');
  plant.hp = 0;
  gameTick(fast, [buildCommand(HouseType.GDI, 'infantry', 'minigunner')]);
  gameTick(slow, [buildCommand(HouseType.GDI, 'infantry', 'minigunner')]);
  const dur = INFANTRY_TYPES.minigunner.buildTime * 2;
  run(fast, dur + 2);
  run(slow, dur + 2);
  assert.equal(fast.houses[HouseType.GDI].queues.infantry, null, 'full power finished');
  assert.ok(slow.houses[HouseType.GDI].queues.infantry, 'low power still going');
});

test('building completes to ready, then places via command', () => {
  const game = base();
  gameTick(game, [buildCommand(HouseType.GDI, 'buildings', 'guard_tower')]);
  const dur = BUILDING_TYPES.guard_tower.buildTime * 2;
  run(game, dur + 2);
  const q = game.houses[HouseType.GDI].queues.buildings;
  assert.ok(q?.ready, 'construction ready, awaiting placement');
  // Bad spot (far away) is rejected, queue stays.
  gameTick(game, [placeCommand(HouseType.GDI, 40, 40)]);
  assert.ok(game.houses[HouseType.GDI].queues.buildings, 'rejected placement keeps it');
  // Good spot next to the yard.
  gameTick(game, [placeCommand(HouseType.GDI, 8, 10)]);
  assert.equal(game.houses[HouseType.GDI].queues.buildings, null);
  const tower = [...game.store.entities.values()].find((e) => e.type === 'guard_tower');
  assert.deepEqual([tower.x, tower.y], [8, 10]);
});

test('findExitCell skirts the factory footprint', () => {
  const game = base();
  const wf = [...game.store.entities.values()].find((e) => e.type === 'war_factory');
  const exit = findExitCell(game, wf);
  assert.ok(exit, 'an exit exists');
  const inside = exit.x >= wf.x && exit.x < wf.x + 3 && exit.y >= wf.y && exit.y < wf.y + 2;
  assert.ok(!inside, 'exit is outside the footprint');
});
