import { test } from 'node:test';
import assert from 'node:assert/strict';
import { TerrainType, HouseType } from '../src/sim/constants.js';
import { createGame, gameTick } from '../src/sim/game.js';
import { placeBuilding } from '../src/sim/placement.js';
import { buildCommand } from '../src/sim/commands.js';
import { layoutSidebarItems, hitSidebarItem } from '../src/render/sidebar.js';

function base() {
  const game = createGame(1, { startingCredits: 10000 });
  game.world.terrain.fill(TerrainType.CLEAR);
  game.world.tiberium.fill(0);
  placeBuilding(game, HouseType.GDI, 'construction_yard', 10, 10, { ignoreAdjacency: true });
  placeBuilding(game, HouseType.GDI, 'power_plant', 14, 10);
  placeBuilding(game, HouseType.GDI, 'barracks', 14, 13);
  return game;
}

test('sidebar lists what the base can build, with headers', () => {
  const game = base();
  const items = layoutSidebarItems(game, HouseType.GDI, 1080, 200, 260);
  const buildables = items.filter((i) => !i.header);
  const types = buildables.map((i) => i.type);
  assert.ok(types.includes('power_plant'));
  assert.ok(types.includes('minigunner'));
  assert.ok(!types.includes('medium_tank'), 'no war factory yet');
  for (const it of buildables) {
    assert.ok(it.cost > 0 && it.name, it.type);
    assert.ok(it.x >= 1080 && it.x + it.w <= 1280, 'inside the sidebar');
  }
});

test('hit test finds items and misses headers/gaps', () => {
  const game = base();
  const items = layoutSidebarItems(game, HouseType.GDI, 1080, 200, 260);
  const first = items.find((i) => !i.header);
  const hit = hitSidebarItem(items, first.x + 5, first.y + 5);
  assert.equal(hit.type, first.type);
  assert.equal(hitSidebarItem(items, first.x + 5, first.y - 2), null);
});

test('queue state flows into the layout (progress and blocked flags)', () => {
  const game = base();
  gameTick(game, [buildCommand(HouseType.GDI, 'infantry', 'minigunner')]);
  for (let i = 0; i < 10; i++) gameTick(game);
  const items = layoutSidebarItems(game, HouseType.GDI, 1080, 200, 260);
  const mini = items.find((i) => i.type === 'minigunner');
  assert.ok(mini.queued, 'minigunner shows its queue');
  assert.ok(mini.queued.ticksLeft < mini.queued.totalTicks, 'progress advanced');
  const gren = items.find((i) => i.type === 'grenadier');
  assert.equal(gren.blocked, true, 'category busy blocks siblings');
  const pp = items.find((i) => i.type === 'power_plant');
  assert.equal(pp.blocked, false, 'other categories unaffected');
});
