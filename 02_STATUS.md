# Current Status

> Last updated: 2026-07-04

## Phase: Step 1 geometry DONE → Step 2 — First-Person Movement

All grey-box geometry is placed. Fog and the zone-size walkthrough are DEFERRED until after
more system work (user decision 2026-07-04) — pick them up before tuning zone sizes.

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
- [x] Tools/GenerateBeachHeightmap.ps1 — generates the zone-profiled heightmap (SourceAssets/BeachHeightmap_1009.png); re-run after editing its Profile/Dunes/Berm/Craters tables
- [x] Tools/PlaceBeachObstacles.py — in-editor Python; spawns grey-box hedgehogs (Zone 2), barbed wire, and debris piles (Zone 3) from editable tables, traced onto the landscape; idempotent (re-run clears prior batch). Requires PythonScriptPlugin + EditorScriptingUtilities (now enabled in .uproject)
- [x] Tools/PlaceHeroPieces.py — in-editor Python; assembles the 3 Zone 4 bunkers (hollow, sea-facing slit, rear door) and 3 Zone 0 landing craft (open hull + dropped ramp) from SM_Cube; idempotent (GreyboxHero tag, per-assembly subfolders), pieces stay individually tweakable

### Level
- [x] Beach heightmap imported at scale 100/100/200 (Decision 022) — zone-profiled terrain with tactical relief (dunes, berm, wavy bluff) confirmed looking right in editor
- [x] Shell craters baked into heightmap and re-imported (Craters table: 3 deep cover craters + 10 shallow dressing craters, raised rims)
- [x] Zone 2/3 obstacles placed: 22 hedgehogs (X-crosses), 151 wire posts, 20 debris blocks via Tools/PlaceBeachObstacles.py — edit its tables and re-run to adjust (idempotent)
- [x] Hero pieces placed: 3 bunkers (Zone 4), 3 landing craft with ramps (Zone 0) via Tools/PlaceHeroPieces.py — same table/re-run workflow
- [ ] Fog — DEFERRED until after more system work (do before judging zone sizes)
- [ ] Duplicate Landscape parent actors cleanup (Landscape/Landscape2/Landscape4) — pending

## Next Steps

- [ ] **RESUME HERE — Step 2: first-person movement** on BreakingWaveCharacter (read
  04_PRINCIPLES.md + 07_CAMERA.md headbob section first):
  1. Prone (instant drop to ground)
  2. Slide-into-prone (momentum slide)
  3. Headbob (primarily vertical bounce, small amplitude)
- [ ] **Step 2**: Run through each zone and record transit times in 05_ZONES.md

### Deferred until after more system work (decided 2026-07-04)
- [ ] Fog setup (values below) — do this before judging zone sizes; fog is load-bearing
- [ ] Walk through the grey-box and judge zone sizes; adjust the heightmap Profile table if needed
- [ ] Clean up duplicate Landscape parent actors: the level has three (`Landscape`, `Landscape2`,
  `Landscape4`, all at −50400/−50400) — likely leftovers from heightmap re-imports. Check in the
  editor which one owns the 64 streaming proxies and delete the empty ones.

### Hero piece coordinate sheet (now driven by PlaceHeroPieces.py tables — kept for reference)
The landscape is CENTERED on the world origin: its min corner sits at world (−50400, −50400),
so world = profile-meters × 100 − 50400 on both axes. Profile coords below with world uu in parens.
- Landing craft (Zone 0, ramp faces +Y inland): A-left 230/270 (−27400, −23400), B-center 510/270 (600, −23400), C-right 790/270 (28600, −23400)
- Bunkers (Zone 4, slit faces −Y sea): MG-left 200/620 (−30400, 11600), command 510/635 (600, 13100), MG-right 800/620 (29600, 11600)
- Fog (ExponentialHeightFog, tune by eye for ~35 m visibility): Density ~0.5, Height Falloff ~0.05, Start Distance ~500
- [ ] Second character ("the one who shook me") narrative writing
- [ ] First enemy character narrative writing

