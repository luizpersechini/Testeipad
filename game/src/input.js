// Input: mouse/keyboard -> selection + command queue. Selection picking and
// box-select are pure functions (node-tested); wireInput attaches DOM events
// once and reads the live session from the shell each event.

import { TILE, HouseType } from './sim/constants.js';
import { EntityKind } from './sim/entity.js';
import {
  moveCommand, stopCommand, attackCommand, attackMoveCommand, forceAttackCommand,
  deployCommand, harvestCommand, buildCommand, cancelBuildCommand, placeCommand,
  sellCommand, repairCommand,
} from './sim/commands.js';
import {
  screenToWorld, screenToCell, centerCameraOn, clampCamera,
} from './render/camera.js';
import { pointInMinimap, minimapToWorldPx } from './render/minimap.js';
import { hitSidebarItem } from './render/sidebar.js';
import { hitCommandBar } from './render/touchbar.js';
import { BUILDING_TYPES } from './sim/data/buildings.js';

// The canvas is CSS-scaled to fit the screen (iPad/fullscreen); every pointer
// coordinate must be mapped from CSS pixels into the fixed internal 1280x720.
export function canvasPos(canvas, clientX, clientY) {
  const r = canvas.getBoundingClientRect();
  return {
    x: (clientX - r.left) * (canvas.width / r.width),
    y: (clientY - r.top) * (canvas.height / r.height),
  };
}

export function createInputState() {
  return {
    selection: new Set(), // entity ids
    commandQueue: [], // drained by main.js into gameTick
    drag: null, // {x0, y0, x1, y1} in screen px while left button held
    attackMoveArmed: false, // A pressed; next left-click is attack-move
    groups: new Map(), // digit -> array of entity ids
    placing: null, // {type, footprint} while positioning a ready building
    sidebarItems: [], // refreshed each frame by main.js
    commandBarButtons: [], // refreshed each frame by main.js
    rightPan: null, // {sx, sy, camX, camY, moved} while right button held
  };
}

// Ctrl+digit stores the current selection; digit recalls it (dead ids drop out).
export function assignGroup(input, digit) {
  input.groups.set(digit, [...input.selection]);
}

export function recallGroup(input, store, digit) {
  const ids = (input.groups.get(digit) ?? []).filter((id) => store.entities.has(id));
  input.groups.set(digit, ids);
  input.selection.clear();
  for (const id of ids) input.selection.add(id);
}

function selectable(e, owner) {
  return e.owner === owner
    && (e.kind === EntityKind.UNIT || e.kind === EntityKind.INFANTRY);
}

// Entity whose footprint contains the world pixel, preferring `owner`'s.
export function pickEntityAt(store, worldPxX, worldPxY, owner = HouseType.GDI) {
  const cx = (worldPxX / TILE) | 0;
  const cy = (worldPxY / TILE) | 0;
  let fallback = null;
  for (const e of store.entities.values()) {
    if (e.kind === EntityKind.PROJECTILE) continue;
    const [fw, fh] = e.footprint ?? [1, 1];
    if (cx >= e.x && cx < e.x + fw && cy >= e.y && cy < e.y + fh) {
      if (e.owner === owner) return e;
      fallback = fallback ?? e;
    }
  }
  return fallback;
}

// `owner`'s mobile entities inside a world-pixel rectangle (any corner order).
export function entitiesInRect(store, ax, ay, bx, by, owner = HouseType.GDI) {
  const x0 = Math.min(ax, bx);
  const x1 = Math.max(ax, bx);
  const y0 = Math.min(ay, by);
  const y1 = Math.max(ay, by);
  const hits = [];
  for (const e of store.entities.values()) {
    if (!selectable(e, owner)) continue;
    const px = (e.x + e.subX + 0.5) * TILE;
    const py = (e.y + e.subY + 0.5) * TILE;
    if (px >= x0 && px <= x1 && py >= y0 && py <= y1) hits.push(e.id);
  }
  return hits;
}

const DRAG_THRESHOLD = 5; // px before a click becomes a box select

// Shared with wireTouch: apply a "primary action" tap/click at map coords —
// the C&C left-click scheme (select own things, order against the world).
function primaryMapAction(session, px, py) {
  const { game, cam, input, player } = session;
  if (input.placing) {
    const cell = screenToCell(cam, px, py);
    input.commandQueue.push(placeCommand(player, cell.x, cell.y));
    input.placing = null;
    return;
  }
  if (input.attackMoveArmed && input.selection.size > 0) {
    input.attackMoveArmed = false;
    const cell = screenToCell(cam, px, py);
    input.commandQueue.push(attackMoveCommand([...input.selection], cell.x, cell.y));
    return;
  }
  const wp = screenToWorld(cam, px, py);
  const hit = pickEntityAt(game.store, wp.x, wp.y, player);
  if (hit && hit.owner === player
    && (selectable(hit, player) || hit.kind === EntityKind.BUILDING)) {
    input.selection.clear();
    input.selection.add(hit.id);
    return;
  }
  if (input.selection.size > 0) {
    if (hit && hit.owner !== player) {
      input.commandQueue.push(attackCommand([...input.selection], hit.id));
    } else {
      const cell = screenToCell(cam, px, py);
      input.commandQueue.push(moveCommand([...input.selection], cell.x, cell.y));
    }
  } else {
    input.selection.clear();
  }
}

