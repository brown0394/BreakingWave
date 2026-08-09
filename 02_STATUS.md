# Current Status

> Last updated: 2026-08-09
>
> This document holds the CURRENT state and what's next — nothing else. Session history
> lives in git log; the why of past choices lives in 03_DECISIONS.md; engine gotchas live
> in 11_ENGINE_NOTES.md. When updating, replace stale facts instead of appending below them.

## Phase: Step 3+ — MG live; shared rifle system + Z3 infantry BUILT 2026-08-09 (pulled forward from Step 6, Decisions 035–037; in-editor placement + feel-check pending)

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
- [x] Placeholder audio: /Game/Audio/MGFireLoop (looping) + MGCrack + MGImpact + MGWhizz
  (all synthesized, disk-verified); regenerate via Tools/GenerateMGPlaceholderAudio.py
  (headless OK — pass the script as an ABSOLUTE path or -script resolves against the
  engine Binaries dir; the per-sound BINKA-decoder ensures in the log are harmless)
- [x] Bullet sound feedback (2026-08-06, crack feel-checked GOOD 2026-08-09): world
  impacts within ImpactSoundRadius (50 m) of the player play MGImpact at the impact
  point (ImpactSoundMinInterval throttle caps mixer load at 60 rounds/sec battery-wide,
  pitch jitter for variety); crack volume fades from full at a graze to
  CrackVolumeAtEdge so loudness reads as closeness. All flyby/impact sounds play
  through runtime USoundAttenuation objects built in BeginPlay, so they are properly
  spatialized + distance-attenuated. Knobs in FMGSettings "MG|Audio"
- [x] Passing-bullet whizz, distance-banded with the crack (2026-08-09, user-picked
  banding): CrackRadius back to 300 — the crack is now the ALMOST-HIT signal (within
  3 m of the head); a bullet passing between CrackRadius and WhizzRadius (1500) plays
  the new MGWhizz "phewww" instead (synthesized falling 1500→400 Hz pitch sweep, reads
  as a bullet Doppler-ing past), volume fades from full at the crack boundary to
  WhizzVolumeAtEdge (0.25) at the outer edge, pitch jitter, own runtime attenuation.
  Flyby detection now waits until the bullet's closest approach is KNOWN (passed abeam
  or terminated) before banding — the old per-segment check could fire a tick early and
  would have misfiled real near-hits as whizzes. WhizzSound lives on the manager with a
  BeginPlay fallback-load of /Game/Audio/MGWhizz (no PlaceMGCrew re-run needed).
  Compiled clean via Build.bat; whizz not yet feel-checked. Telemetry: new "whizz" CSV
  event + whizzes= in run summaries + analyzer column (old sessions read 0). CRACK
  COUNTS CHANGE MEANING AGAIN — now the 3 m band only; not comparable with any session
  before 2026-08-09
