// Audio: synthesized placeholder SFX via WebAudio — no copyrighted samples.
// Event -> sound planning is pure (node-tested); only playSound touches
// AudioContext, which is created lazily on the first user gesture.

// Sound recipes: type 'noise' = filtered noise burst (shots, explosions),
// 'tone' = oscillator blip (UI, EVA-style acknowledgments).
export const SOUNDS = {
  shot_cannon: { type: 'noise', dur: 0.12, freq: 900, gain: 0.25 },
  shot_mg: { type: 'noise', dur: 0.05, freq: 2400, gain: 0.15 },
  shot_rocket: { type: 'noise', dur: 0.3, freq: 500, gain: 0.2 },
  shot_flame: { type: 'noise', dur: 0.25, freq: 300, gain: 0.18 },
  shot_laser: { type: 'tone', dur: 0.2, freq: 1400, slide: -1200, gain: 0.2, wave: 'sawtooth' },
  explosion: { type: 'noise', dur: 0.5, freq: 200, gain: 0.4 },
  hit: { type: 'noise', dur: 0.06, freq: 1200, gain: 0.1 },
  unit_ready: { type: 'tone', dur: 0.09, freq: 880, gain: 0.2, wave: 'square', repeat: 2 },
  construction_ready: { type: 'tone', dur: 0.09, freq: 660, gain: 0.2, wave: 'square', repeat: 3 },
  place: { type: 'tone', dur: 0.12, freq: 220, gain: 0.25, wave: 'triangle' },
  sell: { type: 'tone', dur: 0.15, freq: 440, slide: 220, gain: 0.2, wave: 'triangle' },
  unload: { type: 'tone', dur: 0.08, freq: 1046, gain: 0.12, wave: 'sine' },
  victory: { type: 'tone', dur: 0.5, freq: 523, slide: 523, gain: 0.3, wave: 'square' },
};

const WEAPON_SOUND = {
  rifle: 'shot_mg',
  machinegun: 'shot_mg',
  sniper_rifle: 'shot_cannon',
  grenade: 'shot_rocket',
  dragon_rocket: 'shot_rocket',
  flamethrower: 'shot_flame',
  flame_tank_flamer: 'shot_flame',
  chem_spray: 'shot_flame',
  cannon_75mm: 'shot_cannon',
  cannon_105mm: 'shot_cannon',
  cannon_120mm: 'shot_cannon',
  turret_gun: 'shot_cannon',
  mammoth_tusk: 'shot_rocket',
  mlrs_rocket: 'shot_rocket',
  artillery_shell: 'shot_cannon',
  obelisk_laser: 'shot_laser',
  tower_rocket: 'shot_rocket',
};

const EVENT_SOUND = {
  death: 'explosion',
  hit: 'hit',
  unit_ready: 'unit_ready',
  construction_ready: 'construction_ready',
  place: 'place',
  sell: 'sell',
  unload: 'unload',
  victory: 'victory',
};

// Turn one tick's sim events into a throttled list of sound names: at most
// one instance of each sound per tick, and a global cap so big battles do
// not become white noise.
export function planSounds(events, cap = 5) {
  const seen = new Set();
  const plan = [];
  for (const ev of events) {
    let name = null;
    if (ev.type === 'shot') name = WEAPON_SOUND[ev.weapon] ?? 'shot_cannon';
    else name = EVENT_SOUND[ev.type] ?? null;
    if (!name || seen.has(name)) continue;
    seen.add(name);
    plan.push(name);
    if (plan.length >= cap) break;
  }
  return plan;
}

// ── WebAudio (browser only) ──────────────────────────────────────────────────

// Synth sound name -> original sound-effect basename (imported by
// tools/cnc/import_assets.py into assets/original/audio/*.ogg). When the
// file exists it plays instead of the synthesized recipe.
const ORIGINAL_FOR = {
  shot_cannon: 'tnkfire4',
  shot_mg: 'mgun11',
  shot_rocket: 'rocket1',
  shot_flame: 'flamer2',
  shot_laser: 'obelray1',
  explosion: 'xplobig4',
  hit: 'xplosml2',
  unit_ready: 'unitrdy',
  construction_ready: 'constru2',
  place: 'bldging1',
  sell: 'cashturn',
  unload: 'cashturn',
  victory: 'accom1',
};

