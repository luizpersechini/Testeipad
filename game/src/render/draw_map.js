// Terrain + tiberium rendering. Read-only over sim state.

import { TILE, TerrainType } from '../sim/constants.js';
import { idx } from '../sim/world.js';
import { visibleCells } from './camera.js';
import { tiberiumVariant, TILE_VARIANTS } from './assets.js';
import { SHROUD, FOGGED } from '../sim/fog.js';

// Fallback colors when tile art is missing.
const TERRAIN_COLORS = {
  [TerrainType.CLEAR]: '#8a7a50',
  [TerrainType.ROCK]: '#6e6e66',
  [TerrainType.TREE]: '#3d5c30',
  [TerrainType.WATER]: '#2c4a6e',
};
const TERRAIN_TILE_NAMES = {
  [TerrainType.CLEAR]: 'clear',
  [TerrainType.ROCK]: 'rock',
  [TerrainType.TREE]: 'rough', // no dedicated tree tile yet; rough + tint below
  [TerrainType.WATER]: 'water',
};
const TIBERIUM_FALLBACK = '#32b432';

// Deterministic per-cell art variant so the map doesn't shimmer between frames.
function cellVariant(x, y) {
  return ((x * 7 + y * 13) >>> 0) % TILE_VARIANTS;
}

// fogMap: the viewing house's fog array (or null to disable fog).
export function drawMap(ctx, world, cam, registry, fogMap = null) {
  const { x0, y0, x1, y1 } = visibleCells(cam, world);
  for (let y = y0; y <= y1; y++) {
    for (let x = x0; x <= x1; x++) {
      const i = idx(world, x, y);
      const sx = x * TILE - cam.x;
      const sy = y * TILE - cam.y;

      // Shrouded ground is pure black; skip the terrain work entirely.
      if (fogMap && fogMap[i] === SHROUD) {
        ctx.fillStyle = '#000';
        ctx.fillRect(sx, sy, TILE, TILE);
        continue;
      }
      const terrain = world.terrain[i];

      const tileName = TERRAIN_TILE_NAMES[terrain];
      const img = registry?.tiles?.[tileName]?.[cellVariant(x, y)];
      if (img) {
        ctx.drawImage(img, sx, sy, TILE, TILE);
        if (terrain === TerrainType.TREE) {
          // Tint stand-in tile until dedicated tree art exists.
          ctx.fillStyle = 'rgba(30, 80, 20, 0.45)';
          ctx.fillRect(sx, sy, TILE, TILE);
        }
      } else {
        ctx.fillStyle = TERRAIN_COLORS[terrain] ?? '#f0f';
        ctx.fillRect(sx, sy, TILE, TILE);
      }

      const stage = world.tiberium[i];
      if (stage > 0) {
        const v = tiberiumVariant(stage);
        const tib = registry?.tiberium?.[v];
        if (tib) {
          ctx.drawImage(tib, sx, sy, TILE, TILE);
        } else {
          // Denser stage -> bigger crystals patch.
          const inset = Math.max(2, 12 - stage);
          ctx.fillStyle = TIBERIUM_FALLBACK;
          ctx.fillRect(sx + inset, sy + inset, TILE - inset * 2, TILE - inset * 2);
        }
      }

      // Previously-seen ground sits under a dim veil.
      if (fogMap && fogMap[i] === FOGGED) {
        ctx.fillStyle = 'rgba(0, 0, 0, 0.45)';
        ctx.fillRect(sx, sy, TILE, TILE);
      }
    }
  }
}
