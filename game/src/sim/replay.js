// Replays: determinism means a replay is just {setup, sparse command log}.
// Rebuild the game from the setup and feed the same commands at the same
// ticks — the entire match re-unfolds identically.

import { buildFromSetup } from './setup.js';
import { gameTick } from './game.js';

// setup: {mode:'skirmish', settings} | {mode:'scenario', id}
export function createRecording(setup) {
  return { version: 1, setup, log: [] }; // log: [{t, commands}] sparse
}

// Call once per tick BEFORE gameTick with the human commands for that tick.
export function recordCommands(recording, tick, commands) {
  if (commands.length > 0) {
    recording.log.push({ t: tick, commands });
  }
}

// Fresh playback state over a recording.
export function createPlayback(recording) {
  const built = buildFromSetup(recording.setup);
  if (!built) return null;
  return { ...built, recording, cursor: 0 };
}

// Commands recorded for this tick (advances the cursor).
export function playbackCommands(playback, tick) {
  const { recording } = playback;
  if (playback.cursor < recording.log.length
    && recording.log[playback.cursor].t === tick) {
    return recording.log[playback.cursor++].commands;
  }
  return [];
}

// Convenience for tests/tools: run a whole recording headless for n ticks.
export function runReplay(recording, ticks) {
  const playback = createPlayback(recording);
  for (let i = 0; i < ticks; i++) {
    gameTick(playback.game, playbackCommands(playback, playback.game.tick));
  }
  return playback.game;
}
