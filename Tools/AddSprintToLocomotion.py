# Makes the body animation read as running while sprinting (Shift, RunSpeed 900):
#   1. retargets AnimStarterPack's Sprint_Fwd_Rifle (UE4 skeleton) onto the UE5 mannequin
#      using the rigs + retargeter that Tools/RetargetProneAnims.py already built
#   2. extends the template locomotion blendspace BS_Idle_Walk_Run (speed axis currently
#      tops out at the jog row, 600 == WalkSpeed) with a sprint row at SPRINT_SPEED:
#      forward = the retargeted sprint anim; every other direction = the jog anim
#      rate-scaled by SPRINT_SPEED/JOG_SPEED so feet keep up with the ground
#
# The ABP is untouched — it already feeds ground speed into this blendspace.
#
# Run headless is NOT enough for step 1 (IKRetargetBatchOperation asserts in commandlet
# mode) OR for step 2's finish: editing sample_data does NOT rebuild the blendspace's
# serialized runtime triangulation/grid — only UBlendSpace::ResampleData() does, and the
# only caller is the Persona blendspace editor (it fires once on construction, see
# SAnimationBlendSpace.cpp). So this script must open the asset editor after editing,
# which needs full-editor script mode:
#   UnrealEditor-Cmd <uproject> -ExecutePythonScript="Tools/AddSprintToLocomotion.py"
#      -stdout -unattended -nosplash -RenderOffscreen
#
# Idempotent: an existing Sprint_Fwd_Rifle_UE5 is reused, the sprint row is rebuilt
# from scratch on every run (edit the tables below and re-run to tune).

import unreal
import warnings

warnings.simplefilter("ignore")

SOURCE_MESH_PATH = "/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin"
TARGET_MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
RETARGETER_PATH = "/Game/AnimStarterPack/Retarget/RTG_UE4Mannequin_to_Manny"
RETARGET_DIR = "/Game/AnimStarterPack/Retarget"
SPRINT_SOURCE = "/Game/AnimStarterPack/Sprint_Fwd_Rifle"
SPRINT_UE5_PATH = RETARGET_DIR + "/Sprint_Fwd_Rifle_UE5"
BLENDSPACE_PATH = "/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run"
JOG_DIR = "/Game/Characters/Mannequins/Anims/Unarmed/Jog"

SPRINT_SPEED = 900.0
JOG_SPEED = 600.0
JOG_CATCH_UP_RATE = SPRINT_SPEED / JOG_SPEED
SPEED_AXIS_GRID_NUM = 3

SPRINT_ROW = [
    (0.0, SPRINT_UE5_PATH, 1.0),
    (45.0, JOG_DIR + "/MF_Unarmed_Jog_Fwd_Right", JOG_CATCH_UP_RATE),
    (-45.0, JOG_DIR + "/MF_Unarmed_Jog_Fwd_Left", JOG_CATCH_UP_RATE),
    (90.0, JOG_DIR + "/MF_Unarmed_Jog_Right", JOG_CATCH_UP_RATE),
    (-90.0, JOG_DIR + "/MF_Unarmed_Jog_Left", JOG_CATCH_UP_RATE),
    (135.0, JOG_DIR + "/MF_Unarmed_Jog_Bwd_Right", JOG_CATCH_UP_RATE),
    (-135.0, JOG_DIR + "/MF_Unarmed_Jog_Bwd_Left", JOG_CATCH_UP_RATE),
    (180.0, JOG_DIR + "/MF_Unarmed_Jog_Bwd", JOG_CATCH_UP_RATE),
    (-180.0, JOG_DIR + "/MF_Unarmed_Jog_Bwd", JOG_CATCH_UP_RATE),
]


def ensure_sprint_anim_retargeted():
    if unreal.EditorAssetLibrary.does_asset_exist(SPRINT_UE5_PATH):
        unreal.log_warning("Sprint_Fwd_Rifle_UE5 already exists, skipping retarget")
        return True
    source_mesh = unreal.load_asset(SOURCE_MESH_PATH)
    target_mesh = unreal.load_asset(TARGET_MESH_PATH)
    retargeter = unreal.load_asset(RETARGETER_PATH)
    asset_data = unreal.EditorAssetLibrary.find_asset_data(SPRINT_SOURCE)
    try:
        unreal.IKRetargetBatchOperation.duplicate_and_retarget(
            [asset_data], source_mesh, target_mesh, retargeter,
            search="", replace="", prefix="", suffix="_UE5",
            include_referenced_assets=False)
    except Exception as error:
        unreal.log_warning("retarget raised (%s) — checking whether the asset was produced anyway" % error)
    born_at = "/Game/Sprint_Fwd_Rifle_UE5"
    if unreal.EditorAssetLibrary.does_asset_exist(born_at):
        unreal.EditorAssetLibrary.rename_asset(born_at, SPRINT_UE5_PATH)
    if unreal.EditorAssetLibrary.does_asset_exist(SPRINT_UE5_PATH):
        unreal.EditorAssetLibrary.save_asset(SPRINT_UE5_PATH, only_if_is_dirty=False)
        unreal.log_warning("saved %s" % SPRINT_UE5_PATH)
        return True
    unreal.log_warning("FAILED to retarget Sprint_Fwd_Rifle")
    return False


