# Current Status

> Last updated: 2026-07-18
>
> This document holds the CURRENT state and what's next — nothing else. Session history
> lives in git log; the why of past choices lives in 03_DECISIONS.md; engine gotchas live
> in 11_ENGINE_NOTES.md. When updating, replace stale facts instead of appending below them.

## Phase: Step 2 — First-Person Movement (prone + transitions + F6 + dive-into-prone + headbob + locomotion all feel-checked; zone transit times are the resume point)

All grey-box geometry is placed. Fog and the zone-size walkthrough are DEFERRED until after
more system work (user decision 2026-07-04) — pick them up before tuning zone sizes.

## What Exists

### Documents
- [x] Project mental model (01_SOUL.md)
- [x] Decision log (03_DECISIONS.md) — 26 entries
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
  sprint 900 (Shift), no jumping by design (DoJumpStart/DoJumpEnd are empty shims until the
  BP's touch-UI jump nodes are deleted in-editor)
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

- [ ] **RESUME HERE — Step 2**: Run through each zone and record transit times in
  05_ZONES.md. Note: fog is still deferred and fog is load-bearing for judging
  distances — treat these transit times as provisional until fog is in
- [ ] Editor cleanup while in the BP anyway: delete BP_FirstPersonCharacter's touch-UI jump
  nodes, then delete the DoJumpStart/DoJumpEnd shims in BreakingWaveCharacter

### Writing backlog
- [ ] Second character ("the one who shook me") narrative writing
- [ ] First enemy character narrative writing

### Deferred until after more system work (decided 2026-07-04)
- [ ] Fog setup — do this before judging zone sizes; fog is load-bearing.
  ExponentialHeightFog, tune by eye for ~35 m visibility: Density ~0.5,
  Height Falloff ~0.05, Start Distance ~500
- [ ] Walk through the grey-box and judge zone sizes; adjust the heightmap Profile table if needed
- [ ] Clean up duplicate Landscape parent actors: the level has three (`Landscape`,
  `Landscape2`, `Landscape4`, all at −50400/−50400) — re-import leftovers. Check in the
  editor which one owns the 64 streaming proxies and delete the empty ones
- [ ] Dive-into-prone polish (closed as good-enough 2026-07-17): if the flight ever feels
  flat, the lead is a brief additive camera pitch-down during flight

### Hero piece coordinate sheet (driven by PlaceHeroPieces.py tables — kept for reference)
The landscape is CENTERED on the world origin: its min corner sits at world (−50400, −50400),
so world = profile-meters × 100 − 50400 on both axes. Profile coords below with world uu in parens.
- Landing craft (Zone 0, ramp faces +Y inland): A-left 230/270 (−27400, −23400), B-center 510/270 (600, −23400), C-right 790/270 (28600, −23400)
- Bunkers (Zone 4, slit faces −Y sea): MG-left 200/620 (−30400, 11600), command 510/635 (600, 13100), MG-right 800/620 (29600, 11600)

## What Was Done (last session — 2026-07-18)

- Headbob feel-check PASSED as built — no knob changes needed (numbers stay tentative)
- All-rifle locomotion feel-check PASSED (first check since the 2026-07-12 frozen-legs
  fix)
- Standing-idle gap closed: Tools/SetRifleStandingIdle.py repointed ABP_Unarmed's Idle
  state from MM_Idle to Idle_Rifle_Hip_UE5 headless (anim-graph node edited via
  load_object subobject path + node struct property + compile_blueprint — technique
  recorded in 11_ENGINE_NOTES.md), disk-verified. Feel-checked PASSED same day
