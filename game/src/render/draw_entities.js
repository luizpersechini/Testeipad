// Entity rendering: sprites by facing/walk-frame, selection brackets, health
// bars. Read-only over sim state.

import { TILE, HouseType } from '../sim/constants.js';
import { EntityKind, facingTo8 } from '../sim/entity.js';
import { SHEET_DEFS, vehicleFrame, infantryFrame, buildingFrame } from './assets.js';
import { statsFor } from '../sim/stats.js';
import { entityVisibleTo } from '../sim/fog.js';

const FACTION_PREFIX = {
  [HouseType.GDI]: 'gdi',
  [HouseType.NOD]: 'nod',
};
const FACTION_COLOR = {
  [HouseType.GDI]: '#d4b45a',
  [HouseType.NOD]: '#b03030',
  [HouseType.NEUTRAL]: '#909090',
};

function screenPos(e, cam) {
  return {
    x: (e.x + e.subX) * TILE - cam.x,
    y: (e.y + e.subY) * TILE - cam.y,
  };
}

function drawHealthBar(ctx, sx, sy, w, e) {
  const frac = Math.max(0, e.hp / e.maxHp);
  const color = frac > 0.66 ? '#3ecc3e' : frac > 0.33 ? '#e0c030' : '#d03030';
  ctx.fillStyle = 'rgba(0,0,0,0.7)';
  ctx.fillRect(sx, sy - 6, w, 4);
  ctx.fillStyle = color;
  ctx.fillRect(sx + 1, sy - 5, Math.round((w - 2) * frac), 2);
}

function drawSelectionBrackets(ctx, sx, sy, size) {
  const c = 5; // corner arm length
  ctx.strokeStyle = '#fff';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(sx, sy + c); ctx.lineTo(sx, sy); ctx.lineTo(sx + c, sy);
  ctx.moveTo(sx + size - c, sy); ctx.lineTo(sx + size, sy); ctx.lineTo(sx + size, sy + c);
  ctx.moveTo(sx + size, sy + size - c); ctx.lineTo(sx + size, sy + size); ctx.lineTo(sx + size - c, sy + size);
  ctx.moveTo(sx + c, sy + size); ctx.lineTo(sx, sy + size); ctx.lineTo(sx, sy + size - c);
  ctx.stroke();
}

// Walk frame cycles as the unit accumulates movement progress.
function walkFrameOf(e, tick) {
  if (e.state !== 'moving') return 0;
  return ((tick / 3) | 0) % 3;
}

// game/viewer: pass the sim game and the viewing house to hide what the fog
// hides (omit them to draw everything, e.g. in tests).
export function drawEntities(ctx, store, cam, registry, selection, tick, game = null, viewer = null) {
  for (const e of store.entities.values()) {
    if (e.kind === EntityKind.PROJECTILE) continue;
    if (game && viewer !== null && !entityVisibleTo(game, viewer, e)) continue;
    const { x: sx, y: sy } = screenPos(e, cam);
    if (sx < -TILE * 2 || sy < -TILE * 2 || sx > cam.viewW + TILE || sy > cam.viewH + TILE) continue;

    // Buildings: prefer a dedicated per-building sheet (original assets),
    // else the placeholder buildings grid — scaled to the footprint.
    if (e.kind === EntityKind.BUILDING) {
      const [fw, fh] = e.footprint;
      const w = fw * TILE;
      const h = fh * TILE;
      const spriteName = statsFor(e)?.sprite;
      const nodRow = e.owner === HouseType.NOD ? 1 : 0;
      const pair = spriteName ? registry?.sheets?.[`building_${e.type}`]
        ?? registry?.sheets?.[`building_${spriteName}`] : null;
      let f = null;
      let img = null;
      if (pair?.img) {
        const d = pair.def;
        f = { sx: 0, sy: nodRow * d.frameH, sw: d.frameW, sh: d.frameH };
        img = pair.img;
      } else {
        const bSheet = registry?.sheets?.buildings;
        if (bSheet?.img && spriteName) {
          f = buildingFrame(bSheet.def, nodRow, spriteName);
          img = bSheet.img;
        }
      }
      if (f && img) {
        ctx.drawImage(img, f.sx, f.sy, f.sw, f.sh, sx, sy, w, h);
      } else {
        ctx.fillStyle = FACTION_COLOR[e.owner] ?? '#f0f';
        ctx.fillRect(sx + 2, sy + 2, w - 4, h - 4);
        ctx.strokeStyle = '#111';
        ctx.strokeRect(sx + 2.5, sy + 2.5, w - 5, h - 5);
      }
      const selected = selection?.has(e.id);
      if (selected) drawSelectionBrackets(ctx, sx, sy, Math.max(w, h));
      if (selected || e.hp < e.maxHp) drawHealthBar(ctx, sx + 2, sy, w - 4, e);
      continue;
    }

    const prefix = FACTION_PREFIX[e.owner];
    const sheetName = prefix ? `${prefix}_${e.sprite ?? e.type}` : null;
    const sheet = sheetName ? registry?.sheets?.[sheetName] : null;
    let drawn = false;

    if (sheet?.img) {
      // Use the loaded sheet's own geometry so original-asset overrides with
      // different frame sizes render correctly.
      const def = sheet.def ?? SHEET_DEFS[sheetName];
      let f = null;
      if (e.kind === EntityKind.UNIT) {
        f = vehicleFrame(def, facingTo8(e.facing));
      } else if (e.kind === EntityKind.INFANTRY) {
        f = infantryFrame(def, facingTo8(e.facing), walkFrameOf(e, tick));
      }
      if (f) {
        // Center the frame on the cell.
        const dx = sx + (TILE - f.sw) / 2;
        const dy = sy + (TILE - f.sh) / 2;
        ctx.drawImage(sheet.img, f.sx, f.sy, f.sw, f.sh, dx, dy, f.sw, f.sh);
        drawn = true;
      }
    }

    if (!drawn) {
      // Fallback: faction-colored shape with a facing tick mark.
      const inset = e.kind === EntityKind.INFANTRY ? 9 : 5;
      ctx.fillStyle = FACTION_COLOR[e.owner] ?? '#f0f';
      ctx.fillRect(sx + inset, sy + inset, TILE - inset * 2, TILE - inset * 2);
      const angle = (e.facing / 32) * Math.PI * 2;
      const cx = sx + TILE / 2;
      const cy = sy + TILE / 2;
      ctx.strokeStyle = '#111';
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.lineTo(cx + Math.sin(angle) * (TILE / 2 - inset), cy - Math.cos(angle) * (TILE / 2 - inset));
      ctx.stroke();
    }

    const selected = selection?.has(e.id);
    if (selected) drawSelectionBrackets(ctx, sx, sy, TILE);
    if (selected || e.hp < e.maxHp) drawHealthBar(ctx, sx + 2, sy, TILE - 4, e);
  }
}

export function drawDragBox(ctx, drag) {
  if (!drag) return;
  ctx.strokeStyle = 'rgba(120, 255, 120, 0.9)';
  ctx.lineWidth = 1;
  ctx.strokeRect(
    Math.min(drag.x0, drag.x1) + 0.5,
    Math.min(drag.y0, drag.y1) + 0.5,
    Math.abs(drag.x1 - drag.x0),
    Math.abs(drag.y1 - drag.y0),
  );
}
