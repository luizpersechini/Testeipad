// Tests the pure event->sound planning of render/audio.js. Importing this in
// node also proves the module has no top-level DOM/AudioContext dependency.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { planSounds, SOUNDS, createAudio, toggleMute } from '../src/render/audio.js';
import { WEAPONS } from '../src/sim/data/weapons.js';

test('events map to sounds, deduped per tick', () => {
  const plan = planSounds([
    { type: 'shot', weapon: 'cannon_105mm' },
    { type: 'shot', weapon: 'cannon_105mm' }, // duplicate collapses
    { type: 'shot', weapon: 'rifle' },
    { type: 'hit' },
    { type: 'death' },
  ]);
  assert.deepEqual(plan, ['shot_cannon', 'shot_mg', 'hit', 'explosion']);
});

test('global cap keeps big battles from becoming noise', () => {
  const events = [
    { type: 'shot', weapon: 'rifle' },
    { type: 'shot', weapon: 'cannon_75mm' },
    { type: 'shot', weapon: 'dragon_rocket' },
    { type: 'shot', weapon: 'flamethrower' },
    { type: 'hit' },
    { type: 'death' },
    { type: 'unload' },
  ];
  assert.equal(planSounds(events, 3).length, 3);
});

test('every weapon in the game has a sound recipe', () => {
  for (const weapon of Object.keys(WEAPONS)) {
    const plan = planSounds([{ type: 'shot', weapon }]);
    assert.equal(plan.length, 1, weapon);
    assert.ok(SOUNDS[plan[0]], `${weapon} -> ${plan[0]} exists`);
  }
});

test('every recipe is well-formed', () => {
  for (const [name, r] of Object.entries(SOUNDS)) {
    assert.ok(['noise', 'tone'].includes(r.type), name);
    assert.ok(r.dur > 0 && r.dur <= 1, name);
    assert.ok(r.freq > 0 && r.gain > 0 && r.gain <= 1, name);
  }
});

test('mute toggles and unknown events are silent', () => {
  const audio = createAudio();
  assert.equal(audio.muted, false);
  assert.equal(toggleMute(audio), true);
  assert.equal(toggleMute(audio), false);
  assert.deepEqual(planSounds([{ type: 'mystery' }]), []);
});
