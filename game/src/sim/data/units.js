// Vehicle stats ported from the original UDATA.CPP via engine/game/game_data.cpp.
// speed: cells per 10 ticks (relative scale from the original port).
// armor: 'none' | 'aluminum' | 'steel' — feeds the warhead table in weapons.js.
// weapon: id into weapons.js, or null for unarmed-by-design (harvester, mcv, apc ferry role aside).
// prereq: building type ids (checked by production.js in M5.2).

export const UNIT_TYPES = {
  medium_tank: {
    name: 'Medium Tank', faction: 'gdi', sprite: 'medium_tank',
    hp: 400, cost: 800, buildTime: 120, sight: 4, speed: 5, speedType: 'track',
    armor: 'steel', techLevel: 5, weapon: 'cannon_105mm', crushes: true,
    prereq: ['war_factory'],
  },
  mammoth_tank: {
    name: 'Mammoth Tank', faction: 'gdi', sprite: 'mammoth_tank',
    hp: 600, cost: 1500, buildTime: 180, sight: 5, speed: 3, speedType: 'track',
    armor: 'steel', techLevel: 10, weapon: 'mammoth_tusk', crushes: true,
    prereq: ['war_factory', 'comm_center'],
  },
  humvee: {
    name: 'Humvee', faction: 'gdi', sprite: 'humvee',
    hp: 150, cost: 400, buildTime: 60, sight: 4, speed: 8, speedType: 'wheel',
    armor: 'aluminum', techLevel: 2, weapon: 'machinegun', crushes: false,
    prereq: ['war_factory'],
  },
  apc: {
    name: 'APC', faction: 'gdi', sprite: 'apc',
    hp: 200, cost: 700, buildTime: 100, sight: 4, speed: 6, speedType: 'track',
    armor: 'steel', techLevel: 3, weapon: 'machinegun', crushes: true,
    prereq: ['war_factory'], passengers: 5,
  },
  mlrs: {
    name: 'MLRS', faction: 'gdi', sprite: 'mlrs',
    hp: 150, cost: 800, buildTime: 120, sight: 5, speed: 5, speedType: 'track',
    armor: 'aluminum', techLevel: 6, weapon: 'mlrs_rocket', crushes: false,
    prereq: ['war_factory', 'comm_center'],
  },
  light_tank: {
    name: 'Light Tank', faction: 'nod', sprite: 'light_tank',
    hp: 300, cost: 600, buildTime: 90, sight: 4, speed: 6, speedType: 'track',
    armor: 'steel', techLevel: 3, weapon: 'cannon_75mm', crushes: true,
    prereq: ['war_factory'],
  },
  stealth_tank: {
    name: 'Stealth Tank', faction: 'nod', sprite: 'stealth_tank',
    hp: 180, cost: 900, buildTime: 150, sight: 4, speed: 7, speedType: 'track',
    armor: 'aluminum', techLevel: 7, weapon: 'dragon_rocket', crushes: false,
    prereq: ['war_factory', 'temple'], cloaks: true,
  },
  buggy: {
    name: 'Nod Buggy', faction: 'nod', sprite: 'buggy',
    hp: 140, cost: 300, buildTime: 50, sight: 4, speed: 9, speedType: 'wheel',
    armor: 'aluminum', techLevel: 1, weapon: 'machinegun', crushes: false,
    prereq: ['war_factory'],
  },
  recon_bike: {
    name: 'Recon Bike', faction: 'nod', sprite: 'recon_bike',
    hp: 120, cost: 500, buildTime: 70, sight: 5, speed: 10, speedType: 'wheel',
    armor: 'none', techLevel: 3, weapon: 'dragon_rocket', crushes: false,
    prereq: ['war_factory'],
  },
  artillery: {
    name: 'Artillery', faction: 'nod', sprite: 'artillery',
    hp: 100, cost: 450, buildTime: 80, sight: 6, speed: 3, speedType: 'track',
    armor: 'aluminum', techLevel: 5, weapon: 'artillery_shell', crushes: false,
    prereq: ['war_factory'],
  },
  flame_tank: {
    name: 'Flame Tank', faction: 'nod', sprite: 'flame_tank',
    hp: 300, cost: 800, buildTime: 110, sight: 3, speed: 5, speedType: 'track',
    armor: 'steel', techLevel: 5, weapon: 'flame_tank_flamer', crushes: true,
    prereq: ['war_factory'],
  },
  harvester: {
    name: 'Harvester', faction: 'both', sprite: 'harvester',
    hp: 600, cost: 1400, buildTime: 150, sight: 3, speed: 4, speedType: 'track',
    armor: 'steel', techLevel: 1, weapon: null, crushes: true,
    prereq: ['refinery'], harvester: true,
  },
  mcv: {
    name: 'MCV', faction: 'both', sprite: 'mcv',
    hp: 600, cost: 5000, buildTime: 300, sight: 3, speed: 3, speedType: 'track',
    armor: 'steel', techLevel: 10, weapon: null, crushes: true,
    prereq: ['war_factory'], deploysTo: 'construction_yard',
  },
};

export function unitData(type) {
  return UNIT_TYPES[type];
}
