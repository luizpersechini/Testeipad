import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { spawn, get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import {
  attackMoveCommand, forceAttackCommand, moveCommand,
} from '../src/sim/commands.js';
import { createInputState, assignGroup, recallGroup } from '../src/input.js';
import { createStore, despawn } from '../src/sim/entity.js';

function arena() {
  const game = createGame(1);
  game.world.terrain.fill(TerrainType.CLEAR);
  return game;
}

const mkTank = (game, x, y, owner, type = 'medium_tank', hp = 400) => spawn(
  game.store, game.world, { kind: EntityKind.UNIT, type, owner, x, y, hp, facing: 8 },
);

function run(game, n) {
  for (let i = 0; i < n; i++) gameTick(game);
}

test('attack-move engages an enemy on the way, then resumes to destination', () => {
  const game = arena();
  const tank = mkTank(game, 2, 5, HouseType.GDI);
  const victim = spawn(game.store, game.world, {
    kind: EntityKind.INFANTRY, type: 'minigunner', owner: HouseType.NOD, x: 10, y: 5, hp: 50,
  });
  gameTick(game, [attackMoveCommand([tank], 20, 5)]);
  // Engaged somewhere along the way.
  let engaged = false;
  for (let i = 0; i < 600; i++) {
    gameTick(game);
    const e = get(game.store, tank);
    if (e.state === 'attacking' && e.attackTarget === victim) engaged = true;
    if (!get(game.store, victim)) break;
  }
  assert.ok(engaged, 'engaged the blocking infantry');
  assert.equal(get(game.store, victim), undefined, 'enemy destroyed');
  run(game, 400);
  const e = get(game.store, tank);
  assert.equal(e.x, 20, 'resumed and reached the attack-move destination');
  assert.equal(e.state, 'idle');
});

test('plain move ignores enemies; attack-move is the aggressive one', () => {
  const game = arena();
  const tank = mkTank(game, 2, 5, HouseType.GDI);
  spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'artillery', owner: HouseType.NOD, x: 10, y: 9, hp: 100, facing: 0,
  });
  gameTick(game, [moveCommand([tank], 20, 5)]);
  let arrived = false;
  let engagedBeforeArrival = false;
  for (let i = 0; i < 300 && !arrived; i++) {
    gameTick(game);
    const e = get(game.store, tank);
    if (e.x === 20 && e.y === 5) arrived = true;
    else if (e.state === 'attacking') engagedBeforeArrival = true;
  }
  assert.ok(arrived, 'drove straight to the destination');
  assert.equal(engagedBeforeArrival, false, 'never stopped to fight mid-move');
  // (After arriving it may legitimately return fire at its harasser.)
});

test('force-attack ground fires and splash-damages a cluster', () => {
  const game = arena();
  // Artillery (HE, spread 6 -> 1.5 cell splash) shells a spot between two infantry.
  const arty = mkTank(game, 2, 5, HouseType.GDI, 'artillery', 100);
  const a = spawn(game.store, game.world, {
    kind: EntityKind.INFANTRY, type: 'minigunner', owner: HouseType.NOD, x: 7, y: 5, hp: 50,
  });
  const b = spawn(game.store, game.world, {
    kind: EntityKind.INFANTRY, type: 'minigunner', owner: HouseType.NOD, x: 8, y: 5, hp: 50,
  });
  gameTick(game, [forceAttackCommand([arty], 7, 5)]);
  run(game, 200);
  // HE 150 vs none = 131 on the impact cell: a dies to the first shell.
  assert.equal(get(game.store, a), undefined, 'infantry on the impact cell vaporized');
  // The neighbor catches attenuated splash — then (correctly) returns fire
  // and leaves the zone, so it may survive wounded rather than die in place.
  const eb = get(game.store, b);
  assert.ok(!eb || eb.hp < 50, 'neighbor was caught by the splash');
});

test('splash hits friendlies too (no free danger-close fire)', () => {
  const game = arena();
  const arty = mkTank(game, 2, 5, HouseType.GDI, 'artillery', 100);
  const friendly = spawn(game.store, game.world, {
    kind: EntityKind.INFANTRY, type: 'minigunner', owner: HouseType.GDI, x: 8, y: 5, hp: 50,
  });
  gameTick(game, [forceAttackCommand([arty], 8, 5)]);
  run(game, 120);
  assert.equal(get(game.store, friendly), undefined, 'friendly fire is real');
});

test('control groups assign, recall, and drop dead members', () => {
  const game = arena();
  const input = createInputState();
  const a = mkTank(game, 2, 2, HouseType.GDI);
  const b = mkTank(game, 3, 2, HouseType.GDI);
  input.selection.add(a);
  input.selection.add(b);
  assignGroup(input, '1');
  input.selection.clear();
  recallGroup(input, game.store, '1');
  assert.deepEqual([...input.selection].sort(), [a, b].sort());
  despawn(game.store, game.world, a);
  recallGroup(input, game.store, '1');
  assert.deepEqual([...input.selection], [b], 'dead unit dropped from the group');
});
