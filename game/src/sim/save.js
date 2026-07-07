// Save/load: the sim is plain data by design, so serialization is mostly
// converting typed arrays / Map / Set to JSON shapes and back. A loaded game
// must continue bit-identically (there is a test for this).

import { createRng } from './rng.js';
import { createStore } from './entity.js';

export const SAVE_VERSION = 1;

export function serializeGame(game) {
  return {
    version: SAVE_VERSION,
    seed: game.seed,
    tick: game.tick,
    rngState: game.rng.getState(),
    winner: game.winner,
    participants: [...game.participants],
    houses: structuredClone(game.houses),
    ais: structuredClone(game.ais ?? null),
    world: {
      w: game.world.w,
      h: game.world.h,
      terrain: Array.from(game.world.terrain),
      tiberium: Array.from(game.world.tiberium),
      occupancy: Array.from(game.world.occupancy),
      startPositions: structuredClone(game.world.startPositions),
    },
    nextId: game.store.nextId,
    entities: [...game.store.entities.values()].map((e) => structuredClone(e)),
    fog: game.fog.map((f) => (f ? Array.from(f) : null)),
  };
}

export function deserializeGame(data) {
  if (data.version !== SAVE_VERSION) return null;
  const world = {
    w: data.world.w,
    h: data.world.h,
    terrain: Uint8Array.from(data.world.terrain),
    tiberium: Uint8Array.from(data.world.tiberium),
    occupancy: Int32Array.from(data.world.occupancy),
    startPositions: structuredClone(data.world.startPositions),
  };
  const store = createStore();
  store.nextId = data.nextId;
  for (const e of data.entities) {
    store.entities.set(e.id, structuredClone(e));
  }
  const rng = createRng(data.seed);
  rng.setState(data.rngState);
  return {
    seed: data.seed,
    rng,
    world,
    store,
    tick: data.tick,
    events: [],
    houses: structuredClone(data.houses),
    ais: data.ais ? structuredClone(data.ais) : undefined,
    fog: data.fog.map((f) => (f ? Uint8Array.from(f) : null)),
    winner: data.winner,
    participants: new Set(data.participants),
  };
}
