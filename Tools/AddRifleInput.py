# Wires up the rifle inputs for BreakingWaveCharacter:
#   1. creates /Game/Input/Actions/IA_Fire, IA_AimRifle, IA_Reload (duplicated from IA_Sprint, bool actions)
#   2. maps LeftMouseButton -> IA_Fire, RightMouseButton -> IA_AimRifle, R -> IA_Reload in /Game/Input/IMC_Default
#   3. sets the FireAction/AimAction/ReloadAction slots on BP_FirstPersonCharacter's class defaults
#
# Headless OK (no spawning):
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<ABSOLUTE path to this file>
#
# Idempotent: each step is skipped when its result already exists, so re-run freely.
# To change a key, edit KEY_FOR below and re-run; the old mapping is replaced.

import unreal

ACTIONS_DIR = "/Game/Input/Actions"
TEMPLATE_ACTION_PATH = ACTIONS_DIR + "/IA_Sprint"
MAPPING_CONTEXT_PATH = "/Game/Input/IMC_Default"
CHARACTER_BP_PATH = "/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"
CHARACTER_CLASS_PATH = CHARACTER_BP_PATH + ".BP_FirstPersonCharacter_C"

RIFLE_ACTIONS = [
    ("IA_Fire", "LeftMouseButton", "fire_action"),
    ("IA_AimRifle", "RightMouseButton", "aim_action"),
    ("IA_Reload", "R", "reload_action"),
]


def ensure_action(name):
    path = ACTIONS_DIR + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("%s already exists" % name)
        return unreal.load_asset(path)
    template = unreal.load_asset(TEMPLATE_ACTION_PATH)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    action = asset_tools.duplicate_asset(name, ACTIONS_DIR, template)
    if action is None:
        raise RuntimeError("failed to duplicate %s into %s" % (TEMPLATE_ACTION_PATH, name))
    if not unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError("save_asset FAILED for %s" % path)
    unreal.log_warning("created %s" % name)
    return action


def ensure_key_mapping(context, action, key_name, action_label):
    mappings = list(context.get_editor_property("mappings"))
    kept = []
    already_mapped = False
    changed = False
    for mapping in mappings:
        if mapping.get_editor_property("action") == action:
            existing_key = str(mapping.get_editor_property("key").get_editor_property("key_name"))
            if existing_key == key_name:
                already_mapped = True
                kept.append(mapping)
            else:
                unreal.log("replacing old %s key %s" % (action_label, existing_key))
                changed = True
        else:
            kept.append(mapping)
    if already_mapped:
        unreal.log("IMC_Default already maps %s -> %s" % (key_name, action_label))
        return False
    key = unreal.Key()
    key.set_editor_property("key_name", key_name)
    mapping = unreal.EnhancedActionKeyMapping()
    mapping.set_editor_property("action", action)
    mapping.set_editor_property("key", key)
    kept.append(mapping)
    context.set_editor_property("mappings", kept)
    unreal.log_warning("mapped %s -> %s in IMC_Default" % (key_name, action_label))
    return True


def ensure_character_slot(defaults, prop_name, action):
    try:
        current = defaults.get_editor_property(prop_name)
    except Exception:
        raise RuntimeError("BP has no %s slot — compile the C++ first, then re-run" % prop_name)
    if current == action:
        unreal.log("BP %s already set" % prop_name)
        return False
    defaults.set_editor_property(prop_name, action)
    unreal.log_warning("set BP %s = %s" % (prop_name, action.get_name()))
    return True


def run():
    context = unreal.load_asset(MAPPING_CONTEXT_PATH)
    character_class = unreal.load_object(None, CHARACTER_CLASS_PATH)
    if character_class is None:
        raise RuntimeError("could not load %s" % CHARACTER_CLASS_PATH)
    defaults = unreal.get_default_object(character_class)

    context_dirty = False
    bp_dirty = False
    for action_name, key_name, prop_name in RIFLE_ACTIONS:
        action = ensure_action(action_name)
        context_dirty |= ensure_key_mapping(context, action, key_name, action_name)
        bp_dirty |= ensure_character_slot(defaults, prop_name, action)

    if context_dirty:
        if not unreal.EditorAssetLibrary.save_asset(MAPPING_CONTEXT_PATH, only_if_is_dirty=False):
            raise RuntimeError("save_asset FAILED for %s — check for zombie UnrealEditor processes" % MAPPING_CONTEXT_PATH)
    if bp_dirty:
        if not unreal.EditorAssetLibrary.save_asset(CHARACTER_BP_PATH, only_if_is_dirty=False):
            raise RuntimeError("save_asset FAILED for %s — check for zombie UnrealEditor processes" % CHARACTER_BP_PATH)
    unreal.log_warning("rifle input wiring complete")


run()
