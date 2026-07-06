// Seeded deterministic RNG. Every source of randomness in the sim must flow
// through one of these so that seed + command stream => identical game state.
// Numerical Recipes LCG; 32-bit state, good enough for gameplay.

export function createRng(seed) {
  let state = seed >>> 0;
  if (state === 0) state = 0x9e3779b9;

  function nextU32() {
    state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
    return state;
  }

  return {
    // Integer in [0, n)
    int(n) {
      return nextU32() % n;
    },
    // Integer in [lo, hi] inclusive
    range(lo, hi) {
      return lo + (nextU32() % (hi - lo + 1));
    },
    // Float in [0, 1)
    float() {
      return nextU32() / 0x100000000;
    },
    // True with probability p
    chance(p) {
      return nextU32() / 0x100000000 < p;
    },
    // Expose state for save/load determinism.
    getState() {
      return state;
    },
    setState(s) {
      state = s >>> 0;
    },
  };
}
