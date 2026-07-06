// Camera: viewport into the world, in pixels. Pure logic (no DOM) so node
// tests can cover clamping and coordinate math.

import { TILE } from '../sim/constants.js';

export function createCamera(viewW, viewH, worldW, worldH) {
  return {
    x: 0, // top-left of viewport in world pixels
    y: 0,
    viewW,
    viewH,
    worldPxW: worldW * TILE,
    worldPxH: worldH * TILE,
  };
}

export function clampCamera(cam) {
  const maxX = Math.max(0, cam.worldPxW - cam.viewW);
  const maxY = Math.max(0, cam.worldPxH - cam.viewH);
  if (cam.x < 0) cam.x = 0;
  if (cam.y < 0) cam.y = 0;
  if (cam.x > maxX) cam.x = maxX;
  if (cam.y > maxY) cam.y = maxY;
}

export function moveCamera(cam, dx, dy) {
  cam.x += dx;
  cam.y += dy;
  clampCamera(cam);
}

export function centerCameraOn(cam, worldPxX, worldPxY) {
  cam.x = worldPxX - cam.viewW / 2;
  cam.y = worldPxY - cam.viewH / 2;
  clampCamera(cam);
}

export function worldToScreen(cam, wx, wy) {
  return { x: wx - cam.x, y: wy - cam.y };
}

export function screenToWorld(cam, sx, sy) {
  return { x: sx + cam.x, y: sy + cam.y };
}

export function screenToCell(cam, sx, sy) {
  return {
    x: ((sx + cam.x) / TILE) | 0,
    y: ((sy + cam.y) / TILE) | 0,
  };
}

// Inclusive cell range covering the viewport, clamped to the world.
export function visibleCells(cam, world) {
  const x0 = Math.max(0, (cam.x / TILE) | 0);
  const y0 = Math.max(0, (cam.y / TILE) | 0);
  const x1 = Math.min(world.w - 1, ((cam.x + cam.viewW) / TILE) | 0);
  const y1 = Math.min(world.h - 1, ((cam.y + cam.viewH) / TILE) | 0);
  return { x0, y0, x1, y1 };
}
