// Run with: node --test game/test/
// Tests import sim modules only — the sim must stay DOM-free or these break.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TICK_RATE, TILE, MAP_W, MAP_H, TerrainType } from '../src/sim/constants.js';
import { createRng } from '../src/sim/rng.js';

test('constants match original C&C fundamentals', () => {
  assert.equal(TICK_RATE, 15);
  assert.equal(TILE, 32);
  assert.ok(MAP_W >= 32 && MAP_H >= 32);
  assert.equal(TerrainType.CLEAR, 0);
});

test('rng is deterministic for equal seeds', () => {
  const a = createRng(1234);
  const b = createRng(1234);
  for (let i = 0; i < 1000; i++) {
    assert.equal(a.int(100), b.int(100));
  }
});

test('rng diverges for different seeds', () => {
  const a = createRng(1);
  const b = createRng(2);
  let same = 0;
  for (let i = 0; i < 100; i++) {
    if (a.int(1000) === b.int(1000)) same++;
  }
  assert.ok(same < 10);
});

test('rng.range stays inclusive within bounds', () => {
  const rng = createRng(7);
  for (let i = 0; i < 1000; i++) {
    const v = rng.range(5, 8);
    assert.ok(v >= 5 && v <= 8);
  }
});

test('rng state save/restore reproduces the sequence', () => {
  const rng = createRng(99);
  rng.int(10);
  const saved = rng.getState();
  const next = [rng.int(1000), rng.int(1000), rng.int(1000)];
  rng.setState(saved);
  assert.deepEqual([rng.int(1000), rng.int(1000), rng.int(1000)], next);
});

test('rng.float is in [0,1) and chance behaves at extremes', () => {
  const rng = createRng(3);
  for (let i = 0; i < 1000; i++) {
    const f = rng.float();
    assert.ok(f >= 0 && f < 1);
  }
  assert.equal(rng.chance(0), false);
  assert.equal(createRng(5).chance(1), true);
});
