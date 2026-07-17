# Current Status

> Last updated: 2026-07-17

## Phase: Step 2 — First-Person Movement (prone + transitions + F6 + dive-into-prone done; headbob BUILT, feel-check is the resume point)

All grey-box geometry is placed. Fog and the zone-size walkthrough are DEFERRED until after
more system work (user decision 2026-07-04) — pick them up before tuning zone sizes.

## What Exists

### Documents
- [x] Project mental model (01_SOUL.md)
- [x] Decision log (03_DECISIONS.md) — 24 entries
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
- [x] BreakingWaveCharacter — base first-person character (walk + sprint + prone)
- [x] Prone (Decisions 023 + 025): C toggle (rebound from LeftControl 2026-07-15 — Ctrl was
  too hard to reach while holding Shift+W), rides engine crouch (instant capsule
  shrink, clearance check on stand-up), camera drops to ProneEyeHeight above ground; STATIONARY
  by design — movement input is ignored while prone and during transitions (Decision 025,
  feel-checked 2026-07-06); tunables ProneCapsuleHalfHeight/ProneEyeHeight on the character. Jump binding removed
  (no jumping by design); DoJumpStart/DoJumpEnd are empty shims until the BP touch-UI jump
  nodes are deleted in-editor
- [x] Prone body animation: user added Epic's AnimStarterPack (UE4 skeleton); all 8 prone anims
  retargeted to the UE5 mannequin (Tools/RetargetProneAnims.py). While prone the body mesh
  plays Prone_Idle_UE5 (correct lying shadow) and the FP arms hide; the ABP resumes on
  stand-up. Known greybox quirk: arms invisible while prone — fine until the shooting/visual
  pass. The missing crawl anim is a NON-issue now: prone is stationary (Decision 025).
  Prone_Fire/Reload/Death anims are retargeted and waiting in
  Content/AnimStarterPack/Retarget/ for later systems
- [x] Prone transition animation (2026-07-06): gameplay stays instant (capsule/speed change on
  the toggle frame — 06_COMBAT.md "drop instantly"), visuals catch up. Body plays
  Stand_To_Prone_UE5 time-compressed into ProneDropDuration (0.35 s, a dive) then settles into
  the prone idle loop; on stand-up plays Prone_To_Stand_UE5 over ProneStandUpDuration (0.9 s —
  getting up is deliberately slower) then hands back to the ABP. First-person camera no longer
  teleports: the FP-mesh offset smoothsteps down/up over the same durations (mid-toggle safe —
  blends start from the current position). Movement input is IGNORED during both transitions
  (user request 2026-07-06 — moving under the transition motion looked wrong); look input
  stays live. Tunables ProneDropDuration/ProneStandUpDuration on the character; BP slots set
  by Tools/SetProneTransitionAnims.py (run headless, idempotent). Both durations are
  tentative feel numbers
- [x] Debug third-person view (2026-07-06, Decision 024): **F6** in PIE (DebugExecBindings in
  Config/DefaultInput.ini — dev-builds only, ignored in Shipping) or `DebugThirdPerson` in the
  console toggles a spring-arm orbit camera, un-hides the world-space body mesh, hides the FP
  arms. Note: Enhanced Input's component DELETES BindKey, so debug keys can't be bound in C++ —
  DebugExecBindings is the engine-sanctioned route (engine's own F1–F5 viewmode keys use it;
  F5 was taken, hence F6).
  Gotcha encoded in the code: the renderer FORCES bOwnerNoSee=true on any
  FirstPersonPrimitiveType::WorldSpaceRepresentation proxy, so the toggle must also swap the
  body mesh's primitive type to None and back (SetFirstPersonPrimitiveType). Debug-only —
  the game stays first-person only. Tunable: DebugThirdPersonDistance (400)