// Command-bar button -> command (shared by touch taps and mouse clicks).
function runBarButton(session, id) {
  const { input } = session;
  const ids = [...input.selection];
  switch (id) {
    case 'stop': if (ids.length) input.commandQueue.push(stopCommand(ids)); break;
    case 'attackmove': input.attackMoveArmed = !input.attackMoveArmed && ids.length > 0; break;
    case 'deploy': if (ids.length) input.commandQueue.push(deployCommand(ids)); break;
    case 'harvest': if (ids.length) input.commandQueue.push(harvestCommand(ids)); break;
    case 'repair': if (ids.length) input.commandQueue.push(repairCommand(ids)); break;
    case 'sell': if (ids.length) input.commandQueue.push(sellCommand(ids)); break;
    case 'pause': session.paused = !session.paused; break;
    default: break;
  }
}

// shell.session (nullable): {game, cam, minimap, input, player}.
export function wireInput(canvas, shell) {
  const mousePos = (e) => canvasPos(canvas, e.clientX, e.clientY);

  canvas.addEventListener('contextmenu', (e) => e.preventDefault());

  canvas.addEventListener('mousedown', (e) => {
    const s = shell.session;
    if (!s) return;
    const { game, cam, minimap, input, player } = s;
    const { store } = game;
    const world = game.world;
    const p = mousePos(e);

    if (e.button === 0) {
      if (pointInMinimap(minimap, p.x, p.y)) {
        const w = minimapToWorldPx(minimap, world, p.x, p.y);
        centerCameraOn(cam, w.x, w.y);
        return;
      }
      if (p.x >= cam.viewW) {
        // Sidebar build menu.
        const item = hitSidebarItem(input.sidebarItems, p.x, p.y);
        if (item && !item.blocked) {
          if (item.queued?.ready) {
            input.placing = {
              type: item.queued.type,
              footprint: BUILDING_TYPES[item.queued.type].footprint,
            };
          } else if (item.queued) {
            input.commandQueue.push(cancelBuildCommand(player, item.category));
          } else {
            input.commandQueue.push(buildCommand(player, item.category, item.type));
          }
        }
        return;
      }
      const barHit = hitCommandBar(input.commandBarButtons, p.x, p.y);
      if (barHit) {
        runBarButton(s, barHit.id);
        return;
      }
      if (input.placing) {
        const cell = screenToCell(cam, p.x, p.y);
        input.commandQueue.push(placeCommand(player, cell.x, cell.y));
        input.placing = null;
        return;
      }
      if (input.attackMoveArmed && input.selection.size > 0) {
        input.attackMoveArmed = false;
        const cell = screenToCell(cam, p.x, p.y);
        input.commandQueue.push(attackMoveCommand([...input.selection], cell.x, cell.y));
        return;
      }
      if (e.ctrlKey && input.selection.size > 0) {
        const cell = screenToCell(cam, p.x, p.y);
        input.commandQueue.push(forceAttackCommand([...input.selection], cell.x, cell.y));
        return;
      }
      input.drag = { x0: p.x, y0: p.y, x1: p.x, y1: p.y };
    } else if (e.button === 2 && p.x < cam.viewW) {
      // Right button: drag pans the camera; a clean click issues orders
      // (decided on mouseup once we know whether it moved).
      input.rightPan = { sx: p.x, sy: p.y, camX: cam.x, camY: cam.y, moved: false };
    }
  });

  canvas.addEventListener('mousemove', (e) => {
    const s = shell.session;
    if (!s) return;
    const { input, cam } = s;
    const p = mousePos(e);
    if (input.drag) {
      input.drag.x1 = p.x;
      input.drag.y1 = p.y;
    }
    if (input.rightPan) {
      const dx = p.x - input.rightPan.sx;
      const dy = p.y - input.rightPan.sy;
      if (input.rightPan.moved || Math.abs(dx) > 5 || Math.abs(dy) > 5) {
        input.rightPan.moved = true;
        cam.x = input.rightPan.camX - dx;
        cam.y = input.rightPan.camY - dy;
        clampCamera(cam);
      }
    }
  });

  canvas.addEventListener('mouseup', (e) => {
    const s = shell.session;
    if (!s || e.button !== 2 || !s.input.rightPan) return;
    const { game, cam, input, player } = s;
    const pan = input.rightPan;
    input.rightPan = null;
    if (pan.moved) return; // it was a camera drag, not an order
    const p = mousePos(e);
    if (input.placing) {
      input.placing = null; // right-click cancels placement mode
      return;
    }
    if (input.selection.size === 0) return;
    // Clean right-click: attack an enemy under the cursor, otherwise move.
    const wp = screenToWorld(cam, p.x, p.y);
    const hit = pickEntityAt(game.store, wp.x, wp.y, player);
    if (hit && hit.owner !== player) {
      input.commandQueue.push(attackCommand([...input.selection], hit.id));
    } else {
      const cell = screenToCell(cam, p.x, p.y);
      input.commandQueue.push(moveCommand([...input.selection], cell.x, cell.y));
    }
  });

  window.addEventListener('mouseup', (e) => {
    const s = shell.session;
    if (!s || e.button !== 0 || !s.input.drag) return;
    const { game, cam, input, player } = s;
    const d = input.drag;
    input.drag = null;
    const w = Math.abs(d.x1 - d.x0);
    const h = Math.abs(d.y1 - d.y0);
    if (w < DRAG_THRESHOLD && h < DRAG_THRESHOLD) {
      // Click select: own mobiles and own buildings (for sell/repair).
      const wp = screenToWorld(cam, d.x0, d.y0);
      const hit = pickEntityAt(game.store, wp.x, wp.y, player);
      input.selection.clear();
      if (hit && hit.owner === player
        && (selectable(hit, player) || hit.kind === EntityKind.BUILDING)) {
        input.selection.add(hit.id);
      }
    } else {
      const a = screenToWorld(cam, d.x0, d.y0);
      const b = screenToWorld(cam, d.x1, d.y1);
      const hits = entitiesInRect(game.store, a.x, a.y, b.x, b.y, player);
      input.selection.clear();
      for (const id of hits) input.selection.add(id);
    }
  });

  window.addEventListener('keydown', (e) => {
    const s = shell.session;
    if (!s) return;
    const { input, game } = s;
    if (e.key === 's' && input.selection.size > 0) {
      input.commandQueue.push(stopCommand([...input.selection]));
      input.attackMoveArmed = false;
    } else if (e.key === 'd' && input.selection.size > 0) {
      input.commandQueue.push(deployCommand([...input.selection]));
    } else if (e.key === 'h' && input.selection.size > 0) {
      input.commandQueue.push(harvestCommand([...input.selection]));
    } else if (e.key === 'r' && input.selection.size > 0) {
      input.commandQueue.push(repairCommand([...input.selection]));
    } else if (e.key === 'x' && input.selection.size > 0) {
      input.commandQueue.push(sellCommand([...input.selection]));
    } else if (e.key === 'a' || e.key === 'A') {
      input.attackMoveArmed = input.selection.size > 0;
    } else if (e.key === 'Escape') {
      input.attackMoveArmed = false;
      input.placing = null;
      input.selection.clear();
    } else if (e.key >= '1' && e.key <= '9') {
      const digit = e.key;
      if (e.ctrlKey) {
        assignGroup(input, digit);
        e.preventDefault();
      } else {
        recallGroup(input, game.store, digit);
      }
    }
  });
}

