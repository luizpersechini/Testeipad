// Menu shell: main menu and skirmish setup. Model + layout + hit-testing are
// pure (node-tested); draw* needs a ctx.

export function createMenu() {
  return {
    screen: 'main', // 'main' | 'skirmish' | 'game' | 'end'
    skirmish: {
      faction: 'gdi',
      credits: 5000,
      difficulty: 'normal',
      seed: 42,
    },
  };
}

const CYCLE = {
  faction: ['gdi', 'nod'],
  credits: [2500, 5000, 7500, 10000],
  difficulty: ['easy', 'normal', 'hard'],
  seed: null, // free-running integer
};

export function cycleOption(menu, id, dir = 1) {
  const s = menu.skirmish;
  if (id === 'seed') {
    s.seed = Math.max(1, s.seed + dir);
    return;
  }
  const options = CYCLE[id];
  if (!options) return;
  const i = options.indexOf(s[id]);
  s[id] = options[(i + dir + options.length) % options.length];
}

// Item rects for the current screen, centered on the canvas.
export function layoutMenu(menu, canvasW, canvasH) {
  const cx = canvasW / 2;
  const items = [];
  const W = 380;
  const H = 46;
  const push = (id, label, value, y) => items.push({
    id, label, value, x: cx - W / 2, y, w: W, h: H,
  });

  if (menu.screen === 'main') {
    push('skirmish', 'SKIRMISH', null, canvasH / 2 - 40);
    push('about', 'ABOUT', null, canvasH / 2 + 20);
  } else if (menu.screen === 'skirmish') {
    const s = menu.skirmish;
    let y = canvasH / 2 - 150;
    push('faction', 'FACTION', s.faction.toUpperCase(), y); y += 56;
    push('difficulty', 'ENEMY AI', s.difficulty.toUpperCase(), y); y += 56;
    push('credits', 'STARTING CREDITS', `$${s.credits}`, y); y += 56;
    push('seed', 'MAP SEED', `${s.seed}`, y); y += 76;
    push('start', 'START GAME', null, y); y += 56;
    push('back', 'BACK', null, y);
  }
  return items;
}

export function hitMenuItem(items, px, py) {
  return items.find((it) => px >= it.x && px < it.x + it.w
    && py >= it.y && py < it.y + it.h) ?? null;
}

// Returns an action for main.js to perform: {type:'start', settings} | null.
export function menuClick(menu, item, dir = 1) {
  if (!item) return null;
  if (menu.screen === 'main') {
    if (item.id === 'skirmish') menu.screen = 'skirmish';
    return null;
  }
  if (menu.screen === 'skirmish') {
    if (item.id === 'start') {
      menu.screen = 'game';
      return { type: 'start', settings: { ...menu.skirmish } };
    }
    if (item.id === 'back') {
      menu.screen = 'main';
      return null;
    }
    cycleOption(menu, item.id, dir);
  }
  return null;
}

export function drawMenu(ctx, menu, items, canvasW, canvasH) {
  ctx.fillStyle = '#0c0c0c';
  ctx.fillRect(0, 0, canvasW, canvasH);

  ctx.textAlign = 'center';
  ctx.font = 'bold 44px monospace';
  ctx.fillStyle = '#d4b45a';
  ctx.fillText('COMMAND & CONQUER', canvasW / 2, 130);
  ctx.font = '20px monospace';
  ctx.fillStyle = '#8c8c8c';
  ctx.fillText('TIBERIAN DAWN — WEB PORT', canvasW / 2, 165);

  ctx.font = '18px monospace';
  for (const it of items) {
    ctx.fillStyle = '#1e1e1e';
    ctx.fillRect(it.x, it.y, it.w, it.h);
    ctx.strokeStyle = '#4a4a3a';
    ctx.strokeRect(it.x + 0.5, it.y + 0.5, it.w - 1, it.h - 1);
    ctx.fillStyle = '#d0d0d0';
    if (it.value !== null && it.value !== undefined) {
      ctx.textAlign = 'left';
      ctx.fillText(it.label, it.x + 16, it.y + 29);
      ctx.textAlign = 'right';
      ctx.fillStyle = '#7ec97e';
      ctx.fillText(`< ${it.value} >`, it.x + it.w - 16, it.y + 29);
      ctx.textAlign = 'center';
    } else {
      ctx.fillText(it.label, it.x + it.w / 2, it.y + 29);
    }
  }

  if (menu.screen === 'skirmish') {
    ctx.font = '12px monospace';
    ctx.fillStyle = '#666';
    ctx.fillText('click a setting to cycle it (right-click cycles back)', canvasW / 2, canvasH - 60);
  }
  ctx.textAlign = 'left';
}
