// Skirmish AI, following the state-machine design from engine/game/ai_player.cpp:
// bootstrap (deploy MCV) -> economy (power/refinery/harvesting) -> army
// (factories + units) -> attack waves. All decisions run inside the sim and
// draw randomness from game.rng, so AI games stay deterministic.

import { HouseType } from './constants.js';
import { EntityKind, get } from './entity.js';
import { statsFor } from './stats.js';
import { orderAttackMove } from './combat.js';
import { orderHarvest } from './economy.js';
import { deployMcv, canPlaceBuilding } from './placement.js';
import {
  startProduction, placeReadyBuilding, availableToBuild,
} from './production.js';
import { BUILDING_TYPES } from './data/buildings.js';

const THINK_INTERVAL = 15; // one decision pass per second

// incomeMult is the classic AI handicap knob: hard AIs squeeze more credits
// out of every harvester load, easy ones less.
export const DIFFICULTY = {
  easy: { waveSize: 4, waveCooldown: 1800, incomeMult: 0.8 },
  normal: { waveSize: 7, waveCooldown: 1200, incomeMult: 1.0 },
  hard: { waveSize: 10, waveCooldown: 700, incomeMult: 1.4 },
};

// The AI controlling `owner`, if any.
export function aiFor(game, owner) {
  return game.ais?.find((ai) => ai.owner === owner) ?? null;
}

export function incomeMultiplier(game, owner) {
  const ai = aiFor(game, owner);
  return ai ? DIFFICULTY[ai.difficulty].incomeMult : 1.0;
}

// Build priorities per faction (first missing one gets built).
const BUILD_ORDER = {
  gdi: ['power_plant', 'refinery', 'barracks', 'war_factory', 'guard_tower',
    'power_plant', 'comm_center', 'adv_guard_tower'],
  nod: ['power_plant', 'refinery', 'hand_of_nod', 'war_factory', 'gun_turret',
    'power_plant', 'comm_center', 'obelisk'],
};
const INFANTRY_CHOICES = {
  gdi: ['minigunner', 'minigunner', 'grenadier', 'rocket_soldier'],
  nod: ['minigunner', 'minigunner', 'flamethrower', 'rocket_soldier'],
};
const VEHICLE_CHOICES = {
  gdi: ['medium_tank', 'medium_tank', 'humvee'],
  nod: ['light_tank', 'light_tank', 'buggy', 'recon_bike'],
};

export function createAI(owner, difficulty = 'normal') {
  return {
    owner,
    difficulty,
    lastWaveTick: 0,
    placeCooldown: 0,
  };
}

export function enableAI(game, owner, difficulty = 'normal') {
  game.ais = game.ais ?? [];
  game.ais.push(createAI(owner, difficulty));
}

function myEntities(game, owner) {
  const mine = { buildings: [], army: [], harvesters: [], mcvs: [], counts: {} };
  for (const e of game.store.entities.values()) {
    if (e.owner !== owner || e.hp <= 0) continue;
    if (e.kind === EntityKind.BUILDING) {
      mine.buildings.push(e);
      mine.counts[e.type] = (mine.counts[e.type] ?? 0) + 1;
    } else if (e.kind === EntityKind.UNIT || e.kind === EntityKind.INFANTRY) {
      const stats = statsFor(e);
      if (e.type === 'mcv') mine.mcvs.push(e);
      else if (stats?.harvester) mine.harvesters.push(e);
      else if (stats?.weapon) mine.army.push(e);
    }
  }
  return mine;
}

function enemyTargetPoint(game, owner) {
  // Prefer the nearest enemy building; fall back to the far start position.
  let best = null;
  for (const e of game.store.entities.values()) {
    if (e.owner === owner || e.hp <= 0) continue;
    if (e.kind === EntityKind.BUILDING) {
      best = best ?? e;
      // Construction yard is the prize.
      if (e.type === 'construction_yard') return { x: e.x, y: e.y };
    }
  }
  if (best) return { x: best.x, y: best.y };
  const start = game.world.startPositions[owner === HouseType.GDI ? 1 : 0];
  return { x: start.x, y: start.y };
}

