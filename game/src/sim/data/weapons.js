// Weapons and warheads, transcribed from the original CONST.CPP (Weapons[]
// and Warheads[]) with bullet->warhead mapping from BBDATA.CPP.
// damage: raw hit points; rof: ticks between shots; range: cells (original
// leptons / 256); speed: projectile cells-per-tick * 100 (integer math).

// Warhead-vs-armor modifiers, out of 256 (verbatim from Warheads[]):
//                       none  wood  alum  steel concrete
export const WARHEADS = {
  sa: { spread: 2, vs: { none: 0xFF, wood: 0x80, aluminum: 0x90, steel: 0x40, concrete: 0x40 } },
  he: { spread: 6, vs: { none: 0xE0, wood: 0xC0, aluminum: 0x90, steel: 0x40, concrete: 0xFF } },
  ap: { spread: 6, vs: { none: 0x40, wood: 0xC0, aluminum: 0xC0, steel: 0xFF, concrete: 0x80 } },
  fire: { spread: 8, vs: { none: 0xE0, wood: 0xFF, aluminum: 0xB0, steel: 0x40, concrete: 0x80 } },
  laser: { spread: 4, vs: { none: 0xFF, wood: 0xFF, aluminum: 0xFF, steel: 0xFF, concrete: 0xFF } },
  hollow_point: { spread: 4, vs: { none: 0xFF, wood: 0x08, aluminum: 0x08, steel: 0x08, concrete: 0x08 } },
};

export const WEAPONS = {
  // WEAPON_M16: infantry small arms.
  rifle: { warhead: 'sa', damage: 15, rof: 20, range: 2, speed: 100, projectile: 'bullet' },
  // WEAPON_M60MG: vehicle machine gun (humvee, buggy, APC).
  machinegun: { warhead: 'sa', damage: 15, rof: 30, range: 4, speed: 100, projectile: 'bullet' },
  // WEAPON_RIFLE: commando sniper shot.
  sniper_rifle: { warhead: 'hollow_point', damage: 125, rof: 40, range: 5.5, speed: 100, projectile: 'bullet' },
  // WEAPON_GRENADE.
  grenade: { warhead: 'he', damage: 50, rof: 60, range: 3.25, speed: 35, projectile: 'grenade' },
  // WEAPON_DRAGON: rocket soldier / recon bike / stealth tank TOW.
  dragon_rocket: { warhead: 'ap', damage: 30, rof: 60, range: 4, speed: 40, projectile: 'missile' },
  // WEAPON_FLAMETHROWER (E4).
  flamethrower: { warhead: 'fire', damage: 35, rof: 50, range: 2, speed: 50, projectile: 'flame' },
  // WEAPON_FLAME_TONGUE (flame tank).
  flame_tank_flamer: { warhead: 'fire', damage: 50, rof: 50, range: 2, speed: 50, projectile: 'flame' },
  // WEAPON_CHEMSPRAY (E5).
  chem_spray: { warhead: 'he', damage: 80, rof: 70, range: 2, speed: 50, projectile: 'flame' },
  // WEAPON_75MM (light tank).
  cannon_75mm: { warhead: 'ap', damage: 25, rof: 60, range: 4, speed: 60, projectile: 'shell' },
  // WEAPON_105MM (medium tank).
  cannon_105mm: { warhead: 'ap', damage: 30, rof: 50, range: 4.75, speed: 60, projectile: 'shell' },
  // WEAPON_120MM (mammoth cannon pair).
  cannon_120mm: { warhead: 'ap', damage: 40, rof: 80, range: 4.75, speed: 60, projectile: 'shell' },
  // WEAPON_TURRET_GUN (gunboat / guard tower cannon).
  turret_gun: { warhead: 'ap', damage: 40, rof: 60, range: 6, speed: 60, projectile: 'shell' },
  // WEAPON_MAMMOTH_TUSK (SSM warhead is HE).
  mammoth_tusk: { warhead: 'he', damage: 75, rof: 80, range: 5, speed: 40, projectile: 'missile' },
  // WEAPON_MLRS (SSM2).
  mlrs_rocket: { warhead: 'he', damage: 75, rof: 80, range: 6, speed: 40, projectile: 'missile' },
  // WEAPON_155MM (artillery).
  artillery_shell: { warhead: 'he', damage: 150, rof: 65, range: 6, speed: 30, projectile: 'shell' },
  // WEAPON_OBELISK_LASER.
  obelisk_laser: { warhead: 'laser', damage: 200, rof: 90, range: 7.5, speed: 100, projectile: 'laser' },
};

export function weaponData(id) {
  return WEAPONS[id];
}

// Damage after armor modifier, exactly like the original COMBAT.CPP
// Modify_Damage: damage * modifier / 256, minimum 1 for a real hit.
export function modifyDamage(damage, weaponId, armor) {
  const weapon = WEAPONS[weaponId];
  if (!weapon || damage <= 0) return 0;
  const mod = WARHEADS[weapon.warhead].vs[armor];
  const out = (damage * mod) >> 8;
  return Math.max(1, out);
}
