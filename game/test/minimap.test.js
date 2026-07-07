import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TILE } from '../src/sim/constants.js';
import { createWorld } from '../src/sim/world.js';
import {
  createMinimapLayout, pointInMinimap, minimapToWorldPx,
} from '../src/render/minimap.js';

const CANVAS_W = 1280;
const SIDEBAR_W = 200;

test('layout sits inside the sidebar with margins', () => {
  const world = createWorld(42);
  const l = createMinimapLayout(CANVAS_W, SIDEBAR_W, world);
  assert.equal(l.x, CANVAS_W - SIDEBAR_W + 8);
  assert.equal(l.w, SIDEBAR_W - 16);
  assert.equal(l.h, l.w, 'square minimap for a square world');
  assert.ok(l.x + l.w <= CANVAS_W);
});

test('pointInMinimap hit-tests the layout rect', () => {
  const world = createWorld(42);
  const l = createMinimapLayout(CANVAS_W, SIDEBAR_W, world);
  assert.equal(pointInMinimap(l, l.x, l.y), true);
  assert.equal(pointInMinimap(l, l.x + l.w - 1, l.y + l.h - 1), true);
  assert.equal(pointInMinimap(l, l.x - 1, l.y), false);
  assert.equal(pointInMinimap(l, l.x, l.y + l.h), false);
});

test('minimap corners map to world corners', () => {
  const world = createWorld(42);
  const l = createMinimapLayout(CANVAS_W, SIDEBAR_W, world);
  const tl = minimapToWorldPx(l, world, l.x, l.y);
  assert.deepEqual(tl, { x: 0, y: 0 });
  const br = minimapToWorldPx(l, world, l.x + l.w, l.y + l.h);
  assert.equal(br.x, world.w * TILE);
  assert.equal(br.y, world.h * TILE);
});

test('minimap center maps to world center and out-of-range clamps', () => {
  const world = createWorld(42);
  const l = createMinimapLayout(CANVAS_W, SIDEBAR_W, world);
  const c = minimapToWorldPx(l, world, l.x + l.w / 2, l.y + l.h / 2);
  assert.equal(c.x, world.w * TILE / 2);
  assert.equal(c.y, world.h * TILE / 2);
  const clamped = minimapToWorldPx(l, world, l.x - 100, l.y + l.h + 100);
  assert.equal(clamped.x, 0);
  assert.equal(clamped.y, world.h * TILE);
});
