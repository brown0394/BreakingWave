# Wires up the prone input for BreakingWaveCharacter:
#   1. creates /Game/Input/Actions/IA_Prone (duplicated from IA_Sprint, bool action)
#   2. maps LeftControl -> IA_Prone in /Game/Input/IMC_Default
#   3. sets the ProneAction slot on BP_FirstPersonCharacter's class defaults
#
# Run from inside the UE editor (AFTER compiling the C++ that adds ProneAction):
#   Tools menu > Execute Python Script... > pick this file
#
# Idempotent: each step is skipped when its result already exists, so re-run freely.
# To change the key, edit PRONE_KEY_NAME (UE key names, e.g. "LeftControl", "Z", "C")
# and re-run; the old mapping is replaced.

import unreal

ACTIONS_DIR = "/Game/Input/Actions"
PRONE_ACTION_PATH = ACTIONS_DIR + "/IA_Prone"
TEMPLATE_ACTION_PATH = ACTIONS_DIR + "/IA_Sprint"
MAPPING_CONTEXT_PATH = "/Game/Input/IMC_Default"
CHARACTER_BP_PATH = "/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"
CHARACTER_CLASS_PATH = CHARACTER_BP_PATH + ".BP_FirstPersonCharacter_C"
PRONE_KEY_NAME = "LeftControl"


def ensure_prone_action():
    if unreal.EditorAssetLibrary.does_asset_exist(PRONE_ACTION_PATH):
        unreal.log("IA_Prone already exists")
        return unreal.load_asset(PRONE_ACTION_PATH)
    template = unreal.load_asset(TEMPLATE_ACTION_PATH)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    action = asset_tools.duplicate_asset("IA_Prone", ACTIONS_DIR, template)
    if action is None:
        raise RuntimeError("failed to duplicate %s into IA_Prone" % TEMPLATE_ACTION_PATH)
    unreal.EditorAssetLibrary.save_asset(PRONE_ACTION_PATH, only_if_is_dirty=False)
    unreal.log("created IA_Prone")
    return action


def ensure_key_mapping(prone_action):
    context = unreal.load_asset(MAPPING_CONTEXT_PATH)
    mappings = list(context.get_editor_property("mappings"))
    kept = []
    already_mapped = False
    for mapping in mappings:
        if mapping.get_editor_property("action") == prone_action:
            key_name = str(mapping.get_editor_property("key").get_editor_property("key_name"))
            if key_name == PRONE_KEY_NAME:
                already_mapped = True
                kept.append(mapping)
            else:
                unreal.log("replacing old prone key %s" % key_name)
        else:
            kept.append(mapping)
    if already_mapped:
        unreal.log("IMC_Default already maps %s -> IA_Prone" % PRONE_KEY_NAME)
        return
    key = unreal.Key()
    key.set_editor_property("key_name", PRONE_KEY_NAME)
    mapping = unreal.EnhancedActionKeyMapping()
    mapping.set_editor_property("action", prone_action)
    mapping.set_editor_property("key", key)
    kept.append(mapping)
    context.set_editor_property("mappings", kept)
    unreal.EditorAssetLibrary.save_asset(MAPPING_CONTEXT_PATH, only_if_is_dirty=False)
    unreal.log("mapped %s -> IA_Prone in IMC_Default" % PRONE_KEY_NAME)


def ensure_character_prone_slot(prone_action):
    character_class = unreal.load_object(None, CHARACTER_CLASS_PATH)
    if character_class is None:
        raise RuntimeError("could not load %s — was the C++ ProneAction property compiled in?" % CHARACTER_CLASS_PATH)
    defaults = unreal.get_default_object(character_class)
    try:
        current = defaults.get_editor_property("prone_action")
    except Exception:
        raise RuntimeError("BP_FirstPersonCharacter has no ProneAction slot — compile the C++ first, then re-run")
    if current == prone_action:
        unreal.log("BP_FirstPersonCharacter ProneAction already set")
        return
    defaults.set_editor_property("prone_action", prone_action)
    unreal.EditorAssetLibrary.save_asset(CHARACTER_BP_PATH, only_if_is_dirty=False)
    unreal.log("set BP_FirstPersonCharacter ProneAction = IA_Prone")


def run():
    prone_action = ensure_prone_action()
    ensure_key_mapping(prone_action)
    ensure_character_prone_slot(prone_action)
    unreal.log("prone input wiring complete")


run()
