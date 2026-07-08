// Sprite sheet registry. The frame-math and metadata here are pure and run in
// node (tests import them); only loadAssets() touches the DOM (Image), so this
// module must never reference document/window at the top level.

// All paths are relative to game/index.html.
const SHEETS_BASE = '../assets/sheets/';
const TILES_BASE = '../assets/tiles/';
const UI_BASE = '../assets/ui/';

// Sheet layouts, matching tools/generate_sprites_v2.py output:
//   vehicle:  8 facing columns x 1 row, 32x32 frames (mammoth authored 40px but
//             packed at 32px stride — frames are clipped; acceptable placeholder)
//   infantry: 8 facing columns x 3 walk-frame rows, 24x24 frames
//   buildings: 7 columns x 2 rows (row 0 GDI, row 1 Nod), 64x64 cells
//   effects:  11 columns x 1 row, 32x32 (0-5 explosion, 6-10 smoke)
export const SHEET_DEFS = {
  gdi_medium_tank: { file: 'gdi_medium_tank_sheet.png', kind: 'vehicle', frameW: 32, frameH: 32, cols: 8, rows: 1 },
  gdi_mammoth_tank: { file: 'gdi_mammoth_tank_sheet.png', kind: 'vehicle', frameW: 32, frameH: 32, cols: 8, rows: 1 },
  nod_light_tank: { file: 'nod_light_tank_sheet.png', kind: 'vehicle', frameW: 32, frameH: 32, cols: 8, rows: 1 },
  nod_stealth_tank: { file: 'nod_stealth_tank_sheet.png', kind: 'vehicle', frameW: 32, frameH: 32, cols: 8, rows: 1 },
  gdi_minigunner: { file: 'gdi_minigunner_sheet.png', kind: 'infantry', frameW: 24, frameH: 24, cols: 8, rows: 3 },
  gdi_grenadier: { file: 'gdi_grenadier_sheet.png', kind: 'infantry', frameW: 24, frameH: 24, cols: 8, rows: 3 },
  gdi_rocket_soldier: { file: 'gdi_rocket_soldier_sheet.png', kind: 'infantry', frameW: 24, frameH: 24, cols: 8, rows: 3 },
  nod_rifle_infantry: { file: 'nod_rifle_infantry_sheet.png', kind: 'infantry', frameW: 24, frameH: 24, cols: 8, rows: 3 },
  nod_chemical_warrior: { file: 'nod_chemical_warrior_sheet.png', kind: 'infantry', frameW: 24, frameH: 24, cols: 8, rows: 3 },
  nod_flamethrower: { file: 'nod_flamethrower_sheet.png', kind: 'infantry', frameW: 24, frameH: 24, cols: 8, rows: 3 },
  buildings: { file: 'buildings_sheet.png', kind: 'grid', frameW: 64, frameH: 64, cols: 7, rows: 2 },
  effects: { file: 'effects_sheet.png', kind: 'strip', frameW: 32, frameH: 32, cols: 11, rows: 1 },
};

// Column order in buildings_sheet.png (per generate_sprites_v2.py main()).
export const BUILDING_COLUMNS = [
  'construction_yard', 'power_plant', 'refinery', 'barracks',
  'war_factory', 'radar', 'guard_tower',
];

export const EFFECT_FRAMES = {
  explosion: { start: 0, count: 6 },
  smoke: { start: 6, count: 5 },
};

// Standalone terrain tiles (48x48 source, drawn scaled to TILE).
// 4 variants each to break up repetition.
export const TILE_FILES = {
  clear: 'terrain/clear_', rock: 'terrain/rock_', water: 'terrain/water_',
  sand: 'terrain/sand_', rough: 'terrain/rough_',
};
export const TILE_VARIANTS = 4;
export const TIBERIUM_OVERLAYS = 4; // overlays/tiberium_0..3.png

// ── Pure frame math ──────────────────────────────────────────────────────────

export function frameRect(def, col, row) {
  return { sx: col * def.frameW, sy: row * def.frameH, sw: def.frameW, sh: def.frameH };
}

// facing8: 0=N .. 7=NW clockwise, matching sheet column order.
export function vehicleFrame(def, facing8) {
  return frameRect(def, facing8 & 7, 0);
}

export function infantryFrame(def, facing8, walkFrame) {
  return frameRect(def, facing8 & 7, walkFrame % def.rows);
}

// faction: 0 = GDI (row 0), 1 = Nod (row 1).
export function buildingFrame(def, faction, buildingName) {
  const col = BUILDING_COLUMNS.indexOf(buildingName);
  if (col === -1) return null;
  return frameRect(def, col, faction & 1);
}

export function effectFrame(def, effectName, animFrame) {
  const info = EFFECT_FRAMES[effectName];
  if (!info) return null;
  return frameRect(def, info.start + (animFrame % info.count), 0);
}

