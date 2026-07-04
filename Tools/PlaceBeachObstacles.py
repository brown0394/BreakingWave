# Places grey-box beach obstacles onto the BreakingWave landscape.
#
# Run from inside the UE editor (with Lvl_FirstPerson open):
#   Tools menu > Execute Python Script... > pick this file
#   or in the Output Log's Cmd (set to "Python"):  exec(open(r"<path>\Tools\PlaceBeachObstacles.py").read())
#
# It is idempotent: every actor it spawns is tagged GREYBOX_TAG and put in the
# "GreyboxObstacles" world outliner folder. Re-running deletes the previous batch
# first, so you can edit the tables below and re-run freely.
#
# Coordinate convention (matches GenerateBeachHeightmap.ps1 and 05_ZONES.md):
#   table values are heightmap-profile meters: +Y = inland (advance direction),
#   X = along-shore 0..1008 m. World position = landscape min corner + meters * 100
#   (the corner is auto-detected from the landscape bounds; the imported landscape
#   is centered on the origin, so the corner sits at about -50400,-50400).
#   Ground Z is found by tracing straight down onto the landscape collision.
#
# Hero pieces (3 bunkers in Zone 4, 3 landing craft in Zone 0) are placed BY HAND,
# not here - see the coordinate sheet in the chat / 05_ZONES.md.
#
# All numbers tentative per CLAUDE.md - edit the tables and re-run.

import unreal
import math

M = 100.0  # meters -> unreal units
GREYBOX_TAG = "GreyboxObstacle"
FOLDER = "GreyboxObstacles"

CUBE = "/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"
CYLINDER = "/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"
CHAMFER = "/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"

# Zone 2 obstacle field (rows / world Y 390-480): two staggered rows of Czech hedgehogs.
# Each row: spread `count` hedgehogs evenly between x_start..x_end at the given inland Y.
HEDGEHOG_ROWS = [
    {"y": 408, "x_start": 70,  "x_end": 950, "count": 11},
    {"y": 446, "x_start": 105, "x_end": 985, "count": 11},
]
HEDGEHOG_LEG_M = 2.2     # bar length (m)
HEDGEHOG_THICK_M = 0.18  # bar thickness (m)

# One barbed-wire line just in front of the Zone 2/3 berm (berm sits at Y 478).
BARBED_WIRE = {"y": 470, "x_start": 60, "x_end": 960, "post_spacing_m": 6}
WIRE_POST_HEIGHT_M = 1.0
WIRE_POST_RADIUS_M = 0.08

