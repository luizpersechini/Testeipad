// Game shell: main menu -> skirmish setup -> game -> end screen -> menu.
// Sim ticks at a fixed 15/s (accumulator); rendering runs at rAF rate.

import { MS_PER_TICK, TILE, HouseType } from './sim/constants.js';
import { createGame, gameTick } from './sim/game.js';
import { spawn, EntityKind } from './sim/entity.js';
import { placeBuilding, canPlaceBuilding } from './sim/placement.js';
import { enableAI } from './sim/ai.js';
import { loadAssets } from './render/assets.js';
import {
  createCamera, moveCamera, centerCameraOn, clampCamera, screenToCell,
} from './render/camera.js';
import { drawMap } from './render/draw_map.js';
import { createMinimapLayout, drawMinimap } from './render/minimap.js';
import { drawEntities, drawDragBox } from './render/draw_entities.js';
import {
  createEffects, spawnFromEvents, pruneEffects, drawEffects, drawProjectiles,
} from './render/draw_effects.js';
import {
  layoutSidebarItems, drawSidebar, drawPlacementGhost,
} from './render/sidebar.js';
import {
  createMenu, layoutMenu, hitMenuItem, menuClick, drawMenu,
} from './render/menu.js';
import { createInputState, wireInput } from './input.js';
import { serializeGame, deserializeGame } from './sim/save.js';
import { createAudio, playForEvents, toggleMute } from './render/audio.js';
import {
  createFeedback, markersFromCommands, feedbackFromEvents, pruneFeedback,
  decayShake, drawMarkers, drawPings,
} from './render/feedback.js';

const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');
ctx.imageSmoothingEnabled = false; // pixel art

const SIDEBAR_W = 200;

let registry = null; // assets may still be loading; draw code falls back to rects
loadAssets().then((r) => {
  registry = r;
  if (r.missing.length) console.warn('Missing assets (using fallbacks):', r.missing);
});

// ── Session (one running match) ──────────────────────────────────────────────

function startSession(settings) {
  const player = settings.faction === 'nod' ? HouseType.NOD : HouseType.GDI;
  const enemy = player === HouseType.GDI ? HouseType.NOD : HouseType.GDI;
  const game = createGame(settings.seed, { startingCredits: settings.credits });
  const { world } = game;

  const starts = world.startPositions;
  const playerStart = starts[player === HouseType.GDI ? 0 : 1];
  const enemyStart = starts[player === HouseType.GDI ? 1 : 0];

  const escort = (owner) => {
    const heavy = owner === HouseType.GDI ? 'medium_tank' : 'light_tank';
    const heavyHp = owner === HouseType.GDI ? 400 : 300;
    return [
      [EntityKind.UNIT, heavy, heavyHp],
      [EntityKind.UNIT, heavy, heavyHp],
      [EntityKind.INFANTRY, 'minigunner', 50],
      [EntityKind.INFANTRY, 'minigunner', 50],
      [EntityKind.INFANTRY, 'rocket_soldier', 45],
    ];
  };
  for (const [owner, start] of [[player, playerStart], [enemy, enemyStart]]) {
    placeBuilding(game, owner, 'construction_yard', start.x - 1, start.y - 1, { ignoreAdjacency: true });
    placeBuilding(game, owner, 'power_plant', start.x + 3, start.y - 1);
    escort(owner).forEach(([kind, type, hp], i) => {
      spawn(game.store, world, {
        kind, type, owner: owner, hp,
        x: start.x + (i % 4) - 1,
        y: start.y + ((i / 4) | 0) + 3,
      });
    });
  }
  enableAI(game, enemy, settings.difficulty);

  const cam = createCamera(canvas.width - SIDEBAR_W, canvas.height, world.w, world.h);
  centerCameraOn(cam, playerStart.x * TILE, playerStart.y * TILE);

  return {
    game,
    cam,
    player,
    minimap: createMinimapLayout(canvas.width, SIDEBAR_W, world),
    input: createInputState(),
    effects: createEffects(),
    feedback: createFeedback(),
    shownCredits: settings.credits,
    paused: false,
    accumulator: 0,
  };
}

