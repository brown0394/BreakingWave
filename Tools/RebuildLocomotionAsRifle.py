# Rebuilds BS_Idle_Walk_Run as an all-rifle-carry locomotion set (supersedes
# AddSprintToLocomotion.py, whose sprint row this rewrites too):
#   1. retargets AnimStarterPack's Idle_Rifle_Hip + the 4 Jog_*_Rifle anims (UE4 skeleton)
#      onto the UE5 mannequin via the retargeter Tools/RetargetProneAnims.py built
#      (Sprint_Fwd_Rifle_UE5 already exists from the sprint pass)
#   2. replaces EVERY sample row so idle/walk/run/sprint all carry the rifle, with
#      rate scales that match foot speed to ground speed:
#      the ASP anims were authored for a 270-speed character — measured planted-foot
#      speeds are ~285 cm/s (jogs) and ~617 cm/s (sprint), so playing sprint at rate 1.0
#      under our 900 ground speed slid the feet at 2/3 ground speed (the "broken legs")
#
# Layout (game speeds: 600 = normal move, 900 = Shift sprint; 300 = analog half-stick):
#   speed   0: Idle_Rifle_Hip_UE5
#   speed 300: Jog_*_Rifle_UE5 at ~1.05 (their natural pace)
#   speed 600: fwd = Sprint_Fwd_Rifle_UE5 at ~0.97 (near-perfect foot match),
#              other directions = jogs rate-scaled to 600
#   speed 900: fwd = sprint at ~1.46, other directions = jogs rate-scaled to 900
#   ASP has only 4 move directions, so diagonals come from blendspace triangulation.
#
# The ABP is untouched. Needs full-editor script mode (retarget asserts in commandlet):
#   UnrealEditor-Cmd <uproject> -ExecutePythonScript="Tools/RebuildLocomotionAsRifle.py"
#      -stdout -unattended -nosplash -RenderOffscreen
# The serialized runtime triangulation is rebuilt via UBlendSpaceTool (project C++) —
# editing sample_data alone does not rebuild it, and the old Persona open/save/close
# route crashes offscreen (Slate paint crash in AnimationEditor, seen 2026-07-12).
# Idempotent: existing *_UE5 retargets are reused, all rows rebuilt from the tables.
#
# AFTER any run that produced NEW retargets, also run Tools/BakeIKBonesFromFK.py —
# retargeted anims have frozen ik_* helper bones, which pins the legs under
# ABP_Unarmed's foot-IK control rig (the 2026-07-12 "legs not moving" bug).

import unreal
import warnings

warnings.simplefilter("ignore")

SOURCE_MESH_PATH = "/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin"
TARGET_MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
RETARGETER_PATH = "/Game/AnimStarterPack/Retarget/RTG_UE4Mannequin_to_Manny"
RETARGET_DIR = "/Game/AnimStarterPack/Retarget"
BLENDSPACE_PATH = "/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run"

RIFLE_ANIMS_TO_RETARGET = [
    "Idle_Rifle_Hip",
    "Jog_Fwd_Rifle",
    "Jog_Bwd_Rifle",
    "Jog_Lt_Rifle",
    "Jog_Rt_Rifle",
]

IDLE = RETARGET_DIR + "/Idle_Rifle_Hip_UE5"
JOG_FWD = RETARGET_DIR + "/Jog_Fwd_Rifle_UE5"
JOG_BWD = RETARGET_DIR + "/Jog_Bwd_Rifle_UE5"
JOG_LT = RETARGET_DIR + "/Jog_Lt_Rifle_UE5"
JOG_RT = RETARGET_DIR + "/Jog_Rt_Rifle_UE5"
SPRINT = RETARGET_DIR + "/Sprint_Fwd_Rifle_UE5"

JOG_AUTHORED_SPEED = 285.0
SPRINT_AUTHORED_SPEED = 617.0
WALK_SPEED = 300.0
RUN_SPEED = 600.0
SPRINT_SPEED = 900.0

IDLE_DIRECTIONS = [0.0, 45.0, 90.0, 135.0, 180.0, -45.0, -90.0, -135.0, -180.0]


def moving_row(speed, forward_anim, forward_rate):
    jog_rate = speed / JOG_AUTHORED_SPEED
    return [
        (0.0, speed, forward_anim, forward_rate),
        (90.0, speed, JOG_RT, jog_rate),
        (-90.0, speed, JOG_LT, jog_rate),
        (180.0, speed, JOG_BWD, jog_rate),
        (-180.0, speed, JOG_BWD, jog_rate),
    ]


def all_rows():
    rows = [(d, 0.0, IDLE, 1.0) for d in IDLE_DIRECTIONS]
    rows += moving_row(WALK_SPEED, JOG_FWD, WALK_SPEED / JOG_AUTHORED_SPEED)
    rows += moving_row(RUN_SPEED, SPRINT, RUN_SPEED / SPRINT_AUTHORED_SPEED)
    rows += moving_row(SPRINT_SPEED, SPRINT, SPRINT_SPEED / SPRINT_AUTHORED_SPEED)
    return rows


