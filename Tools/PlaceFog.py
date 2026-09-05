# Places the beach fog (Decision 055 workstream A: fog goes FIRST, so nothing downstream
# is tuned twice) plus an optional range-marker ruler for calibrating it by eye.
#
# Fog is load-bearing per 01_SOUL.md Principle 5: it removes time sense, limits visibility,
# and licenses NPC spawn/despawn outside view. It also sets the numbers several systems read
# (Decision 055) - see the WHAT FOG FEEDS note below before propagating anything.
#
# Run from inside the UE editor (with Lvl_FirstPerson open) - spawning crashes headless:
#   Tools menu > Execute Python Script... > pick this file
#
# Idempotent: everything is tagged FOG_TAG; re-running clears the previous batch. Edit the
# constants below and re-run to iterate. All numbers tentative per CLAUDE.md.
#
# WHAT FOG FEEDS (Decision 055) - do NOT blanket-propagate one number into all of these:
#   FAllySimSettings.TakeoverRadius   3500  (35 m)  - "the man you could have seen". Tracks
#                                                     player visibility directly.
#   FInfantrySettings.MaxEngagementRange 12000 (120 m) - comment calls it a fog stand-in.
#   FMGSettings.VisibilityMaxRange   50000 (500 m)  - the MGs must still reach Zone 1, the
#                                                     kill zone at 230-310 m. Setting this to
#                                                     player visibility deletes the kill zone.
# Player visibility and weapon engagement range are different distances. Settle them in the
# walkthrough, one at a time, not with a find-and-replace.

import unreal

M = 100.0
FOG_TAG = "BeachFog"
MARKER_TAG = "BeachFogMarker"
FOLDER = "Fog"
CYLINDER = "/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"

# --- fog knobs -------------------------------------------------------------------------
# Drafted in 02_STATUS.md, never yet run. Target is ~35 m visibility (09_ALLY_NPC.md still
# lists 30 m vs 40 m as open - this walkthrough is what settles it).
# Iterating: extinction is exponential, so DOUBLING density roughly HALVES the distance at
# which a shape disappears. Change one knob at a time and re-run.
FOG_DENSITY = 0.5           # the main dial
FOG_HEIGHT_FALLOFF = 0.05   # low = the fog layer stays thick well above head height
FOG_MAX_OPACITY = 1.0       # 1.0 = things genuinely vanish rather than staying ghosted
FOG_START_DISTANCE = 500.0  # cm; keeps the near field clear so held items stay readable
FOG_ACTOR_Z_M = 0.0         # fog origin at the waterline; falloff is measured from here
VOLUMETRIC_FOG = False      # off for the greybox pass - it is a large frame cost
FOG_COLOR = (0.42, 0.44, 0.47)  # flat overcast grey; "gray, hazy, only shapes visible"

# --- calibration ruler ------------------------------------------------------------------
# A stand-here post plus numbered posts at known distances inland, so "35 m visibility" is
# measured instead of guessed: stand at the base post, look inland, note which post is the
# last one you can make out. Set to False and re-run to remove them once fog is settled.
PLACE_RANGE_MARKERS = True
MARKER_LANE_X = 510         # profile x - the centre lane, between the craft and the bunkers
MARKER_BASE_Y = 320         # profile y - Zone 1, flat ground, clear sightline inland
MARKER_RANGES_M = [10, 20, 30, 40, 50, 60, 80, 100]
MARKER_HEIGHT_M = 1.8       # a man's height, so it reads as "could I see a soldier there"
MARKER_RADIUS_M = 0.15

CORNER = {"x": 0.0, "y": 0.0}


def landscape_min_corner(eas):
    # Match on class name, not isinstance(unreal.Landscape): the 64 LandscapeStreamingProxy
    # actors are siblings of ALandscape rather than instances of it, and the level carries
    # three duplicate Landscape parents of which only one owns the proxies. Taking the first
    # match can return a degenerate corner - min across every non-empty one instead.
    min_x = min_y = None
    for actor in eas.get_all_level_actors():
        if "Landscape" not in actor.get_class().get_name():
            continue
        origin, extent = actor.get_actor_bounds(False)
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


def clear_previous(eas):
    # FOG_TAG also matches markers left by script versions that predate MARKER_TAG, so sweep
    # both - minus the fog actor itself, which now carries FOG_TAG and must survive.
    removed = 0
    for actor in eas.get_all_level_actors():
        if isinstance(actor, unreal.ExponentialHeightFog):
            continue
        if actor.actor_has_tag(MARKER_TAG) or actor.actor_has_tag(FOG_TAG):
            eas.destroy_actor(actor)
            removed += 1
    return removed


def try_set(obj, name, value):
    """Property names drift between engine versions; report misses instead of aborting."""
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as exc:
        unreal.log_warning("fog: could not set '%s' (%s) - set it by hand in Details" % (name, exc))
        return False


def fog_component(actor):
    try:
        component = actor.get_editor_property("component")
        if component is not None:
            return component
    except Exception:
        pass
    return actor.get_component_by_class(unreal.ExponentialHeightFogComponent)


