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
  layoutSidebarItems, drawSidebar, drawPlacementGhost,
} from './render/sidebar.js';
import { placeBuilding, canPlaceBuilding } from './sim/placement.js';
import { screenToCell } from './render/camera.js';
import {
  createEffects, spawnFromEvents, pruneEffects, drawEffects, drawProjectiles,
} from './render/draw_effects.js';
import { createInputState, wireInput } from './input.js';
import { enableAI } from './sim/ai.js';

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

// Starting bases until scenarios land (M8): yard + power for each side,
// plus a small escort force.
const [gdiStart, nodStart] = world.startPositions;
function spawnBase(owner, start, escort) {
  placeBuilding(game, owner, 'construction_yard', start.x - 1, start.y - 1, { ignoreAdjacency: true });
  placeBuilding(game, owner, 'power_plant', start.x + 3, start.y - 1);
  escort.forEach(([kind, type, hp], i) => {
    spawn(store, world, {
      kind, type, owner, hp,
      x: start.x + (i % 4) - 1,
      y: start.y + ((i / 4) | 0) + 3,
    });
  });
}
spawnBase(HouseType.GDI, gdiStart, [
  [EntityKind.UNIT, 'medium_tank', 400],
  [EntityKind.UNIT, 'medium_tank', 400],
  [EntityKind.UNIT, 'harvester', 600],
  [EntityKind.INFANTRY, 'minigunner', 50],
  [EntityKind.INFANTRY, 'rocket_soldier', 45],
]);
spawnBase(HouseType.NOD, nodStart, [
  [EntityKind.UNIT, 'light_tank', 300],
  [EntityKind.UNIT, 'stealth_tank', 180],
  [EntityKind.UNIT, 'harvester', 600],
  [EntityKind.INFANTRY, 'minigunner', 50],
  [EntityKind.INFANTRY, 'flamethrower', 60],
]);

// Nod is AI-controlled. Pick difficulty with ?ai=easy|normal|hard|off.
const aiSetting = new URLSearchParams(window.location.search).get('ai') ?? 'normal';
if (aiSetting !== 'off') {
  enableAI(game, HouseType.NOD, ['easy', 'normal', 'hard'].includes(aiSetting) ? aiSetting : 'normal');
}

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
let shownCredits = 0;

function render() {
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  drawMap(ctx, world, cam, registry);
  drawEntities(ctx, store, cam, registry, input.selection, game.tick);
  drawProjectiles(ctx, store, cam);
  drawEffects(ctx, effects, cam, registry, game.tick);
  drawDragBox(ctx, input.drag);

  // Sidebar panel.
  const sbx = canvas.width - SIDEBAR_W;
  ctx.fillStyle = '#1e1e1e';
  ctx.fillRect(sbx, 0, SIDEBAR_W, canvas.height);
  drawMinimap(ctx, minimap, world, cam);

  // Credits ticker (rolls toward the real value like the original).
  const house = game.houses[HouseType.GDI];
  shownCredits += Math.sign(house.credits - shownCredits)
    * Math.min(Math.abs(house.credits - shownCredits), 7);
  const mmBottom = minimap.y + minimap.h;
  ctx.fillStyle = '#000';
  ctx.fillRect(sbx + 8, mmBottom + 8, SIDEBAR_W - 16, 22);
  ctx.fillStyle = '#54d454';
  ctx.font = 'bold 15px monospace';
  ctx.textAlign = 'right';
  ctx.fillText(`$ ${Math.round(shownCredits)}`, sbx + SIDEBAR_W - 14, mmBottom + 24);
  ctx.textAlign = 'left';

  // Power bar: output vs drain.
  const pbY = mmBottom + 38;
  const pbW = SIDEBAR_W - 16;
  const maxShown = Math.max(house.powerOutput, house.powerDrain, 100);
  ctx.fillStyle = '#000';
  ctx.fillRect(sbx + 8, pbY, pbW, 10);
  ctx.fillStyle = house.lowPower ? '#c03030' : '#30a030';
  ctx.fillRect(sbx + 8, pbY, Math.round(pbW * house.powerOutput / maxShown), 10);
  ctx.strokeStyle = '#e0e040'; // drain marker
  const dx = sbx + 8 + Math.round(pbW * house.powerDrain / maxShown);
  ctx.beginPath();
  ctx.moveTo(dx, pbY - 2);
  ctx.lineTo(dx, pbY + 12);
  ctx.stroke();
  ctx.fillStyle = '#888';
  ctx.font = '10px monospace';
  ctx.fillText(house.lowPower ? 'LOW POWER' : 'POWER', sbx + 8, pbY + 22);

  // Build menu.
  input.sidebarItems = layoutSidebarItems(game, HouseType.GDI, sbx, SIDEBAR_W, pbY + 32);
  drawSidebar(ctx, input.sidebarItems, registry, game.tick);

  // Placement ghost under the cursor.
  if (input.placing && mouseX >= 0 && mouseX < cam.viewW) {
    const cell = screenToCell(cam, mouseX, mouseY);
    const legal = canPlaceBuilding(game, HouseType.GDI, input.placing.type, cell.x, cell.y);
    drawPlacementGhost(ctx, cam, cell, input.placing.footprint, legal, TILE);
  }

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