// ── Shell state ──────────────────────────────────────────────────────────────

const menu = createMenu();
const shell = { menu, session: null };
wireInput(canvas, shell);

const audio = createAudio();
audio.muted = window.localStorage.getItem('cnc-td-muted') === '1';

const SCROLL_SPEED = 12;
const EDGE_PAN_MARGIN = 24;
const keys = new Set();
let mouseX = -1;
let mouseY = -1;

// ── Save/load: F2/F3/F4 save slots 1-3, F6/F7/F8 load them ─────────────────

const SAVE_KEY = (slot) => `cnc-td-save-${slot}`;

function saveSlot(slot) {
  const s = shell.session;
  if (!s || s.game.winner !== null) return;
  const blob = { player: s.player, game: serializeGame(s.game) };
  try {
    window.localStorage.setItem(SAVE_KEY(slot), JSON.stringify(blob));
    s.toast = { text: `Saved to slot ${slot}`, until: s.game.tick + 30 };
  } catch {
    s.toast = { text: 'Save failed (storage full?)', until: s.game.tick + 30 };
  }
}

function loadSlot(slot) {
  const raw = window.localStorage.getItem(SAVE_KEY(slot));
  if (!raw) {
    if (shell.session) {
      shell.session.toast = { text: `Slot ${slot} is empty`, until: shell.session.game.tick + 30 };
    }
    return;
  }
  const blob = JSON.parse(raw);
  const game = deserializeGame(blob.game);
  if (!game) return;
  const cam = createCamera(canvas.width - SIDEBAR_W, canvas.height, game.world.w, game.world.h);
  const start = game.world.startPositions[blob.player === HouseType.GDI ? 0 : 1];
  centerCameraOn(cam, start.x * TILE, start.y * TILE);
  shell.session = {
    game,
    cam,
    player: blob.player,
    minimap: createMinimapLayout(canvas.width, SIDEBAR_W, game.world),
    input: createInputState(),
    effects: createEffects(),
    shownCredits: game.houses[blob.player].credits,
    feedback: createFeedback(),
    paused: false,
    accumulator: 0,
    toast: { text: `Loaded slot ${slot}`, until: game.tick + 30 },
  };
  menu.screen = 'game';
}

