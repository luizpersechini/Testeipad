// Building stats ported from the original BDATA.CPP via engine/game/game_data.cpp.
// power: positive = generates, negative = drains. footprint: [w, h] in cells.
// sprite maps into the buildings sheet columns where art exists.

export const BUILDING_TYPES = {
  construction_yard: {
    name: 'Construction Yard', faction: 'both', sprite: 'construction_yard',
    hp: 1000, cost: 5000, buildTime: 300, sight: 5, power: 0,
    armor: 'concrete', footprint: [3, 3], techLevel: 1, weapon: null,
    prereq: [], builds: 'buildings',
  },
  power_plant: {
    name: 'Power Plant', faction: 'both', sprite: 'power_plant',
    hp: 400, cost: 300, buildTime: 60, sight: 3, power: 100,
    armor: 'wood', footprint: [2, 2], techLevel: 1, weapon: null,
    prereq: ['construction_yard'],
  },
  adv_power_plant: {
    name: 'Advanced Power Plant', faction: 'both', sprite: 'power_plant',
    hp: 600, cost: 700, buildTime: 90, sight: 3, power: 200,
    armor: 'concrete', footprint: [2, 2], techLevel: 5, weapon: null,
    prereq: ['power_plant'],
  },
  refinery: {
    name: 'Tiberium Refinery', faction: 'both', sprite: 'refinery',
    hp: 450, cost: 2000, buildTime: 120, sight: 4, power: -30,
    armor: 'wood', footprint: [3, 2], techLevel: 1, weapon: null,
    prereq: ['power_plant'], givesHarvester: true,
  },
  silo: {
    name: 'Tiberium Silo', faction: 'both', sprite: null,
    hp: 150, cost: 150, buildTime: 30, sight: 2, power: 0,
    armor: 'wood', footprint: [1, 1], techLevel: 1, weapon: null,
    prereq: ['refinery'],
  },
  barracks: {
    name: 'Barracks', faction: 'gdi', sprite: 'barracks',
    hp: 400, cost: 300, buildTime: 60, sight: 3, power: -10,
    armor: 'wood', footprint: [2, 2], techLevel: 1, weapon: null,
    prereq: ['power_plant'], builds: 'infantry',
  },
  hand_of_nod: {
    name: 'Hand of Nod', faction: 'nod', sprite: 'barracks',
    hp: 400, cost: 300, buildTime: 60, sight: 3, power: -10,
    armor: 'wood', footprint: [2, 2], techLevel: 1, weapon: null,
    prereq: ['power_plant'], builds: 'infantry',
  },
  war_factory: {
    name: 'Weapons Factory', faction: 'both', sprite: 'war_factory',
    hp: 500, cost: 2000, buildTime: 150, sight: 3, power: -30,
    armor: 'concrete', footprint: [3, 2], techLevel: 2, weapon: null,
    prereq: ['power_plant', 'refinery'], builds: 'units',
  },
  comm_center: {
    name: 'Communications Center', faction: 'both', sprite: 'radar',
    hp: 500, cost: 1000, buildTime: 90, sight: 10, power: -40,
    armor: 'wood', footprint: [2, 2], techLevel: 3, weapon: null,
    prereq: ['power_plant'], radar: true,
  },
  guard_tower: {
    name: 'Guard Tower', faction: 'gdi', sprite: 'guard_tower',
    hp: 300, cost: 500, buildTime: 60, sight: 5, power: -10,
    armor: 'concrete', footprint: [1, 1], techLevel: 2,
    weapon: 'machinegun',
    prereq: ['barracks'],
  },
  adv_guard_tower: {
    name: 'Advanced Guard Tower', faction: 'gdi', sprite: 'guard_tower',
    hp: 500, cost: 1000, buildTime: 90, sight: 6, power: -20,
    armor: 'concrete', footprint: [1, 1], techLevel: 6,
    weapon: 'tower_rocket',
    prereq: ['guard_tower', 'comm_center'],
  },
  gun_turret: {
    name: 'Gun Turret', faction: 'nod', sprite: 'guard_tower',
    hp: 200, cost: 600, buildTime: 60, sight: 5, power: -20,
    armor: 'concrete', footprint: [1, 1], techLevel: 3,
    weapon: 'turret_gun',
    prereq: ['war_factory'],
  },
  obelisk: {
    name: 'Obelisk of Light', faction: 'nod', sprite: 'guard_tower',
    hp: 300, cost: 1500, buildTime: 120, sight: 6, power: -150,
    armor: 'concrete', footprint: [1, 1], techLevel: 7,
    weapon: 'obelisk_laser',
    prereq: ['war_factory', 'temple'],
  },
  temple: {
    name: 'Temple of Nod', faction: 'nod', sprite: 'construction_yard',
    hp: 1000, cost: 3000, buildTime: 180, sight: 4, power: -100,
    armor: 'concrete', footprint: [3, 3], techLevel: 9, weapon: null,
    prereq: ['power_plant', 'war_factory'],
  },
  adv_comm_center: {
    name: 'Advanced Comm Center', faction: 'gdi', sprite: 'radar',
    hp: 500, cost: 2800, buildTime: 150, sight: 10, power: -80,
    armor: 'concrete', footprint: [2, 2], techLevel: 9, weapon: null,
    prereq: ['power_plant', 'war_factory'], radar: true,
  },
};

export function buildingData(type) {
  return BUILDING_TYPES[type];
}
