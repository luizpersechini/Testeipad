// Combat: attack orders, guard-range target acquisition, projectiles, damage
// application (via the original warhead math), death and kill credit.

import { EntityKind, get, spawn, despawn, facingToward } from './entity.js';
import { findPath } from './path.js';
import { weaponData, modifyDamage } from './data/weapons.js';
import { statsFor } from './stats.js';
import { NO_ENTITY } from './world.js';

const GUARD_SCAN_INTERVAL = 8; // ticks between idle target scans
const CHASE_REPATH_DIST = 2; // re-path when target strays this far from last goal

export function weaponOf(e) {
  const stats = statsFor(e);
  return stats?.weapon ? weaponData(stats.weapon) : null;
}

function centerDist(a, b) {
  const dx = (a.x + a.subX) - (b.x + b.subX);
  const dy = (a.y + a.subY) - (b.y + b.subY);
  return Math.sqrt(dx * dx + dy * dy);
}

function isCombatant(e) {
  return e.kind === EntityKind.UNIT || e.kind === EntityKind.INFANTRY
    || e.kind === EntityKind.BUILDING;
}

// Nearest live enemy within `range` cells; ties break on lower id (determinism).
export function acquireTarget(game, e, range) {
  let best = null;
  let bestDist = Infinity;
  for (const other of game.store.entities.values()) {
    if (other.owner === e.owner || !isCombatant(other) || other.hp <= 0) continue;
    const d = centerDist(e, other);
    if (d <= range && (d < bestDist - 1e-9 || (Math.abs(d - bestDist) <= 1e-9 && other.id < (best?.id ?? Infinity)))) {
      best = other;
      bestDist = d;
    }
  }
  return best;
}

export function orderAttack(game, id, targetId) {
  const e = get(game.store, id);
  const target = get(game.store, targetId);
  if (!e || !target || !weaponOf(e)) return;
  e.attackTarget = targetId;
  e.state = 'attacking';
  e.chaseGoal = null;
}

function applyDamage(game, target, weaponId, rawDamage, attackerId) {
  const stats = statsFor(target);
  const dealt = modifyDamage(rawDamage, weaponId, stats?.armor ?? 'steel');
  target.hp -= dealt;
  target.lastAttacker = attackerId; // enables return fire beyond sight range
  game.events.push({ type: 'hit', x: target.x, y: target.y, targetId: target.id });
  if (target.hp <= 0) {
    game.events.push({
      type: 'death', x: target.x, y: target.y, kind: target.kind, entityType: target.type,
    });
    const attacker = get(game.store, attackerId);
    if (attacker) attacker.kills = (attacker.kills ?? 0) + 1;
    despawn(game.store, game.world, target.id);
  }
}

function fireAt(game, e, target, weapon) {
  const stats = statsFor(e);
  e.reload = weapon.rof;
  game.events.push({ type: 'shot', x: e.x, y: e.y, facing: e.facing, weapon: stats.weapon });
  const pid = spawn(game.store, game.world, {
    kind: EntityKind.PROJECTILE, type: weapon.projectile, owner: e.owner,
    x: e.x, y: e.y, hp: 1,
  });
  const p = get(game.store, pid);
  p.subX = e.subX;
  p.subY = e.subY;
  p.weaponId = stats.weapon;
  p.targetId = target.id;
  p.firedBy = e.id;
}

function tickProjectile(game, p) {
  const target = get(game.store, p.targetId);
  if (!target || target.hp <= 0) {
    despawn(game.store, game.world, p.id);
    return;
  }
  const weapon = weaponData(p.weaponId);
  const step = weapon.speed / 100; // cells per tick
  const dx = (target.x + target.subX) - (p.x + p.subX);
  const dy = (target.y + target.subY) - (p.y + p.subY);
  const dist = Math.sqrt(dx * dx + dy * dy);
  if (dist <= step) {
    applyDamage(game, target, p.weaponId, weapon.damage, p.firedBy);
    despawn(game.store, game.world, p.id);
    return;
  }
  const nx = p.x + p.subX + (dx / dist) * step;
  const ny = p.y + p.subY + (dy / dist) * step;
  p.x = Math.round(nx);
  p.y = Math.round(ny);
  p.subX = nx - p.x;
  p.subY = ny - p.y;
  p.facing = facingToward(0, 0, dx, dy);
}

// Per-entity combat logic; runs after movement in gameTick.
export function tickCombat(game, e) {
  if (e.kind === EntityKind.PROJECTILE) {
    tickProjectile(game, e);
    return;
  }
  if (e.hp <= 0) return;
  const weapon = weaponOf(e);
  if (!weapon) return;
  if (e.reload > 0) e.reload--;

  // Return fire: when hit, an idle armed unit fights back even if the
  // attacker sits beyond sight range (original guard-mission behavior).
  if (e.state === 'idle' && e.lastAttacker) {
    const aggressor = get(game.store, e.lastAttacker);
    e.lastAttacker = null;
    if (aggressor && aggressor.hp > 0 && aggressor.owner !== e.owner) {
      orderAttack(game, e.id, aggressor.id);
      return;
    }
  }

  // Guard: idle armed entities acquire targets within sight range.
  if (e.state === 'idle' && (game.tick + e.id) % GUARD_SCAN_INTERVAL === 0) {
    const stats = statsFor(e);
    const target = acquireTarget(game, e, stats.sight);
    if (target) orderAttack(game, e.id, target.id);
    return;
  }

  if (e.state !== 'attacking') return;
  const target = get(game.store, e.attackTarget);
  if (!target || target.hp <= 0) {
    e.attackTarget = NO_ENTITY;
    e.chaseGoal = null;
    e.path = null;
    e.state = 'idle';
    return;
  }

  const dist = centerDist(e, target);
  if (dist <= weapon.range) {
    // In range: hold position, face target, shoot on reload.
    e.path = null;
    const desired = facingToward(e.x, e.y, target.x, target.y);
    e.facing = desired; // turret snap; gradual turret turn is a polish item
    if (e.reload <= 0) fireAt(game, e, target, weapon);
  } else if (e.kind !== EntityKind.BUILDING) {
    // Chase: path toward the target, refreshing when it strays.
    const stale = !e.chaseGoal
      || Math.max(Math.abs(e.chaseGoal.x - target.x), Math.abs(e.chaseGoal.y - target.y)) >= CHASE_REPATH_DIST
      || !e.path || e.path.length === 0;
    if (stale) {
      e.chaseGoal = { x: target.x, y: target.y };
      e.dest = { x: target.x, y: target.y };
      e.path = findPath(game.world, e.x, e.y, target.x, target.y, {
        infantry: e.kind === EntityKind.INFANTRY, moverId: e.id,
      });
      e.moveProgress = 0;
      e.repaths = 0;
      e.repathCooldown = 0;
    }
  }
}
