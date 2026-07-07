// Game-feel feedback: order markers, screen shake, and "base under attack"
// minimap pings. All planning logic is pure (node-tested); drawing needs ctx.

import { TILE } from '../sim/constants.js';
import { EntityKind, get } from '../sim/entity.js';

export const MARKER_TTL = 12; // ticks
export const PING_TTL = 60;
export const PING_THROTTLE = 150; // min ticks between "under attack" alerts

export function createFeedback() {
  return {
    markers: [], // {x, y, kind: 'move'|'attack', start}
    pings: [], // {x, y, start}
    shake: 0, // px magnitude, decays per frame
    lastAttackAlert: -Infinity,
  };
}

// Order markers from the commands the player just issued.
export function markersFromCommands(fb, commands, tick) {
  for (const cmd of commands) {
    if (cmd.type === 'move' || cmd.type === 'attackmove') {
      fb.markers.push({ x: cmd.x, y: cmd.y, kind: cmd.type === 'move' ? 'move' : 'attack', start: tick });
    } else if (cmd.type === 'forceattack') {
      fb.markers.push({ x: cmd.x, y: cmd.y, kind: 'attack', start: tick });
    }
  }
}

// Shake on big deaths, pings + alert when the player's buildings take hits.
// Returns true when a fresh "base under attack" alert should fire.
export function feedbackFromEvents(fb, events, game, player, tick) {
  let alert = false;
  for (const ev of events) {
    if (ev.type === 'death') {
      fb.shake = Math.min(8, fb.shake + (ev.kind === EntityKind.BUILDING ? 6 : 2));
    } else if (ev.type === 'hit' && ev.targetId) {
      const target = get(game.store, ev.targetId);
      if (target && target.owner === player && target.kind === EntityKind.BUILDING) {
        fb.pings.push({ x: ev.x, y: ev.y, start: tick });
        if (tick - fb.lastAttackAlert >= PING_THROTTLE) {
          fb.lastAttackAlert = tick;
          alert = true;
        }
      }
    }
  }
  return alert;
}

export function pruneFeedback(fb, tick) {
  fb.markers = fb.markers.filter((m) => tick - m.start < MARKER_TTL);
  fb.pings = fb.pings.filter((p) => tick - p.start < PING_TTL);
}

// Frame-rate independent-ish decay; called once per rendered frame.
export function decayShake(fb) {
  fb.shake *= 0.85;
  if (fb.shake < 0.3) fb.shake = 0;
}

export function drawMarkers(ctx, fb, cam, tick) {
  for (const m of fb.markers) {
    const age = tick - m.start;
    const r = 4 + age * 1.2;
    const alpha = 1 - age / MARKER_TTL;
    const sx = (m.x + 0.5) * TILE - cam.x;
    const sy = (m.y + 0.5) * TILE - cam.y;
    ctx.strokeStyle = m.kind === 'move'
      ? `rgba(90, 255, 90, ${alpha})`
      : `rgba(255, 80, 60, ${alpha})`;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.arc(sx, sy, r, 0, Math.PI * 2);
    ctx.stroke();
    ctx.lineWidth = 1;
  }
}

export function drawPings(ctx, fb, minimap, world, tick) {
  for (const p of fb.pings) {
    if (((tick - p.start) / 8 | 0) % 2 === 1) continue; // blink
    const mx = minimap.x + (p.x / world.w) * minimap.w;
    const my = minimap.y + (p.y / world.h) * minimap.h;
    ctx.strokeStyle = '#ff4030';
    ctx.strokeRect(mx - 3, my - 3, 6, 6);
  }
}
