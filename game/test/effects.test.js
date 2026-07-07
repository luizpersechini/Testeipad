import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { spawn, get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { attackCommand } from '../src/sim/commands.js';
import {
  createEffects, spawnFromEvents, pruneEffects, explosionFrameAt,
} from '../src/render/draw_effects.js';

function arena() {
  const game = createGame(1);
  game.world.terrain.fill(TerrainType.CLEAR);
  return game;
}

test('sim events become effects with correct lifetimes', () => {
  const fx = createEffects();
  spawnFromEvents(fx, [
    { type: 'death', x: 5, y: 5 },
    { type: 'shot', x: 2, y: 2, facing: 8 },
    { type: 'hit', x: 5, y: 5, targetId: 9 },
  ], 100);
  assert.equal(fx.list.length, 3);

  pruneEffects(fx, 103); // muzzle (ttl 3) expires
  assert.deepEqual(fx.list.map((f) => f.kind), ['explosion', 'hit']);
  pruneEffects(fx, 104); // hit (ttl 4) expires
  assert.deepEqual(fx.list.map((f) => f.kind), ['explosion']);
  pruneEffects(fx, 112); // explosion (ttl 12) expires
  assert.equal(fx.list.length, 0);
});

test('explosion animates through all six frames', () => {
  const fx = { start: 100, ttl: 12 };
  assert.equal(explosionFrameAt(fx, 100), 0);
  assert.equal(explosionFrameAt(fx, 105), 2);
  assert.equal(explosionFrameAt(fx, 111), 5);
  assert.equal(explosionFrameAt(fx, 999), 5, 'clamps at the last frame');
});

test('a real skirmish produces shot, hit, and death events end to end', () => {
  const game = arena();
  const a = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI, x: 5, y: 5, hp: 400, facing: 8,
  });
  const b = spawn(game.store, game.world, {
    kind: EntityKind.INFANTRY, type: 'minigunner', owner: HouseType.NOD, x: 8, y: 5, hp: 50,
  });
  const fx = createEffects();
  const seen = new Set();
  gameTick(game, [attackCommand([a], b)]);
  for (let i = 0; i < 500 && get(game.store, b); i++) {
    gameTick(game);
    spawnFromEvents(fx, game.events, game.tick);
    for (const ev of game.events) seen.add(ev.type);
  }
  assert.ok(seen.has('shot'), 'shots fired');
  assert.ok(seen.has('hit'), 'hits landed');
  assert.ok(seen.has('death'), 'target died');
  assert.ok(fx.list.some((f) => f.kind === 'explosion'), 'death spawned an explosion');
});

test('return fire: artillery outranging sight still gets attacked back', () => {
  const game = arena();
  // Artillery range 6, tank sight 4: tank cannot see it, but gets hit.
  const arty = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'artillery', owner: HouseType.NOD, x: 2, y: 5, hp: 100, facing: 8,
  });
  const tank = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI, x: 8, y: 5, hp: 400, facing: 24,
  });
  gameTick(game, [attackCommand([arty], tank)]);
  let returned = false;
  for (let i = 0; i < 300; i++) {
    gameTick(game);
    const t = get(game.store, tank);
    if (t && t.state === 'attacking' && t.attackTarget === arty) {
      returned = true;
      break;
    }
  }
  assert.ok(returned, 'tank turned on its attacker beyond sight range');
});
