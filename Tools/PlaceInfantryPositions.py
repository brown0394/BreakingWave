# Places the Zone 3 enemy infantry positions (08_ENEMY_AI.md, first infantry pass):
#   2 flank foxholes (2 men each) sited on the between-bunker seam lanes the 55-degree
#     MG slit arcs cannot cover near the line (the by-design blind spot infantry owns),
#   1 center trench parapet line (3 men) in front of the center bunker,
#   the AInfantrySoldier shell actors behind each parapet, and one AInfantryManager.
#
# Above-grade sandbag-style parapets (grill decision): dug-in terrain versions join the
# visual pass once positions stop moving. Parapet height 1.1 m: a ducked soldier (0.9 m)
# is fully covered, a risen one (1.7 m, eye 1.5 m) shoots over it.
#
# Run from inside the UE editor (with Lvl_FirstPerson open) — spawning crashes headless:
#   Tools menu > Execute Python Script... > pick this file
#
# Idempotent: everything is tagged INFANTRY_TAG; re-running clears the previous batch.
# Coordinate convention matches PlaceHeroPieces.py: heightmap-profile meters, +Y inland,
# world = landscape min corner + meters * 100. All numbers tentative per CLAUDE.md.

import math

import unreal

M = 100.0
INFANTRY_TAG = "InfantrySystem"
FOLDER = "InfantryPositions"
CUBE = "/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"

PARAPET_HEIGHT_M = 1.1
PARAPET_THICK_M = 0.4
SEA_FACING_YAW = -90.0

# Bunkers sit at profile x 200 / 510 / 800; the seam lanes are ~355 and ~655.
# Foxholes sit on those lanes in upper Zone 3 (Z3 = profile y 480..580).
POSITIONS = [
    {
        "label": "Foxhole_Left", "x": 355, "y": 550, "yaw": 0.0,
        "front_width": 3.5, "side_depth": 2.4,
        "soldiers": [(-0.8, 0.0), (0.8, 0.0)],
    },
    {
        "label": "Trench_Center", "x": 510, "y": 560, "yaw": 0.0,
        "front_width": 12.0, "side_depth": 1.5,
        "soldiers": [(-4.0, 0.0), (0.0, 0.0), (4.0, 0.0)],
    },
    {
        "label": "Foxhole_Right", "x": 655, "y": 550, "yaw": 0.0,
        "front_width": 3.5, "side_depth": 2.4,
        "soldiers": [(-0.8, 0.0), (0.8, 0.0)],
    },
]

CORNER = {"x": 0.0, "y": 0.0}


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


def world_xy(x_m, y_m):
    return CORNER["x"] + x_m * M, CORNER["y"] + y_m * M


def ground_z(world, x, y):
    result = unreal.SystemLibrary.line_trace_single(
        world, unreal.Vector(x, y, 50000.0), unreal.Vector(x, y, -50000.0),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        True, [], unreal.DrawDebugTrace.NONE, True)
    if result is None:
        return None
    fields = result.to_tuple()
    if not fields[0]:
        return None
    for f in fields:
        if isinstance(f, unreal.Vector):
            return f.z
    return None


def yaw_rotate(dx, dy, yaw_deg):
    r = math.radians(yaw_deg)
    return dx * math.cos(r) - dy * math.sin(r), dx * math.sin(r) + dy * math.cos(r)


def parapet_pieces(front_width, side_depth):
    half_w = front_width / 2.0
    front_y = -(side_depth / 2.0 + PARAPET_THICK_M / 2.0)
    wall_z = PARAPET_HEIGHT_M / 2.0
    return [
        ("parapet_front", 0.0, front_y, wall_z, front_width + 2.0 * PARAPET_THICK_M, PARAPET_THICK_M, PARAPET_HEIGHT_M),
        ("parapet_left", -(half_w + PARAPET_THICK_M / 2.0), 0.0, wall_z, PARAPET_THICK_M, side_depth, PARAPET_HEIGHT_M),
        ("parapet_right", half_w + PARAPET_THICK_M / 2.0, 0.0, wall_z, PARAPET_THICK_M, side_depth, PARAPET_HEIGHT_M),
    ]


