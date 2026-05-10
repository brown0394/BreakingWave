# BreakingWave — CLAUDE.md

## Project Summary

**THE SHORE** — A first-person narrative war game built with Unreal Engine 5 + C++.
One beach, one day of war. The player is not a hero; they are each person who was there, on both sides.
Death is not failure — it opens the next pair of eyes. The chain of gazes is the core loop.

Two-person team: writer/developer + Ash (Claude). Scope is intentionally small. Finish over feature.

---

## Document Index

| File | Contents |
|------|----------|
| `01_SOUL.md` | Game vision, core emotions, confirmed decisions, priorities, open questions |
| `02_STATUS.md` | Current phase, what exists, what's done, next steps |
| `03_DECISIONS.md` | 20 logged decisions with reasoning (why, not just what) |
| `04_PRINCIPLES.md` | 7 design principles — the "why" behind code and design choices |
| `05_ZONES.md` | Beach map, 5-zone layout, character pool structure, gaze-crossing examples |
| `06_COMBAT.md` | Player controls, damage model, cover system, combat rhythm per zone |
| `07_CAMERA.md` | Headbob, hit camera, death camera, narrative screen, transition system |
| `08_ENEMY_AI.md` | MG bunker AI (priority targeting, accuracy, stops), infantry AI, difficulty curve |
| `09_ALLY_NPC.md` | Allied NPC personality types, density/corpse management, ammo looting |
| `10_CHECKLIST.md` | 8-phase development order with per-step goals and document assignments |

---

## When to Read Each Document

### Always read at the start of every session
- **`01_SOUL.md`** — Without this, all direction is lost. Core premise, priorities, what this game is NOT.
- **`02_STATUS.md`** — Current phase and next steps. Never assume; always check.

### Read once at the start of a new session (design alignment)
- **`04_PRINCIPLES.md`** — 7 principles that govern every code and design choice. Read first if starting fresh work.

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

---

## Key Constraints to Remember

- **No UI**: No health bar, crosshair, ammo counter, or minimap. Sensation only.
- **No invincibility**: Not even on character transition. Enemy targeting delay instead.
- **No weapon looting**: Corpses are people, not resource nodes. Ammo only, when empty.
- **No help mechanic**: Wounded NPCs call for help. The player cannot respond. Intentional.
- **Death is not retry**: The death → narrative → transition sequence must be seamless, no loading.
- **Two-shot damage**: Headshot kills instantly. Torso takes two hits. Wounded state does not recover.
- **Fog is load-bearing**: Removes time sense + limits visibility + enables NPC spawn/despawn outside view.
- **Code readability is priority 3**: Function and variable names reveal intent. Comments explain why, not what.

---

## Current Phase

**Step 1 — Grey Box Beach** (In Progress)

Terrain heightmap exists. Next: place grey-box geometry (craters, hedgehogs, berms, bunkers, landing craft, fog).
See `10_CHECKLIST.md` Step 1 and `02_STATUS.md` for exact status.
