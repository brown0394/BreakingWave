# Current Status

> Last updated: 2026-06-10

## Phase: Step 1 — Grey Box Beach (In Progress)

Code exists. Terrain exists (heightmap-generated grey landscape). Grey-box detailing in progress.

## What Exists

### Documents
- [x] Project mental model (01_SOUL.md)
- [x] Decision log (03_DECISIONS.md) — 21 entries
- [x] Design principles (04_PRINCIPLES.md) — 7 principles
- [x] Beach map v2 (05_ZONES.md) — 5 zones, 3 bunkers, 3 infantry positions, comm trenches
- [x] Combat system design (06_COMBAT.md) — controls, damage, cover, per-zone rhythm
- [x] Camera system design (07_CAMERA.md) — headbob, death, narrative screen, transition
- [x] Enemy AI design (08_ENEMY_AI.md) — MG priority tracking, infantry behavior, difficulty curve
- [x] Ally NPC AI design (09_ALLY_NPC.md) — personality types, density management, corpses, ammo looting
- [x] Development checklist (10_CHECKLIST.md) — 8-phase dev order, document usage guide
- [x] First character narrative draft (landing craft soldier — writer's original)

### Code (Source/BreakingWave/)
- [x] UE5 project created (first-person template base)
- [x] BreakingWaveCharacter — base first-person character (walk + sprint)
- [x] BreakingWaveCameraManager — pitch-limited camera manager stub
- [x] BreakingWaveGameMode / PlayerController — base classes

### Tools
- [x] Tools/GenerateBeachHeightmap.ps1 — generates the zone-profiled heightmap (SourceAssets/BeachHeightmap_1009.png); re-run after editing its Profile table

### Level
- [x] Beach heightmap imported — grey landscape exists (old version; zone-profiled replacement generated, not yet imported)
- [ ] Zone 0–4 geometry, obstacles, bunkers not yet placed
- [ ] Fog not yet configured

## Next Steps

- [ ] **Step 1 (current)**: Place grey-box geometry on the terrain — craters, hedgehogs, berms, bunker boxes, landing craft, fog
- [ ] Walk through finished grey-box and judge zone sizes
- [ ] **Step 2**: Add prone, slide-to-prone, headbob to BreakingWaveCharacter
- [ ] **Step 2**: Run through each zone and record transit times in 05_ZONES.md
- [ ] Second character ("the one who shook me") narrative writing
- [ ] First enemy character narrative writing

## What Was Done (since last update)

- Deleted Variant_Shooter template code and content (unused UE template exploration; recoverable from git history)
- Deleted 00_CLAUDE_GUIDE.md (obsolete — CLAUDE.md now covers collaboration)
- Logged Decision 021 (data-oriented NPC managers, not per-NPC AIControllers) and updated 08/09 implementation notes to match
- Doc cleanup: removed open questions already settled by decisions, aligned comment rule in 04_PRINCIPLES.md with CLAUDE.md, removed the "Ash" naming, added tunable-values rule and doc-update rule to CLAUDE.md
