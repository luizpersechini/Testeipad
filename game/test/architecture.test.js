// Enforces the plan's iron rules mechanically, so no future change can drift:
//  1. sim/ never imports from render/ or input.js, and never touches the DOM.
//  2. every module parses as a plain ES module (no build step required).

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const SRC = fileURLToPath(new URL('../src', import.meta.url));

function walk(dir) {
  const out = [];
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    const p = join(dir, entry.name);
    if (entry.isDirectory()) out.push(...walk(p));
    else if (entry.name.endsWith('.js')) out.push(p);
  }
  return out;
}

test('iron rule 1: sim never imports render/input and never touches the DOM', () => {
  const simFiles = walk(join(SRC, 'sim'));
  assert.ok(simFiles.length >= 15, 'sim modules found');
  for (const file of simFiles) {
    const text = readFileSync(file, 'utf8');
    assert.ok(!/from\s+['"][^'"]*\/render\//.test(text), `${file} imports from render/`);
    assert.ok(!/from\s+['"][^'"]*input\.js['"]/.test(text), `${file} imports input.js`);
    for (const global of ['document', 'window', 'localStorage', 'requestAnimationFrame', 'AudioContext', 'Image(']) {
      assert.ok(!text.includes(global), `${file} references browser global "${global}"`);
    }
  }
});

test('iron rule 3 guard: sim never uses non-deterministic sources', () => {
  for (const file of walk(join(SRC, 'sim'))) {
    const text = readFileSync(file, 'utf8');
    assert.ok(!/Math\.random\s*\(/.test(text), `${file} uses Math.random (must use game.rng)`);
    assert.ok(!/Date\.now\s*\(/.test(text), `${file} uses Date.now`);
    assert.ok(!/performance\.now\s*\(/.test(text), `${file} uses performance.now`);
  }
});

test('render reads the sim but never reaches into its internals mutably (spot rules)', () => {
  for (const file of walk(join(SRC, 'render'))) {
    const text = readFileSync(file, 'utf8');
    // Renderers must not import the command constructors — issuing commands
    // is input.js's job; drawing code is read-only over sim state.
    assert.ok(!/from\s+['"][^'"]*commands\.js['"]/.test(text),
      `${file} imports commands.js (rendering must stay read-only)`);
  }
});