// ── Touch controls (iPad) ────────────────────────────────────────────────────
// Gesture scheme:
//   tap            select own unit/building; with a selection: attack the
//                  enemy / move to the ground under the finger (C&C style)
//   drag (1 finger, from empty map ground) box-select marquee
//   drag (2 fingers)                       pan the camera
//   long-press (with selection)            attack-move to that spot
//   taps on sidebar / minimap / command bar work like clicks

const TAP_MS = 350;
const TAP_SLOP = 12; // px of movement still counting as a tap
const LONG_PRESS_MS = 450;

export function wireTouch(canvas, shell, { onUiTap, onAnyTouch } = {}) {
  let touch = null; // active single-touch gesture state
  let pan = null; // two-finger pan state
  let longPressTimer = null;

  const posOf = (t) => canvasPos(canvas, t.clientX, t.clientY);

  const clearLongPress = () => {
    if (longPressTimer) {
      clearTimeout(longPressTimer);
      longPressTimer = null;
    }
  };

  canvas.addEventListener('touchstart', (e) => {
    e.preventDefault();
    onAnyTouch?.(); // unlock WebAudio on first gesture
    const s = shell.session;

    if (e.touches.length === 2) {
      // Second finger: cancel any tap/marquee, start panning.
      clearLongPress();
      if (s) s.input.drag = null;
      touch = null;
      const a = posOf(e.touches[0]);
      const b = posOf(e.touches[1]);
      pan = s ? {
        x: (a.x + b.x) / 2,
        y: (a.y + b.y) / 2,
        camX: s.cam.x,
        camY: s.cam.y,
      } : null;
      return;
    }
    if (e.touches.length !== 1) return;

    const p = posOf(e.touches[0]);
    touch = { start: p, last: p, t0: performance.now(), moved: false };
    pan = null;

    if (!s || shell.menu.screen !== 'game') return;
    const { cam, input, game, player } = s;

    // Long-press on the map with a selection = attack-move.
    if (p.x < cam.viewW && input.selection.size > 0 && !input.placing) {
      clearLongPress();
      longPressTimer = setTimeout(() => {
        if (!touch || touch.moved) return;
        const cell = screenToCell(cam, touch.last.x, touch.last.y);
        input.commandQueue.push(attackMoveCommand([...input.selection], cell.x, cell.y));
        touch = null; // consumed
      }, LONG_PRESS_MS);
    }

    // Start a marquee only from empty own-map ground (so taps stay taps and
    // panning stays two-finger).
    if (p.x < cam.viewW && !input.placing) {
      const wp = screenToWorld(cam, p.x, p.y);
      const under = pickEntityAt(game.store, wp.x, wp.y, player);
      if (!under) {
        input.drag = { x0: p.x, y0: p.y, x1: p.x, y1: p.y };
      }
    }
  }, { passive: false });

  canvas.addEventListener('touchmove', (e) => {
    e.preventDefault();
    const s = shell.session;
    if (pan && s && e.touches.length === 2) {
      const a = posOf(e.touches[0]);
      const b = posOf(e.touches[1]);
      const cx = (a.x + b.x) / 2;
      const cy = (a.y + b.y) / 2;
      s.cam.x = pan.camX - (cx - pan.x);
      s.cam.y = pan.camY - (cy - pan.y);
      clampCamera(s.cam);
      return;
    }
    if (!touch || e.touches.length !== 1) return;
    const p = posOf(e.touches[0]);
    touch.last = p;
    if (Math.abs(p.x - touch.start.x) > TAP_SLOP
      || Math.abs(p.y - touch.start.y) > TAP_SLOP) {
      touch.moved = true;
      clearLongPress();
    }
    if (s?.input.drag) {
      s.input.drag.x1 = p.x;
      s.input.drag.y1 = p.y;
    }
    // Placement ghost follows the finger.
    if (s?.input.placing) shell.pointer = { ...p, isMouse: false };
  }, { passive: false });

  canvas.addEventListener('touchend', (e) => {
    e.preventDefault();
    clearLongPress();
    const s = shell.session;

    if (pan) {
      if (e.touches.length < 2) pan = null;
      return;
    }
    if (!touch) return;
    const gesture = touch;
    touch = null;

    // Finish a marquee drag.
    if (s && s.input.drag) {
      const d = s.input.drag;
      s.input.drag = null;
      if (gesture.moved) {
        const a = screenToWorld(s.cam, d.x0, d.y0);
        const b = screenToWorld(s.cam, d.x1, d.y1);
        const hits = entitiesInRect(s.game.store, a.x, a.y, b.x, b.y, s.player);
        s.input.selection.clear();
        for (const id of hits) s.input.selection.add(id);
        return;
      }
    }
    if (gesture.moved) return;
    if (performance.now() - gesture.t0 > TAP_MS * 2) return; // long-press already fired or stale

    const p = gesture.start;
    // Menus and end screens are handled by the shell.
    if (!s || shell.menu.screen !== 'game' || s.game.winner !== null) {
      onUiTap?.(p.x, p.y);
      return;
    }
    const { cam, input, game, player } = s;

    const barHit = hitCommandBar(input.commandBarButtons, p.x, p.y);
    if (barHit) {
      runBarButton(s, barHit.id);
      return;
    }
    if (p.x >= cam.viewW) {
      // Sidebar: minimap jump or build menu, same as a click.
      if (pointInMinimap(s.minimap, p.x, p.y)) {
        const w = minimapToWorldPx(s.minimap, game.world, p.x, p.y);
        centerCameraOn(cam, w.x, w.y);
        return;
      }
      const item = hitSidebarItem(input.sidebarItems, p.x, p.y);
      if (item && !item.blocked) {
        if (item.queued?.ready) {
          input.placing = {
            type: item.queued.type,
            footprint: BUILDING_TYPES[item.queued.type].footprint,
          };
          shell.pointer = { x: cam.viewW / 2, y: cam.viewH / 2, isMouse: false };
        } else if (item.queued) {
          input.commandQueue.push(cancelBuildCommand(player, item.category));
        } else {
          input.commandQueue.push(buildCommand(player, item.category, item.type));
        }
      }
      return;
    }
    primaryMapAction(s, p.x, p.y);
  }, { passive: false });

  canvas.addEventListener('touchcancel', () => {
    clearLongPress();
    touch = null;
    pan = null;
    if (shell.session) shell.session.input.drag = null;
  });
}
