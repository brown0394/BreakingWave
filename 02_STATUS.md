# Current Status

> Last updated: 2026-06-11

## Phase: Step 1 — Grey Box Beach (In Progress)

Code exists. Terrain exists (heightmap-generated grey landscape). Grey-box detailing in progress.

## What Exists

### Documents
- [x] Project mental model (01_SOUL.md)
- [x] Decision log (03_DECISIONS.md) — 22 entries
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
- [x] Beach heightmap imported at scale 100/100/200 (Decision 022) — zone-profiled terrain with tactical relief (dunes, berm, wavy bluff) confirmed looking right in editor
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

- Added tactical relief to GenerateBeachHeightmap.ps1: 3 sightline-blocking dunes in Zone 3, shingle berm with gaps at the Zone 2/3 boundary, bluff line wavering ±15 m laterally; Zone 1 stays flat by design
- Settled landscape vertical scale at Z=200 after walkthrough — profile-true heights read flat in first person (Decision 022). Script tables converted to world meters at that scale; PNG unchanged byte-for-byte, no re-import needed
- Updated 05_ZONES.md heightmap section with world heights and relief features
