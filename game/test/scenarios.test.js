import { test } from 'node:test';
import assert from 'node:assert/strict';
import { HouseType } from '../src/sim/constants.js';
import { EntityKind } from '../src/sim/entity.js';
import { gameTick } from '../src/sim/game.js';
import { enableAI } from '../src/sim/ai.js';
import { SCENARIOS, createScenarioGame } from '../src/sim/scenarios.js';
import { attackMoveCommand } from '../src/sim/commands.js';
import { createMenu, layoutMenu, menuClick } from '../src/render/menu.js';

test('every scenario loads with all scripted entities placed', () => {
  for (const [id, s] of Object.entries(SCENARIOS)) {
    const setup = createScenarioGame(id);
    assert.ok(setup, id);
    const expected = s.buildings.length + s.units.length + s.infantry.length;
    assert.equal(setup.game.store.entities.size, expected,
      `${id}: every scripted entity found a legal cell`);
    assert.equal(setup.player, s.player);
    assert.notEqual(setup.aiHouse, s.player);
  }
  assert.equal(createScenarioGame('nope'), null);
});

test('scenario loading is deterministic', () => {
  const fp = () => {
    const { game } = createScenarioGame('gdi_1');
    let acc = '';
    for (const e of game.store.entities.values()) acc += `${e.id}:${e.type}:${e.x},${e.y}|`;
    let t = 0;
    for (let i = 0; i < game.world.tiberium.length; i++) t += game.world.tiberium[i];
    return acc + t;
  };
  assert.equal(fp(), fp());
});

test('nod_1 strike mission: razing GDI production wins the game', () => {
  const setup = createScenarioGame('nod_1');
  const { game } = setup;
  // The strike team alone is the whole Nod presence — no production, so give
  // the sim its combatants and march everything at the GDI base.
  const strikeTeam = [...game.store.entities.values()]
    .filter((e) => e.owner === HouseType.NOD && e.kind !== EntityKind.BUILDING)
    .map((e) => e.id);
  assert.ok(strikeTeam.length >= 8, 'strike team present');
  // Buff the team so the head-on assault succeeds without micro.
  for (const id of strikeTeam) {
    const e = game.store.entities.get(id);
    e.hp = e.maxHp = 2000;
  }
  gameTick(game, [attackMoveCommand(strikeTeam, 7, 7)]);
  for (let i = 0; i < 12000 && game.winner === null; i++) gameTick(game);
  assert.equal(game.winner, HouseType.NOD, 'mission accomplished');
});

test('gdi_1 plays as a full battle when the AI drives Nod', () => {
  const setup = createScenarioGame('gdi_1');
  enableAI(setup.game, setup.aiHouse, setup.aiDifficulty);
  for (let i = 0; i < 2000; i++) gameTick(setup.game);
  // The Nod AI is alive and doing something: producing or attacking.
  const nod = [...setup.game.store.entities.values()].filter((e) => e.owner === HouseType.NOD);
  assert.ok(nod.length >= 10, `Nod base active (${nod.length} entities)`);
  assert.equal(setup.game.winner, null, 'battle still raging at tick 2000');
});

test('missions are reachable from the menu', () => {
  const menu = createMenu();
  let items = layoutMenu(menu, 1280, 720);
  menuClick(menu, items.find((i) => i.id === 'missions'));
  assert.equal(menu.screen, 'missions');
  items = layoutMenu(menu, 1280, 720);
  const first = items.find((i) => i.id.startsWith('mission:'));
  assert.ok(first, 'missions listed');
  const action = menuClick(menu, first);
  assert.equal(action.type, 'startScenario');
  assert.ok(SCENARIOS[action.id]);
  assert.equal(menu.screen, 'game');
});
