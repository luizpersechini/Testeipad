import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { spawn, despawn, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { moveCommand } from '../src/sim/commands.js';
import {
  SHROUD, FOGGED, VISIBLE, fogAt, entityVisibleTo, recomputeFog,
} from '../src/sim/fog.js';

function arena() {
  const game = createGame(1);
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  return game;
}

const mkTank = (game, x, y, owner = HouseType.GDI) => spawn(
  game.store, game.world,
  { kind: EntityKind.UNIT, type: 'medium_tank', owner, x, y, hp: 400, facing: 8 },
);

test('world starts shrouded; a unit reveals its sight radius', () => {
  const game = arena();
  mkTank(game, 20, 20); // sight 4
  gameTick(game); // tick 0 runs fog
  assert.equal(fogAt(game, HouseType.GDI, 20, 20), VISIBLE);
  assert.equal(fogAt(game, HouseType.GDI, 24, 20), VISIBLE, 'edge of sight');
  assert.equal(fogAt(game, HouseType.GDI, 25, 20), SHROUD, 'beyond sight');
  assert.equal(fogAt(game, HouseType.GDI, 40, 40), SHROUD);
  // The enemy's map is untouched by our unit.
  assert.equal(fogAt(game, HouseType.NOD, 20, 20), SHROUD);
});

test('moving away leaves fog (seen), never back to shroud', () => {
  const game = arena();
  const id = mkTank(game, 5, 5);
  gameTick(game, [moveCommand([id], 25, 5)]);
  for (let i = 0; i < 300; i++) gameTick(game);
  assert.equal(fogAt(game, HouseType.GDI, 25, 5), VISIBLE, 'current position lit');
  assert.equal(fogAt(game, HouseType.GDI, 5, 5), FOGGED, 'old position remembered');
  assert.equal(fogAt(game, HouseType.GDI, 5, 40), SHROUD, 'unvisited stays dark');
});

test('death of the only scout decays its view to fog', () => {
  const game = arena();
  const id = mkTank(game, 20, 20);
  gameTick(game);
  assert.equal(fogAt(game, HouseType.GDI, 20, 20), VISIBLE);
  despawn(game.store, game.world, id);
  recomputeFog(game, HouseType.GDI);
  assert.equal(fogAt(game, HouseType.GDI, 20, 20), FOGGED);
});

test('enemy units hide in fog and shroud; enemy buildings linger on fogged ground', () => {
  const game = arena();
  mkTank(game, 10, 10, HouseType.GDI); // sight 4
  const farNod = mkTank(game, 40, 40, HouseType.NOD);
  const nearNod = mkTank(game, 13, 10, HouseType.NOD);
  const nodBld = spawn(game.store, game.world, {
    kind: EntityKind.BUILDING, type: 'power_plant', owner: HouseType.NOD,
    x: 12, y: 12, hp: 400, footprint: [2, 2],
  });
  gameTick(game);
  const g = (id) => game.store.entities.get(id);
  assert.equal(entityVisibleTo(game, HouseType.GDI, g(farNod)), false, 'shrouded unit hidden');
  assert.equal(entityVisibleTo(game, HouseType.GDI, g(nearNod)), true, 'sighted unit shown');
  assert.equal(entityVisibleTo(game, HouseType.GDI, g(nodBld)), true, 'sighted building shown');
  assert.equal(entityVisibleTo(game, HouseType.NOD, g(farNod)), true, 'own units always visible');
});

test('fog updates are deterministic', () => {
  const play = () => {
    const game = createGame(9);
    const s = game.world.startPositions[0];
    const id = spawn(game.store, game.world, {
      kind: EntityKind.UNIT, type: 'medium_tank', owner: HouseType.GDI,
      x: s.x, y: s.y, hp: 400,
    });
    gameTick(game, [moveCommand([id], s.x + 12, s.y + 4)]);
    for (let i = 0; i < 300; i++) gameTick(game);
    let acc = 0;
    const fogMap = game.fog[HouseType.GDI];
    for (let i = 0; i < fogMap.length; i++) acc = (acc * 31 + fogMap[i]) >>> 0;
    return acc;
  };
  assert.equal(play(), play());
});
