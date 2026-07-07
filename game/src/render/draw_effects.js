// Visual effects: explosions, muzzle flashes, hit sparks, and projectile
// rendering. Effect bookkeeping is pure (node-tested); only draw* touch a ctx.

import { TILE } from '../sim/constants.js';
import { EntityKind } from '../sim/entity.js';
import { SHEET_DEFS, effectFrame } from './assets.js';

// Effect timings in sim ticks.
const EXPLOSION_FRAMES = 6;
const EXPLOSION_TICKS_PER_FRAME = 2;
const MUZZLE_TICKS = 3;
const HIT_TICKS = 4;

export function createEffects() {
  return { list: [] };
}

// Turn this tick's sim events into animated effects.
export function spawnFromEvents(effects, events, tick) {
  for (const ev of events) {
    if (ev.type === 'death') {
      effects.list.push({
        kind: 'explosion', x: ev.x, y: ev.y, start: tick,
        ttl: EXPLOSION_FRAMES * EXPLOSION_TICKS_PER_FRAME,
      });
    } else if (ev.type === 'shot') {
      effects.list.push({ kind: 'muzzle', x: ev.x, y: ev.y, facing: ev.facing, start: tick, ttl: MUZZLE_TICKS });
    } else if (ev.type === 'hit') {
      effects.list.push({ kind: 'hit', x: ev.x, y: ev.y, start: tick, ttl: HIT_TICKS });
    }
  }
}

// Drop expired effects; call once per sim tick.
export function pruneEffects(effects, tick) {
  effects.list = effects.list.filter((fx) => tick - fx.start < fx.ttl);
}

export function explosionFrameAt(fx, tick) {
  return Math.min(EXPLOSION_FRAMES - 1, ((tick - fx.start) / EXPLOSION_TICKS_PER_FRAME) | 0);
}

const PROJECTILE_COLOR = {
  bullet: '#ffe080',
  shell: '#ffcc60',
  missile: '#ff9040',
  grenade: '#c0c080',
  flame: '#ff6020',
  laser: '#ff3030',
};

export function drawProjectiles(ctx, store, cam) {
  for (const e of store.entities.values()) {
    if (e.kind !== EntityKind.PROJECTILE) continue;
    const sx = (e.x + e.subX + 0.5) * TILE - cam.x;
    const sy = (e.y + e.subY + 0.5) * TILE - cam.y;
    if (sx < -8 || sy < -8 || sx > cam.viewW + 8 || sy > cam.viewH + 8) continue;
    ctx.fillStyle = PROJECTILE_COLOR[e.type] ?? '#fff';
    const r = e.type === 'missile' || e.type === 'shell' ? 3 : 2;
    ctx.beginPath();
    ctx.arc(sx, sy, r, 0, Math.PI * 2);
    ctx.fill();
  }
}

export function drawEffects(ctx, effects, cam, registry, tick) {
  const sheet = registry?.sheets?.effects;
  const def = SHEET_DEFS.effects;
  for (const fx of effects.list) {
    const sx = fx.x * TILE - cam.x;
    const sy = fx.y * TILE - cam.y;
    if (sx < -TILE || sy < -TILE || sx > cam.viewW + TILE || sy > cam.viewH + TILE) continue;

    if (fx.kind === 'explosion') {
      const frame = explosionFrameAt(fx, tick);
      const f = effectFrame(def, 'explosion', frame);
      if (sheet?.img && f) {
        ctx.drawImage(sheet.img, f.sx, f.sy, f.sw, f.sh, sx, sy, TILE, TILE);
      } else {
        const r = 4 + frame * 3;
        ctx.fillStyle = `rgba(255, ${160 - frame * 20}, 40, ${1 - frame / EXPLOSION_FRAMES})`;
        ctx.beginPath();
        ctx.arc(sx + TILE / 2, sy + TILE / 2, r, 0, Math.PI * 2);
        ctx.fill();
      }
    } else if (fx.kind === 'muzzle') {
      const angle = (fx.facing / 32) * Math.PI * 2;
      const mx = sx + TILE / 2 + Math.sin(angle) * (TILE / 2);
      const my = sy + TILE / 2 - Math.cos(angle) * (TILE / 2);
      ctx.fillStyle = 'rgba(255, 240, 160, 0.9)';
      ctx.beginPath();
      ctx.arc(mx, my, 4, 0, Math.PI * 2);
      ctx.fill();
    } else if (fx.kind === 'hit') {
      ctx.fillStyle = 'rgba(255, 200, 80, 0.8)';
      ctx.fillRect(sx + TILE / 2 - 3, sy + TILE / 2 - 3, 6, 6);
    }
  }
}
