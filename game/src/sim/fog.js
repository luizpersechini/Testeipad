// Fog of war per house: 0 = shroud (never seen), 1 = fogged (seen before,
// not currently in sight), 2 = visible. Buildings linger on fogged ground;
// units vanish into it.

import { idx, inBounds } from './world.js';
import { EntityKind } from './entity.js';
import { statsFor } from './stats.js';

export const SHROUD = 0;
export const FOGGED = 1;
export const VISIBLE = 2;

const FOG_INTERVAL = 5; // ticks between visibility recomputes

export function createFog(world) {
  return new Uint8Array(world.w * world.h); // all shroud
}

function revealCircle(fogMap, world, cx, cy, radius) {
  const r2 = radius * radius;
  const x0 = Math.max(0, cx - radius);
  const x1 = Math.min(world.w - 1, cx + radius);
  const y0 = Math.max(0, cy - radius);
  const y1 = Math.min(world.h - 1, cy + radius);
  for (let y = y0; y <= y1; y++) {
    for (let x = x0; x <= x1; x++) {
      const dx = x - cx;
      const dy = y - cy;
      if (dx * dx + dy * dy <= r2) fogMap[idx(world, x, y)] = VISIBLE;
    }
  }
}

// Recompute a house's visibility from scratch: everything currently VISIBLE
// decays to FOGGED, then live sight sources re-reveal.
export function recomputeFog(game, owner) {
  const fogMap = game.fog[owner];
  if (!fogMap) return;
  for (let i = 0; i < fogMap.length; i++) {
    if (fogMap[i] === VISIBLE) fogMap[i] = FOGGED;
  }
  for (const e of game.store.entities.values()) {
    if (e.owner !== owner || e.hp <= 0 || e.kind === EntityKind.PROJECTILE) continue;
    const sight = statsFor(e)?.sight ?? 2;
    // Buildings see from their center-ish cell.
    const [fw, fh] = e.footprint ?? [1, 1];
    revealCircle(fogMap, game.world, e.x + (fw >> 1), e.y + (fh >> 1), sight);
  }
}

export function tickFog(game) {
  if (game.tick % FOG_INTERVAL !== 0) return;
  for (let owner = 0; owner < game.fog.length; owner++) {
    if (game.fog[owner]) recomputeFog(game, owner);
  }
}

export function fogAt(game, owner, x, y) {
  const fogMap = game.fog[owner];
  if (!fogMap || !inBounds(game.world, x, y)) return SHROUD;
  return fogMap[idx(game.world, x, y)];
}

// Can `owner` see this entity right now? Own things always; enemy units only
// on VISIBLE ground; enemy buildings also on FOGGED ground (last-known base).
export function entityVisibleTo(game, owner, e) {
  if (e.owner === owner) return true;
  const level = fogAt(game, owner, e.x, e.y);
  if (e.kind === EntityKind.BUILDING) return level >= FOGGED;
  return level === VISIBLE;
}