## What Was Done (since last update)

- Grey-box geometry COMPLETE: user re-imported the crater heightmap, re-ran the obstacle tool
  (X-cross + bounds-snap fixes), ran the hero-piece tool — everything placed and looks fine
- Fog + zone-size walkthrough deferred until after more system work (user decision) — next
  up is Step 2 movement (prone, slide-to-prone, headbob)
- Reworked craters (user feedback: too small, too organized, too few): Craters table now has
  3 deep cover craters + 10 shallow bombardment-dressing craters, varied sizes, irregular
  spread bleeding into Zone 0/2 edges, raised rims (CraterRim* constants). PNG regenerated
  and probe-verified (biggest crater 2.3 m rim-to-bottom) — needs re-import
- Wrote Tools/PlaceHeroPieces.py (user request — no bunker/craft assets exist; grey-box
  assemblies built from SM_Cube). Bunkers: hollow, slit 1.2–1.6 m in the sea wall, rear door,
  embedded 0.5 m. Craft: open hull, gunwales, stern, flat dropped-ramp slab on its own trace
- Found + fixed two placement bugs in PlaceBeachObstacles.py (needs re-run): Rotator arg order
  is (roll, pitch, yaw) so hedgehogs stood as leaning posts instead of X-crosses; and SM_Cube's
  pivot is at its CORNER, so raw-pivot spawns sat offset — both scripts now snap pieces by
  actual world bounds (pivot/rotation agnostic)
- User ran the obstacle tool in-editor: 22 hedgehogs, 151 wire posts, 20 debris blocks placed
- FIXED the PlaceBeachObstacles.py line-trace bug (two root causes, found via headless
  `UnrealEditor-Cmd -run=pythonscript` diagnostics, no editor session needed):
  1. UE 5.6 Python exposes NO attributes on HitResult (`hasattr(hit, "impact_point")` is
     always False) — `to_tuple()` is the only way to read it. Field 0 is blocking_hit; the
     first Vector field is the hit location.
  2. The landscape is centered on the origin (min corner −50400, −50400), not starting at it —
     table coords are now re-based onto the auto-detected landscape min corner.
  Also switched the trace context to the editor world (landscape-actor context proved flaky).
  Trace-verified headless: all 193 placement points hit ground at profile-correct heights
  (hedgehogs z 5.2–8.8 m, wire 10.1–12.2 m, debris 13.8–20.9 m). Spawning itself can't run
  headless (editor-only API) — needs one in-editor run.
- Found 3 duplicate Landscape parent actors in the level (re-import leftovers) — cleanup queued

### Earlier (2026-06-27)
- Baked 2 Zone 1 shell craters into GenerateBeachHeightmap.ps1 (Craters table, inverted gaussians) and regenerated SourceAssets/BeachHeightmap_1009.png — needs re-import in editor
- Enabled PythonScriptPlugin + EditorScriptingUtilities in BreakingWave.uproject
- Wrote Tools/PlaceBeachObstacles.py — data-table-driven grey-box placement (Zone 2 hedgehogs + barbed wire, Zone 3 debris), idempotent via GreyboxObstacle tag + GreyboxObstacles folder
- Hybrid greybox approach chosen: tool places bulk/repeated obstacles, hero pieces (bunkers, landing craft) hand-placed
- BLOCKED: placement tool's line trace returns None for all points — debugging in progress (see Next Steps)

### Earlier (2026-06-11)
- Added tactical relief to GenerateBeachHeightmap.ps1: 3 sightline-blocking dunes in Zone 3, shingle berm with gaps at the Zone 2/3 boundary, bluff line wavering ±15 m laterally; Zone 1 stays flat by design
- Settled landscape vertical scale at Z=200 after walkthrough — profile-true heights read flat in first person (Decision 022). Script tables converted to world meters at that scale
- Updated 05_ZONES.md heightmap section with world heights and relief features