- [x] Dive-into-prone (2026-07-15, per 06_COMBAT.md + Decision 025, REWORKED same day after
  a failed feel-check): going prone while moving keeps the momentum and turns the entry into
  a real ballistic dive. LaunchCharacter adds ProneDiveUpwardSpeed (300 → ~0.6 s airtime,
  ~46 cm rise; 0 = flat slide), air braking is zeroed so the arc keeps its speed. THE CAMERA
  STAYS AT STANDING EYE HEIGHT DURING FLIGHT — the fast eye-drop blend (ProneDropDuration)
  now fires from a Landed() override, not at launch (the first build dropped the eye during
  flight, which visually erased the arc: "still just sliding"). After touchdown the swapped
  braking (zero friction + SlideDeceleration 2500 cm/s²) gives a SHORT impact skid (~1.6 m
  from sprint, ~0.7 m from walk), not a glide; prior braking restored under SlideSettleSpeed
  (60) or on stand-up (stand-up mid-air also cancels the pending eye drop). The body
  one-shot is compressed to predicted flight time + ProneDropDuration so its floor-impact
  frames land near touchdown. Stationary prone entry (below SlideSettleSpeed) is unchanged —
  eye drop starts immediately as before. All numbers tentative; no BP overrides exist, so
  the C++ defaults are live. IsSliding() exposed for the later headbob/anim passes.
  Feel-check DONE (2026-07-17): good enough to move on; any further dive polish is
  deliberately deferred until more systems exist
- [x] Headbob (2026-07-17, Decision 026, grilled + spec agreed before building): custom
  UHeadbobShakePattern + UHeadbobCameraShake (Source/BreakingWave/HeadbobCameraShake.h/.cpp),
  started once per possession from NotifyControllerChanged (bSingleInstance, infinite);
  reads the view-target character every update so it survives possession changes. Figure-8:
  vertical sine at footfall rate + half-rate lateral sway (LateralRatio 0.4 — set to 0 for
  the vertical-only motion-sickness fallback). Speed-synced amplitude AND frequency
  (phase-continuous), anchors at WalkSpeed/RunSpeed: 1.5 cm / 2.8 Hz at 600, 2.5 cm /
  3.4 Hz at 900, scaling down to zero below walk speed. Stationary = breathing (0.3 cm,
  0.35 Hz); prone = breathing × 0.4 ("near-zero"); airborne/slide/prone-transition/F6 debug
  view = still (those states own the camera). Amplitudes ease over SmoothingTime 0.2 s —
  no pops on stop/land/prone. All knobs in FHeadbobSettings on the character (Category
  "Camera"), C++ defaults live, ALL NUMBERS TENTATIVE. Compiled clean. Feel-check PENDING.
  First in-PIE run felt like NOTHING — the pattern resolved the view target via
  GetViewTarget(), which can return the PlayerController (engine keeps it as a valid
  ViewTarget.Target), so the character cast failed and output was zero every frame.
  Fixed to GetViewTargetPawn(), the engine's controller→pawn resolver. Note for tuning:
  the FP camera rides the head socket, so anims already add real head motion — if the
  bob reads weak over it, raise Walk/SprintAmplitude rather than assuming it's dead
- [x] BreakingWaveCameraManager — pitch-limited camera manager stub
- [x] BreakingWaveGameMode / PlayerController — base classes

