// Scripted scenarios, ported from the mission layouts designed in the frozen
// C++ port (engine/game/scenario.cpp). Scenario worlds are flat and built
// declaratively (terrain patches + tiberium fields + placed forces), so they
// are exactly reproducible.

import { TerrainType, HouseType, TIBERIUM_MAX_STAGE } from './constants.js';
import { idx, inBounds } from './world.js';
import { spawn, EntityKind } from './entity.js';
import { createGame } from './game.js';
import { buildingData } from './data/buildings.js';
import { UNIT_TYPES } from './data/units.js';
import { INFANTRY_TYPES } from './data/infantry.js';

// b/u/i = building/unit/infantry: [type, owner, x, y]
const G = HouseType.GDI;
const N = HouseType.NOD;

export const SCENARIOS = {
  gdi_1: {
    name: 'GDI 1: The Beachhead',
    description: 'Establish a base and destroy the Nod outpost.',
    player: G,
    credits: { [G]: 8000, [N]: 10000 },
    aiDifficulty: 'easy',
    start: { x: 10, y: 42 },
    buildings: [
      ['construction_yard', G, 8, 40], ['power_plant', G, 12, 40],
      ['barracks', G, 6, 38], ['refinery', G, 12, 37],
      ['construction_yard', N, 38, 8], ['power_plant', N, 36, 8],
      ['hand_of_nod', N, 40, 6], ['war_factory', N, 33, 5],
      ['refinery', N, 33, 10], ['gun_turret', N, 34, 13],
      ['gun_turret', N, 42, 10], ['obelisk', N, 38, 12],
    ],
    units: [
      ['medium_tank', G, 10, 43], ['medium_tank', G, 12, 43],
      ['humvee', G, 14, 43], ['harvester', G, 15, 38], ['mcv', G, 8, 44],
      ['light_tank', N, 36, 15], ['light_tank', N, 40, 15],
      ['buggy', N, 38, 16], ['recon_bike', N, 40, 16], ['harvester', N, 33, 3],
    ],
    infantry: [
      ['minigunner', G, 11, 44], ['minigunner', G, 13, 44], ['minigunner', G, 11, 42],
      ['rocket_soldier', G, 7, 42], ['rocket_soldier', G, 13, 42],
      ['minigunner', N, 37, 15], ['minigunner', N, 39, 16],
      ['flamethrower', N, 41, 14], ['rocket_soldier', N, 35, 15],
    ],
    tiberium: [
      { x: 14, y: 30, r: 4 }, { x: 30, y: 16, r: 4 }, { x: 24, y: 24, r: 3 },
    ],
    terrain: [
      // River running diagonally across the middle.
      ...Array.from({ length: 20 }, (_, i) => ({ x: 18 + i, y: 28 - i, w: 2, h: 1, t: TerrainType.WATER })),
      { x: 20, y: 18, w: 4, h: 3, t: TerrainType.ROCK },
      { x: 26, y: 30, w: 3, h: 4, t: TerrainType.ROCK },
    ],
  },
  nod_1: {
    name: 'Nod 1: Silencing Dissent',
    description: 'No base, no reinforcements: raze the GDI outpost with your strike team.',
    player: N,
    credits: { [G]: 8000, [N]: 6000 },
    aiDifficulty: 'easy',
    start: { x: 34, y: 34 },
    buildings: [
      ['construction_yard', G, 6, 6], ['power_plant', G, 4, 9],
      ['barracks', G, 9, 4], ['guard_tower', G, 12, 8],
      ['guard_tower', G, 4, 12], ['adv_comm_center', G, 10, 1],
    ],
    units: [
      ['medium_tank', G, 8, 9], ['humvee', G, 12, 6],
      ['buggy', N, 34, 34], ['buggy', N, 36, 34], ['recon_bike', N, 35, 36],
    ],
    infantry: [
      ['minigunner', G, 5, 5], ['minigunner', G, 7, 5], ['rocket_soldier', G, 12, 5],
      ['minigunner', N, 33, 33], ['minigunner', N, 34, 33], ['minigunner', N, 35, 33],
      ['flamethrower', N, 33, 35], ['rocket_soldier', N, 36, 33],
    ],
    tiberium: [{ x: 20, y: 20, r: 4 }],
    terrain: [
      { x: 16, y: 10, w: 3, h: 8, t: TerrainType.ROCK },
      { x: 24, y: 26, w: 8, h: 2, t: TerrainType.TREE },
    ],
  },
};

function paintRect(world, p) {
  for (let y = p.y; y < p.y + p.h; y++) {
    for (let x = p.x; x < p.x + p.w; x++) {
      if (inBounds(world, x, y)) world.terrain[idx(world, x, y)] = p.t;
    }
  }
}

function paintTiberium(world, f) {
  for (let y = f.y - f.r; y <= f.y + f.r; y++) {
    for (let x = f.x - f.r; x <= f.x + f.r; x++) {
      if (!inBounds(world, x, y)) continue;
      const dx = x - f.x;
      const dy = y - f.y;
      if (dx * dx + dy * dy > f.r * f.r) continue;
      const i = idx(world, x, y);
      if (world.terrain[i] !== TerrainType.CLEAR) continue;
      // Denser toward the center; fully deterministic (no rng).
      const stage = Math.max(2, 9 - ((dx * dx + dy * dy) | 0));
      world.tiberium[i] = Math.min(stage, TIBERIUM_MAX_STAGE);
    }
  }
}

// Build a ready-to-play game from a scenario id. Returns {game, player,
// aiHouse, aiDifficulty, start, name} or null for an unknown id.
export function createScenarioGame(id) {
  const s = SCENARIOS[id];
  if (!s) return null;
  const game = createGame(1);
  const { world } = game;

  // Flat canvas, then scripted terrain and tiberium.
  world.terrain.fill(TerrainType.CLEAR);
  world.tiberium.fill(0);
  for (const p of s.terrain) paintRect(world, p);
  for (const f of s.tiberium) paintTiberium(world, f);

  for (const owner of [G, N]) {
    game.houses[owner].credits = s.credits[owner] ?? 5000;
  }

  // Direct spawns (not placeBuilding): scripted layouts bring their own
  // harvesters and ignore adjacency by design.
  for (const [type, owner, x, y] of s.buildings) {
    const data = buildingData(type);
    spawn(game.store, world, {
      kind: EntityKind.BUILDING, type, owner, x, y, hp: data.hp, footprint: data.footprint,
    });
  }
  for (const [type, owner, x, y] of s.units) {
    spawn(game.store, world, {
      kind: EntityKind.UNIT, type, owner, x, y, hp: unitHp(type), facing: 8,
    });
  }
  for (const [type, owner, x, y] of s.infantry) {
    spawn(game.store, world, {
      kind: EntityKind.INFANTRY, type, owner, x, y, hp: infHp(type), facing: 8,
    });
  }

  const aiHouse = s.player === G ? N : G;
  return {
    game,
    player: s.player,
    aiHouse,
    aiDifficulty: s.aiDifficulty,
    start: s.start,
    name: s.name,
  };
}

function unitHp(type) {
  return UNIT_TYPES[type].hp;
}
function infHp(type) {
  return INFANTRY_TYPES[type].hp;
}
