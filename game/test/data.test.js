import { test } from 'node:test';
import assert from 'node:assert/strict';
import { UNIT_TYPES } from '../src/sim/data/units.js';
import { INFANTRY_TYPES } from '../src/sim/data/infantry.js';

const ARMORS = new Set(['none', 'aluminum', 'steel']);
const FACTIONS = new Set(['gdi', 'nod', 'both']);

function checkCommon(name, d) {
  assert.ok(d.name, name);
  assert.ok(FACTIONS.has(d.faction), `${name}: bad faction ${d.faction}`);
  assert.ok(d.hp > 0, `${name}: hp`);
  assert.ok(d.cost > 0, `${name}: cost`);
  assert.ok(d.buildTime > 0, `${name}: buildTime`);
  assert.ok(d.sight >= 2 && d.sight <= 10, `${name}: sight`);
  assert.ok(d.speed > 0, `${name}: speed`);
  assert.ok(ARMORS.has(d.armor), `${name}: bad armor ${d.armor}`);
  assert.ok(d.techLevel >= 1, `${name}: techLevel`);
  assert.ok(Array.isArray(d.prereq) && d.prereq.length > 0, `${name}: prereq`);
}

test('all vehicles have sane, complete stats', () => {
  assert.ok(Object.keys(UNIT_TYPES).length >= 13);
  for (const [name, d] of Object.entries(UNIT_TYPES)) {
    checkCommon(name, d);
    assert.ok(['track', 'wheel'].includes(d.speedType), name);
  }
});

test('all infantry have sane, complete stats', () => {
  assert.ok(Object.keys(INFANTRY_TYPES).length >= 7);
  for (const [name, d] of Object.entries(INFANTRY_TYPES)) {
    checkCommon(name, d);
    assert.equal(d.armor, 'none', `${name}: TD infantry are unarmored`);
  }
});

test('unarmed types are unarmed by design, not omission', () => {
  const expectedUnarmed = new Set(['harvester', 'mcv', 'engineer']);
  for (const [name, d] of Object.entries({ ...UNIT_TYPES, ...INFANTRY_TYPES })) {
    if (d.weapon === null) {
      assert.ok(expectedUnarmed.has(name), `${name} is unarmed but not expected to be`);
    } else {
      assert.equal(typeof d.weapon, 'string', name);
    }
  }
});

test('MVP roster from the plan is present for both factions', () => {
  for (const t of ['minigunner', 'grenadier', 'rocket_soldier', 'humvee',
    'medium_tank', 'mammoth_tank', 'harvester', 'mcv']) {
    const d = UNIT_TYPES[t] ?? INFANTRY_TYPES[t];
    assert.ok(d, `missing ${t}`);
    assert.ok(d.faction === 'gdi' || d.faction === 'both', `${t} should be GDI-buildable`);
  }
  for (const t of ['minigunner', 'rocket_soldier', 'flamethrower', 'buggy',
    'recon_bike', 'light_tank', 'stealth_tank', 'harvester', 'mcv']) {
    const d = UNIT_TYPES[t] ?? INFANTRY_TYPES[t];
    assert.ok(d, `missing ${t}`);
    assert.ok(d.faction === 'nod' || d.faction === 'both', `${t} should be Nod-buildable`);
  }
});

test('original cost relationships hold (sanity against UDATA)', () => {
  assert.equal(UNIT_TYPES.mcv.cost, 5000);
  assert.equal(UNIT_TYPES.harvester.cost, 1400);
  assert.ok(UNIT_TYPES.mammoth_tank.cost > UNIT_TYPES.medium_tank.cost);
  assert.ok(UNIT_TYPES.medium_tank.cost > UNIT_TYPES.light_tank.cost);
  assert.ok(UNIT_TYPES.recon_bike.speed > UNIT_TYPES.mammoth_tank.speed);
  assert.equal(INFANTRY_TYPES.minigunner.cost, 100);
});
