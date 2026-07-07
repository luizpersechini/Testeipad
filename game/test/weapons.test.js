import { test } from 'node:test';
import assert from 'node:assert/strict';
import { WEAPONS, WARHEADS, modifyDamage } from '../src/sim/data/weapons.js';
import { UNIT_TYPES } from '../src/sim/data/units.js';
import { INFANTRY_TYPES } from '../src/sim/data/infantry.js';

test('every weapon referenced by a unit or infantry type exists', () => {
  for (const [name, d] of Object.entries({ ...UNIT_TYPES, ...INFANTRY_TYPES })) {
    if (d.weapon !== null) {
      assert.ok(WEAPONS[d.weapon], `${name} references missing weapon ${d.weapon}`);
    }
  }
});

test('every weapon is complete and points at a real warhead', () => {
  for (const [id, w] of Object.entries(WEAPONS)) {
    assert.ok(WARHEADS[w.warhead], `${id}: warhead ${w.warhead}`);
    assert.ok(w.damage > 0 && w.rof > 0 && w.range > 0 && w.speed > 0, id);
    assert.ok(w.projectile, id);
  }
});

test('warhead tables carry all five armor classes', () => {
  for (const [id, wh] of Object.entries(WARHEADS)) {
    for (const armor of ['none', 'wood', 'aluminum', 'steel', 'concrete']) {
      const v = wh.vs[armor];
      assert.ok(v >= 1 && v <= 0xFF, `${id} vs ${armor}`);
    }
  }
});

test('spot checks against the original CONST.CPP values', () => {
  assert.equal(WEAPONS.cannon_105mm.damage, 30);
  assert.equal(WEAPONS.cannon_105mm.rof, 50);
  assert.equal(WEAPONS.cannon_105mm.range, 4.75); // 0x04C0 leptons
  assert.equal(WEAPONS.artillery_shell.damage, 150);
  assert.equal(WEAPONS.sniper_rifle.damage, 125);
  assert.equal(WARHEADS.ap.vs.steel, 0xFF);
  assert.equal(WARHEADS.ap.vs.none, 0x40);
  assert.equal(WARHEADS.sa.vs.none, 0xFF);
  assert.equal(WARHEADS.hollow_point.vs.steel, 0x08);
});

test('modifyDamage matches original integer math', () => {
  // AP vs steel: full damage. 30 * 255 / 256 = 29 (integer shift).
  assert.equal(modifyDamage(30, 'cannon_105mm', 'steel'), 29);
  // AP vs unarmored infantry: quartered. 30 * 64 / 256 = 7.
  assert.equal(modifyDamage(30, 'cannon_105mm', 'none'), 7);
  // Small arms vs infantry: full-ish. 15 * 255 / 256 = 14.
  assert.equal(modifyDamage(15, 'rifle', 'none'), 14);
  // Sniper vs tank: nearly nothing but never zero. 125 * 8 / 256 = 3.
  assert.equal(modifyDamage(125, 'sniper_rifle', 'steel'), 3);
  // Minimum 1 on any real hit.
  assert.equal(modifyDamage(2, 'sniper_rifle', 'steel'), 1);
  assert.equal(modifyDamage(0, 'rifle', 'none'), 0);
});

test('the classic matchup: rockets beat tanks, rifles beat infantry', () => {
  const rocketVsTank = modifyDamage(WEAPONS.dragon_rocket.damage, 'dragon_rocket', 'steel');
  const rifleVsTank = modifyDamage(WEAPONS.rifle.damage, 'rifle', 'steel');
  assert.ok(rocketVsTank > rifleVsTank * 3, 'AP rockets shred armor, rifles tickle it');
  const flameVsInf = modifyDamage(WEAPONS.flamethrower.damage, 'flamethrower', 'none');
  assert.ok(flameVsInf >= 30, 'fire cooks infantry');
});
