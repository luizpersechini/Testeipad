// Boot: canvas setup and the fixed-timestep loop.
// Sim ticks at a fixed 15/s (accumulator); rendering runs at requestAnimationFrame rate.

import { MS_PER_TICK, TILE, HouseType } from './sim/constants.js';
import { createGame, gameTick } from './sim/game.js';
import { spawn, EntityKind } from './sim/entity.js';
import { loadAssets } from './render/assets.js';
import { createCamera, moveCamera, centerCameraOn, clampCamera } from './render/camera.js';
import { drawMap } from './render/draw_map.js';
import { createMinimapLayout, minimapToWorldPx, drawMinimap } from './render/minimap.js';
import { drawEntities, drawDragBox } from './render/draw_entities.js';
import {
  createEffects, spawnFromEvents, pruneEffects, drawEffects, drawProjectiles,
} from './render/draw_effects.js';
import { createInputState, wireInput } from './input.js';

const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');
ctx.imageSmoothingEnabled = false; // pixel art

const SIDEBAR_W = 200;
const game = createGame(42);
const { world, store } = game;
const cam = createCamera(canvas.width - SIDEBAR_W, canvas.height, world.w, world.h);
const minimap = createMinimapLayout(canvas.width, SIDEBAR_W, world);
const input = createInputState();
const effects = createEffects();

// Temporary starting forces until scenarios land (M5/M8).
const [gdiStart, nodStart] = world.startPositions;
function spawnGroup(owner, start, types) {
  types.forEach(([kind, type, hp], i) => {
    spawn(store, world, {
      kind, type, owner, hp,
      x: start.x + (i % 3) - 1,
      y: start.y + ((i / 3) | 0) + 2,
    });
  });
}
spawnGroup(HouseType.GDI, gdiStart, [
  [EntityKind.UNIT, 'medium_tank', 400],
  [EntityKind.UNIT, 'medium_tank', 400],
  [EntityKind.UNIT, 'mammoth_tank', 600],
  [EntityKind.INFANTRY, 'minigunner', 50],
  [EntityKind.INFANTRY, 'minigunner', 50],
  [EntityKind.INFANTRY, 'rocket_soldier', 45],
]);
spawnGroup(HouseType.NOD, nodStart, [
  [EntityKind.UNIT, 'light_tank', 300],
  [EntityKind.UNIT, 'light_tank', 300],
  [EntityKind.UNIT, 'stealth_tank', 180],
  [EntityKind.INFANTRY, 'minigunner', 50],
  [EntityKind.INFANTRY, 'flamethrower', 60],
]);

centerCameraOn(cam, gdiStart.x * TILE, gdiStart.y * TILE);

wireInput(canvas, input, { cam, store, minimap, world, centerCameraOn, minimapToWorldPx });

// Camera scrolling (arrows + edge pan; WASD reserved for game hotkeys).
const SCROLL_SPEED = 12;
const EDGE_PAN_MARGIN = 24;
const keys = new Set();
let mouseX = -1;
let mouseY = -1;
window.addEventListener('keydown', (e) => {
  keys.add(e.key);
  if (e.key.startsWith('Arrow')) e.preventDefault();
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
  if (keys.has('ArrowLeft')) dx -= SCROLL_SPEED;
  if (keys.has('ArrowRight')) dx += SCROLL_SPEED;
  if (keys.has('ArrowUp')) dy -= SCROLL_SPEED;
  if (keys.has('ArrowDown')) dy += SCROLL_SPEED;
  if (mouseX >= 0 && !input.drag) {
    if (mouseX < EDGE_PAN_MARGIN) dx -= SCROLL_SPEED;
    if (mouseX > cam.viewW - EDGE_PAN_MARGIN && mouseX < cam.viewW + 4) dx += SCROLL_SPEED;
    if (mouseY < EDGE_PAN_MARGIN) dy -= SCROLL_SPEED;
    if (mouseY > cam.viewH - EDGE_PAN_MARGIN) dy += SCROLL_SPEED;
  }
  if (dx || dy) moveCamera(cam, dx, dy);
}

let registry = null; // assets may still be loading; draw code falls back to rects
loadAssets().then((r) => {
  registry = r;
  if (r.missing.length) console.warn('Missing assets (using fallbacks):', r.missing);
});

let accumulator = 0;
let lastTime = performance.now();

function render() {
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  drawMap(ctx, world, cam, registry);
  drawEntities(ctx, store, cam, registry, input.selection, game.tick);
  drawProjectiles(ctx, store, cam);
  drawEffects(ctx, effects, cam, registry, game.tick);
  drawDragBox(ctx, input.drag);

  // Sidebar panel.
  ctx.fillStyle = '#1e1e1e';
  ctx.fillRect(canvas.width - SIDEBAR_W, 0, SIDEBAR_W, canvas.height);
  drawMinimap(ctx, minimap, world, cam);

  ctx.fillStyle = 'rgba(0,0,0,0.55)';
  ctx.fillRect(0, 0, 430, 30);
  ctx.fillStyle = '#c8ffc8';
  ctx.font = '13px monospace';
  ctx.fillText(
    `tick ${game.tick}  sel ${input.selection.size}  drag-select, right-click move, S stop`,
    8, 19,
  );
}

function frame(now) {
  accumulator += now - lastTime;
  lastTime = now;
  if (accumulator > 250) accumulator = 250; // background-tab pause guard
  while (accumulator >= MS_PER_TICK) {
    gameTick(game, input.commandQueue.splice(0));
    spawnFromEvents(effects, game.events, game.tick);
    pruneEffects(effects, game.tick);
    accumulator -= MS_PER_TICK;
  }
  updateCamera();
  clampCamera(cam);
  render();
  requestAnimationFrame(frame);
}

requestAnimationFrame(frame);
