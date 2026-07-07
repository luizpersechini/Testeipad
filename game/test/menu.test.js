import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  createMenu, layoutMenu, hitMenuItem, menuClick, cycleOption,
} from '../src/render/menu.js';

test('main menu leads to skirmish setup and back', () => {
  const menu = createMenu();
  assert.equal(menu.screen, 'main');
  let items = layoutMenu(menu, 1280, 720);
  menuClick(menu, items.find((i) => i.id === 'skirmish'));
  assert.equal(menu.screen, 'skirmish');
  items = layoutMenu(menu, 1280, 720);
  menuClick(menu, items.find((i) => i.id === 'back'));
  assert.equal(menu.screen, 'main');
});

test('options cycle forward and backward with wraparound', () => {
  const menu = createMenu();
  assert.equal(menu.skirmish.faction, 'gdi');
  cycleOption(menu, 'faction', 1);
  assert.equal(menu.skirmish.faction, 'nod');
  cycleOption(menu, 'faction', 1);
  assert.equal(menu.skirmish.faction, 'gdi');
  cycleOption(menu, 'difficulty', -1);
  assert.equal(menu.skirmish.difficulty, 'easy');
  cycleOption(menu, 'credits', 1);
  assert.equal(menu.skirmish.credits, 7500);
  cycleOption(menu, 'seed', 1);
  assert.equal(menu.skirmish.seed, 43);
  cycleOption(menu, 'seed', -1);
  assert.equal(menu.skirmish.seed, 42);
});

test('start click returns settings and enters game screen', () => {
  const menu = createMenu();
  menu.screen = 'skirmish';
  menu.skirmish.faction = 'nod';
  menu.skirmish.difficulty = 'hard';
  const items = layoutMenu(menu, 1280, 720);
  const action = menuClick(menu, items.find((i) => i.id === 'start'));
  assert.equal(menu.screen, 'game');
  assert.equal(action.type, 'start');
  assert.equal(action.settings.faction, 'nod');
  assert.equal(action.settings.difficulty, 'hard');
  // Settings are a copy — later menu edits don't leak into the running game.
  menu.skirmish.faction = 'gdi';
  assert.equal(action.settings.faction, 'nod');
});

test('hit testing respects item bounds', () => {
  const menu = createMenu();
  const items = layoutMenu(menu, 1280, 720);
  const first = items[0];
  assert.equal(hitMenuItem(items, first.x + 1, first.y + 1), first);
  assert.equal(hitMenuItem(items, first.x - 2, first.y), null);
});
