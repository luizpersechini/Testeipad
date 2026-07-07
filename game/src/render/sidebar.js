// Sidebar build menu: icon buttons for everything currently buildable, queue
// progress overlays, READY flashing, and item layout/hit-testing kept pure
// for node tests. Drawing needs a ctx.

import { HouseType } from '../sim/constants.js';
import { availableToBuild, CATEGORIES } from '../sim/production.js';
import { BUILDING_TYPES } from '../sim/data/buildings.js';
import { INFANTRY_TYPES } from '../sim/data/infantry.js';
import { UNIT_TYPES } from '../sim/data/units.js';

const TABLES = { buildings: BUILDING_TYPES, infantry: INFANTRY_TYPES, units: UNIT_TYPES };

const ICON_FOR = {
  power_plant: 'icon_pp', adv_power_plant: 'icon_pp', barracks: 'icon_bar',
  hand_of_nod: 'icon_bar', refinery: 'icon_ref', war_factory: 'icon_wf',
  silo: 'icon_silo', guard_tower: 'icon_gt', adv_guard_tower: 'icon_agt',
  gun_turret: 'icon_gt', obelisk: 'icon_agt', comm_center: 'icon_hpad',
  adv_comm_center: 'icon_hpad', temple: 'icon_rep',
};

export const ITEM_H = 34;
const ITEM_GAP = 3;

// Compute clickable build items. `top` is where the list starts (below the
// minimap/credits/power block).
export function layoutSidebarItems(game, owner, sbx, sbw, top) {
  const items = [];
  let y = top;
  const house = game.houses[owner];
  for (const category of CATEGORIES) {
    const avail = availableToBuild(game, owner, category);
    if (avail.length === 0) continue;
    items.push({ header: true, category, x: sbx + 6, y, w: sbw - 12, h: 14 });
    y += 17;
    for (const type of avail) {
      const q = house.queues?.[category];
      items.push({
        category,
        type,
        name: TABLES[category][type].name,
        cost: TABLES[category][type].cost,
        icon: ICON_FOR[type] ?? null,
        x: sbx + 6,
        y,
        w: sbw - 12,
        h: ITEM_H,
        queued: q?.type === type ? q : null,
        blocked: !!q && q.type !== type, // category busy with something else
      });
      y += ITEM_H + ITEM_GAP;
    }
    y += 4;
  }
  return items;
}

export function hitSidebarItem(items, px, py) {
  for (const it of items) {
    if (it.header) continue;
    if (px >= it.x && px < it.x + it.w && py >= it.y && py < it.y + it.h) return it;
  }
  return null;
}

const CATEGORY_LABEL = { buildings: 'STRUCTURES', infantry: 'INFANTRY', units: 'VEHICLES' };

export function drawSidebar(ctx, items, registry, tick) {
  ctx.font = '11px monospace';
  for (const it of items) {
    if (it.header) {
      ctx.fillStyle = '#777';
      ctx.fillText(CATEGORY_LABEL[it.category], it.x, it.y + 10);
      continue;
    }
    ctx.fillStyle = it.blocked ? '#242424' : '#333';
    ctx.fillRect(it.x, it.y, it.w, it.h);

    const icon = it.icon ? registry?.icons?.[it.icon] : null;
    if (icon) ctx.drawImage(icon, it.x + 2, it.y + 2, ITEM_H - 4, ITEM_H - 4);

    ctx.fillStyle = it.blocked ? '#666' : '#ddd';
    ctx.fillText(it.name, it.x + ITEM_H + 4, it.y + 14);
    ctx.fillStyle = '#8c8';
    ctx.fillText(`$${it.cost}`, it.x + ITEM_H + 4, it.y + 27);

    if (it.queued) {
      const q = it.queued;
      if (q.ready) {
        // Flash READY.
        if ((tick / 8 | 0) % 2 === 0) {
          ctx.fillStyle = 'rgba(80, 220, 80, 0.25)';
          ctx.fillRect(it.x, it.y, it.w, it.h);
        }
        ctx.fillStyle = '#6f6';
        ctx.fillText('READY', it.x + it.w - 44, it.y + 14);
      } else {
        // Progress sweep: darken the unbuilt portion.
        const frac = 1 - q.ticksLeft / q.totalTicks;
        ctx.fillStyle = 'rgba(0, 0, 0, 0.55)';
        ctx.fillRect(it.x + it.w * frac, it.y, it.w * (1 - frac), it.h);
        ctx.fillStyle = '#fc6';
        ctx.fillText(`${Math.round(frac * 100)}%`, it.x + it.w - 36, it.y + 14);
      }
    }
    ctx.strokeStyle = '#555';
    ctx.strokeRect(it.x + 0.5, it.y + 0.5, it.w - 1, it.h - 1);
  }
}

// Placement ghost over the map: green when legal, red when not.
export function drawPlacementGhost(ctx, cam, cell, footprint, legal, TILE) {
  const [fw, fh] = footprint;
  const sx = cell.x * TILE - cam.x;
  const sy = cell.y * TILE - cam.y;
  ctx.fillStyle = legal ? 'rgba(60, 220, 60, 0.35)' : 'rgba(220, 50, 50, 0.35)';
  ctx.fillRect(sx, sy, fw * TILE, fh * TILE);
  ctx.strokeStyle = legal ? '#4c4' : '#c44';
  ctx.strokeRect(sx + 0.5, sy + 0.5, fw * TILE - 1, fh * TILE - 1);
}
