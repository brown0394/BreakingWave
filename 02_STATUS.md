# Current Status

> Last updated: 2026-08-29
>
> This document holds the CURRENT state and what's next — nothing else. Session history
> lives in git log; the why of past choices lives in 03_DECISIONS.md; engine gotchas live
> in 11_ENGINE_NOTES.md. When updating, replace stale facts instead of appending below them.

## Phase: THE BEACH ARC — workstream A (fog) is DONE: visibility measured and settled at ~35 m. Workstream B (landscape carve + duplicate-Landscape cleanup) is next. NO C++ WRITTEN FOR THIS ARC YET

The 2026-08-22 direction change stands: **tuning detail on a beach whose systems do not
exist yet is measuring absence, not balance.** Decisions 042–047 set the shape of the arc
that fixes that. The 2026-08-29 session **finished the grilling** (Q9–Q27, 19 questions) and
logged **Decisions 048–055**, which settle every branch that was left open — ally supply,
the trench, fire discipline, personalities, the enemy garrison, and the build order itself.

In one sentence each, the new eight:

- **048** — no density director; allies come only from craft, **7–9 of them ~75 m apart**, in
  **boatloads** of ~25–30, `MaxAlive` ~300. The per-zone density curve becomes an outcome we
  measure, not a rule the game enforces.
- **049** — craft **sail in, ground, drop the ramp, disgorge and back off**; takeover
  manufacture is kept forever as an **invisible** safety net (it fires behind a black screen).
- **050** — `DespawnY` is removed; allies **break into the trench and fight there**, sections
  fall locally and are retaken, and the **global ending stays the player's**.
- **051** — continuous **fire trench at y ≈ 600**, bunkers as strongpoints behind it, **three
  communication trenches**, Zone 3 positions **stay forward on the MG seam lanes** with saps.
- **052** — the trench is a **hybrid**: a 6 m × 1.2 m heightmap carve plus scripted parapet;
  a hand-authored **centerline table** is the one artifact and the graph derives from it.
- **053** — blocked fire is **not a state**; oscillation needs no rule (the blocking relation
  is geometrically asymmetric); the player gets a **muzzle lift**.
- **054** — ally accuracy is a **data row** (~0.5–1% at Z3); personalities land **in full**;
  cover is one flat array in which **corpses register as cover points**.
- **055** — the defense is **strongpoints, thin at the seams** (~90 men); the arc is built
  **terrain-first**, and Decision 043's merge exits on **parity**.

**Two measurements from the 2026-08-22 CSVs drove most of this**, and they overturned the
working assumption. Attrition is *not* what empties the top of the beach: 175 ally deaths in
194.5 s is 0.90/s against a 1.5/s ceiling, so ~40% of men walk off the top alive. The
emptiness is **lateral** — ally deaths form three columns with **130–140 m of frontage
containing essentially nobody**, and the measured 90% play band (x 463→797) sits across one
of those gaps.

