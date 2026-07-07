import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { spawn, EntityKind } from '../src/sim/entity.js';
import { createGame } from '../src/sim/game.js';
import { moveCommand, attackMoveCommand, stopCommand } from '../src/sim/commands.js';
import {
  createFeedback, markersFromCommands, feedbackFromEvents, pruneFeedback,
  decayShake, MARKER_TTL, PING_THROTTLE,
} from '../src/render/feedback.js';

test('order markers appear for movement commands only, and expire', () => {
  const fb = createFeedback();
  markersFromCommands(fb, [
    moveCommand([1], 10, 12),
    attackMoveCommand([1], 20, 22),
    stopCommand([1]),
  ], 100);
  assert.equal(fb.markers.length, 2);
  assert.equal(fb.markers[0].kind, 'move');
  assert.equal(fb.markers[1].kind, 'attack');
  pruneFeedback(fb, 100 + MARKER_TTL);
  assert.equal(fb.markers.length, 0);
});

test('hits on player buildings ping and alert, throttled', () => {
  const game = createGame(1);
  game.world.terrain.fill(TerrainType.CLEAR);
  const pp = spawn(game.store, game.world, {
    kind: EntityKind.BUILDING, type: 'power_plant', owner: HouseType.GDI,
    x: 10, y: 10, hp: 400, footprint: [2, 2],
  });
  const fb = createFeedback();
  const hit = { type: 'hit', x: 10, y: 10, targetId: pp };

  assert.equal(feedbackFromEvents(fb, [hit], game, HouseType.GDI, 100), true, 'first alert fires');
  assert.equal(fb.pings.length, 1);
  assert.equal(feedbackFromEvents(fb, [hit], game, HouseType.GDI, 110), false, 'throttled');
  assert.equal(fb.pings.length, 2, 'ping still recorded');
  assert.equal(
    feedbackFromEvents(fb, [hit], game, HouseType.GDI, 100 + PING_THROTTLE), true,
    'alert fires again after the throttle window',
  );
});

test('hits on units or enemy buildings do not raise the base alarm', () => {
  const game = createGame(1);
  game.world.terrain.fill(TerrainType.CLEAR);
  const tank = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI, x: 5, y: 5, hp: 400,
  });
  const enemyBld = spawn(game.store, game.world, {
    kind: EntityKind.BUILDING, type: 'power_plant', owner: HouseType.NOD,
    x: 20, y: 20, hp: 400, footprint: [2, 2],
  });
  const fb = createFeedback();
  const events = [
    { type: 'hit', x: 5, y: 5, targetId: tank },
    { type: 'hit', x: 20, y: 20, targetId: enemyBld },
  ];
  assert.equal(feedbackFromEvents(fb, events, game, HouseType.GDI, 50), false);
  assert.equal(fb.pings.length, 0);
});

test('building deaths shake harder than unit deaths; shake decays to zero', () => {
  const game = createGame(1);
  const fb = createFeedback();
  feedbackFromEvents(fb, [{ type: 'death', kind: EntityKind.UNIT }], game, 0, 10);
  const unitShake = fb.shake;
  feedbackFromEvents(fb, [{ type: 'death', kind: EntityKind.BUILDING }], game, 0, 11);
  assert.ok(fb.shake > unitShake);
  for (let i = 0; i < 60; i++) decayShake(fb);
  assert.equal(fb.shake, 0);
});
