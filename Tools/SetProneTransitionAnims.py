# Sets the prone transition anim slots on BP_FirstPersonCharacter's class defaults:
#   StandToProneAnim  -> /Game/AnimStarterPack/Retarget/Stand_To_Prone_UE5
#   ProneToStandAnim  -> /Game/AnimStarterPack/Retarget/Prone_To_Stand_UE5
#
# Run AFTER compiling the C++ that adds the two properties. Works headless:
#   UnrealEditor-Cmd <uproject> -run=pythonscript -script=<this file> -stdout
# or in-editor via Tools > Execute Python Script...
#
# Idempotent: skips a slot that is already set to the right anim.

import unreal

CHARACTER_BP_PATH = "/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"
CHARACTER_CLASS_PATH = CHARACTER_BP_PATH + ".BP_FirstPersonCharacter_C"

SLOTS = [
    ("stand_to_prone_anim", "/Game/AnimStarterPack/Retarget/Stand_To_Prone_UE5"),
    ("prone_to_stand_anim", "/Game/AnimStarterPack/Retarget/Prone_To_Stand_UE5"),
]


def run():
    character_class = unreal.load_object(None, CHARACTER_CLASS_PATH)
    if character_class is None:
        raise RuntimeError("could not load %s" % CHARACTER_CLASS_PATH)
    defaults = unreal.get_default_object(character_class)

    dirty = False
    for property_name, anim_path in SLOTS:
        anim = unreal.load_asset(anim_path)
        if anim is None:
            raise RuntimeError("missing anim asset %s" % anim_path)
        try:
            current = defaults.get_editor_property(property_name)
        except Exception:
            raise RuntimeError("BP has no %s slot — compile the C++ first, then re-run" % property_name)
        if current == anim:
            unreal.log_warning("%s already set" % property_name)
            continue
        defaults.set_editor_property(property_name, anim)
        dirty = True
        unreal.log_warning("set %s = %s" % (property_name, anim_path))

    if dirty:
        unreal.EditorAssetLibrary.save_asset(CHARACTER_BP_PATH, only_if_is_dirty=False)
    unreal.log_warning("prone transition anim wiring complete")


run()
