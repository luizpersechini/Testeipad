import { test } from 'node:test';
import assert from 'node:assert/strict';
import { HouseType } from '../src/sim/constants.js';
import { createWorld, idx, NO_ENTITY, isOccupied } from '../src/sim/world.js';
import {
  createStore, spawn, despawn, get, count, relocate,
  EntityKind, facingTo8, facingToward,
} from '../src/sim/entity.js';

function setup() {
  return { store: createStore(), world: createWorld(42) };
}

const tank = (x, y) => ({
  kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI, x, y, hp: 400,
});

test('spawn assigns unique ids and marks occupancy', () => {
  const { store, world } = setup();
  const a = spawn(store, world, tank(8, 8));
  const b = spawn(store, world, tank(9, 8));
  assert.ok(a !== NO_ENTITY && b !== NO_ENTITY && a !== b);
  assert.equal(world.occupancy[idx(world, 8, 8)], a);
  assert.equal(isOccupied(world, 9, 8), true);
  assert.equal(count(store), 2);
  assert.equal(get(store, a).maxHp, 400);
});

test('spawn refuses an occupied or out-of-bounds cell', () => {
  const { store, world } = setup();
  spawn(store, world, tank(8, 8));
  assert.equal(spawn(store, world, tank(8, 8)), NO_ENTITY);
  assert.equal(spawn(store, world, tank(-1, 0)), NO_ENTITY);
  assert.equal(count(store), 1);
});

test('projectiles do not occupy cells', () => {
  const { store, world } = setup();
  const a = spawn(store, world, tank(8, 8));
  const p = spawn(store, world, {
    kind: EntityKind.PROJECTILE, type: 'bullet', owner: HouseType.GDI, x: 8, y: 8, hp: 1,
  });
  assert.ok(p !== NO_ENTITY);
  assert.equal(world.occupancy[idx(world, 8, 8)], a, 'tank keeps the cell');
  despawn(store, world, p);
  assert.equal(world.occupancy[idx(world, 8, 8)], a);
});

test('despawn frees the cell and forgets the entity', () => {
  const { store, world } = setup();
  const a = spawn(store, world, tank(8, 8));
  assert.equal(despawn(store, world, a), true);
  assert.equal(get(store, a), undefined);
  assert.equal(isOccupied(world, 8, 8), false);
  assert.equal(despawn(store, world, a), false, 'double despawn is a no-op');
});

test('ids are never reused after despawn', () => {
  const { store, world } = setup();
  const a = spawn(store, world, tank(8, 8));
  despawn(store, world, a);
  const b = spawn(store, world, tank(8, 8));
  assert.ok(b > a);
});

test('relocate moves occupancy atomically and refuses blocked cells', () => {
  const { store, world } = setup();
  const a = spawn(store, world, tank(8, 8));
  const b = spawn(store, world, tank(9, 8));
  assert.equal(relocate(store, world, a, 9, 8), false, 'blocked by b');
  assert.equal(get(store, a).x, 8);
  assert.equal(relocate(store, world, a, 8, 9), true);
  assert.equal(isOccupied(world, 8, 8), false);
  assert.equal(world.occupancy[idx(world, 8, 9)], a);
  assert.equal(world.occupancy[idx(world, 9, 8)], b);
});

test('iteration order is insertion order (determinism)', () => {
  const { store, world } = setup();
  const ids = [spawn(store, world, tank(8, 8)), spawn(store, world, tank(9, 8)),
    spawn(store, world, tank(10, 8))];
  assert.deepEqual([...store.entities.keys()], ids);
});

test('facingTo8 maps 32 facings onto 8 sheet columns', () => {
  assert.equal(facingTo8(0), 0); // N
  assert.equal(facingTo8(4), 1); // NE
  assert.equal(facingTo8(8), 2); // E
  assert.equal(facingTo8(16), 4); // S
  assert.equal(facingTo8(24), 6); // W
  assert.equal(facingTo8(31), 0); // wraps back to N
  assert.equal(facingTo8(-4), facingTo8(28), 'negative facings normalize');
});

test('facingToward points at the target', () => {
  assert.equal(facingToward(5, 5, 5, 0), 0); // due north
  assert.equal(facingToward(5, 5, 10, 5), 8); // due east
  assert.equal(facingToward(5, 5, 5, 10), 16); // due south
  assert.equal(facingToward(5, 5, 0, 5), 24); // due west
  assert.equal(facingToward(5, 5, 10, 0), 4); // northeast
});
