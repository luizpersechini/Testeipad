import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { spawn, get, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { attackCommand, moveCommand } from '../src/sim/commands.js';
import { modifyDamage, WEAPONS } from '../src/sim/data/weapons.js';

function arena() {
  const game = createGame(1);
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  return game;
}

const mkTank = (game, x, y, owner, type = 'medium_tank', hp = 400) => spawn(
  game.store, game.world,
  { kind: EntityKind.UNIT, type, owner, x, y, hp, facing: 8 },
);
const mkInf = (game, x, y, owner, type = 'minigunner', hp = 50) => spawn(
  game.store, game.world,
  { kind: EntityKind.INFANTRY, type, owner, x, y, hp, facing: 8 },
);

function run(game, n) {
  const events = [];
  for (let i = 0; i < n; i++) {
    gameTick(game);
    events.push(...game.events);
  }
  return events;
}

test('ordered attack in range kills the target in the expected time', () => {
  const game = arena();
  const gdi = mkTank(game, 5, 5, HouseType.GDI);
  const nod = mkTank(game, 8, 5, HouseType.NOD, 'light_tank', 300);
  gameTick(game, [attackCommand([gdi], nod)]);
  // 105mm AP vs steel: 29 per shot -> ceil(300/29) = 11 shots.
  // First shot immediate, then every 50 ticks: 10 * 50 = 500 more ticks.
  const events = run(game, 520);
  assert.equal(get(game.store, nod), undefined, 'light tank destroyed');
  assert.ok(events.some((e) => e.type === 'death'), 'death event emitted');
  assert.equal(get(game.store, gdi).kills, 1, 'kill credited');
  assert.equal(get(game.store, gdi).state, 'idle', 'attacker stands down');
});

test('damage math drives matchups: rifle barely scratches a tank', () => {
  const game = arena();
  const inf = mkInf(game, 5, 5, HouseType.GDI);
  const tank = mkTank(game, 6, 5, HouseType.NOD, 'light_tank', 300);
  gameTick(game, [attackCommand([inf], tank)]);
  run(game, 200);
  const t = get(game.store, tank);
  assert.ok(t, 'tank survives 200 ticks of rifle fire');
  // 10 shots possible in ~200 ticks at rof 20; each deals 15*64/256=3.
  const perShot = modifyDamage(WEAPONS.rifle.damage, 'rifle', 'steel');
  assert.equal(perShot, 3);
  assert.ok(t.hp > 300 - 12 * perShot, `hp ${t.hp} consistent with SA-vs-steel chip damage`);
});

test('attacker chases a target that is out of range', () => {
  const game = arena();
  const gdi = mkTank(game, 2, 2, HouseType.GDI);
  const nod = mkTank(game, 20, 2, HouseType.NOD, 'light_tank', 300);
  gameTick(game, [attackCommand([gdi], nod)]);
  run(game, 120);
  const e = get(game.store, gdi);
  assert.ok(e.x > 10, `chased from x=2 to x=${e.x}`);
  run(game, 600);
  assert.equal(get(game.store, nod), undefined, 'caught and destroyed');
});

test('idle armed units auto-acquire enemies in sight (guard)', () => {
  const game = arena();
  const gdi = mkTank(game, 5, 5, HouseType.GDI); // sight 4
  const nod = mkInf(game, 8, 5, HouseType.NOD, 'minigunner', 50);
  run(game, 30);
  const e = get(game.store, gdi);
  assert.equal(e.state, 'attacking', 'guard picked up the intruder');
  // Authentic AP-vs-none modifier (0x40): 7 dmg per 105mm shot, rof 50 —
  // 8 shots to kill 50hp infantry. Slow, exactly like the original.
  run(game, 450);
  assert.equal(get(game.store, nod), undefined, 'intruder eliminated');
});

test('unarmed units never auto-attack', () => {
  const game = arena();
  const harv = spawn(game.store, game.world, {
    kind: EntityKind.UNIT, type: 'harvester', owner: HouseType.GDI, x: 5, y: 5, hp: 600,
  });
  mkInf(game, 7, 5, HouseType.NOD);
  run(game, 40);
  assert.equal(get(game.store, harv).state, 'idle');
});

test('death of the target releases attackers; projectiles fizzle', () => {
  const game = arena();
  const a = mkTank(game, 5, 5, HouseType.GDI);
  const b = mkTank(game, 6, 6, HouseType.GDI);
  const victim = mkInf(game, 8, 5, HouseType.NOD);
  gameTick(game, [attackCommand([a, b], victim)]);
  run(game, 300); // two tanks x 7 dmg per volley, 50hp target
  assert.equal(get(game.store, victim), undefined);
  const survivors = [a, b].map((id) => get(game.store, id));
  for (const s of survivors) {
    assert.equal(s.state, 'idle');
    // The victim returns fire before dying (guard), so allow rifle chip
    // damage — but nothing tank-caliber (no friendly fire).
    assert.ok(s.hp > s.maxHp - 30, `only return-fire scratches, hp ${s.hp}`);
  }
  // No stray projectiles left circling.
  for (const e of game.store.entities.values()) {
    assert.notEqual(e.kind, EntityKind.PROJECTILE);
  }
});

test('combat is deterministic (iron rule 3)', () => {
  const play = () => {
    const game = arena();
    const gdi = [mkTank(game, 2, 2, HouseType.GDI), mkInf(game, 2, 4, HouseType.GDI)];
    const nod = [mkTank(game, 14, 2, HouseType.NOD, 'light_tank', 300), mkInf(game, 14, 4, HouseType.NOD)];
    gameTick(game, [attackCommand([gdi[0]], nod[0]), moveCommand([gdi[1]], 10, 4)]);
    for (let i = 0; i < 400; i++) gameTick(game);
    return [...game.store.entities.values()].map((e) => `${e.id}:${e.x},${e.y},${e.hp},${e.state}`).join('|');
  };
  assert.equal(play(), play());
});
