# Repoints ABP_Unarmed's standing Idle state (Locomotion state machine -> Idle ->
# AnimGraphNode_SequencePlayer_1) from the unarmed MM_Idle to the retargeted
# Idle_Rifle_Hip_UE5, then recompiles and saves the ABP. This closes the "walk/run
# carry the rifle but standing idle doesn't" gap left by RebuildLocomotionAsRifle.py
# (the blendspace idle sample only shows while moving; standing idle is this ABP node).
#
# Headless OK: UnrealEditor-Cmd <uproject> -run=pythonscript -script=<this> -stdout -unattended
# Idempotent: skips the edit if the node already plays Idle_Rifle_Hip_UE5.

import unreal

ABP_PATH = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"
IDLE_NODE_PATH = (
    "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed:"
    "AnimGraph.AnimGraphNode_StateMachine_0.Locomotion.AnimStateNode_1.Idle."
    "AnimGraphNode_SequencePlayer_1"
)
RIFLE_IDLE_PATH = "/Game/AnimStarterPack/Retarget/Idle_Rifle_Hip_UE5"


def run():
    abp = unreal.load_asset(ABP_PATH)
    rifle_idle = unreal.load_asset(RIFLE_IDLE_PATH)
    if abp is None or rifle_idle is None:
        unreal.log_warning("SUMMARY: FAILED — could not load ABP (%s) or rifle idle (%s)" % (abp, rifle_idle))
        return

    node = unreal.load_object(None, IDLE_NODE_PATH)
    if node is None:
        unreal.log_warning("SUMMARY: FAILED — Idle sequence player node not found at %s" % IDLE_NODE_PATH)
        return

    player = node.get_editor_property("node")
    current = player.get_editor_property("sequence")
    current_name = current.get_name() if current else "None"
    if current_name == rifle_idle.get_name():
        unreal.log_warning("Idle state already plays %s — nothing to edit" % current_name)
    else:
        player.set_editor_property("sequence", rifle_idle)
        node.set_editor_property("node", player)
        unreal.log_warning("Idle state sequence: %s -> %s" % (current_name, rifle_idle.get_name()))

    unreal.BlueprintEditorLibrary.compile_blueprint(abp)

    if not unreal.EditorAssetLibrary.save_asset(ABP_PATH, only_if_is_dirty=False):
        unreal.log_warning("SUMMARY: FAILED to save %s (file locked? zombie UnrealEditor?)" % ABP_PATH)
        return

    reread = unreal.load_object(None, IDLE_NODE_PATH).get_editor_property("node").get_editor_property("sequence")
    unreal.log_warning("SUMMARY: standing idle now plays %s (verify from a FRESH process before trusting disk)" % (
        reread.get_name() if reread else "None"))


run()
