import { test } from 'node:test';
import assert from 'node:assert/strict';
import { HouseType } from '../src/sim/constants.js';
import { spawn, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { enableAI } from '../src/sim/ai.js';
import { moveCommand } from '../src/sim/commands.js';
import { serializeGame, deserializeGame, SAVE_VERSION } from '../src/sim/save.js';

function fingerprint(game) {
  let acc = `${game.tick}|${game.rng.getState()}|${game.winner}`;
  for (const h of game.houses) {
    acc += `|${h.credits?.toFixed?.(4) ?? h.credits}:${JSON.stringify(h.queues ?? null)}`;
  }
  for (const e of game.store.entities.values()) {
    acc += `|${e.id}:${e.type}:${e.x},${e.y}:${e.hp}:${e.state}:${e.facing}`;
  }
  let fogAcc = 0;
  for (const f of game.fog) {
    if (!f) continue;
    for (let i = 0; i < f.length; i++) fogAcc = (fogAcc * 31 + f[i]) >>> 0;
  }
  return `${acc}|fog:${fogAcc}`;
}

// A busy mid-game: AI war in progress with a human move order in flight.
function busyGame() {
  const game = createGame(13, { startingCredits: 6000 });
  const [s0, s1] = game.world.startPositions;
  const tank = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI,
    x: s0.x, y: s0.y, hp: 400,
  });
  spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'mcv', owner: HouseType.NOD, x: s1.x, y: s1.y, hp: 600,
  });
  enableAI(game, HouseType.NOD, 'normal');
  gameTick(game, [moveCommand([tank], s0.x + 15, s0.y + 10)]);
  for (let i = 0; i < 700; i++) gameTick(game);
  return game;
}

test('serialize -> deserialize preserves the exact game state', () => {
  const game = busyGame();
  const restored = deserializeGame(serializeGame(game));
  assert.ok(restored);
  assert.equal(fingerprint(restored), fingerprint(game));
});

test('a loaded game continues bit-identically (the gold test)', () => {
  const original = busyGame();
  const restored = deserializeGame(serializeGame(original));
  for (let i = 0; i < 800; i++) {
    gameTick(original);
    gameTick(restored);
  }
  assert.equal(fingerprint(restored), fingerprint(original));
});

test('serialization survives a JSON round trip (localStorage shape)', () => {
  const game = busyGame();
  const json = JSON.stringify(serializeGame(game));
  const restored = deserializeGame(JSON.parse(json));
  for (let i = 0; i < 300; i++) {
    gameTick(game);
    gameTick(restored);
  }
  assert.equal(fingerprint(restored), fingerprint(game));
});

test('version mismatch is rejected, not mis-loaded', () => {
  const data = serializeGame(busyGame());
  data.version = SAVE_VERSION + 1;
  assert.equal(deserializeGame(data), null);
});