// Map sim tiberium stage (1..11) to one of the 4 overlay art variants.
export function tiberiumVariant(stage) {
  if (stage <= 0) return -1;
  return Math.min(TIBERIUM_OVERLAYS - 1, ((stage - 1) / 3) | 0);
}

// ── DOM loading (browser only) ───────────────────────────────────────────────

function loadImage(src) {
  return new Promise((resolve) => {
    const img = new Image();
    img.onload = () => resolve(img);
    img.onerror = () => resolve(null); // missing art must never break the game
    img.src = src;
  });
}

// Original-asset override: tools/cnc/import_assets.py writes extracted
// freeware sprites plus a manifest into assets/original/ (git-ignored).
// When that manifest exists, its sheets replace the placeholder art.
const ORIGINAL_BASE = '../assets/original/';

async function loadOriginalOverrides(registry) {
  let manifest;
  try {
    const res = await fetch(`${ORIGINAL_BASE}manifest.json`);
    if (!res.ok) return;
    manifest = await res.json();
  } catch {
    return; // no original assets installed; placeholders remain
  }
  const jobs = [];
  for (const [name, def] of Object.entries(manifest.sheets ?? {})) {
    jobs.push(loadImage(ORIGINAL_BASE + def.file).then((img) => {
      if (img) registry.sheets[name] = { img, def };
    }));
  }
  // Terrain floor tiles (up to 4 variants per terrain type).
  for (const [terrain, files] of Object.entries(manifest.tiles ?? {})) {
    files.forEach((file, v) => {
      jobs.push(loadImage(ORIGINAL_BASE + file).then((img) => {
        if (!img) return;
        registry.tiles[terrain] = registry.tiles[terrain] ?? [];
        registry.tiles[terrain][v] = img;
        // Pad missing variants so cellVariant() always hits something.
        for (let i = 0; i < TILE_VARIANTS; i++) {
          registry.tiles[terrain][i] = registry.tiles[terrain][i] ?? img;
        }
      }));
    });
  }
  (manifest.tiberium ?? []).forEach((file, v) => {
    jobs.push(loadImage(ORIGINAL_BASE + file).then((img) => {
      if (img) registry.tiberium[v] = img;
    }));
  });
  if (manifest.tree) {
    jobs.push(loadImage(ORIGINAL_BASE + manifest.tree).then((img) => {
      if (img) registry.tree = img;
    }));
  }
  // Names of converted original sound effects (audio.js resolves and plays).
  registry.originalAudio = new Set(manifest.audio ?? []);
  registry.originalAudioBase = `${ORIGINAL_BASE}audio/`;

  await Promise.all(jobs);
  registry.usingOriginals = jobs.length > 0;
}

// Loads everything; resolves to a registry. Every lookup can return a null
// image — draw code falls back to colored rects so the game never white-screens.
export async function loadAssets() {
  const registry = {
    sheets: {}, // name -> { img|null, def }
    tiles: {}, // terrainName -> [img|null x variants]
    tiberium: [], // [img|null x variants]
    icons: {}, // icon name -> img|null
    missing: [],
  };

  const jobs = [];

  for (const [name, def] of Object.entries(SHEET_DEFS)) {
    jobs.push(loadImage(SHEETS_BASE + def.file).then((img) => {
      registry.sheets[name] = { img, def };
      if (!img) registry.missing.push(name);
    }));
  }

  for (const [name, prefix] of Object.entries(TILE_FILES)) {
    registry.tiles[name] = new Array(TILE_VARIANTS).fill(null);
    for (let v = 0; v < TILE_VARIANTS; v++) {
      jobs.push(loadImage(`${TILES_BASE}${prefix}${v}.png`).then((img) => {
        registry.tiles[name][v] = img;
        if (!img) registry.missing.push(`tile:${name}${v}`);
      }));
    }
  }

  registry.tiberium = new Array(TIBERIUM_OVERLAYS).fill(null);
  for (let v = 0; v < TIBERIUM_OVERLAYS; v++) {
    jobs.push(loadImage(`${TILES_BASE}overlays/tiberium_${v}.png`).then((img) => {
      registry.tiberium[v] = img;
      if (!img) registry.missing.push(`tiberium:${v}`);
    }));
  }

  for (const icon of ['icon_pp', 'icon_bar', 'icon_ref', 'icon_wf', 'icon_silo',
    'icon_hpad', 'icon_gt', 'icon_agt', 'icon_rep']) {
    jobs.push(loadImage(`${UI_BASE}${icon}.png`).then((img) => {
      registry.icons[icon] = img;
      if (!img) registry.missing.push(`icon:${icon}`);
    }));
  }

  await Promise.all(jobs);
  await loadOriginalOverrides(registry);
  return registry;
}
