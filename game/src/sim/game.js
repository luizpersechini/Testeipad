// Top-level sim: createGame(seed) and gameTick(game, commands).
// Everything in here must stay deterministic (iron rule 3).

import { createRng } from './rng.js';
import { createWorld } from './world.js';
import {
  createStore, get, relocate, EntityKind, facingToward,
} from './entity.js';
import { findPath, cellEnterable } from './path.js';
import { statsFor } from './stats.js';
import {
  tickCombat, orderAttack, orderAttackMove, orderForceAttack,
} from './combat.js';
import { SIM_FACINGS } from './constants.js';

export { statsFor };

// Movement tuning (integer math for determinism):
// a unit gains speed*25 progress per tick; an orthogonal step costs 1000,
// a diagonal step 1410. Medium tank (speed 5) = 8 ticks/cell ≈ 0.53s.
const STEP_ORTHO = 1000;
const STEP_DIAG = 1410;
const PROGRESS_PER_SPEED = 25;
const TURN_RATE = 4; // facings (of 32) per tick
const MAX_FACING_LAG = 4; // may drive while within this many facings of desired
const REPATH_COOLDOWN = 8; // ticks between re-path attempts when blocked
const MAX_REPATHS = 3;

export function createGame(seed) {
  return {
    seed,
    rng: createRng(seed),
    world: createWorld(seed),
    store: createStore(),
    tick: 0,
    events: [], // per-tick render events (shots, hits, deaths); cleared each tick
  };
}

function orderMove(game, id, tx, ty) {
  const e = get(game.store, id);
  if (!e) return;
  if (e.kind !== EntityKind.UNIT && e.kind !== EntityKind.INFANTRY) return;
  // An explicit move order cancels any combat behavior.
  e.attackTarget = null;
  e.attackGround = null;
  e.resumeDest = null;
  e.dest = { x: tx, y: ty };
  e.path = findPath(game.world, e.x, e.y, tx, ty, {
    infantry: e.kind === EntityKind.INFANTRY,
    moverId: id,
  });
  e.moveProgress = 0;
  e.repaths = 0;
  e.repathCooldown = 0;
  e.state = e.path && e.path.length > 0 ? 'moving' : 'idle';
}

function applyCommand(game, cmd) {
  switch (cmd.type) {
    case 'move':
      for (const id of cmd.ids) orderMove(game, id, cmd.x, cmd.y);
      break;
    case 'stop':
      for (const id of cmd.ids) {
        const e = get(game.store, id);
        if (e) {
          e.path = null;
          e.dest = null;
          e.attackTarget = null;
          e.attackGround = null;
          e.resumeDest = null;
          e.state = 'idle';
        }
      }
      break;
    case 'attack':
      for (const id of cmd.ids) {
        const e = get(game.store, id);
        if (e) e.resumeDest = null; // player order overrides attack-move resume
        orderAttack(game, id, cmd.targetId);
      }
      break;
    case 'attackmove':
      for (const id of cmd.ids) orderAttackMove(game, id, cmd.x, cmd.y);
      break;
    case 'forceattack':
      for (const id of cmd.ids) orderForceAttack(game, id, cmd.x, cmd.y);
      break;
    default:
      break;
  }
}

// Rotate entity.facing toward `desired` by at most TURN_RATE, shortest way.
function turnToward(e, desired) {
  let diff = (desired - e.facing + SIM_FACINGS) % SIM_FACINGS;
  if (diff === 0) return 0;
  if (diff > SIM_FACINGS / 2) diff -= SIM_FACINGS; // negative = turn left
  const step = Math.max(-TURN_RATE, Math.min(TURN_RATE, diff));
  e.facing = (e.facing + step + SIM_FACINGS) % SIM_FACINGS;
  diff = (desired - e.facing + SIM_FACINGS) % SIM_FACINGS;
  return Math.min(diff, SIM_FACINGS - diff);
}

const MOBILE_STATES = new Set(['moving', 'attacking', 'attackmove']);

function tickMovement(game, e) {
  // Moves explicit orders, chasing attackers, and attack-movers alike.
  if (!MOBILE_STATES.has(e.state)) return;
  if (!e.path || e.path.length === 0) return;

  const next = e.path[0];
  const desired = facingToward(e.x, e.y, next.x, next.y);
  const lag = turnToward(e, desired);
  if (lag > MAX_FACING_LAG) return; // still turning in place

  const infantry = e.kind === EntityKind.INFANTRY;
  if (!cellEnterable(game.world, next.x, next.y, { infantry, moverId: e.id })) {
    // Blocked: periodically try a fresh path; give up after MAX_REPATHS.
    if (e.repathCooldown > 0) {
      e.repathCooldown--;
      return;
    }
    if (e.repaths >= MAX_REPATHS || !e.dest) {
      e.path = null;
      if (e.state === 'moving') e.state = 'idle';
      return;
    }
    e.repaths++;
    e.repathCooldown = REPATH_COOLDOWN;
    const fresh = findPath(game.world, e.x, e.y, e.dest.x, e.dest.y, { infantry, moverId: e.id });
    if (!fresh || fresh.length === 0) {
      e.path = null;
      if (e.state === 'moving') e.state = 'idle';
    } else {
      e.path = fresh;
      e.moveProgress = 0;
    }
    return;
  }

  const stats = statsFor(e);
  const diagonal = next.x !== e.x && next.y !== e.y;
  const stepCost = diagonal ? STEP_DIAG : STEP_ORTHO;
  e.moveProgress = (e.moveProgress ?? 0) + stats.speed * PROGRESS_PER_SPEED;

  // Fractional offset for smooth rendering (read-only for the renderer).
  const f = Math.min(1, e.moveProgress / stepCost);
  e.subX = (next.x - e.x) * f;
  e.subY = (next.y - e.y) * f;

  if (e.moveProgress >= stepCost) {
    if (relocate(game.store, game.world, e.id, next.x, next.y)) {
      e.path.shift();
      e.moveProgress = 0;
      e.subX = 0;
      e.subY = 0;
      e.repaths = 0;
      if (e.path.length === 0) {
        e.path = null;
        e.dest = null;
        if (e.state === 'moving') e.state = 'idle';
      }
    } else {
      // Someone claimed the cell in the same tick; treat as blocked next tick.
      e.moveProgress = stepCost;
      e.subX = 0;
      e.subY = 0;
      e.repathCooldown = 0;
    }
  }
}

export function gameTick(game, commands = []) {
  game.events = [];
  for (const cmd of commands) applyCommand(game, cmd);
  for (const e of game.store.entities.values()) {
    tickMovement(game, e);
    tickCombat(game, e);
  }
  game.tick++;
}