// Spiral out from the yard looking for a legal spot for `type`.
function findBuildSpot(game, owner, type, yard) {
  const data = BUILDING_TYPES[type];
  if (!data || !yard) return null;
  for (let ring = 1; ring <= 8; ring++) {
    for (let dy = -ring; dy <= ring; dy++) {
      for (let dx = -ring; dx <= ring; dx++) {
        if (Math.max(Math.abs(dx), Math.abs(dy)) !== ring) continue;
        const x = yard.x + dx * 2;
        const y = yard.y + dy * 2;
        if (canPlaceBuilding(game, owner, type, x, y)) return { x, y };
      }
    }
  }
  return null;
}

function thinkBuildings(game, ai, mine) {
  const house = game.houses[ai.owner];
  const queues = house.queues;
  const yard = mine.buildings.find((b) => b.type === 'construction_yard');

  // Place anything that finished building.
  if (queues?.buildings?.ready) {
    const spot = findBuildSpot(game, ai.owner, queues.buildings.type, yard);
    if (spot) placeReadyBuilding(game, ai.owner, spot.x, spot.y);
    return;
  }
  if (queues?.buildings) return; // already constructing

  const faction = ai.owner === HouseType.NOD ? 'nod' : 'gdi';
  const buildable = availableToBuild(game, ai.owner, 'buildings');
  const wanted = BUILD_ORDER[faction];
  // Count how many of each we already have vs how many the order wants.
  const wantedCounts = {};
  for (const t of wanted) wantedCounts[t] = (wantedCounts[t] ?? 0) + 1;
  for (const t of wanted) {
    const have = mine.counts[t] ?? 0;
    if (have >= wantedCounts[t]) continue;
    if (!buildable.includes(t)) continue;
    if (house.credits < BUILDING_TYPES[t].cost * 0.4) return; // save up a bit
    startProduction(game, ai.owner, 'buildings', t);
    return;
  }
}

function thinkProduction(game, ai, mine) {
  const house = game.houses[ai.owner];
  const faction = ai.owner === HouseType.NOD ? 'nod' : 'gdi';
  const diff = DIFFICULTY[ai.difficulty];

  // Replace lost harvesters when we have a refinery.
  if ((mine.counts.refinery ?? 0) > 0 && mine.harvesters.length === 0
    && !house.queues?.units && house.credits > 1400) {
    startProduction(game, ai.owner, 'units', 'harvester');
    return;
  }

  // Keep the war machine turning.
  if (!house.queues?.infantry && house.credits > 400) {
    const pool = INFANTRY_CHOICES[faction];
    const pick = pool[game.rng.int(pool.length)];
    if (availableToBuild(game, ai.owner, 'infantry').includes(pick)) {
      startProduction(game, ai.owner, 'infantry', pick);
    }
  }
  if (!house.queues?.units && house.credits > 900) {
    const pool = VEHICLE_CHOICES[faction];
    const pick = pool[game.rng.int(pool.length)];
    if (availableToBuild(game, ai.owner, 'units').includes(pick)) {
      startProduction(game, ai.owner, 'units', pick);
    }
  }
  void diff;
}

function thinkArmy(game, ai, mine) {
  const diff = DIFFICULTY[ai.difficulty];
  if (mine.army.length < diff.waveSize) return;
  if (game.tick - ai.lastWaveTick < diff.waveCooldown) return;
  ai.lastWaveTick = game.tick;
  const target = enemyTargetPoint(game, ai.owner);
  for (const e of mine.army) {
    if (e.state === 'idle') orderAttackMove(game, e.id, target.x, target.y);
  }
}

export function tickAI(game) {
  if (!game.ais) return;
  for (const ai of game.ais) {
    // Stagger AI thinking across ticks (and off the human's frame spikes).
    if ((game.tick + ai.owner) % THINK_INTERVAL !== 0) continue;
    const mine = myEntities(game, ai.owner);

    // Bootstrap: deploy any idle MCV.
    for (const mcv of mine.mcvs) {
      if (mcv.state === 'idle') deployMcv(game, mcv.id);
    }
    // Idle harvesters get back to work.
    for (const h of mine.harvesters) {
      if (h.state === 'idle') orderHarvest(game, h.id);
    }
    if (mine.buildings.length > 0) {
      thinkBuildings(game, ai, mine);
      thinkProduction(game, ai, mine);
      thinkArmy(game, ai, mine);
    }
  }
}