def sole_fog_actor(eas):
    # The renderer reads Scene->ExponentialFogs[0] and ignores every other height fog in the
    # level (RendererScene.cpp appends in registration order, with no priority or sorting).
    # The FirstPerson template ships its own ExponentialHeightFog, so spawning a second one
    # produces an actor that is tuned but never drawn. Reduce the level to exactly one and
    # retune that, which removes the ordering question entirely rather than betting on it.
    found = [a for a in eas.get_all_level_actors()
             if isinstance(a, unreal.ExponentialHeightFog)]
    if not found:
        actor = eas.spawn_actor_from_class(
            unreal.ExponentialHeightFog,
            unreal.Vector(0.0, 0.0, FOG_ACTOR_Z_M * M),
            unreal.Rotator())
        unreal.log("fog: no height fog in the level, spawned one.")
    else:
        pre_existing = [a for a in found if a.get_actor_label() != "BeachFog"]
        actor = pre_existing[0] if pre_existing else found[0]
        for extra in found:
            if extra != actor:
                unreal.log_warning("fog: destroying redundant height fog '%s' - the renderer "
                                   "only ever draws one." % extra.get_actor_label())
                eas.destroy_actor(extra)
        unreal.log("fog: adopted existing height fog '%s' (%d found)."
                   % (actor.get_actor_label(), len(found)))
    return actor


def place_fog(eas):
    actor = sole_fog_actor(eas)
    actor.tags = [unreal.Name(FOG_TAG)]
    actor.set_folder_path(FOLDER)
    actor.set_actor_label("BeachFog")
    actor.set_actor_location(unreal.Vector(0.0, 0.0, FOG_ACTOR_Z_M * M), False, False)

    component = fog_component(actor)
    if component is None:
        unreal.log_error("fog: found no ExponentialHeightFogComponent on the fog actor.")
        return actor

    colour = unreal.LinearColor(FOG_COLOR[0], FOG_COLOR[1], FOG_COLOR[2], 1.0)
    try_set(component, "fog_density", FOG_DENSITY)
    try_set(component, "fog_height_falloff", FOG_HEIGHT_FALLOFF)
    try_set(component, "fog_max_opacity", FOG_MAX_OPACITY)
    try_set(component, "start_distance", FOG_START_DISTANCE)
    try_set(component, "enable_volumetric_fog", VOLUMETRIC_FOG)
    if not try_set(component, "fog_inscattering_luminance", colour):
        try_set(component, "fog_inscattering_color", colour)
    return actor


def place_markers(eas, world):
    mesh = unreal.load_asset(CYLINDER)
    if mesh is None:
        unreal.log_warning("fog: cylinder mesh missing, skipping the calibration ruler.")
        return 0
    bb = mesh.get_bounding_box()
    native = bb.max - bb.min
    placed = 0

    for label, offset_m in [("STAND_HERE", 0)] + [("%dm" % r, r) for r in MARKER_RANGES_M]:
        x, y = world_xy(MARKER_LANE_X, MARKER_BASE_Y + offset_m)
        gz = ground_z(world, x, y)
        if gz is None:
            unreal.log_warning("fog: no ground under marker %s, skipped" % label)
            continue
        actor = eas.spawn_actor_from_object(mesh, unreal.Vector(x, y, gz), unreal.Rotator())
        actor.set_actor_scale3d(unreal.Vector(
            MARKER_RADIUS_M * 2 * M / native.x,
            MARKER_RADIUS_M * 2 * M / native.y,
            MARKER_HEIGHT_M * M / native.z))
        origin, extent = actor.get_actor_bounds(False)
        actor.add_actor_world_offset(
            unreal.Vector(x - origin.x, y - origin.y, gz + extent.z - origin.z), False, False)
        actor.tags = [unreal.Name(MARKER_TAG)]
        actor.set_folder_path(FOLDER + "/RangeMarkers")
        actor.set_actor_label("FogRange_" + label)
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

    probe_x, probe_y = world_xy(MARKER_LANE_X, MARKER_BASE_Y)
    unreal.log("fog: landscape corner (%.0f, %.0f); stand-here probe profile (%s, %s) "
               "-> world (%.0f, %.0f) -> ground z=%r"
               % (CORNER["x"], CORNER["y"], MARKER_LANE_X, MARKER_BASE_Y,
                  probe_x, probe_y, ground_z(world, probe_x, probe_y)))

    removed = clear_previous(eas)
    place_fog(eas)
    markers = place_markers(eas, world) if PLACE_RANGE_MARKERS else 0

    unreal.log("Fog done. Cleared %d previous markers, tuned 1 ExponentialHeightFog "
               "(density %s, falloff %s, start %s cm) and %d range markers. "
               "SAVE THE LEVEL, then walk it: stand at FogRange_STAND_HERE, look inland, "
               "and note the last post you can make out."
               % (removed, FOG_DENSITY, FOG_HEIGHT_FALLOFF, FOG_START_DISTANCE, markers))


main()
