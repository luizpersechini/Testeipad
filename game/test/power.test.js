import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { spawn, get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { BUILDING_TYPES } from '../src/sim/data/buildings.js';
import { WEAPONS } from '../src/sim/data/weapons.js';
import { attackCommand } from '../src/sim/commands.js';

function arena() {
  const game = createGame(1);
  game.world.terrain.fill(TerrainType.CLEAR);
  return game;
}

const mkBld = (game, type, x, y, owner = HouseType.GDI) => spawn(
  game.store, game.world,
  { kind: EntityKind.BUILDING, type, owner, x, y, hp: BUILDING_TYPES[type].hp },
);

test('building table is sane and weapons resolve', () => {
  assert.ok(Object.keys(BUILDING_TYPES).length >= 14);
  for (const [name, d] of Object.entries(BUILDING_TYPES)) {
    assert.ok(d.hp > 0 && d.cost > 0, name);
    assert.ok(Array.isArray(d.footprint) && d.footprint.length === 2, name);
    assert.ok(['none', 'wood', 'aluminum', 'steel', 'concrete'].includes(d.armor), name);
    if (d.weapon) assert.ok(WEAPONS[d.weapon], `${name}: weapon ${d.weapon}`);
    assert.ok(typeof d.power === 'number', name);
  }
  assert.equal(BUILDING_TYPES.power_plant.power, 100);
  assert.equal(BUILDING_TYPES.obelisk.power, -150);
});

test('power balance sums output and drain per house', () => {
  const game = arena();
  mkBld(game, 'power_plant', 2, 2); // +100
  mkBld(game, 'refinery', 6, 2); // -30
  mkBld(game, 'barracks', 10, 2); // -10
  mkBld(game, 'power_plant', 2, 8, HouseType.NOD); // other house
  gameTick(game);
  const gdi = game.houses[HouseType.GDI];
  assert.equal(gdi.powerOutput, 100);
  assert.equal(gdi.powerDrain, 40);
  assert.equal(gdi.lowPower, false);
  assert.equal(game.houses[HouseType.NOD].powerOutput, 100);
});

test('too much drain flips the low power flag', () => {
  const game = arena();
  mkBld(game, 'power_plant', 2, 2); // +100
  mkBld(game, 'obelisk', 6, 2); // -150
  gameTick(game);
  assert.equal(game.houses[HouseType.GDI].lowPower, true);
});

test('damaged power plants produce proportionally less', () => {
  const game = arena();
  const pp = mkBld(game, 'power_plant', 2, 2);
  gameTick(game);
  assert.equal(game.houses[HouseType.GDI].powerOutput, 100);
  get(game.store, pp).hp = 200; // half damaged
  gameTick(game);
  assert.equal(game.houses[HouseType.GDI].powerOutput, 50);
});

test('defensive buildings fight back with their weapon', () => {
  const game = arena();
  const tower = mkBld(game, 'guard_tower', 5, 5); // machinegun, sight 5
  const raider = spawn(game.store, game.world, {
    kind: EntityKind.INFANTRY, type: 'minigunner', owner: HouseType.NOD, x: 8, y: 5, hp: 50,
  });
  for (let i = 0; i < 400 && get(game.store, raider); i++) gameTick(game);
  assert.equal(get(game.store, raider), undefined, 'tower shot the raider down');
  assert.ok(get(game.store, tower).hp > 250, 'tower survives rifle return fire');
});

test('destroying a building removes its power contribution', () => {
  const game = arena();
  mkBld(game, 'power_plant', 5, 5);
  const tank = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.NOD, x: 9, y: 5, hp: 400, facing: 24,
  });
  // Find the plant's id to target it.
  const plant = [...game.store.entities.values()].find((e) => e.kind === EntityKind.BUILDING);
  gameTick(game, [attackCommand([tank], plant.id)]);
  for (let i = 0; i < 1200 && get(game.store, plant.id); i++) gameTick(game);
  assert.equal(get(game.store, plant.id), undefined, 'plant destroyed');
  gameTick(game);
  assert.equal(game.houses[HouseType.GDI].powerOutput, 0);
});