def clear_previous(eas):
    removed = 0
    for actor in eas.get_all_level_actors():
        if actor.actor_has_tag(INFANTRY_TAG):
            eas.destroy_actor(actor)
            removed += 1
    return removed


def spawn_piece(eas, mesh, base_x, base_y, base_z, piece, yaw_deg, assembly_label):
    name, dx_m, dy_m, dz_m, sx_m, sy_m, sz_m = piece
    wx, wy = yaw_rotate(dx_m * M, dy_m * M, yaw_deg)
    target = unreal.Vector(base_x + wx, base_y + wy, base_z + dz_m * M)
    bb = mesh.get_bounding_box()
    native = bb.max - bb.min
    actor = eas.spawn_actor_from_object(mesh, target, unreal.Rotator(0.0, 0.0, yaw_deg))
    actor.set_actor_scale3d(unreal.Vector(sx_m * M / native.x, sy_m * M / native.y, sz_m * M / native.z))
    origin, extent = actor.get_actor_bounds(False)
    actor.add_actor_world_offset(
        unreal.Vector(target.x - origin.x, target.y - origin.y, target.z - origin.z),
        False, False)
    actor.tags = [unreal.Name(INFANTRY_TAG)]
    actor.set_folder_path(FOLDER + "/" + assembly_label)
    actor.set_actor_label(assembly_label + "_" + name)
    return 1


def spawn_soldier(eas, world, base_x, base_y, slot, yaw_deg, assembly_label, index):
    dx_m, dy_m = slot
    wx, wy = yaw_rotate(dx_m * M, dy_m * M, yaw_deg)
    x, y = base_x + wx, base_y + wy
    gz = ground_z(world, x, y)
    if gz is None:
        unreal.log_warning("%s soldier %d: no ground, skipped" % (assembly_label, index))
        return 0
    actor = eas.spawn_actor_from_class(
        unreal.InfantrySoldier, unreal.Vector(x, y, gz),
        unreal.Rotator(0.0, 0.0, SEA_FACING_YAW + yaw_deg))
    actor.tags = [unreal.Name(INFANTRY_TAG)]
    actor.set_folder_path(FOLDER + "/" + assembly_label)
    actor.set_actor_label("%s_Soldier_%d" % (assembly_label, index))
    return 1


def main():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    corner = landscape_min_corner(eas)
    if corner is None:
        unreal.log_error("No Landscape found in the open level. Open Lvl_FirstPerson and retry.")
        return
    CORNER["x"], CORNER["y"] = corner

    cube = unreal.load_asset(CUBE)
    removed = clear_previous(eas)
    placed_pieces = 0
    placed_soldiers = 0

    for p in POSITIONS:
        x, y = world_xy(p["x"], p["y"])
        gz = ground_z(world, x, y)
        if gz is None:
            unreal.log_warning("%s: no ground at profile (%s, %s), skipped" % (p["label"], p["x"], p["y"]))
            continue
        for piece in parapet_pieces(p["front_width"], p["side_depth"]):
            placed_pieces += spawn_piece(eas, cube, x, y, gz, piece, p["yaw"], p["label"])
        for i, slot in enumerate(p["soldiers"]):
            placed_soldiers += spawn_soldier(eas, world, x, y, slot, p["yaw"], p["label"], i)

    manager = eas.spawn_actor_from_class(unreal.InfantryManager, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator())
    manager.tags = [unreal.Name(INFANTRY_TAG)]
    manager.set_folder_path(FOLDER)
    manager.set_actor_label("InfantryManager")

    unreal.log("Infantry positions done. Cleared %d previous, placed %d parapet pieces, "
               "%d soldiers, 1 manager." % (removed, placed_pieces, placed_soldiers))


main()
