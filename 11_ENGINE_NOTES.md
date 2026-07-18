# Engine Notes — UE 5.6 Gotchas

Hard-won facts about this engine version and this project's assets. Read before writing
editor-Python tools, doing headless asset work, or debugging "the engine is lying to me"
weirdness. Each entry cost a failed session or a failed feel-check; none of it is guesswork.

---

## Headless editor work (two tiers)

- `-run=pythonscript` (commandlet, no Slate): asset create/save, CDO edits, IMC mapping
  edits, anim data edits, tracing, map loading all work. Anything touching editor UI
  crashes: actor spawning (access violation in EditorFramework.dll), IKRetargetBatchOperation
  (Slate assert), Persona.
- `-ExecutePythonScript=<py>` with `-RenderOffscreen -unattended` boots the full editor
  headless — required for the UI-touching batch ops above. EXCEPTION: opening a Persona
  editor offscreen (open_editor_for_assets) crashes ~2 frames later (Slate paint crash in
  AnimationEditor.dll) — never use the Persona open/save/close trick; use BlendSpaceTool (below).
- The commandlet exits 1 if ANY asset it touches logs errors (e.g. a BP compile failure),
  even when the Python succeeded — read the Warning/Error Summary, not just the exit code.
- `unreal.log()` does NOT reach stdout headless — use `unreal.log_warning()`.
- UE_LOG errors inside a called UFUNCTION escalate to Python RuntimeError — call batch ops
  per-asset in try/except and salvage (the asset is often still produced; e.g. a missing
  center_of_mass bone track is harmless).

## Saving and verifying assets

- **ALWAYS check `save_asset`'s bool and fail loudly.** A zombie headless UnrealEditor-Cmd
  holding a .uasset makes the save fail silently with sharing-violation Error Code 32
  (SavePackage retries MoveFile ~10× then gives up). The same zombie breaks the C++ link
  with LNK1104. Check `Get-Process *Unreal*` before any asset-writing run.
- **In-session verification of a save is worthless**: `unreal.load_asset` returns the
  in-memory object. Verify disk truth from a separate fresh process.
- BP CDO property overrides can silently shadow C++ defaults — grep the .uasset for the
  property name strings to prove none are serialized before trusting C++ default changes.

## Editor Python API quirks

- `unreal.HitResult` exposes NO attributes (`hasattr` always False) — `to_tuple()` is the
  only read path: field 0 = blocking_hit, first Vector field = hit location.
- `unreal.Rotator(a, b, c)` argument order is **(roll, pitch, yaw)** — yaw last.
- BlendSpace `sample_data` AND `blend_parameters` are writable from Python (pass the
  modified list back), and BlendSample has per-sample `rate_scale`. BUT editing sample_data
  does NOT rebuild the serialized runtime triangulation — only `UBlendSpace::ResampleData()`
  does, exposed headless via Source/BreakingWave/BlendSpaceTool.* (RebuildRuntimeTriangulation;
  DescribeBlendOutputAt evaluates the blend exactly like the runtime — verify layouts without PIE).
- Anim track edits: `get_data_model()`/`get_controller()` don't exist in 5.6 Python — use
  `anim.get_editor_property("data_model_interface")` + controller property
  (set_bone_track_keys/add_bone_track inside open_bracket).
- T3D export via AssetExportTask reads AnimBlueprint AND ControlRig graphs headless
  (ABP exports UTF-16, CR exports UTF-8) — the way to inspect BP/rig graphs without the editor.