All grey-box geometry is placed. Fog now sets **four** numbers (visibility, `TakeoverRadius`,
the render bubble, and `FInfantrySettings`' perception cap) and goes first for that reason.

## What Exists

### Documents
- [x] Project mental model (01_SOUL.md)
- [x] Decision log (03_DECISIONS.md) — 55 entries
- [x] Design principles (04_PRINCIPLES.md) — 7 principles
- [x] Beach map v2 (05_ZONES.md) — 5 zones, 3 bunkers, 3 infantry positions, comm trenches
- [x] Combat system design (06_COMBAT.md) — controls, damage, cover, per-zone rhythm
- [x] Camera system design (07_CAMERA.md) — headbob, death, narrative screen, transition
- [x] Enemy AI design (08_ENEMY_AI.md) — MG priority tracking, infantry behavior, difficulty curve
- [x] Ally NPC AI design (09_ALLY_NPC.md) — personality types, density management, corpses, ammo looting
- [x] Development checklist (10_CHECKLIST.md) — 8-phase dev order, document usage guide
- [x] Engine gotchas (11_ENGINE_NOTES.md) — UE 5.6 traps for tooling/headless/anim/camera work
- [x] Code architecture (12_ARCHITECTURE.md) — manager/state-array pattern, engine boundary, shared bullet
      pipeline, ID-based cross-system refs, editor-Python authoring layer, telemetry; plus what the
      architecture deliberately is NOT, where it strains, and the invariants to preserve
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
  **`MaxAlive` 32 → 128 on 2026-08-17 (Decision 041, NOT yet run)**: 32 was a Step 3
  placeholder that left a median of ONE live ally in a band spanning the whole beach width,
  against `09_ALLY_NPC.md` §118's spec of 8–12 within visible range — the root cause of the
  260 m lateral takeover jump. **Superseded by Decision 048**: `MaxAlive` goes to
  ~300, fog-edge top-up spawning is struck outright, and the per-zone curve becomes an
  outcome produced by boatload arrival rather than a target the game maintains. The 2026-08-22
  batch showed 32 → 128 thickened the waterline and changed nothing at the top, because the
  problem is lateral: three columns, 130–140 m of empty frontage between them. Watch the frame cost: `AdvanceAlly` is one ground
  trace per ally per tick and `EvaluatePerception` runs per gun over the whole array (cheap
  early-outs first, then 3 exposure traces), so this is ~4× the perception work. If it
  bites, slice perception across ticks rather than shrinking the wave.
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
- [x] **Death → takeover loop (Decisions 038–040, built 2026-08-16, FEEL-CHECKED PASSED
  2026-08-17 — one defect open, the lateral takeover jump; see Next Steps).**
  Lives on `ABreakingWavePlayerController` as a phase machine ticked in `PlayerTick`:
  DeathShake (0.3 s) → DeathDescend (1.2 s) → DeathHold (0.5 s) → FadeOut (0.5 s) →
  [narrative screen, currently zero duration] → takeover → FadeIn (1.25 s) → control.
  Knobs in `FTransitionSettings`. ALL NUMBERS TENTATIVE.
  - Death camera is a spawned `ACameraActor` driven by scripted math, never the ragdoll:
    decaying rattle during the shake phase, ease-out descent to `GroundClearance` (18 cm)
    above a downward-traced floor, roll to `TiltDegrees` (22°) on the side the round was
    travelling. Fade uses `StartCameraFade` with `bFadeAudio` — the audio fade the spec
    wanted, for one bool, no submix work
  - Selection **reworked 2026-08-17 (Decision 041, NOT yet run)**:
    `AAllySimManager::AcquireTakeoverAlly` takes a 2D death anchor and returns a live ally
    inside a disc of `TakeoverRadius` (3500 uu ≈ 35 m, **must track fog visibility**), no
    candidate further forward than `TakeoverForwardReach` (+20 m), random among them. Empty
    disc → `ManufactureTakeoverAlly` spawns a normal `FSimAlly` at a random angle on the
    disc edge (Y-clamped forward), which nobody can see happen (`09_ALLY_NPC.md` §119).
    Guarantees: 8 angle retries → disc grows ×1.5 for 3 steps → no free slot evicts the live
    ally furthest from the anchor. A placement counts as valid only if its ground is within
    `TakeoverMaxGroundStep` (300 uu) of the ground you died on, or the downward trace will
    put a man on a bunker roof. Never on your own corpse; the unreachable total failure
    holds on black and logs an Error. Still runs at the END of the fade, so the man is alive
    when taken. **The rear-expansion ladder and `TakeoverRearStep` are deleted.**
  - A NEW PAWN is spawned and possessed per life. The old one becomes a ragdoll corpse
    (`BecomeCorpse`), capped by `MaxCorpses` (8, EditAnywhere), oldest retired first;
    ragdoll ignores ECC_Pawn so corpses can't wall you in. **Corpse weight pass 2026-08-17**
    (first PIE observation — the body sailed too far and read weightless): the ragdoll was
    inheriting the mesh's full kinematic sprint velocity, so `FCorpseSettings` on the
    character now keeps only `MomentumRetained` (0.2) of it, adds a `DropSpeed` (150) downward
    kick at the moment of death, and applies `LinearDamping` (0.75) / `AngularDamping` (4) to
    every body. ALL FOUR TENTATIVE — `MomentumRetained` is the lever for distance,
    `AngularDamping` for the windmilling
  - Takeover state: control rotation from the ally's `HeadingYaw`, `Crouch()` if he was
    prone, forced forward input through the fade if he was advancing, magazine
    `RandRange(TakeoverMagRoundsMin 3, MagazineSize)`. Input is locked for the whole
    fade-in. **Simplification on the record**: the ally's exact `Speed` (300–550) is NOT
    inherited — you advance at `WalkSpeed`. Nobody can see an unrendered ally, so there is
    no continuity to break; revisit only if the mid-stride handover reads wrong
  - Targeting delay clocked from pawn spawn, `FadeInSeconds + TargetingDelaySeconds`
    (~2.75 s total, ~1.5 s of it after control). MGs: awareness zeroed and accrual blocked
    (`CanAcquirePlayer`), so they forget the spot and must re-acquire through the normal
    perception ramp. Infantry: flat exclusion via `IsTargetAlive`. Bullets in flight stay live
- [x] **Two-shot damage + mesh-authoritative hits (Decision 039, built 2026-08-16,
  FEEL-CHECKED PASSED 2026-08-17)**: head bone = instant death, everything else wounds,
  second wound kills
  (`WoundsToKill`). `ABreakingWaveCharacter::TraceBody` does a mesh-bounds broadphase then
  `LineTraceComponent` against the physics asset; `HeadBones` (default `head`) classifies.
  The capsule is no longer a combat volume — **prone is now a real 180 cm body on the
  ground and genuinely more exposed to the flanking guns than it has ever been**. The mesh
  is forced to `AlwaysTickPoseAndRefreshBones` and gets `PA_Mannequin` if it has no physics
  asset; if it still has no bodies, `bBodyTraceUnavailable` falls back to the old capsule
  test and logs an Error — the failure mode is a warning, never a silently immortal player.
  Tell is the hit moment only: `UHitCameraShake` (directional push + decaying rattle,
  knobs in `FHitShakeSettings`) plus `/Game/Audio/PlayerPain`. Vignette, blur, aim sway,
  speed drop and wounded headbob deliberately deferred
- [x] Playtest telemetry (PlaytestRecorder.h/.cpp, built 2026-08-02; restructured 2026-08-16): FPlaytestRecorder
  lives inside AMGBunkerManager and auto-records every PIE session to
  Saved/Playtests/session_<stamp>.csv — settings snapshot (FMGSettings + FAllySimSettings
  dumped via reflection, so tuning changes stay attributable per session), 2 Hz player
  samples (stance/speed/targeted/stopped-gun-count), per-shot / bullet-impact / crack /
  ally-death / player-hit / stop / target-switch events, and zone-crossing splits (Y bands
  from the 05_ZONES.md profile table — Playtest::ZoneBoundariesY, mirrored in
  AnalyzePlaytests.py; update both if zones move). On death: on-screen + log run summary
  (survival time, deepest zone, shots-at-you/hits/cracks, advance while targeted vs clear,
  zone splits). MGNoDamage taints the run so the analyzer excludes it from combat stats.
  Flushes every 5 s and on death/EndPlay.
  RESTRUCTURED 2026-08-16 — the unit is now a LIFE, with a session track on top, because
  death chains into a takeover instead of resetting to the craft:
  - CSV column and events renamed `run`→`life` (`life_start` / `life_abort`); life summary
    gained `starty`, `ground`, `wounds`, `head`
  - New `takeover` event: death Y, chosen ally X/Y, and — **reworked 2026-08-17 for
    Decision 041** — `made` (was a man manufactured?), `disc_allies` (real candidates the
    disc held) and `dist` (metres to the man you became). The `ladder` column is gone with
    the ladder; `given_back` stays and remains comparable
  - New `session_end` event: lives, takeovers, first/best Y, net advance, total give-back,
    and (2026-08-17) `made`/`made_frac` in place of the ladder means — the ratchet question
    and the density question in one row
  - 2 Hz sample gained `wounds` and (2026-08-17) `disc_allies`, live allies inside a
    takeover disc around the player's current position — replaces `slab_allies`, which
    stopped meaning anything when the slab did
  - `hit_player` gained `bone` and `head`
  - `FInfantrySettings` now in the settings snapshot, so infantry tuning is attributable
  - All three 2026-08-11 known gaps are closed by the above

### Tools
- [x] **Tools/PlaceFog.py — run, fixed and settled (2026-09-05).** In-editor Python; tunes the
  level's `ExponentialHeightFog` from a knob table (density, falloff, max opacity, start
  distance, colour, volumetric off) plus an optional **calibration ruler** — a
  `FogRange_STAND_HERE` post at profile 510/320 and man-height posts at 10/20/30/40/50/60/80/100 m
  inland, which is how the ~35 m number was measured rather than guessed. It **adopts** the one
  fog actor the renderer draws and destroys redundant ones, rather than spawning alongside;
  markers carry `BeachFogMarker` and the fog carries `BeachFog`, and `clear_previous` sweeps
  both. Fog properties go through a defensive `try_set` that **warns instead of aborting** if a
  5.6 property name drifted. Set `PLACE_RANGE_MARKERS = False` and re-run to clear the ruler
  (already done). Re-run it to change the fog; the four traps it exposed are in
  `11_ENGINE_NOTES.md`
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
- [x] Tools/GeneratePlayerPainAudio.py — synthesizes + imports the pain grunt
  (/Game/Audio/PlayerPain); idempotent, headless OK. RUN 2026-08-17, disk-verified
  (0.45 s voiced grunt + breath). The commandlet exits 1 on the harmless BINKA-decoder
  ensure, as every sound import does — judge it by the .uasset timestamp, not the exit code
