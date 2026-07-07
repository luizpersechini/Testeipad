// Victory & defeat: a house is in the game once it owns a production
// structure (or an MCV that could deploy one); it is defeated when it has
// neither. Last house standing wins. Per-house stats feed the score screen.

import { EntityKind } from './entity.js';

const PRODUCTION_TYPES = new Set([
  'construction_yard', 'barracks', 'hand_of_nod', 'war_factory',
]);
const CHECK_INTERVAL = 15;

export function createStats() {
  return {
    unitsBuilt: 0,
    unitsLost: 0,
    buildingsBuilt: 0,
    buildingsLost: 0,
    creditsHarvested: 0,
    kills: 0,
  };
}

function canStillFight(game, owner) {
  for (const e of game.store.entities.values()) {
    if (e.owner !== owner || e.hp <= 0) continue;
    if (e.kind === EntityKind.BUILDING && PRODUCTION_TYPES.has(e.type)) return true;
    if (e.type === 'mcv') return true;
  }
  return false;
}

export function tickVictory(game) {
  if (game.winner !== null || game.tick % CHECK_INTERVAL !== 0) return;

  // A house joins the fight the first time it can produce.
  for (let owner = 0; owner < game.houses.length - 1; owner++) {
    if (!game.participants.has(owner) && canStillFight(game, owner)) {
      game.participants.add(owner);
    }
  }
  if (game.participants.size < 2) return; // not a match yet

  const alive = [...game.participants].filter((owner) => canStillFight(game, owner));
  if (alive.length === 1) {
    game.winner = alive[0];
    game.events.push({ type: 'victory', winner: game.winner });
  } else if (alive.length === 0) {
    game.winner = -1; // mutual destruction: a draw
    game.events.push({ type: 'victory', winner: -1 });
  }
}

// Stats hooks, called from combat/production/economy.
export function noteDeath(game, victim) {
  const stats = game.houses[victim.owner]?.stats;
  if (!stats) return;
  if (victim.kind === EntityKind.BUILDING) stats.buildingsLost++;
  else stats.unitsLost++;
}

export function noteKill(game, attackerOwner) {
  const stats = game.houses[attackerOwner]?.stats;
  if (stats) stats.kills++;
}

export function noteUnitBuilt(game, owner) {
  const stats = game.houses[owner]?.stats;
  if (stats) stats.unitsBuilt++;
}

export function noteBuildingBuilt(game, owner) {
  const stats = game.houses[owner]?.stats;
  if (stats) stats.buildingsBuilt++;
}

export function noteHarvest(game, owner, credits) {
  const stats = game.houses[owner]?.stats;
  if (stats) stats.creditsHarvested += credits;
}
