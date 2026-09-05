# BreakingWave — CLAUDE.md

## Document Index

| File | Contents |
|------|----------|
| `01_SOUL.md` | Game vision, core emotions, confirmed decisions, priorities, open questions |
| `02_STATUS.md` | Current phase, what exists, what's done, next steps |
| `03_DECISIONS.md` | Logged decisions with reasoning (why, not just what) |
| `04_PRINCIPLES.md` | 7 design principles — the "why" behind code and design choices |
| `05_ZONES.md` | Beach map, 5-zone layout, character pool structure, gaze-crossing examples |
| `06_COMBAT.md` | Player controls, damage model, cover system, combat rhythm per zone |
| `07_CAMERA.md` | Headbob, hit camera, death camera, narrative screen, transition system |
| `08_ENEMY_AI.md` | MG bunker AI (priority targeting, accuracy, stops), infantry AI, difficulty curve |
| `09_ALLY_NPC.md` | Allied NPC personality types, density/corpse management, ammo looting |
| `10_CHECKLIST.md` | 8-phase development order with per-step goals and document assignments |
| `11_ENGINE_NOTES.md` | UE 5.6 gotchas — headless/editor-Python patterns, anim/retarget traps, camera/input/rendering quirks |
| `12_ARCHITECTURE.md` | Code architecture — manager/state-array pattern, engine boundary, shared bullet pipeline, tooling layer, and why each was chosen |

## When to Read Each Document

### Always read at the start of every session
- **`01_SOUL.md`** — Without this, all direction is lost. Core premise, priorities, what this game is NOT.
- **`02_STATUS.md`** — Current phase and next steps. Never assume; always check.

### Read once at the start of a new session (design alignment)
- **`04_PRINCIPLES.md`** — 7 principles that govern every code and design choice. Read first if starting fresh work.
- **`12_ARCHITECTURE.md`** — how the code is organised and why. Read before writing or restructuring any system code.

### Read when a past decision is unclear or being reconsidered
- **`03_DECISIONS.md`** — Prevents re-litigating settled questions. Check here before proposing something that might already be decided.

### Read based on the task at hand

| Task | Read These |
|------|-----------|
| Movement, headbob, prone, slide | `07_CAMERA.md` |
| Hit detection, damage, wounded state, death | `06_COMBAT.md`, `07_CAMERA.md` |
| Death camera, narrative screen, transition | `07_CAMERA.md` |
| MG bunker AI, accuracy, stops | `08_ENEMY_AI.md` |
| Infantry AI, enemy behavior | `08_ENEMY_AI.md` |
| Allied NPC behavior, types, spawning | `09_ALLY_NPC.md` |
| Corpse system, ammo looting | `09_ALLY_NPC.md`, `06_COMBAT.md` |
| Level layout, zone geometry, cover placement | `05_ZONES.md` |
| Character pools, gaze-crossing logic | `05_ZONES.md`, `01_SOUL.md` |
| Narrative writing, character perspective | `01_SOUL.md`, `05_ZONES.md` |
| Design discussion (not just coding) | `01_SOUL.md`, `03_DECISIONS.md` + relevant docs |
| Checking what to build next | `10_CHECKLIST.md`, `02_STATUS.md` |
| Editor-Python tools, headless asset work, retargeting, camera/input C++ | `11_ENGINE_NOTES.md` |
| Adding a system, refactoring, or deciding where new code belongs | `12_ARCHITECTURE.md`, `04_PRINCIPLES.md` |

## Key Constraints to Remember

- **No UI**: No health bar, crosshair, ammo counter, or minimap. Sensation only.
- **No invincibility**: Not even on character transition. Enemy targeting delay instead.
- **No weapon looting**: Corpses are people, not resource nodes. Ammo only, when empty.
- **No help mechanic**: Wounded NPCs call for help. The player cannot respond. Intentional.
- **Death is not retry**: The death → narrative → transition sequence must be seamless, no loading.
- **Two-shot damage**: Headshot kills instantly. Torso takes two hits. Wounded state does not recover.
- **Fog is load-bearing**: Removes time sense + limits visibility + enables NPC spawn/despawn outside view.
- **Code readability is priority 3**: Function and variable names reveal intent.
- **All numbers are tentative**: Every numeric value in the design docs (damage multipliers, timings, ratios, distances) is a starting point for tuning unless it is recorded in `03_DECISIONS.md`. Implement them as named constants in one obvious place, never as scattered literals.

## Collaboration Rules

- **Stop and ask on ambiguity**: If a request is unclear, contradictory, or could go multiple ways, stop before writing any code and ask a targeted question. Do not guess and proceed.
- **Flag assumptions explicitly**: If something is being assumed rather than verified (from docs, code, or the user), say so before acting on it. Example: "I'm assuming X — is that right?"
- **No comments in code**: Write readable code through clear naming only. Do not add inline comments or docstrings unless the why is genuinely non-obvious and cannot be expressed by naming alone.
- **Data-oriented over object-oriented**: Prefer flat data structures, arrays of structs, and systems that operate on data in bulk. Avoid deep inheritance hierarchies, virtual dispatch, and encapsulation for its own sake. Design around what data exists and how it flows, not around objects and their behavior.
  - UE framework boundary classes (GameMode, PlayerController, Pawn, Character) are fine — fighting the engine is not the goal. The rule governs game systems: prefer one manager ticking an array of state structs over per-NPC AIControllers, Behavior Trees, and Blackboards. See Decision 021.
- **Update the docs when work completes**: After finishing meaningful work, update `02_STATUS.md` (phase, what exists, next steps). Log new decisions in `03_DECISIONS.md`. Record tuned values in the doc that `10_CHECKLIST.md` names for that step. When a decision settles an open question, delete that question from every doc that lists it.
