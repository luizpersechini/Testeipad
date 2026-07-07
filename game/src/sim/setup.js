// Match construction (skirmish and scenario) lives in the sim so that a
// recorded {setup, command log} can rebuild the identical game anywhere —
// this is what makes replays (and future multiplayer) possible.

import { HouseType } from './constants.js';
import { createGame } from './game.js';
import { spawn, EntityKind } from './entity.js';
import { placeBuilding } from './placement.js';
import { enableAI } from './ai.js';
import { createScenarioGame } from './scenarios.js';

// settings: {faction, credits, difficulty, seed} (the menu's skirmish object).
export function createSkirmishGame(settings) {
  const player = settings.faction === 'nod' ? HouseType.NOD : HouseType.GDI;
  const enemy = player === HouseType.GDI ? HouseType.NOD : HouseType.GDI;
  const game = createGame(settings.seed, { startingCredits: settings.credits });
  const { world } = game;

  const starts = world.startPositions;
  const playerStart = starts[player === HouseType.GDI ? 0 : 1];
  const enemyStart = starts[player === HouseType.GDI ? 1 : 0];

  const escort = (owner) => {
    const heavy = owner === HouseType.GDI ? 'medium_tank' : 'light_tank';
    const heavyHp = owner === HouseType.GDI ? 400 : 300;
    return [
      [EntityKind.UNIT, heavy, heavyHp],
      [EntityKind.UNIT, heavy, heavyHp],
      [EntityKind.INFANTRY, 'minigunner', 50],
      [EntityKind.INFANTRY, 'minigunner', 50],
      [EntityKind.INFANTRY, 'rocket_soldier', 45],
    ];
  };
  for (const [owner, start] of [[player, playerStart], [enemy, enemyStart]]) {
    placeBuilding(game, owner, 'construction_yard', start.x - 1, start.y - 1, { ignoreAdjacency: true });
    placeBuilding(game, owner, 'power_plant', start.x + 3, start.y - 1);
    escort(owner).forEach(([kind, type, hp], i) => {
      spawn(game.store, world, {
        kind, type, owner, hp,
        x: start.x + (i % 4) - 1,
        y: start.y + ((i / 4) | 0) + 3,
      });
    });
  }
  enableAI(game, enemy, settings.difficulty);

  return { game, player, start: playerStart };
}

// setup descriptor -> fresh game. The descriptor is what replays store.
export function buildFromSetup(setup) {
  if (setup.mode === 'skirmish') {
    return createSkirmishGame(setup.settings);
  }
  if (setup.mode === 'scenario') {
    const s = createScenarioGame(setup.id);
    if (!s) return null;
    enableAI(s.game, s.aiHouse, s.aiDifficulty);
    return { game: s.game, player: s.player, start: s.start };
  }
  return null;
}
