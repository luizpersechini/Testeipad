import { test } from 'node:test';
import assert from 'node:assert/strict';
import { layoutCommandBar, hitCommandBar } from '../src/render/touchbar.js';

const VIEW_W = 1080;
const VIEW_H = 720;

test('command bar shows selection actions only with a selection', () => {
  const withSel = layoutCommandBar(true, VIEW_W, VIEW_H);
  const ids = withSel.map((b) => b.id);
  assert.deepEqual(ids, ['stop', 'attackmove', 'deploy', 'harvest', 'repair', 'sell', 'pause']);
  const without = layoutCommandBar(false, VIEW_W, VIEW_H);
  assert.deepEqual(without.map((b) => b.id), ['pause']);
});

test('buttons sit along the bottom of the map view, inside bounds', () => {
  for (const b of layoutCommandBar(true, VIEW_W, VIEW_H)) {
    assert.ok(b.y + b.h <= VIEW_H, b.id);
    assert.ok(b.x >= 0 && b.x + b.w <= VIEW_W, b.id);
    assert.ok(b.h >= 44, `${b.id}: touch targets must be at least 44px tall`);
  }
});

test('hit testing finds buttons and misses the gaps', () => {
  const buttons = layoutCommandBar(true, VIEW_W, VIEW_H);
  const stop = buttons[0];
  assert.equal(hitCommandBar(buttons, stop.x + 5, stop.y + 5).id, 'stop');
  assert.equal(hitCommandBar(buttons, stop.x + stop.w + 2, stop.y + 5), null, 'gap between buttons');
  assert.equal(hitCommandBar(buttons, 500, 100), null, 'mid-map is free');
});

test('buttons never overlap each other', () => {
  const buttons = layoutCommandBar(true, VIEW_W, VIEW_H);
  for (let i = 0; i < buttons.length; i++) {
    for (let j = i + 1; j < buttons.length; j++) {
      const a = buttons[i];
      const b = buttons[j];
      const overlap = a.x < b.x + b.w && b.x < a.x + a.w
        && a.y < b.y + b.h && b.y < a.y + a.h;
      assert.ok(!overlap, `${a.id} overlaps ${b.id}`);
    }
  }
});