# Zone 3 upper beach (rows / world Y 480-580): debris piles for cover.
# Placed between the three dunes (dune centers are at X ~ 250 / 510 / 770).
DEBRIS_PILES = [
    {"x": 150, "y": 512},
    {"x": 390, "y": 540},
    {"x": 640, "y": 508},
    {"x": 880, "y": 545},
]
# deterministic cluster: (dx_m, dy_m, size_x_m, size_y_m, size_z_m, yaw_deg)
DEBRIS_CLUSTER = [
    (0.0,  0.0, 1.6, 1.2, 0.9,  12),
    (1.3, -0.6, 1.0, 1.4, 0.6, -25),
    (-1.1, 0.7, 1.2, 1.0, 1.1,  40),
    (0.4,  1.5, 0.9, 0.9, 0.5,   8),
    (-0.8,-1.2, 0.7, 1.1, 0.7, -55),
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
    start = unreal.Vector(x, y, 50000.0)
    end = unreal.Vector(x, y, -50000.0)
    result = unreal.SystemLibrary.line_trace_single(
        world, start, end,
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        True, [], unreal.DrawDebugTrace.NONE, True)
    if result is None:
        return None
    # UE 5.6 Python exposes no attributes on HitResult; to_tuple() is the only
    # way to read it. Field order: blocking_hit first, location/impact_point are
    # the first Vector fields (identical for a blocking line trace).
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
        if actor.actor_has_tag(GREYBOX_TAG):
            eas.destroy_actor(actor)
            removed += 1
    return removed


def spawn_box(eas, mesh, x, y, ground, sx, sy, sz, rot, label):
    actor = eas.spawn_actor_from_object(mesh, unreal.Vector(x, y, ground), rot)
    actor.set_actor_scale3d(unreal.Vector(sx, sy, sz))
    # SM_Cube's pivot sits at its corner; snap by actual bounds so the piece
    # stands centered on (x, y) with its underside on the ground, whatever the
    # pivot or rotation.
    origin, extent = actor.get_actor_bounds(False)
    actor.add_actor_world_offset(
        unreal.Vector(x - origin.x, y - origin.y, ground + extent.z - origin.z),
        False, False)
    actor.tags = [unreal.Name(GREYBOX_TAG)]
    actor.set_folder_path(FOLDER)
    actor.set_actor_label(label)
    return actor


def place_hedgehog(eas, mesh, world, x_m, y_m, yaw, idx):
    x, y = world_xy(x_m, y_m)
    z = ground_z(world, x, y)
    if z is None:
        unreal.log_warning("Hedgehog %d: no ground at (%.0f, %.0f), skipped" % (idx, x_m, y_m))
        return 0
    leg, thick = HEDGEHOG_LEG_M, HEDGEHOG_THICK_M
    # two bars crossed in the plane facing the advancing soldier
    # (unreal.Rotator argument order is roll, pitch, yaw)
    for tilt in (45.0, -45.0):
        spawn_box(eas, mesh, x, y, z,
                  thick, thick, leg,
                  unreal.Rotator(0.0, tilt, yaw),
                  "Hedgehog_%02d" % idx)
    return 1


def place_debris(eas, mesh, world, cx_m, cy_m, idx):
    placed = 0
    for j, (dx, dy, sx, sy, sz, yaw) in enumerate(DEBRIS_CLUSTER):
        x, y = world_xy(cx_m + dx, cy_m + dy)
        z = ground_z(world, x, y)
        if z is None:
            continue
        spawn_box(eas, mesh, x, y, z, sx, sy, sz,
                  unreal.Rotator(0.0, 0.0, yaw),
                  "Debris_%02d_%d" % (idx, j))
        placed += 1
    return placed


def place_wire(eas, mesh, world):
    span = BARBED_WIRE["x_end"] - BARBED_WIRE["x_start"]
    n = int(span / BARBED_WIRE["post_spacing_m"]) + 1
    placed = 0
    for i in range(n):
        x_m = BARBED_WIRE["x_start"] + i * BARBED_WIRE["post_spacing_m"]
        x, y = world_xy(x_m, BARBED_WIRE["y"])
        z = ground_z(world, x, y)
        if z is None:
            continue
        spawn_box(eas, mesh, x, y, z,
                  WIRE_POST_RADIUS_M * 2, WIRE_POST_RADIUS_M * 2, WIRE_POST_HEIGHT_M,
                  unreal.Rotator(0.0, 0.0, 0.0),
                  "WirePost_%03d" % i)
        placed += 1
    return placed


def main():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    corner = landscape_min_corner(eas)
    if corner is None:
        unreal.log_error("No Landscape found in the open level. Open Lvl_FirstPerson and retry.")
        return
    CORNER["x"], CORNER["y"] = corner
    unreal.log("Landscape min corner: (%.0f, %.0f)" % corner)
    test_x, test_y = world_xy(504, 300)
    unreal.log("Test trace at profile (504m, 300m) -> world (%.0f, %.0f) -> z=%r"
               % (test_x, test_y, ground_z(world, test_x, test_y)))

    cube = unreal.load_asset(CUBE)
    cylinder = unreal.load_asset(CYLINDER)
    chamfer = unreal.load_asset(CHAMFER)

    removed = clear_previous(eas)

    hedgehogs = 0
    idx = 0
    for row in HEDGEHOG_ROWS:
        count = row["count"]
        for i in range(count):
            t = 0.0 if count == 1 else i / float(count - 1)
            x_m = row["x_start"] + t * (row["x_end"] - row["x_start"])
            yaw = ((idx * 37) % 31) - 15  # deterministic facing variation
            hedgehogs += place_hedgehog(eas, cube, world, x_m, row["y"], yaw, idx)
            idx += 1

    wire = place_wire(eas, cylinder, world)

    debris = 0
    for k, pile in enumerate(DEBRIS_PILES):
        debris += place_debris(eas, chamfer, world, pile["x"], pile["y"], k)

    unreal.log("Greybox placement done. Cleared %d previous. "
               "Hedgehogs %d, wire posts %d, debris blocks %d."
               % (removed, hedgehogs, wire, debris))


main()
