# Overnight Loop Prompt — C&C Tiberian Dawn Web Port

You are one iteration of an unattended overnight loop working on this repository.
The owner is asleep and cannot answer questions. Do exactly one unit of work, verify it,
commit it, and stop.

## Your task this iteration

1. **Orient.**
   - Read `docs/REBOOT_PLAN.md` (architecture, iron rules, milestone checklist).
   - Read the last ~5 entries of `docs/PROGRESS.md` (what previous iterations did, open notes, blockers).
   - Run the test suite first: `node --test game/test/*.test.js`. If it fails, **fixing it is your task
     this iteration** — do not start new work on a red suite.

2. **Pick ONE task.** The first unchecked `[ ]` checkbox in the plan, skipping any marked
   `BLOCKED` in the progress log. Do not batch multiple checkboxes. Do not reorder or rewrite
   the plan (adding a small sub-note under a task is fine).

3. **Implement it** following the plan's architecture section exactly:
   - Sim code goes in `game/src/sim/`, is deterministic, and never touches the DOM.
   - Rendering/input never mutates sim state directly — only via `commands.js` + `tick()`.
   - Match the style of existing files. Plain ES modules, no build step, no npm dependencies.
   - Port game rules/stats from `engine/game/game_data.cpp` first (already transcribed),
     falling back to `original_source/` (UDATA/IDATA/BDATA/COMBAT/HOUSE/FINDPATH) when needed.
     Original C&C values beat invented values, always.

4. **Verify.**
   - Write node tests for any sim behavior you added (`game/test/`), run `node --test game/test/*.test.js`
     — everything must pass.
   - Syntax-check every JS file you touched: `node --check <file>`.
   - If the task has a "Playable check", you can't open a browser — instead write/extend a
     headless smoke test that constructs the game and runs 100 ticks without throwing.

5. **Record and commit.**
   - Tick the checkbox `[x]` in `docs/REBOOT_PLAN.md`.
   - Append an entry to `docs/PROGRESS.md`:
     `## <UTC timestamp> — <task id>` / what you did / test count / notes-doubts-guesses for the morning.
   - Commit with message `<task id>: <summary>` and push:
     `git push -u origin claude/cc-modern-port-framework-CVhkW`
     (on network failure retry 4× with 2/4/8/16s backoff).

## Hard rules

- **Never commit with failing tests.** If you can't get green, revert your changes
  (`git checkout -- .`), log the failure honestly in `docs/PROGRESS.md`, and stop.
- If the same task has failed in **two** previous progress entries, mark it
  `BLOCKED: <reason>` in the progress log, skip it, and take the next task instead.
- Do not modify `original_source/` (GPL reference), `engine/` (frozen C++), `demo/index.html`
  (donor reference), or `assets/` sprite files — read them freely, change nothing.
  Exception: task M9.4 explicitly retires the demo.
- Do not add build tools, bundlers, TypeScript, or npm packages. Browser-native ES modules only.
- Do not force-push, rebase, amend published commits, or touch any other branch.
- Keep the working tree clean when you stop: committed or reverted, nothing half-done.
- If everything in M0–M9 is checked, work M10; if M10 is also done, run the full suite,
  do a code-quality pass on the worst file you can find (with tests proving no behavior change),
  and log it.

Begin now: run the test suite, then pick your task.