def ensure_rifle_anims_retargeted():
    source_mesh = unreal.load_asset(SOURCE_MESH_PATH)
    target_mesh = unreal.load_asset(TARGET_MESH_PATH)
    retargeter = unreal.load_asset(RETARGETER_PATH)
    for name in RIFLE_ANIMS_TO_RETARGET:
        target_path = "%s/%s_UE5" % (RETARGET_DIR, name)
        if unreal.EditorAssetLibrary.does_asset_exist(target_path):
            unreal.log_warning("%s_UE5 already exists, skipping retarget" % name)
            continue
        asset_data = unreal.EditorAssetLibrary.find_asset_data("/Game/AnimStarterPack/" + name)
        try:
            unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                [asset_data], source_mesh, target_mesh, retargeter,
                search="", replace="", prefix="", suffix="_UE5",
                include_referenced_assets=False)
        except Exception as error:
            unreal.log_warning("retarget of %s raised (%s) — checking whether the asset was produced anyway" % (name, error))
        born_at = "/Game/%s_UE5" % name
        if unreal.EditorAssetLibrary.does_asset_exist(born_at):
            unreal.EditorAssetLibrary.rename_asset(born_at, target_path)
        if not unreal.EditorAssetLibrary.does_asset_exist(target_path):
            unreal.log_warning("FAILED to retarget %s" % name)
            return False
        if not unreal.EditorAssetLibrary.save_asset(target_path, only_if_is_dirty=False):
            unreal.log_warning("FAILED to save %s (file locked? zombie UnrealEditor?)" % target_path)
            return False
        unreal.log_warning("retargeted + saved %s" % target_path)
    return True


def build_sample(direction, speed, anim_path, rate):
    anim = unreal.load_asset(anim_path)
    if anim is None:
        raise RuntimeError("missing anim %s" % anim_path)
    sample = unreal.BlendSample()
    sample.set_editor_property("animation", anim)
    sample.set_editor_property("sample_value", unreal.Vector(direction, speed, 0.0))
    sample.set_editor_property("rate_scale", rate)
    return sample


def rebuild_all_rows(blendspace):
    samples = [build_sample(*row) for row in all_rows()]
    blendspace.set_editor_property("sample_data", samples)
    unreal.log_warning("sample_data replaced: %d rifle samples" % len(samples))


def verify(blendspace):
    reloaded = unreal.load_asset(BLENDSPACE_PATH)
    samples = list(reloaded.get_editor_property("sample_data"))
    names = set(s.get_editor_property("animation").get_name()
                for s in samples if s.get_editor_property("animation"))
    all_rifle = all("Rifle" in n for n in names)
    fwd_sprint_rates = [round(float(s.get_editor_property("rate_scale")), 2) for s in samples
                        if float(s.get_editor_property("sample_value").x) == 0.0
                        and s.get_editor_property("animation")
                        and s.get_editor_property("animation").get_name() == "Sprint_Fwd_Rifle_UE5"]
    ok = all_rifle and len(samples) == len(all_rows()) and len(fwd_sprint_rates) == 2
    unreal.log_warning("VERIFY samples=%d all_rifle=%s fwd_sprint_rates=%s" % (
        len(samples), all_rifle, sorted(fwd_sprint_rates)))
    return ok


def rebuild_runtime_triangulation(blendspace):
    report = unreal.BlendSpaceTool.rebuild_runtime_triangulation(blendspace)
    unreal.log_warning("triangulation rebuilt: %s" % report)
    if report.startswith("ERROR"):
        return False
    if not unreal.EditorAssetLibrary.save_asset(BLENDSPACE_PATH, only_if_is_dirty=False):
        unreal.log_warning("FAILED to save after triangulation rebuild (file locked?)")
        return False
    return True


def run():
    if not ensure_rifle_anims_retargeted():
        unreal.log_warning("SUMMARY: FAILED during retarget — blendspace untouched")
        return
    blendspace = unreal.load_asset(BLENDSPACE_PATH)
    rebuild_all_rows(blendspace)
    if not unreal.EditorAssetLibrary.save_asset(BLENDSPACE_PATH, only_if_is_dirty=False):
        unreal.log_warning("SUMMARY: FAILED to save the blendspace (file locked? check for zombie UnrealEditor processes) — nothing persisted")
        return
    if verify(blendspace) and rebuild_runtime_triangulation(blendspace):
        unreal.log_warning("SUMMARY: all-rifle locomotion wired — run BakeIKBonesFromFK.py if new retargets were made, then feel-check W and Shift+W in PIE (F6 to watch the body)")
    else:
        unreal.log_warning("SUMMARY: INCOMPLETE — check the log above")


run()
