# Current Status

> Last updated: 2026-08-06
>
> This document holds the CURRENT state and what's next — nothing else. Session history
> lives in git log; the why of past choices lives in 03_DECISIONS.md; engine gotchas live
> in 11_ENGINE_NOTES.md. When updating, replace stale facts instead of appending below them.

## Phase: Step 3 — One MG (systems BUILT 2026-07-19; placement script + feel-check pending)

Spec was /grill-me'd question-by-question 2026-07-19 → Decisions 027–031. Step 3 scaffolding
choices (not durable decisions): any MG hit = bare respawn at the landing craft (Step 4
replaces this — remember the MG reads ~2× more lethal than the final two-shot model),
`MGNoDamage` exec for observation, silence itself signals stops (per-type stop sounds wait
for the sound pass).

All grey-box geometry is placed. Fog and the zone-size walkthrough are DEFERRED until after
more system work (user decision 2026-07-04) — pick them up before tuning zone sizes.

## What Exists

### Documents
- [x] Project mental model (01_SOUL.md)
- [x] Decision log (03_DECISIONS.md) — 33 entries
- [x] Design principles (04_PRINCIPLES.md) — 7 principles
- [x] Beach map v2 (05_ZONES.md) — 5 zones, 3 bunkers, 3 infantry positions, comm trenches
- [x] Combat system design (06_COMBAT.md) — controls, damage, cover, per-zone rhythm
- [x] Camera system design (07_CAMERA.md) — headbob, death, narrative screen, transition
- [x] Enemy AI design (08_ENEMY_AI.md) — MG priority tracking, infantry behavior, difficulty curve
- [x] Ally NPC AI design (09_ALLY_NPC.md) — personality types, density management, corpses, ammo looting
- [x] Development checklist (10_CHECKLIST.md) — 8-phase dev order, document usage guide
- [x] Engine gotchas (11_ENGINE_NOTES.md) — UE 5.6 traps for tooling/headless/anim/camera work
- [x] First character narrative draft (landing craft soldier — writer's original)

### Code (Source/BreakingWave/)
- [x] BreakingWaveCharacter — first-person character on the UE5 FP template: walk 600 /
  sprint 900 (Shift), no jumping by design (BP touch-UI jump nodes deleted in-editor and
  the DoJumpStart/DoJumpEnd shims removed 2026-07-18)
- [x] Prone (Decisions 023 + 025): C toggle, rides engine crouch (instant capsule shrink,
  clearance check on stand-up), camera at ProneEyeHeight above ground. STATIONARY by
  design — movement input ignored while prone and during transitions; look stays live.
  Feel-checked PASSED. Tunables: ProneCapsuleHalfHeight, ProneEyeHeight
- [x] Prone body animation: 8 AnimStarterPack prone anims retargeted to the UE5 mannequin
  (Content/AnimStarterPack/Retarget/). While prone the body plays Prone_Idle_UE5 (correct
  lying shadow) and the FP arms hide; ABP resumes on stand-up. Greybox quirk: arms
  invisible while prone — fine until the shooting/visual pass. Prone_Fire/Reload/Death
  are retargeted and waiting for later systems
- [x] Prone transition animation: gameplay stays instant (capsule/speed change on the
  toggle frame), visuals catch up — body one-shots time-compressed into ProneDropDuration
  (0.35 s, a dive) / ProneStandUpDuration (0.9 s, deliberately slower), FP camera
  smoothstep-blends over the same durations (mid-toggle safe). Feel-checked PASSED
- [x] Debug third-person view (Decision 024): **F6** in PIE (or `DebugThirdPerson` console
  command) toggles a spring-arm orbit camera, un-hides the body mesh, hides the FP arms.
  Dev-builds only. Feel-checked PASSED. Tunable: DebugThirdPersonDistance (400)
- [x] Dive-into-prone: going prone while moving keeps momentum and turns the entry into a
  ballistic dive — LaunchCharacter up at ProneDiveUpwardSpeed (300, ~0.6 s airtime), camera
  HOLDS STANDING EYE HEIGHT during flight, eye drop fires on landing (Landed() override),
  ProneDiveFallGravityScale (2) snaps the fall past the apex, touchdown skids ~1.6 m from
  sprint under SlideDeceleration (2500) then settles below SlideSettleSpeed (60). Body
  one-shot auto-compresses to predicted flight time. Stationary prone entry unchanged.
  Feel-checked DONE 2026-07-17 — good enough to build on; further polish deliberately
  deferred until more systems exist (kept lead if flight ever feels flat: brief additive
  camera pitch-down during flight; bUsePawnControlRotation keeps pitch under the mouse).
  IsSliding() exposed for other systems. All numbers tentative; C++ defaults are live
  (BP has no overrides)
- [x] Headbob (Decision 026): custom UHeadbobShakePattern + UHeadbobCameraShake
  (HeadbobCameraShake.h/.cpp), started once per possession from NotifyControllerChanged
  (single-instance, infinite); resolves the character via GetViewTargetPawn() every update
  so it survives possession changes. Figure-8: vertical sine at footfall rate + half-rate
  lateral sway (LateralRatio 0.4 — set to 0 for the vertical-only motion-sickness
  fallback). Speed-synced amplitude AND frequency (phase-continuous): 1.5 cm / 2.8 Hz at
  600, 2.5 cm / 3.4 Hz at 900, scaling to zero below walk. Stationary = breathing (0.3 cm,
  0.35 Hz); prone = breathing × 0.4; airborne/slide/prone-transition/F6 = still (those
  states own the camera). Amplitudes ease over SmoothingTime (0.2 s) — no pops. Knobs in
  FHeadbobSettings on the character (Category "Camera"). ALL NUMBERS TENTATIVE.
  Feel-checked PASSED 2026-07-18. Tuning note: the FP camera rides the head socket, so
  anims already add real head motion — if the bob ever reads weak, raise
  Walk/SprintAmplitude first
- [x] Locomotion: BS_Idle_Walk_Run rebuilt ALL-RIFLE with foot-true rates (idle rifle;
  300 = jogs ~1.05; 600 fwd = sprint anim ~0.97 — our normal speed is genuinely a run;
  900 fwd = sprint ~1.46; non-fwd = jogs rate-scaled 2.1×/3.2×, may read frantic — rate
  vs foot-slide tradeoff, constants in Tools/RebuildLocomotionAsRifle.py). ik_* bones
  baked (the "legs not moving" fix — see 11_ENGINE_NOTES.md). Feel-checked PASSED
  2026-07-18. Standing idle gap CLOSED same day: ABP_Unarmed's Idle state repointed
  from unarmed MM_Idle to Idle_Rifle_Hip_UE5 (Tools/SetRifleStandingIdle.py,
  disk-verified) — the whole locomotion set is now rifle-carry
- [x] BreakingWaveCameraManager — pitch-limited camera manager stub
- [x] BreakingWaveGameMode / PlayerController — base classes
- [x] MG bunker system (MGBunkerSystem.h/.cpp, Decisions 027–031): AMGBunkerManager ticks
  bunker state structs — 6-man crew w/ takeover + degradation tiers, belt/heat-simulated
  stops + random jam, perception (3-point exposure × muzzle-axis attention × distance →
  awareness w/ 4 s memory), priority ladder w/ broke-cover bonus + switch margin, rotation
  speed limit w/ per-switch jitter, idle scan when unaware of everyone, factor-scaled
  dispersion, projectile bullet array (real travel time; kills sim allies, respawns player,
  sand-impact + tracer debug draw), flyby crack + distance-delayed fire-loop audio.
  AMGBunkerGun is the visual shell (barrel, muzzle, 2 crew mannequins, audio comp).
  ALL NUMBERS TENTATIVE in FMGSettings on the manager. Exec: MGNoDamage, MGKillCrew,
  MGDebug (F7). Fixed 2026-07-25 (first playtest bug): firing/perception/aim-targeting
  now use a new fixed FirePort component (child of the actor root, never rotates)
  instead of the visual Muzzle (child of the rotating Barrel/YawPivot) — the old code
  fired and traced LOS from a point that swings on an arc around a pivot sitting deep
  inside the bunker, so off-center aim (i.e. most of the time) put the muzzle back
  behind the wall, shooting the bunker's own interior. FirePort sits at the same
  rest-position math as before (120cm forward of root = BARREL_LENGTH_CM in
  PlaceMGCrew.py) but stays fixed at the slit opening for every aim angle. Visual
  Muzzle is now cosmetic only (barrel mesh tip, fire-loop audio position) — expected
  to look slightly detached from the true fire point until the placeholder gun mesh
  is replaced. Compiles clean, rebuilt via Build.bat and verified. Priority reworked
  2026-07-30 (Decision 033, after the hang-back exploit): exposed-target score now
  scales with ground speed (MovingTargetScoreBonus / MovingTargetScoreReferenceSpeed —
  a sprinter outranks a nearer prone man) and guns deprioritize targets another gun is
  already working (SharedTargetScorePenalty — the battery splits the wave). Compiled
  clean 2026-07-30; not yet play-tested. Stop-clock desync added 2026-08-02: each gun
  starts with a partial belt (StartingBeltFractionMin) and some heat
  (StartingHeatFractionMax) — first session showed all three guns barrel-changing at
  the same instant (~10 s) because identical clocks all started firing at t=0,
  silencing the whole beach at once instead of opening per-gun windows.
  Target leading added 2026-08-02 (after the second batch of runs): gunners aim ahead
  of a mover by bullet flight time × LeadFraction, rolled in [LeadFractionMin 0.6,
  LeadFractionMax 1.15] per target switch — before this, bullets flew at the target's
  CURRENT position, so flank fire at a 900-speed sprinter missed by a systematic 3.6 m
  at 300 m (0.4 s flight); 7 of 8 recorded hits were the center gun hitting a player
  charging straight at it, and sprint-through defeated the Decision 032 enfilade
  entirely. Lead applies at the aim-target level (UpdateRotation), so the slew limit
  and rotating-dispersion penalty still govern — a close crosser can still outrun the
  barrel. Compiled clean; not yet play-tested.
- [x] Ally simulation (BeachAllySim.h/.cpp, Decision 029): AAllySimManager ticks unrendered
  ally structs — spawn near craft, advance w/ wander, random prone pauses, die to MG fire,
  slot reuse w/ generation counter. Struct shaped for the later full 09_ALLY_NPC.md
  behavior; visual shells come in the ally step. Knobs in FAllySimSettings.
  Pre-warm added 2026-08-02: BeginPlay simulates PreWarmSeconds (60) of assault before
  play, so the player lands mid-wave — fixes the first-session spawn-lock, where an empty
  beach made the player the only target and all three guns killed them at the craft in
  under a second (twice)
- [x] Placeholder audio: /Game/Audio/MGFireLoop (looping) + MGCrack + MGImpact (all
  synthesized, disk-verified); regenerate via Tools/GenerateMGPlaceholderAudio.py
  (headless OK)
- [x] Bullet sound feedback (2026-08-06): world impacts within ImpactSoundRadius (50 m)
  of the player play MGImpact at the impact point (ImpactSoundMinInterval throttle caps
  mixer load at 60 rounds/sec battery-wide, pitch jitter for variety); flyby crack
  reworked — CrackRadius 300 → 1000 (a miss within 10 m of the head now cracks), volume
  fades from full at a graze to CrackVolumeAtEdge at the radius so loudness reads as
  closeness, pitch jitter. Both play through runtime USoundAttenuation objects built in
  BeginPlay, so they are properly spatialized + distance-attenuated (the old crack
  played flat, unspatialized, full volume). Knobs in FMGSettings "MG|Audio". ImpactSound
  lives on the manager — BeginPlay falls back to /Game/Audio/MGImpact if unset, and
  PlaceMGCrew.py wires it on any future re-run (no re-run needed to hear it).
  Compiled clean; not yet feel-checked. Telemetry note: crack counts from sessions
  before 2026-08-06 were recorded at radius 300 — not comparable with new sessions
- [x] Playtest telemetry (PlaytestRecorder.h/.cpp, built 2026-08-02): FPlaytestRecorder
  lives inside AMGBunkerManager and auto-records every PIE session to
  Saved/Playtests/session_<stamp>.csv — settings snapshot (FMGSettings + FAllySimSettings
  dumped via reflection, so tuning changes stay attributable per session), 2 Hz player
  samples (stance/speed/targeted/stopped-gun-count), per-shot / bullet-impact / crack /
  ally-death / player-hit / stop / target-switch events, and zone-crossing splits (Y bands
  from the 05_ZONES.md profile table — Playtest::ZoneBoundariesY, mirrored in
  AnalyzePlaytests.py; update both if zones move). On death: on-screen + log run summary
  (survival time, deepest zone, shots-at-you/hits/cracks, advance while targeted vs clear,
  zone splits). MGNoDamage taints the run so the analyzer excludes it from combat stats.
  Flushes every 5 s and on death/EndPlay; hit=respawn scaffolding means "death" = first
  hit for now

### Tools
- [x] Tools/GenerateBeachHeightmap.ps1 — generates the zone-profiled heightmap
  (SourceAssets/BeachHeightmap_1009.png); re-run after editing its Profile/Dunes/Berm/Craters tables
- [x] Tools/PlaceBeachObstacles.py — in-editor Python; spawns grey-box hedgehogs (Zone 2),
  barbed wire, and debris piles (Zone 3) from editable tables, traced onto the landscape;
  idempotent (re-run clears prior batch)
- [x] Tools/PlaceHeroPieces.py — in-editor Python; assembles the 3 Zone 4 bunkers (hollow,
  sea-facing slit, rear door) and 3 Zone 0 landing craft (open hull + dropped ramp) from
  SM_Cube; idempotent (GreyboxHero tag), pieces stay individually tweakable
- [x] Tools/AddProneInput.py — creates IA_Prone, maps PRONE_KEY_NAME (currently C) in
  IMC_Default replacing any old prone key, sets the BP's ProneAction slot; idempotent;
  edit PRONE_KEY_NAME and re-run to change the key (headless OK)
- [x] Tools/RetargetProneAnims.py — builds IK rigs + UE4→UE5 retargeter, retargets all 8
  AnimStarterPack prone anims, sets the BP's ProneBodyIdleAnim; idempotent (already run).
  Needs `-ExecutePythonScript` full-editor mode
- [x] Tools/SetProneTransitionAnims.py — sets the BP's StandToProneAnim/ProneToStandAnim
  slots; idempotent, headless OK (already run)
- [x] Tools/AddSprintToLocomotion.py — SUPERSEDED by RebuildLocomotionAsRifle.py (kept for history)
- [x] Tools/RebuildLocomotionAsRifle.py — retargets the ASP rifle locomotion set and
  rebuilds ALL of BS_Idle_Walk_Run rifle-carry with foot-true rate scales (authored-speed
  constants in the tool); idempotent; needs `-ExecutePythonScript` full-editor mode
  (already run, disk-verified)
- [x] Tools/BakeIKBonesFromFK.py — bakes FK transforms onto the ik_* helper bones in every
  Retarget anim; **MUST re-run after any new IK retarget** (see 11_ENGINE_NOTES.md);
  idempotent, headless OK (already run on all 14 anims, disk-verified)
- [x] Tools/SetRifleStandingIdle.py — repoints ABP_Unarmed's standing Idle state node
  (Locomotion → Idle → AnimGraphNode_SequencePlayer_1) from MM_Idle to
  Idle_Rifle_Hip_UE5, recompiles + saves the ABP; idempotent, headless OK (already run,
  disk-verified)
- [x] Source/BreakingWave/BlendSpaceTool.* — editor-only C++ for headless blendspace work:
  RebuildRuntimeTriangulation, DescribeRuntimeTriangulation, DescribeBlendOutputAt
  (evaluates blends exactly like the runtime — verify without PIE)
- [x] Tools/GenerateMGPlaceholderAudio.py — synthesizes + imports the MG fire loop and
  supersonic crack WAVs; idempotent, headless OK (already run, disk-verified)
- [x] Tools/PlaceMGCrew.py — spawns an MGBunkerGun into every GUN_BUNKERS entry (barrel
  through slit, crew mannequins, sounds wired) + the two manager actors. All THREE bunkers
  manned since 2026-07-30 (Decision 033): flanks toed in 30° (yaw −60/−120, Decision 032
  enfilade — arcs interlock over the center), center bunker sea-facing (−90, depth 6 m
  handled per-entry); drop the Center entry and re-run to A/B two guns vs three. Gun root
  derived so the FirePort sits just outside the slit plane at any yaw (arc-edge shots can't
  clip the slit jambs). One MGBunkerManager drives all guns (auto-discovers at BeginPlay).
  Idempotent (MGSystem tag); EDITOR-ONLY (spawning crashes headless). Re-run with 3 guns
  done 2026-08-02 (viewport check + level save still pending).
- [x] Tools/AnalyzePlaytests.py (+ AnalyzePlaytests.bat) — offline analyzer for
  Saved/Playtests: per-run table, aggregates (median survival, zone-split medians, hit
  rate, advance-while-targeted vs clear), settings diffs between sessions, and
  analysis.png maps (player paths + deaths, MG fire concentration heatmap, ally-death
  heatmap). No editor needed; the .bat runs it on the UE-bundled Python (system Python is
  only the Store stub; matplotlib pip-installed --user for the UE Python 2026-08-02)

### Level
- [x] Beach heightmap imported at scale 100/100/200 (Decision 022) — zone-profiled terrain
  with tactical relief (dunes, berm, wavy bluff), confirmed in editor
- [x] Shell craters baked in and re-imported (3 deep cover + 10 shallow dressing, raised rims)
- [x] Zone 2/3 obstacles placed: 22 hedgehogs, 151 wire posts, 20 debris blocks
  (PlaceBeachObstacles.py — edit tables, re-run)
- [x] Hero pieces placed: 3 bunkers (Zone 4), 3 landing craft with ramps (Zone 0)
  (PlaceHeroPieces.py — same workflow)
- [ ] Fog — DEFERRED until after more system work (do before judging zone sizes)
- [ ] Duplicate Landscape parent actors cleanup — pending (see Deferred)

## Next Steps

- [ ] **RESUME HERE — feel-check the new bullet sounds, then decide the player-priority
  knob**:
  1. Hit Play, run the beach — listen for impact thuds landing around you and directional
     cracks on near misses (crack loudness now reads as closeness; radius is 10 m).
     Tune in FMGSettings "MG|Audio" if impacts spam or cracks feel weak
  2. After the runs: `Tools\AnalyzePlaytests.bat` (crack counts will jump vs pre-2026-08-06
     sessions — radius widened 300 → 1000, not comparable)
  3. **Pending decision (data ready)**: player-priority knob. Two batches show the player
     targeted only 4–17% of the time, 95% of fire going at sim allies, 87% of ground
     gained while clear — the shared root cause of "too easy" and "not intense"
     (2026-08-06 analysis). Next code change unless the sound pass shifts the read
  4. Step 3 checklist questions (hit frequency, crater survival, stop windows) stay open
     until the priority fix makes being targeted routine
  5. Debug aids: **F7** = debug readout; console `MGNoDamage` = observe without dying
     (taints the run — analyzer excludes it), `MGKillCrew` = takeover windows
  6. Tune in the Details panel: FMGSettings on MGBunkerManager, FAllySimSettings on
     AllySimManager (each session CSV snapshots its settings). Record tuned values in
     08_ENEMY_AI.md when it feels right, per the checklist
  Greybox caveats: tracers converge on invisible sim allies (fog + rendered allies fix
  that later — don't judge it now); any hit = instant respawn scaffolding, so the MG reads
  ~2× more lethal than the final two-shot model — discount accordingly when tuning.
  Known-by-design: the seam between bunkers close to the defense line sits outside every
  gun's 55° slit arc (verified in run data 2026-08-06) — enemy infantry in Z3
  (foxholes/trench, 05_ZONES.md) owns that band later; do not widen MG arcs for it.
  Beach length judgment also deferred: 35–54 s Z0→Z4 was measured while free-sprinting
  96% of the time — re-judge after the priority fix + fog, not before

### Writing backlog
- [ ] Second character ("the one who shook me") narrative writing
- [ ] First enemy character narrative writing

### Deferred until after more system work (decided 2026-07-04; transit times added 2026-07-18)
- [ ] Fog setup — do this before judging zone sizes; fog is load-bearing.
  ExponentialHeightFog, tune by eye for ~35 m visibility: Density ~0.5,
  Height Falloff ~0.05, Start Distance ~500
- [ ] Walk through the grey-box and judge zone sizes; adjust the heightmap Profile table if needed
- [ ] Run through each zone and record transit times in 05_ZONES.md (Step 2's last open
  item) — do together with the fog walkthrough so the times aren't measured twice
- [ ] Clean up duplicate Landscape parent actors: the level has three (`Landscape`,
  `Landscape2`, `Landscape4`, all at −50400/−50400) — re-import leftovers. Check in the
  editor which one owns the 64 streaming proxies and delete the empty ones
- [ ] Dive-into-prone polish (closed as good-enough 2026-07-17): if the flight ever feels
  flat, the lead is a brief additive camera pitch-down during flight

### Hero piece coordinate sheet (driven by PlaceHeroPieces.py tables — kept for reference)
The landscape is CENTERED on the world origin: its min corner sits at world (−50400, −50400),
so world = profile-meters × 100 − 50400 on both axes. Profile coords below with world uu in parens.
- Landing craft (Zone 0, ramp faces +Y inland): A-left 230/270 (−27400, −23400), B-center 510/270 (600, −23400), C-right 790/270 (28600, −23400)
- Bunkers (Zone 4, slit faces −Y sea): MG-left 200/620 (−30400, 11600), MG-center 510/635 (600, 13100), MG-right 800/620 (29600, 11600)

## What Was Done (last session — 2026-08-06)

- Third telemetry batch analyzed (session_20260806_212149, 4 runs, target leading live).
  Leading VERIFIED: kills now come from all three guns including both flanks (previously
  7 of 8 hits were center-gun-only), deaths land in Z3. User's felt reads all confirmed
  by data:
  - **Blind spot**: run 4 spent 8.5 s outside every gun's traverse arc at the seam
    between the center and left bunkers near the defense line (x ≈ −8500..−7300,
    upper Z3 and beyond). Geometric: SlitArcHalfAngleDeg 55 + 30° toe-in can't cover
    the near-line seams. Ruled by-design — Z3 enemy infantry owns that band later
  - **Too easy / not intense (same root cause)**: player targeted 4–17% of the time,
    95% of MG fire at sim allies, 87% of ground gained while clear, sprinting 93–97%
    of every run, cracks 1–7 per run. Player-priority knob now justified (the decision
    deferred on 2026-08-02 finding 2 — two batches, same pattern)
  - **Distance feels short**: measured under free-sprint — hold terrain, re-judge after
    priority fix + fog (zone sizing already booked to the fog walkthrough)
  - Guns also spend ~40% of life stopped (avg 1.2 of 3 down; belt and heat cycle at
    similar periods) — deliberately untouched until being targeted matters
- Built the bullet sound layer (user priority for intensity): MGImpact placeholder +
  world-impact playback + crack rework with distance-scaled volume and spatialization
  (see What Exists). Compiled clean via Build.bat; MGImpact.uasset disk-verified from a
  fresh process. Player-priority knob NOT yet implemented — next code change
- REMAINING: feel-check sounds in PIE, then implement the priority knob and re-batch
