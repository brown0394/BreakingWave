# BreakingWave — CLAUDE.md

A first-person game about one beach landing, played as a chain of the people who die on it.
UE 5.6 + C++. Two-person team: writer/developer + Claude.

## Loaded every session

- Vision, priorities, what this game is not: @01_SOUL.md
- The 7 principles behind every code and design choice: @04_PRINCIPLES.md
- Current phase, RESUME HERE, the build order, open work: @02_STATUS.md

## Other documents — read when the task calls for it

| File | Holds | Read before |
|------|-------|-------------|
| `03_DECISIONS.md` | Every decision with its reasoning; an index with each entry's state at the top | Proposing anything that might already be decided, and any design discussion. Read the index, then only the entries you need |
| `05_ZONES.md` | Map, zones, heightmap, the trench, character pools | Level layout, terrain, cover placement, the trench; character pools and gaze-crossing; narrative perspective |
| `06_COMBAT.md` | Controls, damage, cover, combat rhythm per zone | Hits, damage, wounded state, corpse cover, ammo |
| `07_CAMERA.md` | Headbob, hit camera, death camera, narrative screen, transition | Movement feel, any camera work, death → narrative → transition |
| `08_ENEMY_AI.md` | MG bunkers, infantry, difficulty curve | Any enemy behaviour |
| `09_ALLY_NPC.md` | Ally personalities, density, corpses, ammo looting | Any ally behaviour, corpses, looting |
| `10_CHECKLIST.md` | The original 8-step development order | Long-range ordering. The current build order is the arc in `02_STATUS.md` |
| `11_ENGINE_NOTES.md` | UE 5.6 traps: headless Python, anim/retarget, camera/input C++, fog, landscape | Editor-Python tools, headless asset work, retargeting, camera/input C++, a landscape re-import |
| `12_ARCHITECTURE.md` | How the code is organised and why; invariants | Writing, restructuring or deciding where to put any system code |

## Commands

PowerShell. `$UE` = `C:\Program Files\Epic Games\UE_5.6\Engine`; `$Proj` = the absolute path of
`BreakingWave.uproject`. Before any build or asset-writing run, check `Get-Process *Unreal*`: a
zombie headless editor breaks both saves (silently) and the link (LNK1104).

- **Build**: `& "$UE\Build\BatchFiles\Build.bat" BreakingWaveEditor Win64 Development -project="$Proj" -WaitMutex`
- **Headless Python** (asset work, no UI): `& "$UE\Binaries\Win64\UnrealEditor-Cmd.exe" "$Proj" -run=pythonscript -script="<absolute .py path>" -stdout -unattended -nosplash`
- **Offscreen full editor** (retargeting and other UI-touching batch ops): same executable with `-ExecutePythonScript="<absolute .py path>" -stdout -unattended -nosplash -RenderOffscreen`. With `/Game/FirstPerson/Lvl_FirstPerson` after `"$Proj"` it opens the level and exits on its own, so read-only level checks (`Tools/VerifyHeightmapReimport.py`) need no user; output is in `Saved/Logs/BreakingWave.log`
- **Placement scripts** (`Tools/Place*.py`, `Tools/CleanDuplicateLandscapes.py`) are editor-only — spawning actors crashes headless — so the user runs them in the open editor
- **Heightmap**: `Tools\GenerateBeachHeightmap.ps1` (no arguments) writes `SourceAssets\BeachHeightmap_1009.png`
- **Playtest analysis**: `Tools\AnalyzePlaytests.bat` — runs the analyzer on UE's bundled Python, where matplotlib is installed; system `python` is only the Store stub. Set `$env:PYTHONIOENCODING = 'utf-8'` first until workstream J fixes the cp949 crash

Headless gotchas: pass `-script` an absolute path (a relative one resolves against the engine's
Binaries folder). Exit code 1 does not mean failure — any asset that logs an error sets it, and
every sound import logs a harmless BINKA-decoder ensure. Judge a run by its Error summary and the
`.uasset` timestamp, verified from a fresh process (`11_ENGINE_NOTES.md`).

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
- **Update the docs when work completes**:
  - `02_STATUS.md` is loaded into every session: **replace** stale lines rather than appending, keep it under ~150 lines, and put the session story in the commit message, not the doc.
  - Log new decisions in `03_DECISIONS.md` and add a row to its index. When a decision supersedes or amends an earlier one, add a one-line note under the earlier entry and update its index row.
  - Record tuned values in the doc that `10_CHECKLIST.md` names for that step.
  - When a decision settles an open question, delete the question from every doc that lists it. Don't leave "SETTLED" or struck-through entries behind; state the result once, where it belongs.
  - Give each fact one home and point to it from elsewhere. Refer to code by symbol name, not file:line.
