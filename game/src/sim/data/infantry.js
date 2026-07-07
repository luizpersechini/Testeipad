// Infantry stats ported from the original IDATA.CPP via engine/game/game_data.cpp.
// In Tiberian Dawn E1 (minigunner) and E3 (rocket) are buildable by both sides;
// E2 grenadier is GDI, E4 flamethrower / E5 chem warrior are Nod.

export const INFANTRY_TYPES = {
  minigunner: {
    name: 'Minigunner', faction: 'both', sprite: 'minigunner',
    hp: 50, cost: 100, buildTime: 15, sight: 3, speed: 4,
    armor: 'none', techLevel: 1, weapon: 'rifle',
    prereq: ['barracks'],
  },
  grenadier: {
    name: 'Grenadier', faction: 'gdi', sprite: 'grenadier',
    hp: 50, cost: 160, buildTime: 20, sight: 3, speed: 3,
    armor: 'none', techLevel: 2, weapon: 'grenade',
    prereq: ['barracks'],
  },
  rocket_soldier: {
    name: 'Rocket Soldier', faction: 'both', sprite: 'rocket_soldier',
    hp: 45, cost: 300, buildTime: 30, sight: 4, speed: 3,
    armor: 'none', techLevel: 3, weapon: 'dragon_rocket',
    prereq: ['barracks'],
  },
  flamethrower: {
    name: 'Flamethrower', faction: 'nod', sprite: 'flamethrower',
    hp: 60, cost: 200, buildTime: 25, sight: 3, speed: 3,
    armor: 'none', techLevel: 3, weapon: 'flamethrower',
    prereq: ['barracks'],
  },
  chem_warrior: {
    name: 'Chem Warrior', faction: 'nod', sprite: 'chemical_warrior',
    hp: 70, cost: 300, buildTime: 30, sight: 3, speed: 3,
    armor: 'none', techLevel: 8, weapon: 'chem_spray',
    prereq: ['barracks'],
  },
  engineer: {
    name: 'Engineer', faction: 'both', sprite: 'engineer',
    hp: 25, cost: 500, buildTime: 30, sight: 2, speed: 3,
    armor: 'none', techLevel: 4, weapon: null, captures: true,
    prereq: ['barracks'],
  },
  commando: {
    name: 'Commando', faction: 'both', sprite: 'commando',
    hp: 80, cost: 1000, buildTime: 60, sight: 5, speed: 5,
    armor: 'none', techLevel: 10, weapon: 'sniper_rifle', c4: true,
    prereq: ['barracks', 'comm_center'],
  },
};

export function infantryData(type) {
  return INFANTRY_TYPES[type];
}