- [x] Tools/RetargetInfantryAnims.py — retargets the 5 ASP infantry-cycle anims reusing the
  prone pass's rigs/retargeter; needs `-ExecutePythonScript` full-editor mode (already run;
  BakeIKBonesFromFK.py re-run after, all 24 Retarget anims baked, disk-verified)
- [x] Tools/PlaceInfantryPositions.py — RUN IN-EDITOR 2026-08-11 (editor-only, spawning
  crashes headless): places 2 seam-lane foxholes + center trench parapets, 7
  AInfantrySoldier shells, 1 AInfantryManager; idempotent (InfantrySystem tag); edit tables
  + re-run to move positions. Confirmed live by telemetry — all 7 soldiers fired during the
  2026-08-11 session. **LEVEL SAVE VERIFIED 2026-08-16** from the one-file-per-actor files
  on disk (`Content/__ExternalActors__/FirstPerson/Lvl_FirstPerson/`, written 2026-08-11
  22:15): 7 InfantrySoldier + 1 InfantryManager + 12 parapet pieces, all committed. The
  actors survive an editor restart; no re-run needed
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
- [x] Fog — **settled at ~35 m visibility** (density 1.0, falloff 0.05, max opacity 1.0, start
  distance 500 cm). One `ExponentialHeightFog` in the level, adopted from the FirstPerson
  template's; calibration ruler placed, walked and removed. Revisited once after allies render
- [ ] Duplicate Landscape parent actors cleanup — pending (see Deferred)

## Next Steps

### RESUME HERE — workstream A is done; B (landscape) is next

**Fog is settled at ~35 m** (2026-09-05). `Tools/PlaceFog.py` now runs clean, the calibration
ruler has been removed, and the level is saved carrying exactly one `ExponentialHeightFog`:
density **1.0**, height falloff 0.05, max opacity 1.0, start distance 500 cm.

The number was measured rather than guessed. Standing at profile 510/320 in Zone 1 looking
inland, density 0.5 left the 60 m post readable and fading around 70 m — about 2× too clear —
and doubling the density halved the distance onto the target, as exponential extinction says
it should.

`FAllySimSettings.TakeoverRadius` was **already** `3500.f` (35 m), so Decision 041's
requirement that it track fog visibility holds with no code change.

**Three script defects were found and fixed getting there** (all logged in `11_ENGINE_NOTES.md`,
commits `7a1baaa` and `81fe91b`): the landscape corner was resolved with
`isinstance(unreal.Landscape)`, which misses all 64 streaming proxies and picked a degenerate
duplicate parent, so every marker traced off-map; the script spawned a *second* height fog
beside the template's, and the renderer only ever draws `ExponentialFogs[0]`, so the tuned fog
was never the drawn one; and `volumetric_fog` is spelled `enable_volumetric_fog`.

**Still open from workstream A**, carried forward rather than done:

- [ ] **Revisit the fog once** after allies render (Decision 055). The real test is "can I see
  the man I am about to become", which needs Decision 042's render bubble. ~35 m is good
  enough to build against, not final
- [ ] **Judge the zone sizes on foot** — a Step 2 leftover that was meant to ride along with
  the fog walkthrough and did not happen. Needs another PIE walk
- [ ] **Record zone transit times into `05_ZONES.md`** — the same, and Step 2's last open item

**Do NOT blanket-propagate the fog number.** The three constants called "fog" are three
different distances — `TakeoverRadius` 3500 (35 m), `FInfantrySettings.MaxEngagementRange`
12000 (120 m), `FMGSettings.VisibilityMaxRange` 50000 (500 m). Only the first tracks player
visibility. Setting the MG's vision to 35 m **deletes the Zone 1 kill zone**, which sits
230–310 m from the bunkers and is where `05_ZONES.md` says most deaths happen. The render
bubble (Decision 042) does not exist yet and cannot be set until the merge lands.

**State on entry**: branch `decision-041-takeover-disc`. Master is behind by everything from
`9d08d2f` on. **No C++ has been written for this arc**; B is the next thing to touch, and the
two landmines in the code-health checklist below are the cheapest things to clear before E.