def raise_speed_axis_to_sprint(blendspace):
    params = list(blendspace.get_editor_property("blend_parameters"))
    speed_axis = params[1]
    if float(speed_axis.get_editor_property("max")) == SPRINT_SPEED:
        unreal.log_warning("speed axis max already %s" % SPRINT_SPEED)
        return True
    speed_axis.set_editor_property("max", SPRINT_SPEED)
    speed_axis.set_editor_property("grid_num", SPEED_AXIS_GRID_NUM)
    try:
        blendspace.set_editor_property("blend_parameters", params)
    except Exception as error:
        unreal.log_warning("FAILED to write speed axis (%s) — raise the axis max to %s manually in the blendspace editor" % (error, SPRINT_SPEED))
        return False
    unreal.log_warning("speed axis max %s -> %s (grid %s)" % (JOG_SPEED, SPRINT_SPEED, SPEED_AXIS_GRID_NUM))
    return True


def build_sample(direction, anim_path, rate):
    anim = unreal.load_asset(anim_path)
    if anim is None:
        raise RuntimeError("missing anim %s" % anim_path)
    sample = unreal.BlendSample()
    sample.set_editor_property("animation", anim)
    sample.set_editor_property("sample_value", unreal.Vector(direction, SPRINT_SPEED, 0.0))
    sample.set_editor_property("rate_scale", rate)
    return sample


def rebuild_sprint_row(blendspace):
    kept = [s for s in blendspace.get_editor_property("sample_data")
            if float(s.get_editor_property("sample_value").y) != SPRINT_SPEED]
    sprint_row = [build_sample(*entry) for entry in SPRINT_ROW]
    blendspace.set_editor_property("sample_data", kept + sprint_row)
    unreal.log_warning("sample rows: %d existing kept, %d sprint samples written" % (len(kept), len(sprint_row)))


def verify(blendspace):
    reloaded = unreal.load_asset(BLENDSPACE_PATH)
    axis_max = float(list(reloaded.get_editor_property("blend_parameters"))[1].get_editor_property("max"))
    sprint_samples = [s for s in reloaded.get_editor_property("sample_data")
                      if float(s.get_editor_property("sample_value").y) == SPRINT_SPEED]
    fwd_ok = any(s.get_editor_property("animation") and
                 s.get_editor_property("animation").get_name() == "Sprint_Fwd_Rifle_UE5" and
                 float(s.get_editor_property("sample_value").x) == 0.0
                 for s in sprint_samples)
    ok = axis_max == SPRINT_SPEED and len(sprint_samples) == len(SPRINT_ROW) and fwd_ok
    unreal.log_warning("VERIFY axis max=%s, sprint samples=%d, fwd sprint anim=%s" % (axis_max, len(sprint_samples), fwd_ok))
    return ok


def resample_via_persona(blendspace):
    subsystem = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
    if not subsystem.open_editor_for_assets([blendspace]):
        unreal.log_warning("FAILED to open the blendspace editor — open %s manually once and save it, or the runtime blend data stays stale" % BLENDSPACE_PATH)
        return False
    saved = unreal.EditorAssetLibrary.save_asset(BLENDSPACE_PATH, only_if_is_dirty=False)
    subsystem.close_all_editors_for_asset(blendspace)
    if not saved:
        unreal.log_warning("FAILED to save after resample (file locked?)")
        return False
    unreal.log_warning("runtime blend data rebuilt via Persona open + force save")
    return True


def run():
    if not ensure_sprint_anim_retargeted():
        return
    blendspace = unreal.load_asset(BLENDSPACE_PATH)
    if not raise_speed_axis_to_sprint(blendspace):
        return
    rebuild_sprint_row(blendspace)
    if not unreal.EditorAssetLibrary.save_asset(BLENDSPACE_PATH, only_if_is_dirty=False):
        unreal.log_warning("SUMMARY: FAILED to save the blendspace (file locked? check for zombie UnrealEditor processes) — nothing persisted")
        return
    if verify(blendspace) and resample_via_persona(blendspace):
        unreal.log_warning("SUMMARY: sprint locomotion wired — feel-check Shift-sprint in PIE (F6 to watch the body)")
    else:
        unreal.log_warning("SUMMARY: INCOMPLETE — check the log above")


run()
