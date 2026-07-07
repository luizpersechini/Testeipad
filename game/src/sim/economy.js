// Economy: tiberium growth/spread and the harvester cycle
// (seek field -> load bails -> return to refinery -> unload -> repeat).
// Original scale: 28 bails per full load, 25 credits per bail = 700 credits.

import { TerrainType, TIBERIUM_MAX_STAGE } from './constants.js';
import { idx, inBounds, NO_ENTITY } from './world.js';
import { get, EntityKind } from './entity.js';
import { findPath } from './path.js';
import { statsFor } from './stats.js';

export const HARVESTER_CAPACITY = 28; // bails
export const CREDITS_PER_BAIL = 25;
const LOAD_TICKS_PER_BAIL = 8; // 28 bails ~ 224 ticks of munching
const UNLOAD_TICKS = 20; // docked at the refinery
const GROWTH_INTERVAL = 32; // slow ticks between tiberium updates
const GROWTH_SAMPLES = 24; // random cells considered per update
const SPREAD_STAGE = 6; // cells at this stage or higher may seed neighbors
const FIELD_SEARCH_RADIUS = 24;

// ── Tiberium growth ──────────────────────────────────────────────────────────

export function tickTiberium(game) {
  if (game.tick % GROWTH_INTERVAL !== 0) return;
  const { world, rng } = game;
  for (let s = 0; s < GROWTH_SAMPLES; s++) {
    const x = rng.int(world.w);
    const y = rng.int(world.h);
    const i = idx(world, x, y);
    const stage = world.tiberium[i];
    if (stage === 0) continue;
    if (stage < TIBERIUM_MAX_STAGE && rng.chance(0.5)) {
      world.tiberium[i] = stage + 1;
    } else if (stage >= SPREAD_STAGE) {
      // Seed one random neighbor if it's clear ground.
      const nx = x + rng.range(-1, 1);
      const ny = y + rng.range(-1, 1);
      if (inBounds(world, nx, ny)) {
        const ni = idx(world, nx, ny);
        if (world.tiberium[ni] === 0 && world.terrain[ni] === TerrainType.CLEAR) {
          world.tiberium[ni] = 1;
        }
      }
    }
  }
}

// ── Harvester cycle ──────────────────────────────────────────────────────────

// Nearest tiberium cell within radius; deterministic tie-break by scan order.
export function findNearestTiberium(world, cx, cy, radius = FIELD_SEARCH_RADIUS) {
  let best = null;
  let bestD = Infinity;
  const x0 = Math.max(0, cx - radius);
  const x1 = Math.min(world.w - 1, cx + radius);
  const y0 = Math.max(0, cy - radius);
  const y1 = Math.min(world.h - 1, cy + radius);
  for (let y = y0; y <= y1; y++) {
    for (let x = x0; x <= x1; x++) {
      if (world.tiberium[idx(world, x, y)] === 0) continue;
      const d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
      if (d < bestD) {
        bestD = d;
        best = { x, y };
      }
    }
  }
  return best;
}

function findRefinery(game, owner) {
  for (const e of game.store.entities.values()) {
    if (e.kind === EntityKind.BUILDING && e.type === 'refinery'
      && e.owner === owner && e.hp > 0) return e;
  }
  return null;
}

function pathTo(game, e, x, y) {
  e.dest = { x, y };
  e.path = findPath(game.world, e.x, e.y, x, y, { moverId: e.id });
  e.moveProgress = 0;
  e.repaths = 0;
  e.repathCooldown = 0;
}

export function orderHarvest(game, id) {
  const e = get(game.store, id);
  if (!e || !statsFor(e)?.harvester) return;
  e.bails = e.bails ?? 0;
  e.state = e.bails >= HARVESTER_CAPACITY ? 'harvest_return' : 'harvest_seek';
  e.harvestTimer = 0;
  e.path = null;
}

export function tickHarvester(game, e) {
  if (!statsFor(e)?.harvester) return;
  const world = game.world;

  switch (e.state) {
    case 'harvest_seek': {
      const onTib = world.tiberium[idx(world, e.x, e.y)] > 0;
      if (onTib) {
        e.state = 'harvest_load';
        e.path = null;
        e.harvestTimer = 0;
        return;
      }
      if (!e.path || e.path.length === 0) {
        const field = findNearestTiberium(world, e.x, e.y);
        if (!field) {
          // Nothing to mine: deliver what we carry, or give up.
          e.state = e.bails > 0 ? 'harvest_return' : 'idle';
          return;
        }
        pathTo(game, e, field.x, field.y);
      }
      return;
    }
    case 'harvest_load': {
      const i = idx(world, e.x, e.y);
      if (world.tiberium[i] === 0) {
        e.state = 'harvest_seek';
        return;
      }
      if (++e.harvestTimer >= LOAD_TICKS_PER_BAIL) {
        e.harvestTimer = 0;
        world.tiberium[i] -= 1;
        e.bails += 1;
        if (e.bails >= HARVESTER_CAPACITY) e.state = 'harvest_return';
      }
      return;
    }
    case 'harvest_return': {
      const refinery = findRefinery(game, e.owner);
      if (!refinery) {
        e.state = 'idle'; // nowhere to unload; wait for orders
        return;
      }
      const adjacent = Math.max(Math.abs(e.x - refinery.x), Math.abs(e.y - refinery.y)) <= 1;
      if (adjacent) {
        e.state = 'harvest_unload';
        e.path = null;
        e.harvestTimer = 0;
        return;
      }
      if (!e.path || e.path.length === 0) {
        pathTo(game, e, refinery.x, refinery.y);
      }
      return;
    }
    case 'harvest_unload': {
      if (++e.harvestTimer >= UNLOAD_TICKS) {
        const house = game.houses[e.owner];
        if (house) house.credits += e.bails * CREDITS_PER_BAIL;
        game.events.push({ type: 'unload', x: e.x, y: e.y, credits: e.bails * CREDITS_PER_BAIL });
        e.bails = 0;
        e.state = 'harvest_seek';
      }
      return;
    }
    default:
  }
}
