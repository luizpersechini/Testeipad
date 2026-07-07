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

// Send harvesters mining (they cycle field -> refinery on their own).
export function harvestCommand(ids) {
  return { type: 'harvest', ids: [...ids] };
}

// Deploy an MCV into a Construction Yard on the spot.
export function deployCommand(ids) {
  return { type: 'deploy', ids: [...ids] };
}

// Production (owner = house index issuing the order).
export function buildCommand(owner, category, unitType) {
  return { type: 'build', owner, category, unitType };
}

export function cancelBuildCommand(owner, category) {
  return { type: 'cancelbuild', owner, category };
}

// Place the finished building at (x, y) top-left.
export function placeCommand(owner, x, y) {
  return { type: 'placebuilding', owner, x, y };
}
