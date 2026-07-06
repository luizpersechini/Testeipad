// Boot: canvas setup and the fixed-timestep loop.
// Sim ticks at a fixed 15/s (accumulator); rendering runs at requestAnimationFrame rate.

import { MS_PER_TICK } from './sim/constants.js';
import { createWorld } from './sim/world.js';
import { loadAssets } from './render/assets.js';
import {
  createCamera, moveCamera, centerCameraOn, clampCamera,
} from './render/camera.js';
import { drawMap } from './render/draw_map.js';
import { TILE } from './sim/constants.js';

const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');
ctx.imageSmoothingEnabled = false; // pixel art

const world = createWorld(42);
const cam = createCamera(canvas.width, canvas.height, world.w, world.h);
centerCameraOn(cam, world.startPositions[0].x * TILE, world.startPositions[0].y * TILE);

const SCROLL_SPEED = 12; // px per frame while a key is held
const EDGE_PAN_MARGIN = 24; // px from canvas edge
const keys = new Set();
let mouseX = -1;
let mouseY = -1;

window.addEventListener('keydown', (e) => {
  keys.add(e.key);
  if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(e.key)) {
    e.preventDefault();
  }
});
window.addEventListener('keyup', (e) => keys.delete(e.key));
canvas.addEventListener('mousemove', (e) => {
  const r = canvas.getBoundingClientRect();
  mouseX = e.clientX - r.left;
  mouseY = e.clientY - r.top;
});
canvas.addEventListener('mouseleave', () => {
  mouseX = -1;
  mouseY = -1;
});

function updateCamera() {
  let dx = 0;
  let dy = 0;
  if (keys.has('ArrowLeft') || keys.has('a')) dx -= SCROLL_SPEED;
  if (keys.has('ArrowRight') || keys.has('d')) dx += SCROLL_SPEED;
  if (keys.has('ArrowUp') || keys.has('w')) dy -= SCROLL_SPEED;
  if (keys.has('ArrowDown') || keys.has('s')) dy += SCROLL_SPEED;
  if (mouseX >= 0) {
    if (mouseX < EDGE_PAN_MARGIN) dx -= SCROLL_SPEED;
    if (mouseX > cam.viewW - EDGE_PAN_MARGIN) dx += SCROLL_SPEED;
    if (mouseY < EDGE_PAN_MARGIN) dy -= SCROLL_SPEED;
    if (mouseY > cam.viewH - EDGE_PAN_MARGIN) dy += SCROLL_SPEED;
  }
  if (dx || dy) moveCamera(cam, dx, dy);
}

let registry = null; // assets may still be loading; drawMap falls back to rects
loadAssets().then((r) => {
  registry = r;
  if (r.missing.length) {
    console.warn('Missing assets (using fallbacks):', r.missing);
  }
});

let tick = 0;
let accumulator = 0;
let lastTime = performance.now();

function simTick() {
  tick++;
}

function render() {
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  drawMap(ctx, world, cam, registry);

  ctx.fillStyle = 'rgba(0,0,0,0.55)';
  ctx.fillRect(0, 0, 360, 30);
  ctx.fillStyle = '#c8ffc8';
  ctx.font = '13px monospace';
  ctx.fillText(`tick ${tick}  cam ${cam.x | 0},${cam.y | 0}  arrows/WASD or screen edge to scroll`, 8, 19);
}

function frame(now) {
  accumulator += now - lastTime;
  lastTime = now;
  // Clamp to avoid spiral-of-death after a background tab pause.
  if (accumulator > 250) accumulator = 250;
  while (accumulator >= MS_PER_TICK) {
    simTick();
    accumulator -= MS_PER_TICK;
  }
  updateCamera();
  clampCamera(cam);
  render();
  requestAnimationFrame(frame);
}

requestAnimationFrame(frame);
