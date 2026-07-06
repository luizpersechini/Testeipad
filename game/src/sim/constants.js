// Core simulation constants. The sim is deterministic and headless:
// nothing in game/src/sim/ may import from render/ or touch the DOM.

// Original C&C runs game logic at 15 ticks per second.
export const TICK_RATE = 15;
export const MS_PER_TICK = 1000 / TICK_RATE;

// Our sprite sheets are authored at 32px per cell (original was 24px @ 320x200).
export const TILE = 32;

// Default skirmish map size in cells.
export const MAP_W = 64;
export const MAP_H = 64;

// Facings: sim tracks 32 like the original (FACING.CPP); render collapses to 8.
export const SIM_FACINGS = 32;
export const RENDER_FACINGS = 8;

export const TerrainType = Object.freeze({
  CLEAR: 0,
  ROCK: 1,
  TREE: 2,
  WATER: 3,
});

// Tiberium growth stages per cell, matching the original overlay's 12 stages.
export const TIBERIUM_MAX_STAGE = 11;

export const HouseType = Object.freeze({
  GDI: 0,
  NOD: 1,
  NEUTRAL: 2,
});
