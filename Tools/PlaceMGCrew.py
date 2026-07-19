# Places the Step 3 MG system into the open level (Lvl_FirstPerson):
#   1 MGBunkerGun inside Bunker_MG_Right (barrel through the slit, gunner + loader mannequins)
#   1 MGBunkerManager + 1 AllySimManager
#
# Run from inside the UE editor: Tools menu > Execute Python Script... > pick this file.
# (Actor spawning crashes headless — this one is editor-only, like PlaceHeroPieces.py.)
#
# Idempotent: everything is tagged MG_TAG; re-running deletes the previous batch first.
# To man a different bunker, edit GUN_BUNKER below and re-run.
#
# Geometry facts this script assumes (PlaceHeroPieces.py): bunker embed 0.5 m,
# slit spans 1.2–1.6 m above the embedded base, MG bunkers are 5 m deep, slit faces -Y.

import unreal

M = 100.0
MG_TAG = "MGSystem"
FOLDER = "MGSystem"

GUN_BUNKER = {"x": 800, "y": 620, "yaw": -90.0}
BUNKER_DEPTH_M = 5.0
BUNKER_EMBED_M = 0.5
SLIT_CENTER_Z_M = 1.4
BARREL_LENGTH_CM = 120.0
MUZZLE_SETBACK_FROM_CENTER_CM = 230.0

MANNY = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"
CUBE = "/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"
CREW_IDLE = "/Game/AnimStarterPack/Retarget/Idle_Rifle_Hip_UE5.Idle_Rifle_Hip_UE5"
FIRE_LOOP = "/Game/Audio/MGFireLoop.MGFireLoop"
CRACK = "/Game/Audio/MGCrack.MGCrack"


def landscape_min_corner(eas):
    min_x = min_y = None
    for a in eas.get_all_level_actors():
        if "Landscape" not in a.get_class().get_name():
            continue
        origin, extent = a.get_actor_bounds(False)
        if extent.x <= 0 or extent.y <= 0:
            continue
        lo_x, lo_y = origin.x - extent.x, origin.y - extent.y
        min_x = lo_x if min_x is None else min(min_x, lo_x)
        min_y = lo_y if min_y is None else min(min_y, lo_y)
    if min_x is None:
        return None
    return min_x, min_y


def ground_z(world, x, y, ignore_actors):
    result = unreal.SystemLibrary.line_trace_single(
        world, unreal.Vector(x, y, 50000.0), unreal.Vector(x, y, -50000.0),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        True, ignore_actors, unreal.DrawDebugTrace.NONE, True)
    if result is None:
        return None
    fields = result.to_tuple()
    if not fields[0]:
        return None
    for f in fields:
        if isinstance(f, unreal.Vector):
            return f.z
    return None


def clear_previous(eas):
    removed = 0
    for actor in eas.get_all_level_actors():
        if actor.actor_has_tag(MG_TAG):
            eas.destroy_actor(actor)
            removed += 1
    return removed


def register(actor, label):
    actor.tags = [unreal.Name(MG_TAG)]
    actor.set_folder_path(FOLDER)
    actor.set_actor_label(label)


def main():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    corner = landscape_min_corner(eas)
    if corner is None:
        unreal.log_error("No Landscape found. Open Lvl_FirstPerson and retry.")
        return

    removed = clear_previous(eas)
    bunker_pieces = [a for a in eas.get_all_level_actors() if a.actor_has_tag("GreyboxHero")]

    cx = corner[0] + GUN_BUNKER["x"] * M
    cy = corner[1] + GUN_BUNKER["y"] * M
    gz = ground_z(world, cx, cy, bunker_pieces)
    if gz is None:
        unreal.log_error("No ground under the bunker position — is the landscape loaded?")
        return

    base_z = gz - BUNKER_EMBED_M * M
    gun_z = base_z + SLIT_CENTER_Z_M * M
    gun_y = cy - (MUZZLE_SETBACK_FROM_CENTER_CM - BARREL_LENGTH_CM)

    gun = eas.spawn_actor_from_class(
        unreal.MGBunkerGun, unreal.Vector(cx, gun_y, gun_z),
        unreal.Rotator(0.0, 0.0, GUN_BUNKER["yaw"]))
    register(gun, "MGBunkerGun_Right")

    cube = unreal.load_asset(CUBE)
    barrel = gun.get_editor_property("barrel")
    barrel.set_static_mesh(cube)
    barrel.set_relative_scale3d(unreal.Vector(1.2, 0.12, 0.12))
    barrel.set_relative_location(unreal.Vector(0.0, -6.0, -6.0), False, False)
    gun.get_editor_property("muzzle").set_relative_location(
        unreal.Vector(100.0, 50.0, 50.0), False, False)

    manny = unreal.load_asset(MANNY)
    idle = unreal.load_asset(CREW_IDLE)
    for prop, rel in (("gunner_mesh", unreal.Vector(-80.0, 0.0, -SLIT_CENTER_Z_M * M + BUNKER_EMBED_M * M)),
                      ("loader_mesh", unreal.Vector(-70.0, 90.0, -SLIT_CENTER_Z_M * M + BUNKER_EMBED_M * M))):
        comp = gun.get_editor_property(prop)
        comp.set_skeletal_mesh_asset(manny)
        comp.set_relative_location(rel, False, False)
        comp.set_relative_rotation(unreal.Rotator(0.0, 0.0, -90.0), False, False)
        if idle is not None:
            comp.override_animation_data(idle, True, True, 0.0, 1.0)

    gun.set_editor_property("fire_loop_sound", unreal.load_asset(FIRE_LOOP))
    gun.set_editor_property("crack_sound", unreal.load_asset(CRACK))

    manager = eas.spawn_actor_from_class(
        unreal.MGBunkerManager, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
    register(manager, "MGBunkerManager")

    ally_sim = eas.spawn_actor_from_class(
        unreal.AllySimManager, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
    register(ally_sim, "AllySimManager")

    unreal.log("MG system placed (cleared %d previous). Gun at (%.0f, %.0f, %.0f), muzzle through the MG-right slit."
               % (removed, cx, gun_y, gun_z))


main()
