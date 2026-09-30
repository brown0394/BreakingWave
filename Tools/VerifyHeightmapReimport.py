# Verifies a heightmap re-import by tracing the landscape at every anchor in 05_ZONES.md's
# "Re-import verification table" and comparing against the expected Z. Read-only: spawns,
# moves and saves nothing.
#
# A failed or mis-scaled import still looks like a beach, so this is the check, not the eye
# (11_ENGINE_NOTES.md, Level / landscape). The expected values are parsed from 05_ZONES.md, so
# the table keeps one home: re-derive it when the centerline or the Profile changes and this
# script follows.
#
# Traces ignore every non-landscape actor - the bunker-door anchors sit under the greybox
# bunkers, and a plain trace reports the roof.
#
# Run from inside the UE editor (with Lvl_FirstPerson open):
#   Tools menu > Execute Python Script... > pick this file

import os

import unreal

TOLERANCE_UU = 10.0
EXPECTED_PROXY_COUNT = 64
EXPECTED_CORNER_UU = -50400.0
EXPECTED_LANDSCAPE_SCALE = (100.0, 100.0, 200.0)
TABLE_HEADING = "### Re-import verification table"
TRACE_TOP_Z = 50000.0
TRACE_BOTTOM_Z = -50000.0


def verification_rows():
    zones_doc = os.path.join(
        unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "05_ZONES.md")
    with open(zones_doc, encoding="utf-8") as f:
        lines = f.read().splitlines()
    if TABLE_HEADING not in lines:
        return None
    rows = []
    for line in lines[lines.index(TABLE_HEADING) + 1:]:
        if line.startswith("#"):
            break
        if not line.startswith("| `"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        anchor = cells[0].strip("`")
        world_x, world_y = (float(v.replace("−", "-")) for v in cells[2].split("/"))
        expected_z = float(cells[4].replace("−", "-"))
        rows.append((anchor, world_x, world_y, expected_z))
    return rows


def is_landscape(actor):
    return "Landscape" in actor.get_class().get_name()


def report_landscape(all_actors):
    parents = [a for a in all_actors if a.get_class().get_name() == "Landscape"]
    proxies = [a for a in all_actors if "StreamingProxy" in a.get_class().get_name()]
    unreal.log("landscape: %d parent(s), %d streaming proxies loaded (expected 1 and %d)"
               % (len(parents), len(proxies), EXPECTED_PROXY_COUNT))
    for parent in parents:
        location = parent.get_actor_location()
        scale = parent.get_actor_scale3d()
        unreal.log("  %s: location (%.0f, %.0f, %.0f) scale (%.0f, %.0f, %.0f), expected scale %s"
                   % (parent.get_actor_label(), location.x, location.y, location.z,
                      scale.x, scale.y, scale.z, EXPECTED_LANDSCAPE_SCALE))

    min_x = min_y = None
    for proxy in proxies:
        origin, extent = proxy.get_actor_bounds(False)
        if extent.x <= 0 or extent.y <= 0:
            continue
        min_x = origin.x - extent.x if min_x is None else min(min_x, origin.x - extent.x)
        min_y = origin.y - extent.y if min_y is None else min(min_y, origin.y - extent.y)
    if min_x is not None:
        unreal.log("  min corner from proxy bounds (%.0f, %.0f), expected (%.0f, %.0f)"
                   % (min_x, min_y, EXPECTED_CORNER_UU, EXPECTED_CORNER_UU))

    healthy = len(parents) == 1 and len(proxies) == EXPECTED_PROXY_COUNT
    if not healthy:
        unreal.log_warning("landscape: not exactly 1 parent and %d proxies - a missing proxy "
                           "reads as 'no ground'; a second parent means the re-import made a new "
                           "landscape (run Tools/CleanDuplicateLandscapes.py)."
                           % EXPECTED_PROXY_COUNT)
    return healthy


def landscape_ground_hit(world, x, y, non_landscape_actors):
    result = unreal.SystemLibrary.line_trace_single(
        world, unreal.Vector(x, y, TRACE_TOP_Z), unreal.Vector(x, y, TRACE_BOTTOM_Z),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        True, non_landscape_actors, unreal.DrawDebugTrace.NONE, True)
    if result is None:
        return None, None
    fields = result.to_tuple()
    if not fields[0]:
        return None, None
    hit_z = next((f.z for f in fields if isinstance(f, unreal.Vector)), None)
    hit_actor = next((f for f in fields if isinstance(f, unreal.Actor)), None)
    return hit_z, hit_actor


def main():
    rows = verification_rows()
    if not rows:
        unreal.log_error("Found no rows under '%s' in 05_ZONES.md." % TABLE_HEADING)
        return

    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    all_actors = eas.get_all_level_actors()
    landscape_healthy = report_landscape(all_actors)
    non_landscape_actors = [a for a in all_actors if not is_landscape(a)]

    unreal.log("--- Re-import verification: %d anchors, tolerance %.0f uu ---"
               % (len(rows), TOLERANCE_UU))
    unreal.log("  %-20s %8s %8s %9s %9s %8s  %s"
               % ("anchor", "world x", "world y", "expected", "measured", "diff", "hit"))
    passed = 0
    worst_anchor, worst_diff = None, 0.0
    for anchor, world_x, world_y, expected_z in rows:
        hit_z, hit_actor = landscape_ground_hit(world, world_x, world_y, non_landscape_actors)
        if hit_z is None:
            unreal.log_warning("  %-20s %8.0f %8.0f %9.0f %9s %8s  NO GROUND"
                               % (anchor, world_x, world_y, expected_z, "-", "-"))
            worst_anchor, worst_diff = anchor, float("inf")
            continue
        diff = hit_z - expected_z
        hit_label = hit_actor.get_actor_label() if hit_actor is not None else "?"
        within = abs(diff) <= TOLERANCE_UU
        line = ("  %-20s %8.0f %8.0f %9.0f %9.1f %+8.1f  %s %s"
                % (anchor, world_x, world_y, expected_z, hit_z, diff, hit_label,
                   "ok" if within else "OFF"))
        if within:
            passed += 1
            unreal.log(line)
        else:
            unreal.log_warning(line)
        if abs(diff) > abs(worst_diff):
            worst_anchor, worst_diff = anchor, diff

    summary = ("%d/%d anchors within %.0f uu; worst %s at %+.1f uu"
               % (passed, len(rows), TOLERANCE_UU, worst_anchor, worst_diff))
    if passed == len(rows) and landscape_healthy:
        unreal.log("PASS: " + summary + ". The re-import took.")
    else:
        unreal.log_warning("FAIL: " + summary + ". Paste this report before walking the trench.")


main()
