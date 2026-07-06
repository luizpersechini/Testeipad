// Minimap: scaled-down world in the sidebar with the camera rectangle.
// Layout + coordinate math are pure (node-tested); drawing needs a ctx.

import { TILE, TerrainType } from '../sim/constants.js';
import { idx } from '../sim/world.js';

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

export function drawMinimap(ctx, layout, world, cam) {
  ctx.fillStyle = '#000';
  ctx.fillRect(layout.x - 2, layout.y - 2, layout.w + 4, layout.h + 4);

  // Terrain cells; ceil size so scaled cells leave no black seams.
  const cw = Math.ceil(layout.cellW);
  const ch = Math.ceil(layout.cellH);
  for (let y = 0; y < world.h; y++) {
    for (let x = 0; x < world.w; x++) {
      const i = idx(world, x, y);
      ctx.fillStyle = world.tiberium[i] > 0
        ? MINIMAP_TIBERIUM
        : (MINIMAP_COLORS[world.terrain[i]] ?? '#f0f');
      ctx.fillRect(
        layout.x + x * layout.cellW,
        layout.y + y * layout.cellH,
        cw, ch,
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
