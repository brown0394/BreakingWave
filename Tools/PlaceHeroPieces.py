# Assembles the grey-box hero pieces onto the BreakingWave landscape:
#   3 bunkers in Zone 4 (hollow, sea-facing firing slit, rear door for trench access)
#   3 landing craft in Zone 0 (open hull, dropped ramp slab on the sand at the bow)
#
# Run from inside the UE editor (with Lvl_FirstPerson open):
#   Tools menu > Execute Python Script... > pick this file
#
# Idempotent: every actor is tagged GREYBOX_TAG and grouped in per-assembly subfolders
# under "GreyboxHeroPieces". Re-running deletes the previous batch first, so edit the
# tables below and re-run freely. Pieces stay individually selectable for hand-tweaking.
#
# Coordinate convention (matches PlaceBeachObstacles.py):
#   table values are heightmap-profile meters, +Y = inland; world position =
#   auto-detected landscape min corner + meters * 100.
#   Piece placement is pivot-agnostic: each piece is positioned by its desired volume
#   CENTER, with the actor location derived from the mesh's actual bounds.
#
# All numbers tentative per CLAUDE.md.

import unreal
import math

M = 100.0
GREYBOX_TAG = "GreyboxHero"
FOLDER = "GreyboxHeroPieces"
CUBE = "/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"

WALL_M = 0.5
ROOF_M = 0.5
SLIT_BOTTOM_M = 1.2
SLIT_TOP_M = 1.6
DOOR_WIDTH_M = 1.0
BUNKER_EMBED_M = 0.5

BUNKERS = [
    {"label": "Bunker_MG_Left",  "x": 200, "y": 620, "yaw": 0.0, "width": 6.0, "depth": 5.0, "height": 2.6, "slit_width": 3.0},
    {"label": "Bunker_Command",  "x": 510, "y": 635, "yaw": 0.0, "width": 8.0, "depth": 6.0, "height": 2.8, "slit_width": 4.0},
    {"label": "Bunker_MG_Right", "x": 800, "y": 620, "yaw": 0.0, "width": 6.0, "depth": 5.0, "height": 2.6, "slit_width": 3.0},
]

CRAFT_HULL_WIDTH_M = 3.2
CRAFT_HULL_LENGTH_M = 9.0
CRAFT_FLOOR_M = 0.3
CRAFT_WALL_THICK_M = 0.25
CRAFT_WALL_HEIGHT_M = 1.5
RAMP_WIDTH_M = 3.0
RAMP_LENGTH_M = 2.5
RAMP_THICK_M = 0.15