export function createAudio() {
  return {
    ctx: null, // created on first user gesture
    muted: false,
    noiseBuf: null,
    registry: null, // set via attachRegistry once assets load
    samples: new Map(), // basename -> AudioBuffer | 'loading' | 'failed'
  };
}

export function attachRegistry(audio, registry) {
  audio.registry = registry;
}

// Play an imported original sample if available; returns false to fall back
// to synthesis. Buffers are fetched and decoded lazily, once each.
function playOriginal(audio, name) {
  const reg = audio.registry;
  const base = ORIGINAL_FOR[name];
  if (!reg?.originalAudio?.has(base)) return false;
  const ctx = ensureContext(audio);
  if (!ctx) return false;
  const cached = audio.samples.get(base);
  if (cached === 'failed') return false;
  if (cached && cached !== 'loading') {
    const src = ctx.createBufferSource();
    src.buffer = cached;
    const gain = ctx.createGain();
    gain.gain.value = 0.6;
    src.connect(gain);
    gain.connect(ctx.destination);
    src.start();
    return true;
  }
  if (!cached) {
    audio.samples.set(base, 'loading');
    fetch(`${reg.originalAudioBase}${base}.ogg`)
      .then((r) => (r.ok ? r.arrayBuffer() : Promise.reject(new Error('http'))))
      .then((buf) => ctx.decodeAudioData(buf))
      .then((decoded) => audio.samples.set(base, decoded))
      .catch(() => audio.samples.set(base, 'failed'));
  }
  return true; // swallow this play while loading; next shot uses the buffer
}

function ensureContext(audio) {
  if (audio.ctx) return audio.ctx;
  const Ctor = window.AudioContext ?? window.webkitAudioContext;
  if (!Ctor) return null;
  audio.ctx = new Ctor();
  // One second of white noise, reused by every noise burst.
  const buf = audio.ctx.createBuffer(1, audio.ctx.sampleRate, audio.ctx.sampleRate);
  const data = buf.getChannelData(0);
  for (let i = 0; i < data.length; i++) data[i] = Math.random() * 2 - 1;
  audio.noiseBuf = buf;
  return audio.ctx;
}

function playSound(audio, name) {
  const recipe = SOUNDS[name];
  const ctx = ensureContext(audio);
  if (!recipe || !ctx) return;
  const t = ctx.currentTime;
  const gain = ctx.createGain();
  gain.gain.setValueAtTime(recipe.gain, t);
  gain.gain.exponentialRampToValueAtTime(0.001, t + recipe.dur);
  gain.connect(ctx.destination);

  if (recipe.type === 'noise') {
    const src = ctx.createBufferSource();
    src.buffer = audio.noiseBuf;
    const filter = ctx.createBiquadFilter();
    filter.type = 'lowpass';
    filter.frequency.setValueAtTime(recipe.freq, t);
    src.connect(filter);
    filter.connect(gain);
    src.start(t);
    src.stop(t + recipe.dur);
  } else {
    const repeats = recipe.repeat ?? 1;
    for (let i = 0; i < repeats; i++) {
      const osc = ctx.createOscillator();
      osc.type = recipe.wave ?? 'sine';
      const start = t + i * recipe.dur * 1.4;
      osc.frequency.setValueAtTime(recipe.freq, start);
      if (recipe.slide) {
        osc.frequency.linearRampToValueAtTime(
          Math.max(40, recipe.freq + recipe.slide), start + recipe.dur,
        );
      }
      osc.connect(gain);
      osc.start(start);
      osc.stop(start + recipe.dur);
    }
  }
}

export function playForEvents(audio, events) {
  if (audio.muted) return;
  for (const name of planSounds(events)) {
    if (!playOriginal(audio, name)) playSound(audio, name);
  }
}

export function toggleMute(audio) {
  audio.muted = !audio.muted;
  return audio.muted;
}

// iOS suspends the AudioContext until a user gesture; call this from the
// first touch/click so sound actually plays on iPad.
export function resumeAudio(audio) {
  ensureContext(audio);
  if (audio.ctx && audio.ctx.state === 'suspended') {
    audio.ctx.resume();
  }
}
