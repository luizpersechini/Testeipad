// Shared stat lookup for live entities (kept out of game.js so combat.js and
// game.js can both use it without an import cycle).

import { EntityKind } from './entity.js';
import { unitData } from './data/units.js';
import { infantryData } from './data/infantry.js';

export function statsFor(entity) {
  if (entity.kind === EntityKind.UNIT) return unitData(entity.type);
  if (entity.kind === EntityKind.INFANTRY) return infantryData(entity.type);
  return null;
}