window.addEventListener('keydown', (e) => {
  keys.add(e.key);
  if (e.key.startsWith('Arrow')) e.preventDefault();
  if (shell.session && menu.screen === 'game' && (e.key === 'p' || e.key === 'P')) {
    shell.session.paused = !shell.session.paused;
  }
  if (e.key === 'm' || e.key === 'M') {
    window.localStorage.setItem('cnc-td-muted', toggleMute(audio) ? '1' : '0');
  }
  const saveKeys = { F2: 1, F3: 2, F4: 3 };
  const loadKeys = { F6: 1, F7: 2, F8: 3 };
  if (saveKeys[e.key]) {
    e.preventDefault();
    saveSlot(saveKeys[e.key]);
  } else if (loadKeys[e.key]) {
    e.preventDefault();
    loadSlot(loadKeys[e.key]);
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

// Menu clicks (left cycles forward, right cycles back) + end-screen exit.
canvas.addEventListener('mousedown', (e) => {
  if (menu.screen === 'game') {
    if (shell.session && shell.session.game.winner !== null) {
      menu.screen = 'main';
      shell.session = null;
    }
    return;
  }
  const r = canvas.getBoundingClientRect();
  const px = e.clientX - r.left;
  const py = e.clientY - r.top;
  const items = layoutMenu(menu, canvas.width, canvas.height);
  const item = hitMenuItem(items, px, py);
  const action = menuClick(menu, item, e.button === 2 ? -1 : 1);
  if (action?.type === 'start') {
    shell.session = startSession(action.settings);
  }
});

function updateCamera(s) {
  let dx = 0;
  let dy = 0;
  if (keys.has('ArrowLeft')) dx -= SCROLL_SPEED;
  if (keys.has('ArrowRight')) dx += SCROLL_SPEED;
  if (keys.has('ArrowUp')) dy -= SCROLL_SPEED;
  if (keys.has('ArrowDown')) dy += SCROLL_SPEED;
  if (mouseX >= 0 && !s.input.drag) {
    if (mouseX < EDGE_PAN_MARGIN) dx -= SCROLL_SPEED;
    if (mouseX > s.cam.viewW - EDGE_PAN_MARGIN && mouseX < s.cam.viewW + 4) dx += SCROLL_SPEED;
    if (mouseY < EDGE_PAN_MARGIN) dy -= SCROLL_SPEED;
    if (mouseY > s.cam.viewH - EDGE_PAN_MARGIN) dy += SCROLL_SPEED;
  }
  if (dx || dy) moveCamera(s.cam, dx, dy);
}

// ── Rendering ────────────────────────────────────────────────────────────────

function renderGame(s) {
  const { game, cam, input, player } = s;
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  // Screen shake from nearby destruction (render-only jitter).
  decayShake(s.feedback);
  const shaking = s.feedback.shake > 0;
  if (shaking) {
    ctx.save();
    ctx.translate(
      (Math.random() - 0.5) * s.feedback.shake,
      (Math.random() - 0.5) * s.feedback.shake,
    );
  }

  const fogMap = game.fog[player];
  drawMap(ctx, game.world, cam, registry, fogMap);
  drawEntities(ctx, game.store, cam, registry, input.selection, game.tick, game, player);
  drawProjectiles(ctx, game.store, cam);
  drawEffects(ctx, s.effects, cam, registry, game.tick);
  drawMarkers(ctx, s.feedback, cam, game.tick);
  drawDragBox(ctx, input.drag);
  if (shaking) ctx.restore();

  // Sidebar panel.
  const sbx = canvas.width - SIDEBAR_W;
  ctx.fillStyle = '#1e1e1e';
  ctx.fillRect(sbx, 0, SIDEBAR_W, canvas.height);
  drawMinimap(ctx, s.minimap, game.world, cam, fogMap, game.tick, game, player);
  drawPings(ctx, s.feedback, s.minimap, game.world, game.tick);

  // Credits ticker (rolls toward the real value like the original).
  const house = game.houses[player];
  s.shownCredits += Math.sign(house.credits - s.shownCredits)
    * Math.min(Math.abs(house.credits - s.shownCredits), 7);
  const mmBottom = s.minimap.y + s.minimap.h;
  ctx.fillStyle = '#000';
  ctx.fillRect(sbx + 8, mmBottom + 8, SIDEBAR_W - 16, 22);
  ctx.fillStyle = '#54d454';
  ctx.font = 'bold 15px monospace';
  ctx.textAlign = 'right';
  ctx.fillText(`$ ${Math.round(s.shownCredits)}`, sbx + SIDEBAR_W - 14, mmBottom + 24);
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
  input.sidebarItems = layoutSidebarItems(game, player, sbx, SIDEBAR_W, pbY + 32);
  drawSidebar(ctx, input.sidebarItems, registry, game.tick);

  // Placement ghost under the cursor.
  if (input.placing && mouseX >= 0 && mouseX < cam.viewW) {
    const cell = screenToCell(cam, mouseX, mouseY);
    const legal = canPlaceBuilding(game, player, input.placing.type, cell.x, cell.y);
    drawPlacementGhost(ctx, cam, cell, input.placing.footprint, legal, TILE);
  }

  ctx.fillStyle = 'rgba(0,0,0,0.55)';
  ctx.fillRect(0, 0, 430, 22);
  ctx.fillStyle = '#c8ffc8';
  ctx.font = '12px monospace';
  ctx.fillText(
    `tick ${game.tick}  sel ${input.selection.size}  A attack-move  S stop  P pause  M ${audio.muted ? 'unmute' : 'mute'}  F2-F4/F6-F8 save/load`,
    8, 15,
  );

  if (s.toast && game.tick < s.toast.until) {
    ctx.fillStyle = 'rgba(0,0,0,0.7)';
    ctx.fillRect(canvas.width / 2 - 140, 40, 280, 26);
    ctx.fillStyle = '#ffd24a';
    ctx.textAlign = 'center';
    ctx.fillText(s.toast.text, canvas.width / 2, 57);
    ctx.textAlign = 'left';
  }

  if (s.paused && game.winner === null) {
    ctx.fillStyle = 'rgba(0,0,0,0.5)';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.textAlign = 'center';
    ctx.font = 'bold 40px monospace';
    ctx.fillStyle = '#ffd24a';
    ctx.fillText('PAUSED', canvas.width / 2, canvas.height / 2);
    ctx.textAlign = 'left';
  }
}

function drawEndScreen(s) {
  const won = s.game.winner === s.player;
  ctx.fillStyle = 'rgba(0, 0, 0, 0.75)';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.textAlign = 'center';
  ctx.font = 'bold 52px monospace';
  ctx.fillStyle = won ? '#ffd24a' : '#d04030';
  const title = s.game.winner === -1 ? 'STALEMATE' : won ? 'MISSION ACCOMPLISHED' : 'MISSION FAILED';
  ctx.fillText(title, canvas.width / 2, 250);

  const stats = s.game.houses[s.player].stats;
  ctx.font = '18px monospace';
  ctx.fillStyle = '#cfcfcf';
  const lines = [
    `Units built: ${stats.unitsBuilt}    lost: ${stats.unitsLost}`,
    `Buildings built: ${stats.buildingsBuilt}    lost: ${stats.buildingsLost}`,
    `Enemies destroyed: ${stats.kills}`,
    `Tiberium harvested: $${stats.creditsHarvested}`,
    '',
    'Click anywhere to return to the menu',
  ];
  lines.forEach((line, i) => ctx.fillText(line, canvas.width / 2, 330 + i * 30));
  ctx.textAlign = 'left';
}

// ── Main loop ────────────────────────────────────────────────────────────────

let lastTime = performance.now();

function frame(now) {
  const dt = now - lastTime;
  lastTime = now;

  if (menu.screen !== 'game' || !shell.session) {
    const items = layoutMenu(menu, canvas.width, canvas.height);
    drawMenu(ctx, menu, items, canvas.width, canvas.height);
    requestAnimationFrame(frame);
    return;
  }

  const s = shell.session;
  s.accumulator += dt;
  if (s.accumulator > 250) s.accumulator = 250; // background-tab pause guard
  while (s.accumulator >= MS_PER_TICK) {
    if (!s.paused && s.game.winner === null) {
      const commands = s.input.commandQueue.splice(0);
      markersFromCommands(s.feedback, commands, s.game.tick);
      gameTick(s.game, commands);
      spawnFromEvents(s.effects, s.game.events, s.game.tick);
      pruneEffects(s.effects, s.game.tick);
      pruneFeedback(s.feedback, s.game.tick);
      if (feedbackFromEvents(s.feedback, s.game.events, s.game, s.player, s.game.tick)) {
        s.toast = { text: 'BASE UNDER ATTACK', until: s.game.tick + 45 };
      }
      playForEvents(audio, s.game.events);
    }
    s.accumulator -= MS_PER_TICK;
  }
  updateCamera(s);
  clampCamera(s.cam);
  renderGame(s);
  if (s.game.winner !== null) drawEndScreen(s);
  requestAnimationFrame(frame);
}

requestAnimationFrame(frame);
