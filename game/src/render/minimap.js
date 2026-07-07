// Minimap: scaled-down world in the sidebar with the camera rectangle.
// Layout + coordinate math are pure (node-tested); drawing needs a ctx.

import { TILE, TerrainType, HouseType } from '../sim/constants.js';
import { idx } from '../sim/world.js';
import { EntityKind } from '../sim/entity.js';
import { entityVisibleTo } from '../sim/fog.js';

const MINIMAP_FACTION = {
  [HouseType.GDI]: '#e0c060',
  [HouseType.NOD]: '#d04040',
};

const MINIMAP_COLORS = {
  [TerrainType.CLEAR]: '#7a6c46',
  [TerrainType.ROCK]: '#5e5e58',
  [TerrainType.TREE]: '#35502a',
  [TerrainType.WATER]: '#274262',
};
const MINIMAP_TIBERIUM = '#2ea62e';

export function createMinimapLayout(canvasW, sidebarW, world, margin = 8, top = 8) {
  const size = sidebarW - margin * 2;
  return {
    x: canvasW - sidebarW + margin,
    y: top,
    w: size,
    h: size,
    cellW: size / world.w,
    cellH: size / world.h,
  };
}

export function pointInMinimap(layout, px, py) {
  return px >= layout.x && px < layout.x + layout.w
    && py >= layout.y && py < layout.y + layout.h;
}

// Minimap pixel -> world pixel (center of the pointed-at location).
export function minimapToWorldPx(layout, world, px, py) {
  const fx = (px - layout.x) / layout.w;
  const fy = (py - layout.y) / layout.h;
  return {
    x: Math.max(0, Math.min(1, fx)) * world.w * TILE,
    y: Math.max(0, Math.min(1, fy)) * world.h * TILE,
  };
}

const CACHE_REFRESH_TICKS = 10; // terrain+fog change slowly; entities redraw live

function renderTerrainCache(cache, layout, world, fogMap) {
  const ctx = cache.getContext('2d');
  ctx.fillStyle = '#000';
  ctx.fillRect(0, 0, cache.width, cache.height);
  const cw = Math.ceil(layout.cellW);
  const ch = Math.ceil(layout.cellH);
  for (let y = 0; y < world.h; y++) {
    for (let x = 0; x < world.w; x++) {
      const i = idx(world, x, y);
      if (fogMap && fogMap[i] === 0) continue; // shroud stays black
      ctx.fillStyle = world.tiberium[i] > 0
        ? MINIMAP_TIBERIUM
        : (MINIMAP_COLORS[world.terrain[i]] ?? '#f0f');
      ctx.fillRect(x * layout.cellW, y * layout.cellH, cw, ch);
      if (fogMap && fogMap[i] === 1) {
        ctx.fillStyle = 'rgba(0,0,0,0.5)';
        ctx.fillRect(x * layout.cellW, y * layout.cellH, cw, ch);
      }
    }
  }
}

// fogMap: viewing house's fog array (null = draw everything). tick drives the
// offscreen terrain cache (4096 cells redrawn every 10 ticks, not every frame).
// game/viewer (optional) add live entity dots filtered by fog.
export function drawMinimap(ctx, layout, world, cam, fogMap = null, tick = 0, game = null, viewer = null) {
  ctx.fillStyle = '#000';
  ctx.fillRect(layout.x - 2, layout.y - 2, layout.w + 4, layout.h + 4);

  if (!layout.cache) {
    layout.cache = document.createElement('canvas');
    layout.cache.width = layout.w;
    layout.cache.height = layout.h;
    layout.cacheTick = -1;
  }
  if (layout.cacheTick === -1 || tick - layout.cacheTick >= CACHE_REFRESH_TICKS) {
    renderTerrainCache(layout.cache, layout, world, fogMap);
    layout.cacheTick = tick;
  }
  ctx.drawImage(layout.cache, layout.x, layout.y);

  // Live entity dots (cheap: one small rect per visible entity per frame).
  if (game && viewer !== null) {
    for (const e of game.store.entities.values()) {
      if (e.kind === EntityKind.PROJECTILE || e.hp <= 0) continue;
      if (!entityVisibleTo(game, viewer, e)) continue;
      ctx.fillStyle = e.owner === viewer ? '#7ce07c'
        : (MINIMAP_FACTION[e.owner] ?? '#d0d0d0');
      const [fw, fh] = e.footprint ?? [1, 1];
      ctx.fillRect(
        layout.x + e.x * layout.cellW,
        layout.y + e.y * layout.cellH,
        Math.max(2, fw * layout.cellW),
        Math.max(2, fh * layout.cellH),
      );
    }
  }

  // Camera rectangle.
  const scaleX = layout.w / (world.w * TILE);
  const scaleY = layout.h / (world.h * TILE);
  ctx.strokeStyle = '#fff';
  ctx.lineWidth = 1;
  ctx.strokeRect(
    layout.x + cam.x * scaleX,
    layout.y + cam.y * scaleY,
    cam.viewW * scaleX,
    cam.viewH * scaleY,
  );

  ctx.strokeStyle = '#444';
  ctx.strokeRect(layout.x - 1, layout.y - 1, layout.w + 2, layout.h + 2);
}
