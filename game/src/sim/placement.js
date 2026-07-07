// Building placement rules and MCV deployment.

import { TerrainType } from './constants.js';
import { idx, inBounds, NO_ENTITY } from './world.js';
import { spawn, despawn, get, EntityKind } from './entity.js';
import { buildingData } from './data/buildings.js';
import { noteBuildingBuilt } from './victory.js';

const ADJACENCY_RANGE = 3; // cells from an existing friendly building

// Every footprint cell must be in bounds, clear terrain, unoccupied, and free
// of tiberium (like the original).
export function footprintClear(world, type, x, y) {
  const data = buildingData(type);
  if (!data) return false;
  const [fw, fh] = data.footprint;
  for (let dy = 0; dy < fh; dy++) {
    for (let dx = 0; dx < fw; dx++) {
      if (!inBounds(world, x + dx, y + dy)) return false;
      const i = idx(world, x + dx, y + dy);
      if (world.terrain[i] !== TerrainType.CLEAR) return false;
      if (world.occupancy[i] !== NO_ENTITY) return false;
      if (world.tiberium[i] > 0) return false;
    }
  }
  return true;
}

// New construction must hug the existing base (any friendly building within
// ADJACENCY_RANGE of the footprint rectangle).
export function nearFriendlyBuilding(game, owner, type, x, y) {
  const data = buildingData(type);
  const [fw, fh] = data.footprint;
  for (const e of game.store.entities.values()) {
    if (e.kind !== EntityKind.BUILDING || e.owner !== owner || e.hp <= 0) continue;
    const [ew, eh] = e.footprint;
    // Rectangle-to-rectangle Chebyshev gap.
    const gapX = Math.max(0, Math.max(e.x - (x + fw - 1), x - (e.x + ew - 1)));
    const gapY = Math.max(0, Math.max(e.y - (y + fh - 1), y - (e.y + eh - 1)));
    if (Math.max(gapX, gapY) <= ADJACENCY_RANGE) return true;
  }
  return false;
}

export function canPlaceBuilding(game, owner, type, x, y, { ignoreAdjacency = false } = {}) {
  if (!footprintClear(game.world, type, x, y)) return false;
  if (!ignoreAdjacency && !nearFriendlyBuilding(game, owner, type, x, y)) return false;
  return true;
}

// Free cell on the ring just outside a footprint (deterministic scan).
function freeRingCell(world, x, y, fw, fh) {
  for (let ring = 1; ring <= 3; ring++) {
    for (let cy = y - ring; cy <= y + fh - 1 + ring; cy++) {
      for (let cx = x - ring; cx <= x + fw - 1 + ring; cx++) {
        const onRing = cx === x - ring || cx === x + fw - 1 + ring
          || cy === y - ring || cy === y + fh - 1 + ring;
        if (!onRing || !inBounds(world, cx, cy)) continue;
        const i = idx(world, cx, cy);
        if (world.terrain[i] !== TerrainType.CLEAR) continue;
        if (world.occupancy[i] !== NO_ENTITY) continue;
        return { x: cx, y: cy };
      }
    }
  }
  return null;
}

// Places instantly (production timing/cost is M5.2's job).
export function placeBuilding(game, owner, type, x, y, opts = {}) {
  if (!canPlaceBuilding(game, owner, type, x, y, opts)) return NO_ENTITY;
  const data = buildingData(type);
  const id = spawn(game.store, game.world, {
    kind: EntityKind.BUILDING, type, owner, x, y, hp: data.hp, footprint: data.footprint,
  });
  if (id !== NO_ENTITY) {
    noteBuildingBuilt(game, owner);
    game.events.push({ type: 'place', x, y, buildingType: type });
    // A refinery ships with a free harvester that goes straight to work,
    // exactly like the original.
    if (data.givesHarvester) {
      const cell = freeRingCell(game.world, x, y, data.footprint[0], data.footprint[1]);
      if (cell) {
        const harv = spawn(game.store, game.world, {
          kind: EntityKind.UNIT, type: 'harvester', owner,
          x: cell.x, y: cell.y, hp: 600, facing: 16,
        });
        if (harv !== NO_ENTITY) {
          const e = game.store.entities.get(harv);
          e.bails = 0;
          e.state = 'harvest_seek';
        }
      }
    }
  }
  return id;
}

// MCV -> Construction Yard. The 3x3 yard is anchored so the MCV cell becomes
// its center; the first building of a base ignores adjacency.
export function deployMcv(game, id) {
  const e = get(game.store, id);
  if (!e || e.type !== 'mcv') return NO_ENTITY;
  const ax = e.x - 1;
  const ay = e.y - 1;
  const owner = e.owner;
  // The MCV itself occupies the center cell; lift it out for the check.
  despawn(game.store, game.world, id);
  const yard = placeBuilding(game, owner, 'construction_yard', ax, ay, { ignoreAdjacency: true });
  if (yard === NO_ENTITY) {
    // Blocked: put the MCV back exactly as it was.
    const back = spawn(game.store, game.world, {
      kind: EntityKind.UNIT, type: 'mcv', owner, x: e.x, y: e.y, hp: e.hp, facing: e.facing,
    });
    return NO_ENTITY;
  }
  return yard;
}