- [x] Shared rifle system (Decision 035, built 2026-08-09): RifleProfile.h holds FRifleProfile
  (one system, two data rows — player semi-auto/8, infantry bolt/5). Player side lives on
  BreakingWaveCharacter: LMB semi-auto fire (camera-forward + spread cone), RMB aimed fire
  (AimFieldOfView 55 lerp, movement locked while aiming), R reload (mag + dry click,
  infinite reserves — looting deferred), SK_Rifle mesh on the FP arms' HandGrip_R socket,
  template MM_Rifle_Fire/Reload/DryFire played as dynamic montages on "DefaultSlot"
  (UNVERIFIED whether the template FP ABP has that slot — if arms don't animate in PIE,
  that's why; sound still carries). Inputs IA_Fire/IA_AimRifle/IA_Reload wired via
  Tools/AddRifleInput.py (run + disk-verified). Bullet pipeline generalized: FMGBullet
  carries EMGBulletSource (Gun/PlayerRifle/InfantryRifle); player bullets down infantry
  (one hit), kill MG crew through the slit (segment test vs rendered crew, CrewHitRadius),
  pass through sim allies, and skip crack/whizz (your own rounds don't snap at your ear).
  Rung 1 live: player rounds landing within FiredUponAlertRadius (600) of a gun's slit or
  hitting crew set LastFiredUponTime on THAT gun → decaying FiredUponScoreBonus (800 over
  6 s) on the player. PlayerTargetScoreMultiplier (3.0) CODED but UNTUNED — tune together
  with infantry in one batch (Decision 035). Player bullet tracers draw pale blue.
- [x] Z3 enemy infantry (BeachInfantrySystem.h/.cpp, Decision 036, built 2026-08-09):
  AInfantryManager ticks FInfantrySoldierState structs — cover → rise → aim delay →
  1–3 bolt shots → drop → randomized wait, per-soldier variance; flinch layer (player
  impacts within FlinchRadius or a comrade death within ComradeDeathFlinchRadius drop a
  risen soldier early and stretch his wait); targeting = distance × movement bonus ×
  PlayerTargetScoreMultiplier vs player + sim allies, MaxEngagementRange (120 m) as the
  fog stand-in; bolt rifle shots ride the shared bullet pipeline (crack/whizz for free),
  RifleShotEnemy report + delayed RifleBoltCycle clack (the window tell, Decision 037);
  one player hit downs a soldier → persistent ragdoll + comrade flinch. AInfantrySoldier
  is a mannequin shell; anims fallback-load from AnimStarterPack/Retarget (5 infantry
  anims retargeted + IK-baked 2026-08-09, disk-verified). RELOCATION (layer 3) and Z4
  infantry DEFERRED — see Deferred section. F7 debug shows per-soldier phase/target/mag.
  Knobs in FInfantrySettings on the manager. ALL NUMBERS TENTATIVE.
- [x] Rifle placeholder audio (Decision 037): RifleShotPlayer/RifleShotEnemy/RifleBoltCycle/
  RifleDryClick/RifleReload synthesized + imported via Tools/GenerateRifleAudio.py
  (headless OK, disk-verified); character and infantry manager BeginPlay-fallback-load
  them, no BP wiring needed
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
- [x] Tools/AddRifleInput.py — creates IA_Fire/IA_AimRifle/IA_Reload, maps LMB/RMB/R in
  IMC_Default, sets the BP action slots; idempotent, headless OK (already run, disk-verified)
- [x] Tools/GenerateRifleAudio.py — synthesizes + imports the 5 rifle placeholder sounds;
  idempotent, headless OK (already run, disk-verified)
- [x] Tools/RetargetInfantryAnims.py — retargets the 5 ASP infantry-cycle anims reusing the
  prone pass's rigs/retargeter; needs `-ExecutePythonScript` full-editor mode (already run;
  BakeIKBonesFromFK.py re-run after, all 24 Retarget anims baked, disk-verified)
- [ ] Tools/PlaceInfantryPositions.py — WRITTEN, NOT YET RUN (editor-only, spawning crashes
  headless): places 2 seam-lane foxholes + center trench parapets, 7 AInfantrySoldier
  shells, 1 AInfantryManager; idempotent (InfantrySystem tag); edit tables + re-run to move
  positions
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

- [ ] **RESUME HERE — place the infantry, then feel-check the rifle + Z3 firefight**:
  1. In the editor (Lvl_FirstPerson open): run `Tools/PlaceInfantryPositions.py`
     (Tools > Execute Python Script), then SAVE THE LEVEL. Idempotent — edit its tables
     and re-run to move positions
  2. Hit Play — rifle first: LMB hip fire, RMB aim (view narrows, movement locks),
     R reload, empty mag dry-clicks. Shoot at a bunker slit: that gun should turn on
     you within a beat (rung 1). Kill crew through the slit → takeover silence windows.
     KNOWN UNCERTAINTY: FP arm animations may not play if the template FP ABP lacks a
     "DefaultSlot" montage slot — rifle still fires/sounds; report what you see
  3. Then Z3: bolt rifles crack past you with a "phewww" band, each report followed by
     the bolt-cycle clack (his window — move on it). Suppress a foxhole with hip fire:
     the soldier should duck early and stay down longer (flinch). One hit downs a
     soldier; he ragdolls and stays. The seam lanes should now be owned by the foxholes
  4. After the runs: `Tools\AnalyzePlaytests.bat` — new `fired`/`infdn` columns, player
     rifle aggregate line, infantry markers (orange v) on the fire map, player fire
     excluded from the enemy-fire heatmap
  5. **Then, in one batch**: tune PlayerTargetScoreMultiplier (MG) +
     FInfantrySettings.PlayerTargetScoreMultiplier + infantry knobs together — both
     coded, both untuned (Decision 035). Step 3 checklist questions (hit frequency,
     crater survival, stop windows) stay open until being targeted is routine
  6. Debug aids: **F7** = MG + ally + infantry readout; `MGNoDamage` observe mode
     (taints the run), `MGKillCrew` = takeover windows
  Greybox caveats: tracers converge on invisible sim allies (fog + rendered allies fix
  that later); any hit = instant respawn scaffolding, so ALL enemy fire reads ~2× more
  lethal than the final two-shot model; infantry can only be killed by the player's
  rifle (nothing else shoots at them yet); soldiers re-emerge in the same spot until
  the relocation layer is built — pre-aiming a known spot wins, expected, don't tune
  around it. Beach length judgment still deferred: re-judge after the priority tuning
  batch + fog, not before

### Writing backlog
- [ ] Second character ("the one who shook me") narrative writing
- [ ] First enemy character narrative writing

### Deferred until after more system work (decided 2026-07-04; transit times added 2026-07-18)
- [ ] Infantry relocation (layer 3, Decision 036 — user wants this built later, on the
  record): intra-trench sidesteps under concentrated fire, re-emerging from a different
  spot after being shot at, falling back when the player closes, move sounds. Build after
  the first infantry feel-check shows the pre-aim exploit actually biting
- [ ] Z4 infantry — build together with communication-trench geometry and the bunker
  breakthrough design (open question in 08_ENEMY_AI.md)
- [ ] Dug-in foxhole/trench terrain (heightmap) replacing the parapet greybox — visual
  pass, once infantry positions stop moving
- [ ] FP arms rifle montages: if the template FP ABP turns out to have no DefaultSlot,
  either add a Slot node to ABP_FP_Copy (headless graph edit, see 11_ENGINE_NOTES.md) or
  live without arm motion until the visual pass
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

## What Was Done (last session — 2026-08-09, second sitting)

- Whizz feel-checked PASSED (user: whizz and crack read as different). Batch analyzed:
  17 new runs, whizz counts sane (399 whizzes on an 82 s run — longest ever, died in Z4;
  first Z4 deaths on record), core too-safe pattern unchanged (90% of advance while clear)
- User reorder: enemy infantry before more MG work, rifle system first so player and
  infantry share it. Spec /grill-me'd question-by-question → Decisions 035–037
- Built the whole pass, all compiled clean via Build.bat: bullet pipeline faction
  generalization + rung-1 + crew-kill-by-rifle; player rifle (hip/ADS/reload, inputs +
  FP rifle mesh wired, disk-verified); 5 rifle placeholder sounds (synthesized, imported,
  disk-verified); Z3 infantry system (cycle + flinch + targeting + ragdoll); 5 infantry
  anims retargeted + IK-baked (disk-verified); PlaceInfantryPositions.py written (NOT
  yet run — editor-only); telemetry pshot/inf_shot/inf_down events + analyzer columns.
  MG PlayerTargetScoreMultiplier CODED untuned per Decision 035
- REMAINING: run the placement script in-editor + save level, PIE feel-check (order in
  Next Steps), then the one-batch priority + infantry tuning

## What Was Done (2026-08-06)

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