### The build order (Decision 055) — terrain-first, in this sequence

Only workstream **E** has no playable state. Everything before it is verifiable in PIE with
today's systems still running.

- [x] **A — Fog.** Settled at **~35 m**: density **1.0**, height falloff 0.05, start distance
  500 cm, one `ExponentialHeightFog` in the level. `TakeoverRadius` already matched at 3500 uu.
  The render bubble (042) and `FInfantrySettings`' perception cap still await the merge.
  **Revisit once** after allies render. The two Step 2 leftovers — judge zone sizes, record
  zone transit times into `05_ZONES.md` — did **not** happen and are still open
- [ ] **B — Landscape.** Add the trench channel (6 m wide × 1.2 m deep) to
  `GenerateBeachHeightmap.ps1`, regenerate, re-import. **Clean up the three duplicate
  Landscape parent actors in the same pass** — check which owns the 64 streaming proxies
- [ ] **C — Trench geometry + nodes.** Hand-author the centerline table; one script emits
  parapet, firing steps, saps to the three outposts, and tagged `TrenchNode` actors
  (~115–120). In-editor run plus a level save, as with the infantry pass
- [ ] **D — Craft.** 7–9 hulls in `LANDING_CRAFT`; arrival/ground/ramp/disgorge/depart cycle;
  boatload spawning replaces the 1.5/s trickle; `MaxAlive` → ~300
- [ ] **E — Decision 043 merge. NOTHING IS PLAYABLE WHILE THIS IS IN FLIGHT.** ~850 lines
  across both soldier systems. **Exit condition is parity: the game plays exactly as it does
  today, on one soldier system**, measured against existing telemetry. `BeachAllySim` is
  retained as a switchable, still-runnable debug path (Decision 043)
