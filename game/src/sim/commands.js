// Player commands: the only doorway through which anything outside the sim
// (input, AI, network someday) mutates game state. Commands are plain
// serializable objects; a recorded {tick, command} stream plus the seed
// replays the whole game.

export function moveCommand(ids, x, y) {
  return { type: 'move', ids: [...ids], x, y };
}

export function stopCommand(ids) {
  return { type: 'stop', ids: [...ids] };
}

export function attackCommand(ids, targetId) {
  return { type: 'attack', ids: [...ids], targetId };
}

// Advance toward (x,y), engaging any enemy that comes into sight on the way.
export function attackMoveCommand(ids, x, y) {
  return { type: 'attackmove', ids: [...ids], x, y };
}

// Fire on a ground cell regardless of what is there (flame/artillery work).
export function forceAttackCommand(ids, x, y) {
  return { type: 'forceattack', ids: [...ids], x, y };
}
