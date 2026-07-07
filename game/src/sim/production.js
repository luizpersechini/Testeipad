// Production: tech-tree gating, one build queue per category per house
// (buildings / infantry / units), linear cost drain over the build time
// (credits tick away as it builds, like the original), low-power slowdown.

import { HouseType } from './constants.js';
import { spawn, EntityKind } from './entity.js';
import { NO_ENTITY, inBounds, idx } from './world.js';
import { TerrainType } from './constants.js';
import { UNIT_TYPES } from './data/units.js';
import { INFANTRY_TYPES } from './data/infantry.js';
import { BUILDING_TYPES } from './data/buildings.js';
import { placeBuilding, canPlaceBuilding } from './placement.js';
import { noteUnitBuilt } from './victory.js';

export const CATEGORIES = ['buildings', 'infantry', 'units'];

const CATEGORY_TABLE = {
  buildings: BUILDING_TYPES,
  infantry: INFANTRY_TYPES,
  units: UNIT_TYPES,
};
const CATEGORY_KIND = {
  infantry: EntityKind.INFANTRY,
  units: EntityKind.UNIT,
};
const FACTORY_FOR = {
  infantry: ['barracks', 'hand_of_nod'],
  units: ['war_factory'],
  buildings: ['construction_yard'],
};
// Ticks per point of buildTime; buildTime 60 -> 120 ticks (8s) at speed 2.
const TICKS_PER_BUILD_POINT = 2;

function factionName(owner) {
  return owner === HouseType.NOD ? 'nod' : 'gdi';
}

export function ownedBuildingTypes(game, owner) {
  const owned = new Set();
  for (const e of game.store.entities.values()) {
    if (e.kind === EntityKind.BUILDING && e.owner === owner && e.hp > 0) {
      owned.add(e.type);
    }
  }
  return owned;
}

// Everything this house could start building right now (faction + prereqs).
export function availableToBuild(game, owner, category) {
  const table = CATEGORY_TABLE[category];
  const owned = ownedBuildingTypes(game, owner);
  const faction = factionName(owner);
  const out = [];
  for (const [type, data] of Object.entries(table)) {
    if (data.faction !== 'both' && data.faction !== faction) continue;
    if (data.techLevel > 98) continue; // not player-buildable
    if (!FACTORY_FOR[category].some((f) => owned.has(f))) continue;
    if (!data.prereq.every((p) => owned.has(p))) continue;
    out.push(type);
  }
  return out;
}

function ensureQueues(house) {
  if (!house.queues) {
    house.queues = { buildings: null, infantry: null, units: null };
  }
  return house.queues;
}

export function startProduction(game, owner, category, type) {
  const house = game.houses[owner];
  const data = CATEGORY_TABLE[category]?.[type];
  if (!house || !data) return false;
  const queues = ensureQueues(house);
  if (queues[category]) return false; // one at a time per category
  if (!availableToBuild(game, owner, category).includes(type)) return false;
  queues[category] = {
    type,
    totalCost: data.cost,
    paid: 0,
    ticksLeft: data.buildTime * TICKS_PER_BUILD_POINT,
    totalTicks: data.buildTime * TICKS_PER_BUILD_POINT,
    ready: false,
  };
  return true;
}

export function cancelProduction(game, owner, category) {
  const house = game.houses[owner];
  const q = house?.queues?.[category];
  if (!q) return false;
  house.credits += q.paid; // full refund of what was sunk
  house.queues[category] = null;
  return true;
}

// Free cell next to a factory for a fresh unit (spiral over the ring just
// outside the footprint, deterministic order).
export function findExitCell(game, factory) {
  const [fw, fh] = factory.footprint;
  for (let ring = 1; ring <= 3; ring++) {
    for (let y = factory.y - ring; y <= factory.y + fh - 1 + ring; y++) {
      for (let x = factory.x - ring; x <= factory.x + fw - 1 + ring; x++) {
        const onRing = x === factory.x - ring || x === factory.x + fw - 1 + ring
          || y === factory.y - ring || y === factory.y + fh - 1 + ring;
        if (!onRing || !inBounds(game.world, x, y)) continue;
        const i = idx(game.world, x, y);
        if (game.world.terrain[i] !== TerrainType.CLEAR) continue;
        if (game.world.occupancy[i] !== NO_ENTITY) continue;
        return { x, y };
      }
    }
  }
  return null;
}

function findFactory(game, owner, category) {
  for (const e of game.store.entities.values()) {
    if (e.kind === EntityKind.BUILDING && e.owner === owner && e.hp > 0
      && FACTORY_FOR[category].includes(e.type)) return e;
  }
  return null;
}

function completeUnit(game, owner, category, q) {
  const factory = findFactory(game, owner, category);
  if (!factory) return false; // factory died mid-build; hold the finished item
  const exit = findExitCell(game, factory);
  if (!exit) return false; // exit blocked; try again next tick
  const data = CATEGORY_TABLE[category][q.type];
  const id = spawn(game.store, game.world, {
    kind: CATEGORY_KIND[category], type: q.type, owner,
    x: exit.x, y: exit.y, hp: data.hp, facing: 16, // rolls out facing south
  });
  if (id === NO_ENTITY) return false;
  noteUnitBuilt(game, owner);
  game.events.push({ type: 'unit_ready', unitType: q.type, x: exit.x, y: exit.y, id });
  return true;
}

// Place a finished building from the queue onto the map.
export function placeReadyBuilding(game, owner, x, y) {
  const house = game.houses[owner];
  const q = house?.queues?.buildings;
  if (!q || !q.ready) return NO_ENTITY;
  if (!canPlaceBuilding(game, owner, q.type, x, y)) return NO_ENTITY;
  const id = placeBuilding(game, owner, q.type, x, y);
  if (id !== NO_ENTITY) house.queues.buildings = null;
  return id;
}

export function tickProduction(game) {
  for (let owner = 0; owner < game.houses.length; owner++) {
    const house = game.houses[owner];
    if (!house.queues) continue;
    for (const category of CATEGORIES) {
      const q = house.queues[category];
      if (!q || q.ready) continue;
      // Low power halves speed (original slows production when brown-out).
      if (house.lowPower && game.tick % 2 === 1) continue;
      // Pay-as-you-go: this tick's slice of the total cost.
      const costPerTick = q.totalCost / q.totalTicks;
      const due = Math.min(q.totalCost - q.paid, costPerTick);
      if (house.credits < due) continue; // broke: stall
      house.credits -= due;
      q.paid += due;
      q.ticksLeft--;
      if (q.ticksLeft > 0) continue;
      // Round out bookkeeping (paid may be fractional by epsilon).
      house.credits -= q.totalCost - q.paid;
      q.paid = q.totalCost;
      if (category === 'buildings') {
        q.ready = true;
        game.events.push({ type: 'construction_ready', buildingType: q.type });
      } else if (completeUnit(game, owner, category, q)) {
        house.queues[category] = null;
      } else {
        q.ticksLeft = 1; // completed but couldn't spawn; retry next tick
      }
    }
  }
}