- [ ] **F — Graph movement.** A* over the node graph, node reservation, infantry relocation
  (Decision 036's layer 3, arriving here), MG crew reinforcement up the comm trench (047)
- [ ] **G — Fire.** Ally fire with zone gating and `AllyRifle` collision row (044), muzzle-
  flash conspicuity applying to the player too (044), symmetric fire discipline (045),
  blocked-as-failed-check + node reservation (053), player muzzle lift (053), ally spread
  row (054)
- [ ] **H — Population.** Ally corpses with a cap (**prerequisite — allies leave no body
  today**), the flat cover array with corpses registering as cover points, the four
  personality types, Leader influence on Frozen, Leader shouts (054)
- [ ] **I — The line.** ~90 defenders in strongpoints thin at the seams (055), ally
  breakthrough into the trench, local section fall and retake (050)
- [ ] **J — Telemetry.** Fix the `disc_allies` bug (the 2 Hz sample calls `CountAlliesInDisc`,
  which does **not** apply the `TakeoverForwardReach` filter, while the takeover row's
  `disc_allies` does — two meanings under one name, and the sampled figure overstates
  candidates). Add `FInfantrySettings` to the settings snapshot, record live ally positions,
  fix "run" = one life breaking once deaths chain, and fix `AnalyzePlaytests.bat` crashing on
  a cp949 console at the AGGREGATES em-dash (run with `PYTHONIOENCODING=utf-8` meanwhile)

**Definition of done for the arc** (Decision 055): the player can advance up a seam lane,
fight into the fire trench, and reach a bunker's rear door. Allies visibly fight and die
alongside, and their fire pulls the guns off him. Enemy infantry relocate along the trench
and MG crews reinforce up the communication trench. A bunker can go quiet because allies cut
its communication trench, and the enemy can retake it. Telemetry records enough to tune any
of it. **Not in this arc**: the ending, the high ground, the enemy-playable side.

### Code-health checklist (architecture read, 2026-09-05)

Found by reading the implementations against Decisions 042–055. **Nothing here is broken
today** — the counts are small enough that all of it disappears into the frame. It is listed
because items 8–11 are products of numbers workstreams D and I triple, and items 1–2 are
one-line changes now that become multi-hour debugging sessions if they are found after the
merge. Each is tagged with the workstream it should ride with.

**Landmines — fix BEFORE E, they are cheap now and expensive after**

- [ ] **Split `KillAlly` into `KillAlly` / `ClaimAlly`.** `BreakingWavePlayerController.cpp:240`
  calls `AllySim->KillAlly(Slot)` to remove the man you take over — the same function
  `MGBunkerSystem.cpp:935` calls when a round kills someone. Harmless while `KillAlly` only
  flips `bAlive`. **Blocks H**: the moment ally corpses exist (Decision 054's named
  prerequisite), every takeover drops a body at your feet and you open your eyes standing on
  your predecessor
- [ ] **Un-gate soldier behaviour from the shell.** `BeachInfantrySystem.cpp:121` early-returns
  on `!Soldier.Shell.IsValid()`, so a soldier with no mesh does nothing at all. Decision 042's
  entire premise is that behaviour is independent of rendering — this is one condition sitting
  directly under the merge

**Cheap and independent — no dependency on the arc, each verifiable on its own**

- [ ] **Stagger the MG perception clocks.** `FMGBunkerState::EvalTimer` defaults to `0.f`
  (`MGBunkerSystem.h:346`) and `BeginPlay` never randomises it, so all three guns run their
  0.4 s perception tick — and all of its traces — on the same frame. This is the same bug
  class as the 2026-08-02 stop-clock desync (`StartingBeltFractionMin` /
  `StartingHeatFractionMax`), which exists because identical clocks starting at t=0 made all
  three guns barrel-change at once. One `FRandRange` in `BeginPlay`
- [ ] **Rate-limit the ally ground trace, and give it query params.** `AdvanceAlly` →
  `TraceGroundZ` (`BeachAllySim.cpp:186`) runs a 100 m trace per ally per tick: ~7,700/s at
  `MaxAlive` 128, ~18,000/s at D's 300. An ally moves ≤9 cm per frame on smooth landscape.
  Separately the trace passes **no `FCollisionQueryParams`** and uses `ECC_Visibility`, so it
  snaps allies onto bunkers, beach obstacles and the player — the same failure Decision 041
  patched on the takeover path with `TakeoverMaxGroundStep`, which the walking path never got
- [ ] **Move `CountAlliesInDisc` behind the 2 Hz gate.** `MGBunkerSystem.cpp:242` computes it
  every tick; `FPlaytestRecorder::SamplePlayer` only writes it past `SampleIntervalSeconds`.
  A full-array scan per frame, ~97% discarded, for telemetry. **Pairs with J**, which already
  fixes the *semantics* of the same field (the sample omits the `TakeoverForwardReach` filter
  that the takeover row applies) — do both in one edit
- [ ] **Promote the scoring scale into `FMGSettings`.** `100.f`, `0.25f/0.75f`, the
  unseen-target floor `5.f` and the broke-cover `500.f` are literals
  (`MGBunkerSystem.cpp:421–437`), while `FiredUponScoreBonus` is a `UPROPERTY` whose comment
  reads "sized to outrank the broke-cover bonus (500)". The tunable and the thing it is
  balanced against live in different files, one of them uneditable — and only the tunable
  reaches the settings snapshot. Direct miss against CLAUDE.md's "never scattered literals".
  Same shape at `BeachInfantrySystem.cpp:316`
- [ ] **Randomise or round-robin the shared-target scan order.** `IsTargetedByAnotherGun`
  reads the other guns' already-committed choices, so bunker 0 always picks freely and
  bunker 2 always eats `SharedTargetScorePenalty`. Deterministic battery priority that
  nobody chose — an artifact of iterating the array in index order

**Design into E rather than porting — the merge standardises whichever shape it inherits**

- [ ] **Give soldier target selection a cheap gate before the LOS trace.**
  `SelectSoldierTarget` (`BeachInfantrySystem.cpp:302`) traces once per candidate with only a
  range check in front of it. The MG rejects most of the beach first with the slit-arc test at
  `MGBunkerSystem.cpp:352` — one `FindDeltaAngleDegrees`, no trace. Infantry have no
  equivalent. At I's ~90 defenders against D's ~300 allies that is ~27,000 line traces per
  evaluation round, every 0.4 s
- [ ] **Add a broadphase to bullet-vs-soldier.** `UpdateBullets` iterates the full ally array
  per bullet with only `bAlive` as an early-out — no AABB reject against the travel segment.
  ~12,800 segment-segment tests per frame today, and **both** multiplicands grow at once:
  D raises the population, G gives allies rifles
- [ ] **Slice perception across ticks** rather than shrinking the wave — already the recorded
  intent in this document's ally-sim entry; E is where it becomes structural
- [ ] **Size the awareness array safely.** `MGBunkerSystem.cpp:135` sets
  `AwarenessSlots = AllySim->GetSettings().MaxAlive + 1` at `BeginPlay`, and `AwarenessFor`
  has no bounds check. Fine while the ally array is fixed-size at `BeginPlay`; silently
  out-of-bounds the moment **D**'s boatload arrival wants to grow it at runtime

**Watch, no action yet**

- [ ] **The 60 s pre-warm is a synchronous load hitch.** `AAllySimManager::BeginPlay` runs 240
  `SimulateStep` iterations, each walking the living population with its ground trace —
  roughly 10k traces on the game thread at level load, scaling linearly with `MaxAlive`.
  Fixing the trace rate above mostly removes this; re-measure after D
- [ ] **The controller caches nothing.** `TryTakeover` runs three separate `TActorIterator`
  sweeps per death while every manager caches its peers at `BeginPlay`. Free at
  once-per-40-seconds — listed as an inconsistency, not a cost

### Numbers still open (all tentative, tune after the systems exist)

Exact craft count and arrival cadence; boatload size; final `MaxAlive`; the trench centerline
route; ally corpse cap; every spread value. The fog distance is **no longer open** — settled
at ~35 m on 2026-09-05, to be revisited once after allies render.

### Explicitly OFF the list

- **The MG + infantry `PlayerTargetScoreMultiplier` tuning batch is cancelled**, not
  deferred. Decision 044's muzzle-flash bonus makes the player's score situational rather
  than a flat 3×, and Decisions 043–055 change every input to it
- **Do not tune against the 2026-08-22 or earlier batches.** Decision 047 attaches this
  explicitly, and Decisions 048–055 change ally supply, ally lethality, the defense line and
  the terrain. Those numbers are a record of a different game
- **The wounded presentation stays unbuilt.** First wound → death is median **0.17 s**, 9 of
  12 under one second. At that interval a vignette or blur is never seen. The lever, if a
  wounded *phase* is ever wanted, is MG burst discipline
- **Takeover density tuning is on hold** — Decision 049 keeps manufacture permanently and
  Decision 048 changes every input to the disc

### Kept from the 2026-08-22 batch — the last comparable measurement before everything changes

- **Decision 041 verified**: takeover distance median 27 m / max 35 m (hard cap); lateral
  median 7.9 m; give-back median 18.8 m, max 29 m, 3 of 12 forward. Zero "no valid ground at
  any radius", zero "no usable physics bodies"
- **Ally deaths are three columns, not a curve**: upper beach, 36 men at profile x 175–275,
  **1** at 300–440, 40 at 450–550, **0** at 560–690, 20 at 700–800. The gaps are the MG seam
  lanes — the exact ground the Zone 3 outposts cover and no ally has ever walked
- **Attrition is 0.90 ally deaths/s** over 194.5 s against a 1.5/s spawn ceiling; only **12 of
  175** deaths occur above profile y 575
- **Prone handover: 0 of 12.** Steady-state prone should be ~20%, so p ≈ 7%. Not yet a bug —
  watch it, and if the next batch is also 0 it is real

### Writing backlog
- [ ] Second character ("the one who shook me") narrative writing
- [ ] First enemy character narrative writing

### Deferred until after more system work (decided 2026-07-04; transit times added 2026-07-18)
- [x] ~~Infantry relocation (layer 3, Decision 036)~~ — **NO LONGER DEFERRED**: folded into
  Decision 046. A soldier who can walk the trench graph can sidestep under fire and
  re-emerge elsewhere, so relocation falls out of the navigation work rather than being a
  separate layer. The pre-aim exploit Decision 036 predicted is visible in the 2026-08-22
  batch, which was the stated trigger
- [x] ~~Z4 infantry — build with communication-trench geometry and the breakthrough design~~
  — **NO LONGER DEFERRED**: this IS the 2026-08-22 arc. The breakthrough question is settled
  (Decision 046: you go through the trench)
- [x] ~~Dug-in foxhole/trench terrain replacing the parapet greybox~~ — **NO LONGER
  DEFERRED**: this is workstreams B and C above. Decision 052 settles how it is authored
  (6 m × 1.2 m heightmap carve + scripted parapet), because box walls on grade would leave a
  man's head at grade + 1.7 m and Decision 045's exposure gain would evaporate
- [ ] FP arms rifle montages: if the template FP ABP turns out to have no DefaultSlot,
  either add a Slot node to ABP_FP_Copy (headless graph edit, see 11_ENGINE_NOTES.md) or
  live without arm motion until the visual pass
- [x] ~~Fog setup~~ — **NO LONGER DEFERRED**: workstream A above, and it goes FIRST
  (Decision 055). It sets **four** numbers, not three — visibility, `TakeoverRadius` (041),
  the render bubble (042), and `FInfantrySettings`' perception cap, whose comment already
  says "fog stand-in… until real fog owns the vision cap"
- [x] ~~Walk through the grey-box and judge zone sizes~~ and ~~record transit times~~ — both
  folded into workstream A's single walkthrough
- [x] ~~Clean up duplicate Landscape parent actors~~ (`Landscape`, `Landscape2`, `Landscape4`,
  all at −50400/−50400) — folded into workstream B's re-import, so a fourth is not added
- [ ] Dive-into-prone polish (closed as good-enough 2026-07-17): if the flight ever feels
  flat, the lead is a brief additive camera pitch-down during flight

### Hero piece coordinate sheet (driven by PlaceHeroPieces.py tables — kept for reference)
The landscape is CENTERED on the world origin: its min corner sits at world (−50400, −50400),
so world = profile-meters × 100 − 50400 on both axes. Profile coords below with world uu in parens.
- Landing craft (Zone 0, ramp faces +Y inland): A-left 230/270 (−27400, −23400), B-center 510/270 (600, −23400), C-right 790/270 (28600, −23400)
- Bunkers (Zone 4, slit faces −Y sea): MG-left 200/620 (−30400, 11600), MG-center 510/635 (600, 13100), MG-right 800/620 (29600, 11600)

## What Was Done (2026-09-05)

**Workstream A closed: the fog number exists.** ~35 m, measured on foot against the
range-marker ruler rather than guessed, at density 1.0.

- **Three defects in `Tools/PlaceFog.py`**, none of which the script reported as failure.
  `isinstance(unreal.Landscape)` misses `ALandscapeStreamingProxy` (a sibling, not a subclass)
  and so saw only the three duplicate Landscape parents, returning degenerate bounds — every
  range marker traced off the map. The script spawned a second `ExponentialHeightFog` beside
  the template's, and the renderer hardcodes `Scene->ExponentialFogs[0]` with plain append
  ordering, so the tuned fog was configured and never drawn. And `volumetric_fog` is
  `enable_volumetric_fog` — UHT strips the leading `b`
- **A fourth was self-inflicted and caught before it cost anything**: splitting markers onto
  their own tag orphaned the previous run's nine posts, which would have stayed on the beach
  permanently once `PLACE_RANGE_MARKERS` went False
- `place_fog` now **adopts** the level's single fog actor and destroys redundant ones, rather
  than spawning alongside. All four traps are in `11_ENGINE_NOTES.md`, which gained a **Fog**
  section — the landscape-corner one will bite every future placement script
- **No code change was needed for Decision 041**: `TakeoverRadius` was already 3500 uu
- Docs: the 30-vs-40 question is deleted from `09_ALLY_NPC.md`, `10_CHECKLIST.md` and this
  document's open-numbers list

## What Was Done (2026-08-29)

**The grilling that 2026-08-22 left unfinished is finished.** 19 questions (Q9–Q27),
**Decisions 048–055 logged**, every open branch closed, and the build order settled. Still
**nothing built** — by design; workstream A starts now.

- **Q9's premise was wrong, and reading the code found it.** The inherited worry was that
  rendered allies would make takeover *manufacture* visible — a man appearing from nothing
  35 m away. But `TryTakeover()` runs at the **end of the FadeOut phase with the screen fully
  black**, and `KillAlly(Slot)` consumes the manufactured slot in the same synchronous call.
  The man never survives a frame and never renders. Manufacture is kept permanently
  (Decision 049); what it actually signalled was a density hole, closed upstream instead
- **The density hole was measured, not guessed, and the working assumption was wrong.**
  Attrition is fine: 175 ally deaths / 194.5 s = 0.90/s against a 1.5/s ceiling, ~40% of men
  survive to the top. The emptiness is **lateral** — three columns with 130–140 m of frontage
  holding nobody, and the play band sits in a gap. `MaxAlive` 128 also **binds before a single
  death** (transit ~120 s at ~325 uu/s wants 180 men)
- **The seam lanes turned out to be the hinge of three decisions at once.** The ally-free gaps
  (x 300–440, 560–690) are exactly where `PlaceInfantryPositions.py` sites its outposts, and
  exactly Decision 034's by-design MG blind seams. Decision 048 sends allies up those lanes
  for the first time; Decision 051 keeps the outposts there; Decision 055 thins the garrison
  there. The result is a real choice: **a machine gun up the bunker lane, riflemen up the seam**
- **Geometry verified before adopting, in the style of Decision 045**: a fire trench at
  y 600 clears the flank guns' sightline by **+3.3 m** and the centre gun's by **+3.8 m**, and
  fire-trench muzzles clear a standing outpost head by **+2.6 m**. A trench carve is viable at
  6 m × 1.2 m but **not** at 2 m — the landscape is 1 pixel per metre
- **Two prerequisites surfaced that were on nobody's list**: sim allies **leave no corpse**
  (`KillAlly` just clears `bAlive`), which Decision 054 makes load-bearing because corpses are
  cover; and Decision 046's node estimate was an order of magnitude low (~120, not "a few
  dozen")
- Docs updated: `09_ALLY_NPC.md` §118–138 fog-edge top-up **struck**, two open questions
  closed; `05_ZONES.md` craft 3 → 7–9; `08_ENEMY_AI.md` infantry count settled at ~90 with
  strongpoint layout; Decisions 045 and 046 amended in place

## What Was Done (2026-08-22)

Two PIE sessions of Decision 041 (14 lives, 12 takeovers), analyzed; then a direction change
and a grilling session that produced Decisions 042–047. **No code was written.**

- **PIE'd Decision 041 and analyzed the batch.** The fix works — full numbers under Next
  Steps. The analysis also found the ally-density picture (dense at the waterline, empty at
  the defense line, and it is a spawn-geography problem rather than a `MaxAlive` problem),
  a `disc_allies` telemetry inconsistency, and a cp949 crash in `AnalyzePlaytests.bat`
- **The user redirected the project**: stop improving detail while the systems are half
  built. Allies cannot shoot, enemy infantry are seven static men, there is no trench and no
  navigation. The queued `PlayerTargetScoreMultiplier` tuning batch was cancelled as a result
- **Grilled the arc through eight questions** and logged **Decisions 042–047** in
  `03_DECISIONS.md`. Facts established along the way, all verified in code or telemetry:
  - `FSimAlly` is nine floats with no weapon — ally fire has been specified in
    `09_ALLY_NPC.md` since the docs were written and never built
  - `09_ALLY_NPC.md` §70's promise that ally fire draws MG priority **had no mechanism**:
    `FiredUponScoreBonus` only triggers within `FiredUponAlertRadius` = 600 uu (6 m) of a gun
  - **The project has no navigation system at all** — no navmesh, no `AIController`, no
    pathfinding. Infantry never move; allies dead-reckon
  - **The MGs already shoot over their own infantry everywhere on the beach** (+5.9 m
    clearance firing at Z1, +2.3 m at Z3, +1.0 m at a target just behind the line), computed
    from the `05_ZONES.md` profile and `PlaceHeroPieces.py` slit heights. Decision 045's full
    symmetry is affordable on existing terrain
  - The bunkers already have `back_door_left` / `back_door_right` for trench access, built by
    `PlaceHeroPieces.py` and never usable by anything
  - Enemy shells are found by iterating **placed level actors** (`Soldier.Shell = *It`), which
    is why 40–60 trench infantry needs Decision 043's merge
  - The player ranges across all three bunker lanes: 90% of play in a 334 m band, full range
    221→834 profile m, so the defense line has ~600 m of frontage
- **The grilling was cut short at Q9** and is unfinished — see RESUME HERE

## What Was Done (2026-08-17)

Feel-check completed (all green, see Next Steps) and the **first life/session telemetry
batch** analyzed: `session_20260817_155711` / `_155901` / `_160746` — 26 lives, 23
takeovers, 324 s of play. The corpse weight pass and the pain grunt shipped earlier the
same day.

- **The ratchet works, and the beach got crossed.** Net advance +448 / +331 / +346 m per
  session; deepest reach **714 m** (past the bunker line), against a previous all-time best
  of 437 m under bare respawn. Session 2's fifteen lives climbed 272 → 603 m and then
  **stalled in a 90 m band at the defense line for eight consecutive lives** — the push
  breaking on the strongpoint, which is the shape the design wants
- **Decision 038's rule verified end-to-end from telemetry**: `ally_y = death_y −
  given_back`; ladder bands land exactly on 20 / 40 / 60–80 m. The ±20 m slab held someone
  on **17 of 23** takeovers; median give-back **9 m**; 8 takeovers were net *forward*. The
  6 expansions all cost 66–78 m. **Y is solved. X is not** — see Next Steps
- **Two-shot damage confirmed working and reframed.** 4 headshots in 25 deaths (16%); every
  other death took exactly two wounds. But the median gap between wound and death is
  **0.40 s** — the wounded state is a transition, not a phase
- **Mesh-authoritative hits confirmed by the bone log**: 48 hits across 19 distinct bones —
  pelvis 7, lowerarm_l 6, spine_05 6, calf_l 4, head 4, down to hand_l and foot_r.
  **38% of all wounds are hand / foot / forearm / calf**, each worth exactly as much as a
  spine hit (Decision 039, no limb tier — recorded here as an observation, not a re-open)
- **Transition is now a third of the session.** Death → pawn spawn measures 2.50 s
  (0.3 + 1.2 + 0.5 + 0.5, exactly as specced) plus the 1.25 s locked fade-in = **3.75 s to
  control**. Session 2: 14 takeovers × 3.75 s = 52 s of a 149 s session (**35%** with no
  control) against a **median life of 4.9 s** — you spend nearly as long dead as alive. The
  leash feel-checked fine, so the lever, if any is wanted, is longer lives not shorter fades
- **The too-safe pattern is dead for good**: advance while targeted 34–37% per session,
  deaths spread across all five zones
- **Prone has been abandoned by the player**: 20 of 539 samples (3.7%, ~10 s across the
  whole batch). Mesh hits made it genuinely exposed and it feel-checked as still worth
  doing — but it is not being used. Watch it after the tuning batch
- **Telemetry gap found and FIXED same day**: `hit_player` carries the shooter in the `gun`
  column but `AnalyzePlaytests.py` counted every hit as MG. Now split, and the boundary is a
  named constant (`Playtest::InfantryShooterIdBase`) instead of three scattered `1000`s that
  the analyzer silently mirrored. Corrected figures for this batch: **MG 2.14%
  (98 of 4,571), infantry 29.41% (5 of 17)**

Then the lateral defect was grilled question-by-question (16 of them) and the whole fix
built and compiled clean — **Decision 041**, none of it yet run in PIE:

- `BeachAllySim`: `AcquireTakeoverAlly` (disc + forward clamp) replaces `SelectTakeoverSlot`,
  `ManufactureTakeoverAlly` + `SpawnAllyAt` + `FindReusableSlot` (eviction) added,
  `InitialiseAlly` factored out so both spawn paths cannot drift apart, `CountAlliesInDisc`
  replaces `CountAlliesInSlab`, ladder and `TakeoverRearStep` deleted, `MaxAlive` 32 → 128
- PlayerController: `DeathAnchorY` → a 2D `DeathAnchor`; the no-candidate branch now logs an
  Error naming the anchor instead of silently holding on black
- Telemetry + analyzer reworked as above; verified to still read all twelve older sessions

## What Was Done (2026-08-16)

Grilling resumed at Q5 and ran to Q12; the whole death→transition spec is now settled
(**Decision 038 completed, plus Decisions 039 and 040**). Then the entire pass was built
and compiled clean:

- Ally sim: `DespawnY` 5000 → 15600 (the old line sat *between* the foxholes at profile
  550 and the trench at 560 — allies were evaporating inside the enemy position);
  `SelectTakeoverSlot` ladder + `CountAlliesInSlab`
- Character: wound counter, mesh-authoritative `TraceBody`, `TakeBulletHit`,
  `BecomeCorpse`, `ApplyTakeoverState`, transition input lock + forced advance
- New `HitCameraShake.h/.cpp`; new `Tools/GeneratePlayerPainAudio.py` (NOT yet run)
- PlayerController: the whole phase machine, death camera actor, corpse ring buffer
- MG + infantry: takeover notification, awareness lockout / flat exclusion
- **Bug fixed on the way**: both managers cached the player in a `TWeakObjectPtr` that
  only re-resolved when it went *invalid*. With corpses now lingering in the world, every
  gun and every soldier would have kept tracking and shooting a corpse forever. Both
  now resolve the live player each call and treat a dead one as no target. The
  `PlayerSpawnTransform` bare-respawn path is deleted
- Telemetry restructured (life/session split, takeover event, ladder steps, bone names,
  slab counts, FInfantrySettings snapshot); `AnalyzePlaytests.py` updated and verified to
  still read all nine older sessions (takeover columns show `-`)

Compiled clean via UnrealBuildTool. **None of it has been run in PIE.**

## What Was Done (2026-08-11)

Fourth telemetry batch analyzed (session_20260811_221225, 24 runs, first session with
`PlayerTargetScoreMultiplier` 3.0 live, infantry placed). Three findings:

- **The priority knob works, and it overshoots.** 1,269 rounds aimed at the player in one
  session — more than all eight prior sessions combined (1,163). Advance while targeted
  went 7% → **32%** (405 m targeted vs 880 m clear); the "too safe / 90% of ground gained
  while clear" pattern from three straight batches is gone. Hit rate FELL to 1.8% (from
  3.2%) — volume is doing the killing, not accuracy. Deaths moved down the beach: best run
  of the night 24.4 s / Z2, against 82.4 s / Z4 and several 33–40 s / Z3 runs in the two
  prior batches. Zone-1 split times are unchanged (~5 s), so movement is the same; only
  where you die changed. **The knob is NOT tuned yet** — this is the untuned 3.0 default,
  and Decision 035 still holds: tune it with the infantry knobs in one batch
- **Runs 1–10 are a respawn death-spiral, an artifact, not difficulty.** Ten consecutive
  deaths in 0.0–0.5 s, all at y = 273 m (the craft), targeted 100% from t=0. Runs 6 and 10
  died at 0.01 s and 0.02 s with *zero* shots fired at the player during those runs — they
  were killed by rounds already in flight from the previous life. Bare respawn returns the
  player to the spot the guns are already laid on, with no targeting delay, and
  `SharedTargetScorePenalty` 0.5 cannot split the battery against a 3.0× multiplier, so all
  three guns converge: 27–30 rounds arrived inside half a second. This is what pivoted the
  plan to Steps 4+5 (Decision 038) — the cure is specced in 07_CAMERA.md §4 and lives
  inside the transition, so it should not be patched onto the scaffolding
- **The infantry and rifle feel-checks DID NOT HAPPEN.** The system is alive — all 7
  soldiers fired, 66 bolt shots, cycle works — but **every one of those shots was at a sim
  ally** (no `tgt=-1` rows), zero infantry hits on the player, zero downed. Pure geometry:
  soldiers sit at profile y 550–560 and `MaxEngagementRange` is 120 m, so they open up at
  y ≈ 430–440 m, and the deepest the player reached all session was **437 m**, for a
  moment. The rifle likewise: 6 rounds fired across 24 runs. So the bolt-clack window tell,
  the flinch layer, the ragdoll, seam-lane ownership, the pre-aim exploit, and the FP-arms
  "DefaultSlot" uncertainty are all still unknown

Useful map facts measured the same night: the live-ally population is **bimodal** — 58 of
105 ally deaths in 250–375 m (the craft) and 46 in 425–575 m (Z2–Z3), with a gap at
375–425 m that is a gap in *deaths*, not presence (that band is where allies are safe).
Laterally they stay in their craft's column — 44 deaths around x≈230, 35 around x≈510, 20
around x≈790 — the columns barely mix. Both facts shaped Decision 038's selection rule.

Design work: the death→transition spec was /grill-me'd, four questions settled before the
session ended → **Decision 038**. Grilling PAUSED at Q5; the remaining questions are listed
in Next Steps. No code was written this session.

## What Was Done (2026-08-09, second sitting)

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
