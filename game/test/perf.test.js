// Performance budget (plan M9.3): with 300 units + 60 buildings in active
// combat, the average sim tick must stay under 8ms. Measured headless so it
// tracks the sim, not the canvas.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { spawn, EntityKind } from '../src/sim/entity.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { attackMoveCommand } from '../src/sim/commands.js';

test('sim tick budget: 360 entities in battle average under 8ms', () => {
  const game = createGame(1, { startingCredits: 99999 });
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);

  let buildings = 0;
  for (let i = 0; i < 30; i++) {
    for (const [owner, bx] of [[HouseType.GDI, 2], [HouseType.NOD, 40]]) {
      const id = spawn(game.store, game.world, {
        kind: EntityKind.BUILDING, type: 'power_plant', owner,
        x: bx + (i % 6) * 3, y: 2 + ((i / 6) | 0) * 3, hp: 400, footprint: [2, 2],
      });
      if (id > 0) buildings++;
    }
  }
  const ids = { [HouseType.GDI]: [], [HouseType.NOD]: [] };
  let units = 0;
  for (let i = 0; i < 150; i++) {
    for (const owner of [HouseType.GDI, HouseType.NOD]) {
      const infantry = i % 3 === 2;
      const id = spawn(game.store, game.world, {
        kind: infantry ? EntityKind.INFANTRY : EntityKind.UNIT,
        type: infantry ? 'minigunner' : i % 3 === 0 ? 'medium_tank' : 'humvee',
        owner,
        x: (owner === HouseType.GDI ? 2 : 38) + (i % 12) + owner,
        y: 24 + ((i / 12) | 0) * 2,
        hp: 300,
        facing: 8,
      });
      if (id > 0) {
        ids[owner].push(id);
        units++;
      }
    }
  }
  assert.ok(units >= 290 && buildings >= 55, `spawned ${units} units, ${buildings} buildings`);

  // Everyone converges on the middle: pathfinding + combat + fog all hot.
  gameTick(game, [
    attackMoveCommand(ids[HouseType.GDI], 32, 30),
    attackMoveCommand(ids[HouseType.NOD], 30, 30),
  ]);

  const N = 450;
  const t0 = process.hrtime.bigint();
  for (let i = 0; i < N; i++) gameTick(game);
  const avgMs = Number(process.hrtime.bigint() - t0) / 1e6 / N;

  assert.ok(avgMs < 8, `avg tick ${avgMs.toFixed(2)}ms exceeds the 8ms budget`);
});
