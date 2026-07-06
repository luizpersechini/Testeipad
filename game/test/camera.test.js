import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TILE } from '../src/sim/constants.js';
import { createWorld } from '../src/sim/world.js';
import {
  createCamera, clampCamera, moveCamera, centerCameraOn,
  worldToScreen, screenToWorld, screenToCell, visibleCells,
} from '../src/render/camera.js';

const VIEW_W = 1280;
const VIEW_H = 720;

function cam64() {
  return createCamera(VIEW_W, VIEW_H, 64, 64);
}

test('camera clamps to world bounds', () => {
  const cam = cam64();
  moveCamera(cam, -500, -500);
  assert.equal(cam.x, 0);
  assert.equal(cam.y, 0);
  moveCamera(cam, 1e9, 1e9);
  assert.equal(cam.x, 64 * TILE - VIEW_W);
  assert.equal(cam.y, 64 * TILE - VIEW_H);
});

test('camera clamps to origin when world smaller than viewport', () => {
  const cam = createCamera(VIEW_W, VIEW_H, 16, 16); // 512px world
  moveCamera(cam, 300, 300);
  assert.equal(cam.x, 0);
  assert.equal(cam.y, 0);
});

test('centerCameraOn centers and clamps', () => {
  const cam = cam64();
  centerCameraOn(cam, 1024, 1024);
  assert.equal(cam.x, 1024 - VIEW_W / 2);
  assert.equal(cam.y, 1024 - VIEW_H / 2);
  centerCameraOn(cam, 0, 0);
  assert.equal(cam.x, 0);
  assert.equal(cam.y, 0);
});

test('world/screen conversions round-trip', () => {
  const cam = cam64();
  moveCamera(cam, 200, 120);
  const s = worldToScreen(cam, 500, 400);
  const w = screenToWorld(cam, s.x, s.y);
  assert.equal(w.x, 500);
  assert.equal(w.y, 400);
});

test('screenToCell maps through camera offset', () => {
  const cam = cam64();
  moveCamera(cam, TILE * 3, TILE * 2);
  const c = screenToCell(cam, 0, 0);
  assert.deepEqual(c, { x: 3, y: 2 });
  const c2 = screenToCell(cam, TILE - 1, TILE - 1);
  assert.deepEqual(c2, { x: 3, y: 2 });
  const c3 = screenToCell(cam, TILE, TILE);
  assert.deepEqual(c3, { x: 4, y: 3 });
});

test('visibleCells covers the viewport and clamps to world', () => {
  const world = createWorld(42);
  const cam = cam64();
  clampCamera(cam);
  let r = visibleCells(cam, world);
  assert.equal(r.x0, 0);
  assert.equal(r.y0, 0);
  assert.equal(r.x1, (VIEW_W / TILE) | 0);
  assert.equal(r.y1, (VIEW_H / TILE) | 0);
  moveCamera(cam, 1e9, 1e9); // bottom-right corner
  r = visibleCells(cam, world);
  assert.equal(r.x1, world.w - 1);
  assert.equal(r.y1, world.h - 1);
  assert.ok(r.x0 <= r.x1 && r.y0 <= r.y1);
});
