// Entity store: units, infantry, buildings, projectiles. Insertion-ordered
// Map keeps iteration deterministic (iron rule 3). Ids are monotonic and
// never reused, so stale references can be detected.

import { SIM_FACINGS, RENDER_FACINGS } from './constants.js';
import { idx, inBounds, NO_ENTITY } from './world.js';

export const EntityKind = Object.freeze({
  UNIT: 0,
  INFANTRY: 1,
  BUILDING: 2,
  PROJECTILE: 3,
});

// Kinds that occupy their cell exclusively (projectiles fly over).
function occupies(kind) {
  return kind !== EntityKind.PROJECTILE;
}

export function createStore() {
  return {
    entities: new Map(), // id -> entity
    nextId: 1,
  };
}

export function get(store, id) {
  return store.entities.get(id);
}

export function count(store) {
  return store.entities.size;
}

// props: { kind, type, owner, x, y, hp, facing? }
// Returns the new entity id, or NO_ENTITY if the target cell is blocked for
// an occupying kind.
export function spawn(store, world, props) {
  const { kind, x, y } = props;
  if (occupies(kind)) {
    if (!inBounds(world, x, y)) return NO_ENTITY;
    if (world.occupancy[idx(world, x, y)] !== NO_ENTITY) return NO_ENTITY;
  }
  const id = store.nextId++;
  const entity = {
    id,
    kind,
    type: props.type,
    owner: props.owner,
    x, // cell coords
    y,
    subX: 0, // fractional cell offset in [-0.5, 0.5) for smooth movement
    subY: 0,
    facing: props.facing ?? 0, // 0..31 like the original, 0 = north, clockwise
    hp: props.hp,
    maxHp: props.hp,
    state: 'idle',
    path: null, // filled by pathfinding
    target: NO_ENTITY,
    reload: 0, // ticks until the weapon can fire again
  };
  store.entities.set(id, entity);
  if (occupies(kind)) {
    world.occupancy[idx(world, x, y)] = id;
  }
  return id;
}

export function despawn(store, world, id) {
  const entity = store.entities.get(id);
  if (!entity) return false;
  if (occupies(entity.kind)) {
    const i = idx(world, entity.x, entity.y);
    if (world.occupancy[i] === id) {
      world.occupancy[i] = NO_ENTITY;
    }
  }
  store.entities.delete(id);
  return true;
}

// Move an occupying entity to a new cell, updating the occupancy grid.
// Returns false (and leaves everything unchanged) if the destination is taken.
export function relocate(store, world, id, nx, ny) {
  const entity = store.entities.get(id);
  if (!entity || !inBounds(world, nx, ny)) return false;
  if (occupies(entity.kind)) {
    const to = idx(world, nx, ny);
    if (world.occupancy[to] !== NO_ENTITY && world.occupancy[to] !== id) return false;
    const from = idx(world, entity.x, entity.y);
    if (world.occupancy[from] === id) world.occupancy[from] = NO_ENTITY;
    world.occupancy[to] = id;
  }
  entity.x = nx;
  entity.y = ny;
  return true;
}

// Map sim facing (0..31, 0=N, clockwise) to the 8-column sprite sheet index.
export function facingTo8(facing32) {
  const f = ((facing32 % SIM_FACINGS) + SIM_FACINGS) % SIM_FACINGS;
  return ((f + 2) >> 2) % RENDER_FACINGS;
}

// Facing (0..31) that points from (x0,y0) toward (x1,y1). 0 = north (-y),
// clockwise, matching the original's compass convention.
export function facingToward(x0, y0, x1, y1) {
  const angle = Math.atan2(x1 - x0, -(y1 - y0)); // 0 rad = north, cw positive
  let f = Math.round(angle / (2 * Math.PI) * SIM_FACINGS);
  return ((f % SIM_FACINGS) + SIM_FACINGS) % SIM_FACINGS;
}
