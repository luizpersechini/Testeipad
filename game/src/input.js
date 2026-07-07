// Input: mouse/keyboard -> selection + command queue. Selection picking and
// box-select are pure functions (node-tested); wireInput attaches DOM events.

import { TILE, HouseType } from './sim/constants.js';
import { EntityKind } from './sim/entity.js';
import {
  moveCommand, stopCommand, attackCommand, attackMoveCommand, forceAttackCommand,
  deployCommand, harvestCommand, buildCommand, cancelBuildCommand, placeCommand,
} from './sim/commands.js';
import { hitSidebarItem } from './render/sidebar.js';
import { BUILDING_TYPES } from './sim/data/buildings.js';
import { screenToWorld, screenToCell } from './render/camera.js';
import { pointInMinimap } from './render/minimap.js';

export function createInputState() {
  return {
    selection: new Set(), // entity ids
    commandQueue: [], // drained by main.js into gameTick
    drag: null, // {x0, y0, x1, y1} in screen px while left button held
    attackMoveArmed: false, // A pressed; next left-click is attack-move
    groups: new Map(), // digit -> array of entity ids
    placing: null, // {type, footprint} while positioning a ready building
    sidebarItems: [], // refreshed each frame by main.js
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

// Entity whose cell contains the world pixel, preferring player-owned.
export function pickEntityAt(store, worldPxX, worldPxY, owner = HouseType.GDI) {
  const cx = (worldPxX / TILE) | 0;
  const cy = (worldPxY / TILE) | 0;
  let fallback = null;
  for (const e of store.entities.values()) {
    if (e.kind === EntityKind.PROJECTILE) continue;
    if (e.x === cx && e.y === cy) {
      if (e.owner === owner) return e;
      fallback = fallback ?? e;
    }
  }
  return fallback;
}

// Player-owned mobile entities inside a world-pixel rectangle (any corner order).
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

export function wireInput(canvas, input, deps) {
  const { cam, store, minimap, world, centerCameraOn, minimapToWorldPx } = deps;

  const mousePos = (e) => {
    const r = canvas.getBoundingClientRect();
    return { x: e.clientX - r.left, y: e.clientY - r.top };
  };

  canvas.addEventListener('contextmenu', (e) => e.preventDefault());

  canvas.addEventListener('mousedown', (e) => {
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
            input.commandQueue.push(cancelBuildCommand(HouseType.GDI, item.category));
          } else {
            input.commandQueue.push(buildCommand(HouseType.GDI, item.category, item.type));
          }
        }
        return;
      }
      if (input.placing) {
        const cell = screenToCell(cam, p.x, p.y);
        input.commandQueue.push(placeCommand(HouseType.GDI, cell.x, cell.y));
        input.placing = null;
        return;
      }
      if (input.attackMoveArmed && input.selection.size > 0) {
        // A + click: attack-move to the clicked cell.
        input.attackMoveArmed = false;
        const cell = screenToCell(cam, p.x, p.y);
        input.commandQueue.push(attackMoveCommand([...input.selection], cell.x, cell.y));
        return;
      }
      if (e.ctrlKey && input.selection.size > 0) {
        // Ctrl + click: force-attack the ground.
        const cell = screenToCell(cam, p.x, p.y);
        input.commandQueue.push(forceAttackCommand([...input.selection], cell.x, cell.y));
        return;
      }
      input.drag = { x0: p.x, y0: p.y, x1: p.x, y1: p.y };
    } else if (e.button === 2 && input.placing) {
      input.placing = null; // right-click cancels placement mode
    } else if (e.button === 2 && input.selection.size > 0 && p.x < cam.viewW) {
      // Right-click: attack an enemy under the cursor, otherwise move there.
      const wp = screenToWorld(cam, p.x, p.y);
      const hit = pickEntityAt(store, wp.x, wp.y);
      if (hit && hit.owner !== HouseType.GDI) {
        input.commandQueue.push(attackCommand([...input.selection], hit.id));
      } else {
        const cell = screenToCell(cam, p.x, p.y);
        input.commandQueue.push(moveCommand([...input.selection], cell.x, cell.y));
      }
    }
  });

  canvas.addEventListener('mousemove', (e) => {
    if (input.drag) {
      const p = mousePos(e);
      input.drag.x1 = p.x;
      input.drag.y1 = p.y;
    }
  });

  window.addEventListener('mouseup', (e) => {
    if (e.button !== 0 || !input.drag) return;
    const d = input.drag;
    input.drag = null;
    const w = Math.abs(d.x1 - d.x0);
    const h = Math.abs(d.y1 - d.y0);
    if (w < DRAG_THRESHOLD && h < DRAG_THRESHOLD) {
      // Click select.
      const wp = screenToWorld(cam, d.x0, d.y0);
      const hit = pickEntityAt(store, wp.x, wp.y);
      input.selection.clear();
      if (hit && selectable(hit, HouseType.GDI)) input.selection.add(hit.id);
    } else {
      const a = screenToWorld(cam, d.x0, d.y0);
      const b = screenToWorld(cam, d.x1, d.y1);
      const hits = entitiesInRect(store, a.x, a.y, b.x, b.y);
      input.selection.clear();
      for (const id of hits) input.selection.add(id);
    }
  });

  window.addEventListener('keydown', (e) => {
    if (e.key === 's' && input.selection.size > 0) {
      input.commandQueue.push(stopCommand([...input.selection]));
      input.attackMoveArmed = false;
    } else if (e.key === 'd' && input.selection.size > 0) {
      input.commandQueue.push(deployCommand([...input.selection]));
    } else if (e.key === 'h' && input.selection.size > 0) {
      input.commandQueue.push(harvestCommand([...input.selection]));
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
        recallGroup(input, store, digit);
      }
    }
  });
}
