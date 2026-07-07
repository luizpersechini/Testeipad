import { test } from 'node:test';
import assert from 'node:assert/strict';
import { HouseType } from '../src/sim/constants.js';
import { EntityKind } from '../src/sim/entity.js';
import { gameTick } from '../src/sim/game.js';
import { createSkirmishGame, buildFromSetup } from '../src/sim/setup.js';
import {
  createRecording, recordCommands, createPlayback, playbackCommands, runReplay,
} from '../src/sim/replay.js';
import { moveCommand, attackMoveCommand, buildCommand } from '../src/sim/commands.js';

const SETTINGS = { faction: 'gdi', credits: 5000, difficulty: 'normal', seed: 21 };

function fingerprint(game) {
  let acc = `${game.tick}|${game.rng.getState()}|${game.winner}`;
  for (const h of game.houses) acc += `|${h.credits?.toFixed?.(4)}`;
  for (const e of game.store.entities.values()) {
    acc += `|${e.id}:${e.type}:${e.x},${e.y}:${e.hp}:${e.state}`;
  }
  return acc;
}

// Play a live match, issuing human orders on specific ticks, while recording.
function playLive(ticks) {
  const { game, player } = createSkirmishGame(SETTINGS);
  const recording = createRecording({ mode: 'skirmish', settings: SETTINGS });
  const myUnits = () => [...game.store.entities.values()]
    .filter((e) => e.owner === player && e.kind === EntityKind.UNIT)
    .map((e) => e.id);
  for (let i = 0; i < ticks; i++) {
    let commands = [];
    if (game.tick === 30) commands = [moveCommand(myUnits(), 20, 20)];
    if (game.tick === 300) commands = [buildCommand(player, 'infantry', 'minigunner')];
    if (game.tick === 600) commands = [attackMoveCommand(myUnits(), 40, 40)];
    recordCommands(recording, game.tick, commands);
    gameTick(game, commands);
  }
  return { game, recording };
}

test('replaying a recorded match reproduces it bit-identically', () => {
  const { game, recording } = playLive(1200);
  const replayed = runReplay(recording, 1200);
  assert.equal(fingerprint(replayed), fingerprint(game));
});

test('recording is sparse (only ticks with commands are stored)', () => {
  const { recording } = playLive(800);
  assert.equal(recording.log.length, 3);
  assert.deepEqual(recording.log.map((entry) => entry.t), [30, 300, 600]);
});

test('replays survive a JSON round trip (localStorage shape)', () => {
  const { game, recording } = playLive(900);
  const rehydrated = JSON.parse(JSON.stringify(recording));
  const replayed = runReplay(rehydrated, 900);
  assert.equal(fingerprint(replayed), fingerprint(game));
});

test('playback cursor feeds commands exactly once at the right ticks', () => {
  const { recording } = playLive(700);
  const playback = createPlayback(recording);
  let fed = 0;
  for (let i = 0; i < 700; i++) {
    const commands = playbackCommands(playback, playback.game.tick);
    fed += commands.length;
    gameTick(playback.game, commands);
  }
  assert.equal(fed, recording.log.reduce((n, entry) => n + entry.commands.length, 0));
  assert.equal(playbackCommands(playback, playback.game.tick).length, 0, 'log exhausted');
});

test('scenario setups rebuild through the same door', () => {
  const built = buildFromSetup({ mode: 'scenario', id: 'gdi_1' });
  assert.ok(built);
  assert.equal(built.player, HouseType.GDI);
  assert.ok(built.game.ais?.length === 1, 'scenario AI enabled by buildFromSetup');
  assert.equal(buildFromSetup({ mode: 'scenario', id: 'nope' }), null);
});