### Tools
- [x] Tools/GenerateBeachHeightmap.ps1 — generates the zone-profiled heightmap (SourceAssets/BeachHeightmap_1009.png); re-run after editing its Profile/Dunes/Berm/Craters tables
- [x] Tools/PlaceBeachObstacles.py — in-editor Python; spawns grey-box hedgehogs (Zone 2), barbed wire, and debris piles (Zone 3) from editable tables, traced onto the landscape; idempotent (re-run clears prior batch). Requires PythonScriptPlugin + EditorScriptingUtilities (now enabled in .uproject)
- [x] Tools/PlaceHeroPieces.py — in-editor Python; assembles the 3 Zone 4 bunkers (hollow, sea-facing slit, rear door) and 3 Zone 0 landing craft (open hull + dropped ramp) from SM_Cube; idempotent (GreyboxHero tag, per-assembly subfolders), pieces stay individually tweakable
- [x] Tools/AddProneInput.py — creates IA_Prone, maps PRONE_KEY_NAME (currently C; was LeftControl until 2026-07-15) in IMC_Default replacing any old prone key, sets the BP's ProneAction slot; idempotent; edit PRONE_KEY_NAME and re-run to change the key (works headless; save failure now raises loudly)
- [x] Tools/RetargetProneAnims.py — builds IK rigs + UE4→UE5 retargeter (auto chains/mapping/alignment), retargets all 8 AnimStarterPack prone anims to Content/AnimStarterPack/Retarget/*_UE5, sets the BP's ProneBodyIdleAnim; idempotent (already run). NOTE: needs full editor, not commandlet — run in-editor or via `-ExecutePythonScript` (the batch op touches Slate)
- [x] Tools/SetProneTransitionAnims.py — sets the BP's StandToProneAnim/ProneToStandAnim slots to the retargeted transition anims; idempotent, works headless via `-run=pythonscript` (already run)
- [x] Tools/AddSprintToLocomotion.py — SUPERSEDED by RebuildLocomotionAsRifle.py (kept for history; its 900-row is rewritten by the newer tool)
- [x] Tools/RebuildLocomotionAsRifle.py — retargets AnimStarterPack's rifle locomotion set (Idle_Rifle_Hip + 4 Jog_*_Rifle) UE4→UE5 and rebuilds ALL of BS_Idle_Walk_Run as rifle-carry with foot-true rate scales (measured authored speeds: jogs ~285 cm/s, sprint ~617 cm/s — constants in the tool); idempotent (rows rebuilt from tables each run); needs `-ExecutePythonScript` full-editor mode (already run, disk-verified). Triangulation rebuild now goes through UBlendSpaceTool (see below) — the old Persona open/save route crashes offscreen
- [x] Tools/BakeIKBonesFromFK.py — bakes FK bone transforms onto the ik_* helper bones (ik_foot_l/r, ik_hand_gun/l/r) in every /Game/AnimStarterPack/Retarget anim; MUST run after any new retarget (neither ASP sources nor the retargeter animate ik bones, and ABP_Unarmed's CR_Mannequin_FootIK pins the legs to ik_foot_l/r — the "legs not moving" bug). Idempotent, runs headless via `-run=pythonscript` (already run on all 14 retargeted anims, disk-verified: ik_foot spans now match FK feet)
- [x] Source/BreakingWave/BlendSpaceTool.* — editor-only C++ UFUNCTION library for headless tools: RebuildRuntimeTriangulation (the only non-Persona way to rebuild a blendspace's serialized triangulation), DescribeRuntimeTriangulation, DescribeBlendOutputAt (evaluates the blendspace exactly like the runtime — use to verify sample layouts without PIE)

### Level
- [x] Beach heightmap imported at scale 100/100/200 (Decision 022) — zone-profiled terrain with tactical relief (dunes, berm, wavy bluff) confirmed looking right in editor
- [x] Shell craters baked into heightmap and re-imported (Craters table: 3 deep cover craters + 10 shallow dressing craters, raised rims)
- [x] Zone 2/3 obstacles placed: 22 hedgehogs (X-crosses), 151 wire posts, 20 debris blocks via Tools/PlaceBeachObstacles.py — edit its tables and re-run to adjust (idempotent)
- [x] Hero pieces placed: 3 bunkers (Zone 4), 3 landing craft with ramps (Zone 0) via Tools/PlaceHeroPieces.py — same table/re-run workflow
- [ ] Fog — DEFERRED until after more system work (do before judging zone sizes)
- [ ] Duplicate Landscape parent actors cleanup (Landscape/Landscape2/Landscape4) — pending

## Next Steps

- [ ] **RESUME HERE — Step 2 continues**:
  1. FEEL-CHECK the headbob (built 2026-07-17, Decision 026): PIE. W then Shift+W —
     cadence and amplitude should visibly differ and read as footfalls against the FP
     arm swing; release W — camera should settle into faint breathing, no pop; C while
     sprinting — bob must go silent for the whole dive/slide/transition (the arc is the
     motion), then near-still breathing while prone; F6 debug orbit must be bob-free.
     Watch for: lateral figure-8 queasiness (fallback = LateralRatio 0), frequency
     mismatch with the visible arm-swing cadence (retune Walk/SprintFrequency), bob too
     strong/weak (Walk/SprintAmplitude). All knobs in FHeadbobSettings on the character
  2. RE-feel-check the all-rifle locomotion if not yet done (assets last changed
     2026-07-12, frozen-legs fix — restart the editor first if it hasn't been since):
     W (600) = rifle-carry run, feet matching ground; Shift+W (900) same anim at ~1.46;
     strafe/backpedal may read frantic (rate-scaled 2.1×/3.2× — lower the rates in
     RebuildLocomotionAsRifle.py accepting foot slide if so). Standing idle is still
     the ABP's separate unarmed MM_Idle state — making it carry the rifle means editing
     ABP_Unarmed's Idle state
- [ ] **Step 2**: Run through each zone and record transit times in 05_ZONES.md
- [ ] Editor cleanup while in the BP anyway: delete BP_FirstPersonCharacter's touch-UI jump
  nodes, then delete the DoJumpStart/DoJumpEnd shims in BreakingWaveCharacter

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

- Headbob built (2026-07-17): spec grilled question-by-question and agreed with the user
  before any code (scope, home, motion shape, state gating each put as an explicit
  decision — the grilling caught one collision: the user first picked hard amplitude
  cuts, reversed after seeing it recreates the prone camera-teleport artifact). See
  Decision 026 and the What Exists entry for the shape. Compiled clean; feel-check is
  the resume point
- Dive-into-prone feel-check DONE (2026-07-17): user closed it — good enough to build on;
  further polish deliberately deferred until more systems exist. Unbuilt lead kept for
  that later pass, if the flight ever feels flat: a brief additive camera pitch-down
  during flight (bUsePawnControlRotation keeps pitch under the mouse — needs care)
- Dive iteration 2 (2026-07-15, user: "jump is better" but wants a faster fall + anim to
  keep up + a reachable key): (a) new ProneDiveFallGravityScale (2) — gravity doubles once
  vertical velocity crosses zero at the apex, restored on landing/stand-up; the rise keeps
  its pop, the fall snaps (~0.31 s → ~0.22 s); (b) PredictDiveFlightTime() accounts for the
  asymmetric arc, so the body one-shot compresses to the true shorter flight and its
  floor-impact frames still land at touchdown (the "make animation faster" ask — it tracks
  automatically); (c) prone rebound LeftControl → C (Ctrl unreachable during LeftShift+W):
  PRONE_KEY_NAME edited in Tools/AddProneInput.py, run headless, DISK-VERIFIED from a
  fresh process — C → IA_Prone is the only C mapping, old LeftControl mapping replaced;
  the tool now raises loudly on save_asset failure. Compiled clean. Feel-check pending
- Dive-into-prone REWORKED (2026-07-15, same day as the failed feel-check; user confirmed
  the diagnosis: "does not jump at all… immediately hit ground then slide like on snow"):
  (a) eye-drop blend moved from launch to touchdown — new Landed() override +
  StartEyeDropToProne(); during flight the camera holds standing eye height so the arc is
  visible; bDiveEyeDropPending set by the launch, cleared by Landed or stand-up mid-air;
  (b) ProneDiveUpwardSpeed 180 → 300 (~0.37 s → ~0.6 s airtime, 17 → 46 cm rise);
  (c) SlideDeceleration 900 → 2500 (sprint skid ~4.5 m → ~1.6 m — user: friction should
  stop you quickly); (d) body one-shot now compressed to predicted flight + drop time.
  Verified the BP has no serialized overrides of the tunables, so the new C++ defaults are
  live. Compiled clean (4.7 s). Feel-check pending
- Dive feel-check FAILED same day (2026-07-15): user verdict "even more just sliding on the
  ground, not like jump to ground". Working diagnosis: the arc is physically there but
  invisible from first person — the camera only ever moves down (17 cm pawn rise vs 130 cm
  simultaneous eye-drop blend). Rework leads documented in Next Steps item 2; the strongest
  is re-sequencing the camera so the eye drop happens on LANDING, not during flight
- Ballistic dive added on top of the slide (2026-07-15, user request — "projectile jump to
  prone" reads more real than skimming the ground): BeginSlideFromMomentum now also
  LaunchCharacter(0,0,ProneDiveUpwardSpeed) (Z-override, keeps horizontal velocity) and zeroes
  BrakingDecelerationFalling for the slide window (the template's 1500 would eat ~300 cm/s of
  the arc mid-air); landing hands over to the already-set slide braking, SettleSlide restores
  all four saved braking values. Dive only fires from the ground (IsMovingOnGround gate).
  180 up ≈ 0.37 s airtime, tuned to roughly match ProneDropDuration (0.35 s) so the camera
  drop and dive one-shot land with the body. Committed the flat-slide checkpoint first
  (1f0b497), dive compiled clean on top
- Slide-into-prone built (2026-07-15): the last-but-one Step 2 movement item (06_COMBAT.md
  "momentum slide"; Decision 025 explicitly kept it — momentum, not input). Implementation
  rides what already exists: Crouch() never zeroed velocity — the instant stop came from CMC
  braking against MaxWalkSpeedCrouched 0 — so BeginSlideFromMomentum() (called from
  OnStartCrouch when ground speed > SlideSettleSpeed) just swaps braking to
  bUseSeparateBrakingFriction + BrakingFriction 0 + BrakingDecelerationWalking =
  SlideDeceleration, giving a linear momentum bleed with zero input (DoMove already dead
  while prone). SettleSlide() restores the saved braking values when Tick sees speed ≤
  SlideSettleSpeed or on stand-up mid-slide. The existing dive one-shot + camera drop play
  over the slide unchanged. Compiled clean (19 s, no zombie processes this time).
  Feel-check pending — same PIE session as the locomotion re-check
- FIXED "legs not moving while walking/running" (2026-07-12): user's feel-check of the
  all-rifle locomotion failed — body played the anim but the legs stayed planted. Root
  cause chain, each link verified headless: (1) the blendspace was HEALTHY — 24 samples,
  valid triangulation, and a new C++ probe (UBlendSpaceTool::DescribeBlendOutputAt,
  calling the same GetSamplesFromBlendInput the runtime uses) returned the correct
  anims/weights at every speed; (2) the retargeted anims' FK legs move fine (foot travel
  28–49 cm); (3) BUT their ik_foot_l/r helper bones were frozen at the reference pose —
  neither AnimStarterPack's UE4 sources nor the IK retargeter animate ik_* bones, while
  every Epic anim keeps them glued to the FK feet; (4) ABP_Unarmed ends in the
  CR_Mannequin_FootIK control rig whose FootTrace items are ik_foot_l/r, so the rig
  pinned both legs to two fixed points while everything else animated. Prone anims never
  showed this because single-node playback bypasses the ABP and its rig. Fix:
  Tools/BakeIKBonesFromFK.py bakes FK→ik transforms (feet + hands, Epic convention) into
  all 14 retargeted anims — disk-verified, ik_foot now tracks foot exactly. Feel-check
  pending (restart the editor first).
  Also found while diagnosing: the Persona open/save/close trick for rebuilding
  blendspace triangulation CRASHES the offscreen editor in UE 5.6 (Slate paint crash in
  AnimationEditor.dll two frames after open) — it cannot be trusted headless. Replaced
  with Source/BreakingWave/BlendSpaceTool.* exposing UBlendSpace::ResampleData() to
  Python; RebuildLocomotionAsRifle.py now uses it. The 07-11 triangulation had actually
  been saved correctly (28 triangles, max sample index 23 — verified), so triangulation
  was NOT the cause this time; the ik bones were.
- Locomotion rebuilt as all-rifle-carry (2026-07-11): user flagged two problems with the
  sprint pass — legs slid/broke at sprint, and only sprint carried the rifle while
  walk/jog stayed unarmed. Root cause of the legs: AnimStarterPack anims were authored
  for a 270-speed character (its BS_Jog axis and Ue4ASP_Character MaxWalkSpeed both say
  270), so Sprint_Fwd_Rifle at rate 1.0 under our 900 ground speed moved the feet at
  ~2/3 ground speed. Authored speeds were MEASURED from the anims themselves (planted-
  foot velocity relative to root, validated within 5% against the UE5 anims' known root
  motion): jogs ~285 cm/s, sprint ~617 cm/s. Tools/RebuildLocomotionAsRifle.py retargets
  Idle_Rifle_Hip + 4 Jog_*_Rifle and replaces every BS_Idle_Walk_Run row: idle = rifle
  idle; 300 = rifle jogs at ~1.05 (their natural pace); 600 fwd = rifle SPRINT at ~0.97
  (near-perfect foot match — our normal move speed is genuinely a run); 900 fwd = same
  at ~1.46; non-forward directions = jogs rate-scaled foot-true (2.1×/3.2× — tentative,
  may read frantic; the tradeoff is rate vs foot slide, tune in the tool's constants).
  ASP has no hip-carry walk anims (Walk_*_Rifle are all Ironsights/aiming), hence jogs
  everywhere. Disk-verified from a fresh process: 24 samples, all rifle, correct rates.
  Persona resample done. Feel-check pending. Note: if the standing idle still shows the
  unarmed pose in PIE, the ABP has a separate idle state outside the blendspace — flag it
- Sprint body animation wired (2026-07-08): user flagged that Shift-sprint (RunSpeed 900)
  left the body jogging in place — BS_Idle_Walk_Run's speed axis topped out at 600 (the jog
  row, which is also our WalkSpeed), so sprint clamped there and the feet slid at 1.5×.
  Tools/AddSprintToLocomotion.py retargeted AnimStarterPack's Sprint_Fwd_Rifle to the UE5
  mannequin and added a 900-speed row to the blendspace (fwd = sprint anim; other
  directions = jog anims with per-sample rate_scale 1.5 so feet keep up). Verified in the
  reloaded asset (axis max 900, 9 samples, fwd sprint in place). NOT yet feel-checked in
  PIE. UE5.6 gotcha for the record: BlendSpace sample_data AND blend_parameters are
  writable from Python (blend_parameters despite being a fixed C array), and BlendSample
  has a per-sample rate_scale — blendspaces can be extended headless without touching
  the ABP. Sprint anim carries a rifle pose next to unarmed walk/jog — fine for greybox,
  and everything becomes rifle anims later anyway.
  TWO FEEL-CHECKS FAILED (2026-07-08, "no difference between W and Shift+W") — real root
  cause found on the second: a ZOMBIE headless UnrealEditor-Cmd (leftover from an earlier
  probe run) held BS_Idle_Walk_Run.uasset, so the tool's original save silently FAILED
  with sharing-violation Error Code 32 (SavePackage retries MoveFile ~10x then gives up;
  the Python save_asset return was unchecked) — the same zombie also broke the C++ build
  with LNK1104 until killed. Both feel-checks ran against the untouched template asset.
  In-session "verify" was worthless: unreal.load_asset returns the in-memory object, so
  verification of a save must happen in a SEPARATE process. Second (theoretical, also
  fixed) trap: editing sample_data doesn't rebuild the blendspace's serialized runtime
  triangulation — only UBlendSpace::ResampleData() does, whose only caller is the Persona
  editor (fires on construction), so the tool opens the asset in Persona + force-saves +
  closes (works under -ExecutePythonScript -RenderOffscreen). Tool re-run 2026-07-08
  with no editor running: DISK-VERIFIED from a fresh process — axis 0–900 grid 3,
  36 samples (900 row: fwd sprint + 8 rate-scaled jogs), tool now bails loudly on save
  failure. Related finding: ABP_FP_Copy (first-person view) has NO anim dependencies —
  it copy-poses from the body mesh + CtrlRig_FPWarp, so body anim fixes show up in first
  person automatically. Feel-check pending (third attempt)
- Feel-check PASSED (2026-07-08): prone transitions (fast dive / slow rise, no camera
  teleport, WASD dead while prone and mid-transition) and the F6 debug third-person view
  all confirmed good in PIE by the user. Durations kept at 0.35 s / 0.9 s
- Prone made stationary, crawl sway reverted (2026-07-06, Decision 025): a procedural crawl
  placeholder (speed-scaled idle play rate + yaw/roll sway) was built earlier the same day —
  user feel-checked it and rejected it; moving while prone itself was cut. DoMove now ignores
  input while prone (and MaxWalkSpeedCrouched = 0); ProneSpeed and the ProneCrawl* tunables
  are deleted. 06_COMBAT.md prone bullet updated. Slide-into-prone stays planned — it is
  momentum, not input
- Prone transition animation wired (2026-07-06): user confirmed the prone feel-check passed,
  then asked for the missing transition anims. OnStartCrouch/OnEndCrouch now sequence
  transition one-shot → idle loop / → ABP restore via a timer, with the one-shots
  time-compressed via UAnimSingleNodeInstance::SetPlayRate; the FP camera drop/rise is a
  smoothstep blend in Tick instead of an instant SetRelativeLocation. Movement input is
  gated off while a transition runs (DoMove checks IsProneTransitionActive, a world-time
  stamp set by both crouch handlers). Compiled clean; Tools/SetProneTransitionAnims.py run
  headless set both BP slots (verified in the log). NOT yet feel-checked in PIE — that is
  the resume point
- Debug third-person view toggle built (2026-07-06): `DebugThirdPerson` exec command on
  BreakingWaveCharacter, spring-arm orbit camera (pawn-control-rotation), keybound to F6 via
  DebugExecBindings in DefaultInput.ini (UEnhancedInputComponent deletes BindKey, so config
  is the only clean debug-key route; F5 already = engine shadercomplexity). Compiled clean.
  Engine finding worth remembering: FPrimitiveSceneProxy force-sets bOwnerNoSee=true for
  WorldSpaceRepresentation primitives (PrimitiveSceneProxy.cpp ~line 746), so un-hiding the
  body requires SetFirstPersonPrimitiveType(None) too, not just SetOwnerNoSee(false).
  NOT yet feel-checked in PIE — that is the resume point

### Earlier (2026-07-05)
- Session ended 2026-07-05 after wiring the prone body animation; the retargeted anim/shadow
  has NOT been eyeballed in-game yet. Agreed next action: build a debug third-person view
  toggle first (see RESUME HERE), then feel-check prone with it
- Prone body animation wired (2026-07-05): user feel-checked prone (good) but flagged the
  standing shadow/look; user added AnimStarterPack; wrote Tools/RetargetProneAnims.py which
  auto-built IK rigs + retargeter in Python and retargeted the 8 prone anims UE4→UE5 headless.
  Gotcha for the record: IKRetargetBatchOperation crashes in commandlet mode (Slate assert) —
  use `-ExecutePythonScript` (full editor boot) instead; also call it per-asset so a harmless
  missing-bone error (center_of_mass) can't abort the batch
- Prone implemented (2026-07-05, Decision 023): engine-crouch-based, LeftControl toggle,
  camera lowered by offsetting FirstPersonMesh (head-socket camera doesn't lower without
  anims). C++ compiled clean; Tools/AddProneInput.py run headless — IA_Prone created,
  LeftControl mapped in IMC_Default, BP ProneAction slot set, all verified by re-load
- Jump removed per 06_COMBAT.md: input binding deleted; BP's template touch-UI graph still
  calls DoJumpStart/DoJumpEnd so they stay as empty shims (BP failed to compile without
  them) — cleanup queued in Next Steps

### Earlier (2026-07-04)
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