LANDING_CRAFT = [
    {"label": "LandingCraft_A", "x": 230, "y": 270, "yaw": 4.0},
    {"label": "LandingCraft_B", "x": 510, "y": 270, "yaw": -3.0},
    {"label": "LandingCraft_C", "x": 790, "y": 270, "yaw": 7.0},
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


def bunker_pieces(width, depth, height, slit_width):
    wall_h = height - ROOF_M
    half_w, half_d = width / 2.0, depth / 2.0
    interior_w = width - 2.0 * WALL_M
    pier_w = (interior_w - slit_width) / 2.0
    front_y = -(half_d - WALL_M / 2.0)
    back_y = half_d - WALL_M / 2.0
    seg_w = (interior_w - DOOR_WIDTH_M) / 2.0
    return [
        ("wall_left",        -(half_w - WALL_M / 2.0), 0.0, wall_h / 2.0, WALL_M, depth, wall_h),
        ("wall_right",         half_w - WALL_M / 2.0,  0.0, wall_h / 2.0, WALL_M, depth, wall_h),
        ("front_pier_left",  -(slit_width / 2.0 + pier_w / 2.0), front_y, wall_h / 2.0, pier_w, WALL_M, wall_h),
        ("front_pier_right",   slit_width / 2.0 + pier_w / 2.0,  front_y, wall_h / 2.0, pier_w, WALL_M, wall_h),
        ("front_below_slit", 0.0, front_y, SLIT_BOTTOM_M / 2.0, slit_width, WALL_M, SLIT_BOTTOM_M),
        ("front_above_slit", 0.0, front_y, (SLIT_TOP_M + wall_h) / 2.0, slit_width, WALL_M, wall_h - SLIT_TOP_M),
        ("back_door_left",   -(DOOR_WIDTH_M / 2.0 + seg_w / 2.0), back_y, wall_h / 2.0, seg_w, WALL_M, wall_h),
        ("back_door_right",    DOOR_WIDTH_M / 2.0 + seg_w / 2.0,  back_y, wall_h / 2.0, seg_w, WALL_M, wall_h),
        ("roof", 0.0, 0.0, wall_h + ROOF_M / 2.0, width, depth, ROOF_M),
    ]


def craft_pieces():
    half_w = CRAFT_HULL_WIDTH_M / 2.0
    half_l = CRAFT_HULL_LENGTH_M / 2.0
    wall_z = CRAFT_FLOOR_M + CRAFT_WALL_HEIGHT_M / 2.0
    return [
        ("floor", 0.0, 0.0, CRAFT_FLOOR_M / 2.0, CRAFT_HULL_WIDTH_M, CRAFT_HULL_LENGTH_M, CRAFT_FLOOR_M),
        ("gunwale_left",  -(half_w - CRAFT_WALL_THICK_M / 2.0), 0.0, wall_z, CRAFT_WALL_THICK_M, CRAFT_HULL_LENGTH_M, CRAFT_WALL_HEIGHT_M),
        ("gunwale_right",   half_w - CRAFT_WALL_THICK_M / 2.0,  0.0, wall_z, CRAFT_WALL_THICK_M, CRAFT_HULL_LENGTH_M, CRAFT_WALL_HEIGHT_M),
        ("stern", 0.0, -(half_l - 0.15), wall_z, CRAFT_HULL_WIDTH_M, 0.3, CRAFT_WALL_HEIGHT_M),
    ]


def ramp_local_offset():
    return 0.0, CRAFT_HULL_LENGTH_M / 2.0 + RAMP_LENGTH_M / 2.0


def clear_previous(eas):
    removed = 0
    for actor in eas.get_all_level_actors():
        if actor.actor_has_tag(GREYBOX_TAG):
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
    # SM_Cube's pivot sits at its corner; snap by actual bounds so the piece's
    # volume center lands exactly on the target, whatever the pivot or rotation.
    origin, extent = actor.get_actor_bounds(False)
    actor.add_actor_world_offset(
        unreal.Vector(target.x - origin.x, target.y - origin.y, target.z - origin.z),
        False, False)
    actor.tags = [unreal.Name(GREYBOX_TAG)]
    actor.set_folder_path(FOLDER + "/" + assembly_label)
    actor.set_actor_label(assembly_label + "_" + name)
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
    placed = 0

    for b in BUNKERS:
        x, y = world_xy(b["x"], b["y"])
        gz = ground_z(world, x, y)
        if gz is None:
            unreal.log_warning("%s: no ground at profile (%s, %s), skipped" % (b["label"], b["x"], b["y"]))
            continue
        base_z = gz - BUNKER_EMBED_M * M
        for piece in bunker_pieces(b["width"], b["depth"], b["height"], b["slit_width"]):
            placed += spawn_piece(eas, cube, x, y, base_z, piece, b["yaw"], b["label"])

    for c in LANDING_CRAFT:
        x, y = world_xy(c["x"], c["y"])
        gz = ground_z(world, x, y)
        if gz is None:
            unreal.log_warning("%s: no ground at profile (%s, %s), skipped" % (c["label"], c["x"], c["y"]))
            continue
        for piece in craft_pieces():
            placed += spawn_piece(eas, cube, x, y, gz, piece, c["yaw"], c["label"])
        rdx, rdy = ramp_local_offset()
        rwx, rwy = yaw_rotate(rdx * M, rdy * M, c["yaw"])
        ramp_gz = ground_z(world, x + rwx, y + rwy)
        if ramp_gz is None:
            unreal.log_warning("%s: no ground for ramp, skipped" % c["label"])
            continue
        ramp = ("ramp", rdx, rdy, 0.0, RAMP_WIDTH_M, RAMP_LENGTH_M, RAMP_THICK_M)
        placed += spawn_piece(eas, cube, x, y, ramp_gz + RAMP_THICK_M / 2.0 * M, ramp, c["yaw"], c["label"])

    unreal.log("Hero pieces done. Cleared %d previous, placed %d pieces "
               "(%d bunkers, %d landing craft)." % (removed, placed, len(BUNKERS), len(LANDING_CRAFT)))


main()
