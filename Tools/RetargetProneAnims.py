# Retargets the AnimStarterPack prone animations (UE4 mannequin skeleton) onto the
# project's UE5 mannequin (SKM_Manny_Simple) and points the character at them:
#   1. builds IK rigs for both mannequins (auto-generated retarget chains)
#   2. builds a UE4->UE5 IK retargeter (auto-mapped chains, auto-aligned retarget pose)
#   3. duplicates + retargets every prone anim as Retarget/<Name>_UE5
#   4. sets ProneBodyIdleAnim on BP_FirstPersonCharacter to Prone_Idle_UE5
#
# Run from inside the UE editor (Tools > Execute Python Script...) or headless:
#   UnrealEditor-Cmd <uproject> -run=pythonscript -script=Tools/RetargetProneAnims.py
#
# Idempotent: existing rigs/retargeter are reused, already-retargeted anims are skipped.
#
# If the automated retarget ever misbehaves, the manual editor equivalent is:
#   select the prone anims in /Game/AnimStarterPack > right-click > Retarget Animations
#   > target SKM_Manny_Simple > Export.

import unreal
import warnings

warnings.simplefilter("ignore")

SOURCE_MESH_PATH = "/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin"
TARGET_MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
TARGET_SKELETON_PATH = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
RETARGET_DIR = "/Game/AnimStarterPack/Retarget"
ANIM_DIR = "/Game/AnimStarterPack"
RETARGET_SUFFIX = "_UE5"
CHARACTER_BP_PATH = "/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"
CHARACTER_CLASS_PATH = CHARACTER_BP_PATH + ".BP_FirstPersonCharacter_C"

PRONE_ANIMS = [
    "Prone_Idle",
    "Stand_To_Prone",
    "Prone_To_Stand",
    "Prone_Fire_1",
    "Prone_Fire_2",
    "Prone_Reload_Rifle",
    "Prone_Death_1",
    "Prone_Death_2",
]


def ensure_ik_rig(name, mesh):
    path = RETARGET_DIR + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_warning("%s already exists" % name)
        return unreal.load_asset(path)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    rig = asset_tools.create_asset(name, RETARGET_DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    controller = unreal.IKRigController.get_controller(rig)
    controller.set_skeletal_mesh(mesh)
    controller.apply_auto_generated_retarget_definition()
    chains = [str(c) for c in controller.get_retarget_chains()] if hasattr(controller, "get_retarget_chains") else []
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log_warning("created %s (root=%s, %d chains)" % (name, controller.get_retarget_root(), len(chains)))
    return rig


def ensure_retargeter(source_rig, target_rig, target_mesh):
    path = RETARGET_DIR + "/RTG_UE4Mannequin_to_Manny"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log_warning("retargeter already exists")
        return unreal.load_asset(path)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    retargeter = asset_tools.create_asset("RTG_UE4Mannequin_to_Manny", RETARGET_DIR, unreal.IKRetargeter, unreal.IKRetargetFactory())
    controller = unreal.IKRetargeterController.get_controller(retargeter)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target_mesh)
    for step, call in [
        ("add_default_ops", lambda: controller.add_default_ops()),
        ("auto_map_chains", lambda: controller.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)),
        ("auto_align_all_bones", lambda: controller.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)),
    ]:
        try:
            call()
            unreal.log_warning("retargeter setup: %s ok" % step)
        except Exception as e:
            unreal.log_warning("retargeter setup: %s failed (%s) — often fine, editor defaults may already cover it" % (step, e))
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    unreal.log_warning("created RTG_UE4Mannequin_to_Manny")
    return retargeter


def retargeted_path(name):
    return "%s/%s%s" % (RETARGET_DIR, name, RETARGET_SUFFIX)


def retarget_prone_anims(source_mesh, target_mesh, retargeter):
    for name in PRONE_ANIMS:
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


def verify_retargeted_anims():
    target_skeleton = unreal.load_asset(TARGET_SKELETON_PATH)
    all_ok = True
    for name in PRONE_ANIMS:
        path = retargeted_path(name)
        anim = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        skeleton = anim.get_editor_property("skeleton") if anim else None
        ok = skeleton == target_skeleton
        all_ok = all_ok and ok
        unreal.log_warning("VERIFY %s%s: %s" % (name, RETARGET_SUFFIX, "ok" if ok else "MISSING/WRONG SKELETON"))
    return all_ok


def set_character_prone_idle():
    prone_idle = unreal.load_asset(retargeted_path("Prone_Idle"))
    character_class = unreal.load_object(None, CHARACTER_CLASS_PATH)
    defaults = unreal.get_default_object(character_class)
    if defaults.get_editor_property("prone_body_idle_anim") == prone_idle:
        unreal.log_warning("BP ProneBodyIdleAnim already set")
        return
    defaults.set_editor_property("prone_body_idle_anim", prone_idle)
    unreal.EditorAssetLibrary.save_asset(CHARACTER_BP_PATH, only_if_is_dirty=False)
    unreal.log_warning("set BP ProneBodyIdleAnim = Prone_Idle%s" % RETARGET_SUFFIX)


def run():
    source_mesh = unreal.load_asset(SOURCE_MESH_PATH)
    target_mesh = unreal.load_asset(TARGET_MESH_PATH)
    source_rig = ensure_ik_rig("IKR_UE4Mannequin", source_mesh)
    target_rig = ensure_ik_rig("IKR_Manny", target_mesh)
    retargeter = ensure_retargeter(source_rig, target_rig, target_mesh)
    retarget_prone_anims(source_mesh, target_mesh, retargeter)
    if verify_retargeted_anims():
        set_character_prone_idle()
        unreal.log_warning("SUMMARY: prone animation retarget complete")
    else:
        unreal.log_warning("SUMMARY: retarget INCOMPLETE — use the manual editor path in this file's header")


run()