- Measuring an anim's authored ground speed headless: AnimPoseExtensions.get_anim_pose_at_time
  + get_bone_pose(WORLD); implied speed = median horizontal velocity of the planted foot
  (frames within ~3 cm of that foot's min Z) RELATIVE TO THE ROOT BONE — works for in-place
  and root-motion anims, validated within 5% against known root motion.

## Animation / retargeting (this project's assets)

- **IK-retargeted anims have frozen ik_* helper bones** (neither AnimStarterPack sources nor
  the IK retargeter animate them; Epic anims keep them glued to the FK bones), and
  ABP_Unarmed ends in CR_Mannequin_FootIK whose FootTrace items are ik_foot_l/r — frozen ik
  bones pin the legs while everything else animates ("legs not moving").
  **Tools/BakeIKBonesFromFK.py MUST re-run after ANY new IK retarget.** Single-node playback
  (PlayAnimation) bypasses the ABP and its rig, which is why prone anims never showed it.
- AnimStarterPack is authored for a 270-speed character (jogs ~285 cm/s, sprint ~617 cm/s,
  measured) — rate-scale against our 600/900 speeds or the feet slide. ASP has no hip-carry
  walk anims (Walk_*_Rifle are all ironsights).
- ABP_FP_Copy (first-person view) has no anim dependencies — it copy-poses from the body
  mesh + CtrlRig_FPWarp, so body-ABP anim fixes show up in first person automatically.
- ABP_Unarmed's standing Idle state plays a direct sequence player, outside
  BS_Idle_Walk_Run — the blendspace idle sample only shows while moving slowly. It now
  plays Idle_Rifle_Hip_UE5 (was MM_Idle; Tools/SetRifleStandingIdle.py, 2026-07-18).
- **Anim-graph nodes ARE editable headless** (`-run=pythonscript`): load the node by its
  subobject path (find it via T3D export, e.g.
  `ABP_Unarmed.ABP_Unarmed:AnimGraph.AnimGraphNode_StateMachine_0.Locomotion.AnimStateNode_1.Idle.AnimGraphNode_SequencePlayer_1`)
  with `unreal.load_object(None, path)`, get the `node` struct property, set its
  `sequence`, write the struct back, then `BlueprintEditorLibrary.compile_blueprint` +
  save. Edit only nodes under `:AnimGraph...` — the duplicate graphs under
  `ExecuteUbergraph_*` in the T3D are compiled artifacts that regenerate on compile.

## Rendering / camera / input (C++)

- FPrimitiveSceneProxy force-sets bOwnerNoSee=true on any
  FirstPersonPrimitiveType::WorldSpaceRepresentation primitive (PrimitiveSceneProxy.cpp ~746)
  — un-hiding the body mesh to its owner needs SetFirstPersonPrimitiveType(None) too,
  not just SetOwnerNoSee(false).
- UEnhancedInputComponent `= delete`s BindKey — debug keys go through
  `DebugExecBindings=(Key=...,Command="...")` under `[/Script/Engine.PlayerInput]` in
  DefaultInput.ini (dev-only, ignored in Shipping; engine claims F1–F5, F8, F9, F11 —
  project debug keys start at F6). See Decision 024.
- **PlayerCameraManager->GetViewTarget() can return the PLAYERCONTROLLER**, not the pawn —
  a valid ViewTarget.Target state. Use GetViewTargetPawn(), the engine's controller→pawn
  resolver. (First headbob run output zero forever because of this.) Applies to all future
  view-target code, including the death camera.
- UCameraShakePattern virtuals carry the `Impl` suffix (UpdateShakePatternImpl etc.);
  a wrapper shake sets its pattern via
  `Super(ObjectInitializer.SetDefaultSubobjectClass<UMyPattern>(TEXT("RootShakePattern")))`.
- The FP camera is socketed to the head bone, so locomotion anims already move the camera —
  a weak-feeling camera effect layered on top may need bigger amplitudes, not a bug hunt.
- BP_FirstPersonCharacter's template touch-UI graph calls DoJumpStart/DoJumpEnd — grep-proof
  any C++ UFUNCTION deletion against BP usage by running any headless pythonscript (it
  loads/compiles the BP) before trusting it.

## Level / landscape

- The landscape is CENTERED on the world origin (min corner −50400, −50400) — world =
  profile-meters × 100 − 50400. Placement tools auto-detect the corner from proxy bounds.
- LevelPrototyping meshes (SM_Cube etc.) have their pivot at the CORNER — snap spawned
  pieces by `actor.get_actor_bounds()` + add_actor_world_offset (pivot/rotation agnostic).
- Line traces for placement: use the editor-world trace context (landscape-actor context
  proved flaky).
