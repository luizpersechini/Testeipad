// Boot: canvas setup and the fixed-timestep loop.
// Sim ticks at a fixed 15/s (accumulator); rendering runs at requestAnimationFrame rate.
// As milestones land, this file wires sim <- commands <- input and render -> canvas.

import { MS_PER_TICK, TILE, MAP_W, MAP_H } from './sim/constants.js';
import { createRng } from './sim/rng.js';

const canvas = document.getElementById('game');
const ctx = canvas.getContext('2d');

// Placeholder world visual until M1 lands: seeded noise tiles prove the
// module graph, canvas, and loop all work end to end.
const rng = createRng(42);
const tiles = [];
for (let i = 0; i < MAP_W * MAP_H; i++) {
  tiles.push(rng.int(3));
}

let tick = 0;
let accumulator = 0;
let lastTime = performance.now();

function simTick() {
  tick++;
}

function render() {
  const colors = ['#8a7a50', '#94845a', '#7e6e48'];
  const cols = Math.ceil(canvas.width / TILE);
  const rows = Math.ceil(canvas.height / TILE);
  for (let y = 0; y < rows; y++) {
    for (let x = 0; x < cols; x++) {
      ctx.fillStyle = colors[tiles[(y * MAP_W + x) % tiles.length]];
      ctx.fillRect(x * TILE, y * TILE, TILE, TILE);
    }
  }
  ctx.fillStyle = '#c8ffc8';
  ctx.font = '16px monospace';
  ctx.fillText('C&C: Tiberian Dawn — web port skeleton (M0)', 12, 24);
  ctx.fillText(`sim tick: ${tick}`, 12, 44);
}

function frame(now) {
  accumulator += now - lastTime;
  lastTime = now;
  // Clamp to avoid spiral-of-death after a background tab pause.
  if (accumulator > 250) accumulator = 250;
  while (accumulator >= MS_PER_TICK) {
    simTick();
    accumulator -= MS_PER_TICK;
  }
  render();
  requestAnimationFrame(frame);
}

requestAnimationFrame(frame);
