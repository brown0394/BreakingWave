# Retargets the AnimStarterPack infantry-cycle animations (UE4 mannequin) onto the UE5
# mannequin, reusing the IK rigs + retargeter RetargetProneAnims.py already built.
# The infantry manager fallback-loads these by path at BeginPlay — no BP wiring needed.
#
# Needs full-editor Python (IK retarget batch touches UI; -run=pythonscript crashes):
#   UnrealEditor-Cmd <uproject> -ExecutePythonScript="<ABSOLUTE path to this file>"
#
# AFTER this: run Tools/BakeIKBonesFromFK.py on the new anims (MUST, per 11_ENGINE_NOTES.md
# — retargeted anims ship with frozen ik_* bones).
#
# Idempotent: rigs/retargeter are reused, already-retargeted anims are skipped.

import unreal
import warnings

warnings.simplefilter("ignore")

SOURCE_MESH_PATH = "/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin"
TARGET_MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
TARGET_SKELETON_PATH = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
RETARGET_DIR = "/Game/AnimStarterPack/Retarget"
ANIM_DIR = "/Game/AnimStarterPack"
RETARGET_SUFFIX = "_UE5"

INFANTRY_ANIMS = [
    "Crouch_Idle_Rifle_Hip",
    "Crouch_to_Stand_Rifle_Ironsights",
    "Idle_Rifle_Ironsights",
    "Fire_Rifle_Ironsights",
    "Stand_to_Crouch_Rifle_Ironsights",
]


def retargeted_path(name):
    return "%s/%s%s" % (RETARGET_DIR, name, RETARGET_SUFFIX)


def retarget_anims(source_mesh, target_mesh, retargeter):
    for name in INFANTRY_ANIMS:
        home = retargeted_path(name)
        if unreal.EditorAssetLibrary.does_asset_exist(home):
            unreal.log_warning("%s%s already exists, skipping" % (name, RETARGET_SUFFIX))
            continue
        asset_data = unreal.EditorAssetLibrary.find_asset_data("%s/%s" % (ANIM_DIR, name))
        try:
            unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                [asset_data], source_mesh, target_mesh, retargeter,
                search="", replace="", prefix="", suffix=RETARGET_SUFFIX,
                include_referenced_assets=False)
        except Exception as error:
            unreal.log_warning("retarget of %s raised (%s) — checking whether the asset was produced anyway" % (name, error))
        born_at = "/Game/%s%s" % (name, RETARGET_SUFFIX)
        if unreal.EditorAssetLibrary.does_asset_exist(born_at):
            unreal.EditorAssetLibrary.rename_asset(born_at, home)
        if unreal.EditorAssetLibrary.does_asset_exist(home):
            unreal.EditorAssetLibrary.save_asset(home, only_if_is_dirty=False)
            unreal.log_warning("saved %s" % home)
        else:
            unreal.log_warning("FAILED to retarget %s" % name)


def verify():
    target_skeleton = unreal.load_asset(TARGET_SKELETON_PATH)
    all_ok = True
    for name in INFANTRY_ANIMS:
        path = retargeted_path(name)
        anim = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        skeleton = anim.get_editor_property("skeleton") if anim else None
        ok = skeleton == target_skeleton
        all_ok = all_ok and ok
        unreal.log_warning("VERIFY %s%s: %s" % (name, RETARGET_SUFFIX, "ok" if ok else "MISSING/WRONG SKELETON"))
    return all_ok


def run():
    source_mesh = unreal.load_asset(SOURCE_MESH_PATH)
    target_mesh = unreal.load_asset(TARGET_MESH_PATH)
    retargeter_path = RETARGET_DIR + "/RTG_UE4Mannequin_to_Manny"
    if not unreal.EditorAssetLibrary.does_asset_exist(retargeter_path):
        unreal.log_error("retargeter missing — run RetargetProneAnims.py first")
        return
    retargeter = unreal.load_asset(retargeter_path)
    retarget_anims(source_mesh, target_mesh, retargeter)
    if verify():
        unreal.log_warning("SUMMARY: infantry animation retarget complete — NOW RUN BakeIKBonesFromFK.py")
    else:
        unreal.log_warning("SUMMARY: retarget INCOMPLETE — use the manual editor path (right-click anims > Retarget)")


run()
