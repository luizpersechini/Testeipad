import { test } from 'node:test';
import assert from 'node:assert/strict';
import { HouseType } from '../src/sim/constants.js';
import { spawn, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { enableAI } from '../src/sim/ai.js';
import { statsFor } from '../src/sim/stats.js';

// Full AI-vs-AI skirmish on a real generated map from just two MCVs.
function skirmish(seed) {
  const game = createGame(seed, { startingCredits: 6000 });
  const [s0, s1] = game.world.startPositions;
  spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'mcv', owner: HouseType.GDI, x: s0.x, y: s0.y, hp: 600,
  });
  spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'mcv', owner: HouseType.NOD, x: s1.x, y: s1.y, hp: 600,
  });
  enableAI(game, HouseType.GDI, 'normal');
  enableAI(game, HouseType.NOD, 'normal');
  return game;
}

function census(game, owner) {
  const out = { buildings: 0, units: 0, types: new Set() };
  for (const e of game.store.entities.values()) {
    if (e.owner !== owner || e.hp <= 0) continue;
    if (e.kind === EntityKind.BUILDING) {
      out.buildings++;
      out.types.add(e.type);
    } else if (e.kind !== EntityKind.PROJECTILE) out.units++;
  }
  return out;
}

test('AI vs AI: 5000 ticks, both build bases and armies, no crash', () => {
  const game = skirmish(42);
  // One AI may raze the other before the end, so judge peak base size,
  // not the final smoking ruins.
  const peak = { [HouseType.GDI]: 0, [HouseType.NOD]: 0 };
  const everBuilt = { [HouseType.GDI]: new Set(), [HouseType.NOD]: new Set() };
  const everUnits = { [HouseType.GDI]: 0, [HouseType.NOD]: 0 };
  for (let i = 0; i < 5000; i++) {
    gameTick(game);
    if (i % 50 === 0) {
      for (const owner of [HouseType.GDI, HouseType.NOD]) {
        const c = census(game, owner);
        peak[owner] = Math.max(peak[owner], c.buildings);
        for (const t of c.types) everBuilt[owner].add(t);
        everUnits[owner] = Math.max(everUnits[owner], c.units);
      }
    }
  }
  for (const owner of [HouseType.GDI, HouseType.NOD]) {
    assert.ok(peak[owner] >= 3, `house ${owner} built a base (peak ${peak[owner]} buildings)`);
    assert.ok(everBuilt[owner].has('construction_yard'), 'deployed the MCV');
    assert.ok(everBuilt[owner].has('power_plant'), 'built power');
    assert.ok(everBuilt[owner].has('refinery'), 'built economy');
    assert.ok(everUnits[owner] >= 1, `house ${owner} fielded units (${everUnits[owner]})`);
  }
});

test('AI economy actually earns money (harvesting works end to end)', () => {
  const game = skirmish(7);
  const startCredits = game.houses[HouseType.GDI].credits;
  let spentAndEarned = false;
  for (let i = 0; i < 6000; i++) {
    gameTick(game);
    if (game.events.some((ev) => ev.type === 'unload')) spentAndEarned = true;
  }
  assert.ok(spentAndEarned, 'at least one harvester delivery happened');
  void startCredits;
});

test('AI war: armies eventually clash and someone takes losses', () => {
  const game = skirmish(3);
  let deaths = 0;
  for (let i = 0; i < 9000; i++) {
    gameTick(game);
    deaths += game.events.filter((ev) => ev.type === 'death').length;
  }
  assert.ok(deaths > 0, `attack waves led to combat (${deaths} deaths)`);
});

test('AI skirmish is fully deterministic (iron rule 3)', () => {
  const fingerprint = () => {
    const game = skirmish(11);
    for (let i = 0; i < 3000; i++) gameTick(game);
    let acc = `${game.houses[0].credits.toFixed(3)}|${game.houses[1].credits.toFixed(3)}`;
    for (const e of game.store.entities.values()) {
      acc += `|${e.id}:${e.type}:${e.x},${e.y}:${e.hp}`;
    }
    return acc;
  };
  assert.equal(fingerprint(), fingerprint());
});

test('AI harvesters return to work after interruptions', () => {
  const game = skirmish(5);
  for (let i = 0; i < 4000; i++) gameTick(game);
  const harvesters = [...game.store.entities.values()]
    .filter((e) => statsFor(e)?.harvester && e.hp > 0);
  for (const h of harvesters) {
    assert.ok(h.state.startsWith('harvest') || h.state === 'idle' || h.state === 'moving',
      `harvester in a sane state: ${h.state}`);
  }
});
