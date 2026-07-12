# Fixes the frozen-legs bug in IK-retargeted AnimStarterPack anims: neither the UE4
# sources nor the retargeter animate the ik_* helper bones, so they stay at the
# reference pose — and ABP_Unarmed's CR_Mannequin_FootIK control rig places the feet
# from ik_foot_l/r, pinning both legs to two fixed points while the body plays the
# anim. Epic's own anims keep every ik bone glued to its FK counterpart; this tool
# bakes that convention into every retargeted anim:
#   ik_foot_l <- foot_l, ik_foot_r <- foot_r,
#   ik_hand_gun <- hand_r, ik_hand_l <- hand_l, ik_hand_r <- hand_r
# (hands included so the aim/shoot pass never hits the same bug).
#
# Idempotent: baking is a pure function of the FK tracks, which are never touched.
# Run headless after ANY new retarget (RebuildLocomotionAsRifle.py, RetargetProneAnims.py):
#   UnrealEditor-Cmd <uproject> -run=pythonscript -script="Tools/BakeIKBonesFromFK.py"

import unreal
import warnings

warnings.simplefilter("ignore")

RETARGET_DIR = "/Game/AnimStarterPack/Retarget"

IK_BONE_SOURCES = [
    ("ik_foot_l", "foot_l", "ik_foot_root"),
    ("ik_foot_r", "foot_r", "ik_foot_root"),
    ("ik_hand_gun", "hand_r", "ik_hand_root"),
    ("ik_hand_l", "hand_l", "hand_r"),
    ("ik_hand_r", "hand_r", "hand_r"),
]


def log(msg):
    unreal.log_warning(msg)


def world_pose_at(anim, time, bone, options):
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, time, options)
    return unreal.AnimPoseExtensions.get_bone_pose(pose, bone, unreal.AnimPoseSpaces.WORLD)


def bake_anim(anim):
    model = anim.get_editor_property("data_model_interface")
    controller = anim.get_editor_property("controller")
    frame_rate = model.get_frame_rate()
    seconds_per_frame = float(frame_rate.denominator) / float(frame_rate.numerator)
    key_count = model.get_number_of_keys()
    options = unreal.AnimPoseEvaluationOptions()

    key_times = [key_index * seconds_per_frame for key_index in range(key_count)]
    world_cache = {}
    needed_bones = set()
    for ik_bone, fk_source, new_parent_source in IK_BONE_SOURCES:
        needed_bones.add(fk_source)
        needed_bones.add(new_parent_source)
    for bone in needed_bones:
        world_cache[bone] = [world_pose_at(anim, t, bone, options) for t in key_times]

    controller.open_bracket("Bake IK bones from FK")
    for ik_bone, fk_source, new_parent_source in IK_BONE_SOURCES:
        positions = []
        rotations = []
        scales = []
        for key_index in range(key_count):
            target_world = world_cache[fk_source][key_index]
            parent_world = world_cache[new_parent_source][key_index]
            local = unreal.MathLibrary.make_relative_transform(target_world, parent_world)
            positions.append(local.translation)
            rotations.append(local.rotation)
            scales.append(unreal.Vector(1.0, 1.0, 1.0))
        if ik_bone not in list(model.get_bone_track_names()):
            controller.add_bone_track(ik_bone)
        controller.set_bone_track_keys(ik_bone, positions, rotations, scales)
    controller.close_bracket()
    return key_count


def run():
    log("==== BAKE IK BONES FROM FK ====")
    all_assets = unreal.EditorAssetLibrary.list_assets(RETARGET_DIR, recursive=False, include_folder=False)
    anim_paths = sorted(a.split(".")[0] for a in all_assets)
    failed = []
    for path in anim_paths:
        anim = unreal.load_asset(path)
        if not isinstance(anim, unreal.AnimSequence):
            continue
        try:
            key_count = bake_anim(anim)
        except Exception as error:
            log("FAILED to bake %s: %s" % (path, error))
            failed.append(path)
            continue
        if not unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False):
            log("FAILED to save %s (file locked? zombie UnrealEditor?)" % path)
            failed.append(path)
            continue
        log("baked + saved %s (%d keys)" % (path, key_count))
    if failed:
        log("SUMMARY: FAILED for %d anim(s): %s" % (len(failed), failed))
    else:
        log("SUMMARY: all retargeted anims baked — verify ik_foot spans from a fresh process")


run()
