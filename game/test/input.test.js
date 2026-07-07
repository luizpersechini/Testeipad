// Pure selection logic from input.js, plus the M2.5 "playable check" as a
// headless smoke test: build the real starting scene and run it.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TILE, HouseType, TerrainType } from '../src/sim/constants.js';
import { spawn, get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { pickEntityAt, entitiesInRect } from '../src/input.js';
import { moveCommand } from '../src/sim/commands.js';

function scene() {
  const game = createGame(7);
  game.world.terrain.fill(TerrainType.CLEAR);
  const gdi = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI, x: 5, y: 5, hp: 400,
  });
  const nod = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'light_tank', owner: HouseType.NOD, x: 5, y: 6, hp: 300,
  });
  const inf = spawn(game.store, game.world, {
    kind: EntityKind.INFANTRY, type: 'minigunner', owner: HouseType.GDI, x: 8, y: 5, hp: 50,
  });
  return { game, gdi, nod, inf };
}

test('pickEntityAt finds the entity in the clicked cell', () => {
  const { game, gdi, nod } = scene();
  const hit = pickEntityAt(game.store, 5 * TILE + 10, 5 * TILE + 10);
  assert.equal(hit.id, gdi);
  const enemy = pickEntityAt(game.store, 5 * TILE + 10, 6 * TILE + 10);
  assert.equal(enemy.id, nod, 'enemy is pickable (for target info)');
  assert.equal(pickEntityAt(game.store, 20 * TILE, 20 * TILE), null);
});

test('entitiesInRect box-selects own mobile units only, any corner order', () => {
  const { game, gdi, nod, inf } = scene();
  const hits = entitiesInRect(
    game.store,
    9 * TILE, 7 * TILE, 4 * TILE, 4 * TILE, // inverted corners on purpose
  );
  assert.ok(hits.includes(gdi));
  assert.ok(hits.includes(inf));
  assert.ok(!hits.includes(nod), 'enemy units are not box-selectable');
});

test('playable check (headless): starting scene runs 100 ticks with orders', () => {
  const game = createGame(42);
  const [gdiStart] = game.world.startPositions;
  const ids = [];
  for (let i = 0; i < 6; i++) {
    const id = spawn(game.store, game.world, {
      kind: i < 3 ? EntityKind.UNIT : EntityKind.INFANTRY,
      type: i < 3 ? 'medium_tank' : 'minigunner',
      owner: HouseType.GDI,
      x: gdiStart.x + (i % 3) - 1,
      y: gdiStart.y + ((i / 3) | 0) + 2,
      hp: i < 3 ? 400 : 50,
    });
    assert.ok(id > 0, 'starting layout must not overlap');
    ids.push(id);
  }
  gameTick(game, [moveCommand(ids, gdiStart.x + 8, gdiStart.y + 8)]);
  for (let i = 0; i < 99; i++) gameTick(game);
  assert.equal(game.tick, 100);
  let moved = 0;
  for (const id of ids) {
    const e = get(game.store, id);
    if (e.x !== gdiStart.x + (ids.indexOf(id) % 3) - 1
      || e.y !== gdiStart.y + ((ids.indexOf(id) / 3) | 0) + 2) moved++;
  }
  assert.ok(moved >= 4, `most units moved (${moved}/6)`);
});
