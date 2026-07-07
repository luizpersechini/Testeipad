// On-screen command bar: touch replacement for the keyboard hotkeys, shown
// along the bottom of the map view. Layout + hit-testing are pure
// (node-tested); drawing needs a ctx.

const BTN_W = 86;
const BTN_H = 44;
const GAP = 8;

// Buttons shown while units/buildings are selected, plus PAUSE always.
const SELECTION_BUTTONS = [
  { id: 'stop', label: 'STOP' },
  { id: 'attackmove', label: 'ATTACK' },
  { id: 'deploy', label: 'DEPLOY' },
  { id: 'harvest', label: 'HARVEST' },
  { id: 'repair', label: 'REPAIR' },
  { id: 'sell', label: 'SELL' },
];

export function layoutCommandBar(hasSelection, viewW, viewH) {
  const buttons = [];
  let x = GAP;
  const y = viewH - BTN_H - GAP;
  if (hasSelection) {
    for (const b of SELECTION_BUTTONS) {
      buttons.push({ ...b, x, y, w: BTN_W, h: BTN_H });
      x += BTN_W + GAP;
    }
  }
  buttons.push({ id: 'pause', label: 'PAUSE', x: viewW - BTN_W - GAP, y, w: BTN_W, h: BTN_H });
  return buttons;
}

export function hitCommandBar(buttons, px, py) {
  return buttons.find((b) => px >= b.x && px < b.x + b.w
    && py >= b.y && py < b.y + b.h) ?? null;
}

export function drawCommandBar(ctx, buttons, input) {
  ctx.font = 'bold 13px monospace';
  ctx.textAlign = 'center';
  for (const b of buttons) {
    const armed = b.id === 'attackmove' && input.attackMoveArmed;
    ctx.fillStyle = armed ? 'rgba(160, 40, 30, 0.85)' : 'rgba(20, 20, 20, 0.75)';
    ctx.fillRect(b.x, b.y, b.w, b.h);
    ctx.strokeStyle = armed ? '#ff8060' : '#5a5a4a';
    ctx.strokeRect(b.x + 0.5, b.y + 0.5, b.w - 1, b.h - 1);
    ctx.fillStyle = armed ? '#ffd0c0' : '#d8d8c8';
    ctx.fillText(b.label, b.x + b.w / 2, b.y + b.h / 2 + 5);
  }
  ctx.textAlign = 'left';
}
