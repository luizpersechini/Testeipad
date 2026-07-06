// Tests the pure metadata/frame-math of render/assets.js. Importing this
// module in node also proves it has no top-level DOM dependency (iron rule:
// only loadAssets() may touch the browser).

import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  SHEET_DEFS, BUILDING_COLUMNS, vehicleFrame, infantryFrame,
  buildingFrame, effectFrame, tiberiumVariant,
} from '../src/render/assets.js';

test('every sheet def is internally consistent', () => {
  for (const [name, def] of Object.entries(SHEET_DEFS)) {
    assert.ok(def.file.endsWith('.png'), name);
    assert.ok(def.frameW > 0 && def.frameH > 0, name);
    assert.ok(def.cols > 0 && def.rows > 0, name);
  }
});

test('vehicle frames map facing to column', () => {
  const def = SHEET_DEFS.gdi_medium_tank;
  assert.deepEqual(vehicleFrame(def, 0), { sx: 0, sy: 0, sw: 32, sh: 32 });
  assert.deepEqual(vehicleFrame(def, 7), { sx: 224, sy: 0, sw: 32, sh: 32 });
  // Facing wraps instead of reading off the sheet.
  assert.deepEqual(vehicleFrame(def, 8), vehicleFrame(def, 0));
});

test('infantry frames map facing to column and walk frame to row', () => {
  const def = SHEET_DEFS.gdi_minigunner;
  assert.deepEqual(infantryFrame(def, 2, 0), { sx: 48, sy: 0, sw: 24, sh: 24 });
  assert.deepEqual(infantryFrame(def, 2, 2), { sx: 48, sy: 48, sw: 24, sh: 24 });
  // Walk cycle wraps.
  assert.deepEqual(infantryFrame(def, 2, 3), infantryFrame(def, 2, 0));
});

test('building frames: faction picks the row, unknown name is null', () => {
  const def = SHEET_DEFS.buildings;
  const gdiCy = buildingFrame(def, 0, 'construction_yard');
  const nodCy = buildingFrame(def, 1, 'construction_yard');
  assert.deepEqual(gdiCy, { sx: 0, sy: 0, sw: 64, sh: 64 });
  assert.deepEqual(nodCy, { sx: 0, sy: 64, sw: 64, sh: 64 });
  const wf = buildingFrame(def, 0, 'war_factory');
  assert.equal(wf.sx, BUILDING_COLUMNS.indexOf('war_factory') * 64);
  assert.equal(buildingFrame(def, 0, 'obelisk'), null);
});

test('effect frames offset into the strip and wrap per-effect', () => {
  const def = SHEET_DEFS.effects;
  assert.equal(effectFrame(def, 'explosion', 0).sx, 0);
  assert.equal(effectFrame(def, 'explosion', 5).sx, 160);
  assert.equal(effectFrame(def, 'smoke', 0).sx, 192);
  assert.equal(effectFrame(def, 'smoke', 5).sx, 192); // wraps at 5 frames
  assert.equal(effectFrame(def, 'nova', 0), null);
});

test('tiberium stage maps into the 4 art variants', () => {
  assert.equal(tiberiumVariant(0), -1);
  assert.equal(tiberiumVariant(1), 0);
  assert.equal(tiberiumVariant(3), 0);
  assert.equal(tiberiumVariant(4), 1);
  assert.equal(tiberiumVariant(11), 3);
});
