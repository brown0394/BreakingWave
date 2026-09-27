# Reports the level's Landscape actors, then optionally deletes the duplicate parents.
# Workstream B's second half (Decision 055): the level carries three ALandscape parents -
# Landscape, Landscape2, Landscape4, all sitting at -50400/-50400 - of which only ONE owns the
# 64 ALandscapeStreamingProxy actors that hold the actual geometry.
#
# This is not cosmetic. It already cost a session: PlaceFog.py detected the landscape corner
# with isinstance(unreal.Landscape), which misses every proxy (ALandscapeStreamingProxy is a
# SIBLING of ALandscape, not a subclass), matched a degenerate duplicate parent first, and so
# traced every range marker off the map with no error reported. See 11_ENGINE_NOTES.md.
#
# Run from inside the UE editor (with Lvl_FirstPerson open):
#   Tools menu > Execute Python Script... > pick this file
#
# REPORT FIRST. Deleting a Landscape parent that owns proxies destroys the terrain, and which
# name owns them is not guaranteed to be the lowest-numbered one. Leave DELETE_DUPLICATES False,
# read the report, confirm exactly one parent is reported as owning 64 proxies, and only then
# set it True and re-run. Save the level afterwards.

import unreal

DELETE_DUPLICATES = False

# Belt and braces: even with DELETE_DUPLICATES True, a parent is only ever deleted if it owns
# zero proxies AND another parent was found owning some. Nothing is deleted on an unclear report.
EXPECTED_PROXY_COUNT = 64


def landscape_actors():
    found = []
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        class_name = actor.get_class().get_name()
        if "Landscape" in class_name:
            found.append((actor, class_name))
    return found


def read_property(actor, name):
    try:
        return actor.get_editor_property(name)
    except Exception:
        return None


def guid_text(actor):
    value = read_property(actor, "landscape_guid")
    return str(value) if value is not None else "<unreadable>"


def owning_parent_name(proxy):
    owner = read_property(proxy, "landscape_actor")
    if owner is None:
        return None
    try:
        return owner.get_name()
    except Exception:
        return None


def main():
    actors = landscape_actors()
    parents = [(a, c) for a, c in actors if c == "Landscape"]
    proxies = [(a, c) for a, c in actors if "StreamingProxy" in c]
    others = [(a, c) for a, c in actors if c != "Landscape" and "StreamingProxy" not in c]

    unreal.log("--- Landscape actors in the level ---")
    unreal.log("parents: %d   streaming proxies: %d   other Landscape-named: %d"
               % (len(parents), len(proxies), len(others)))

    proxies_by_guid = {}
    proxies_by_owner = {}
    for proxy, _ in proxies:
        proxies_by_guid.setdefault(guid_text(proxy), []).append(proxy)
        owner = owning_parent_name(proxy)
        if owner:
            proxies_by_owner.setdefault(owner, []).append(proxy)

    owners = []
    for parent, class_name in parents:
        name = parent.get_name()
        guid = guid_text(parent)
        by_guid = len(proxies_by_guid.get(guid, []))
        by_owner = len(proxies_by_owner.get(name, []))
        origin, extent = parent.get_actor_bounds(False)
        unreal.log("  %-14s label=%-14s guid=%s  proxies_by_guid=%d proxies_by_owner=%d "
                   "loc=(%.0f, %.0f) extent=(%.0f, %.0f)"
                   % (name, parent.get_actor_label(), guid, by_guid, by_owner,
                      parent.get_actor_location().x, parent.get_actor_location().y,
                      extent.x, extent.y))
        if max(by_guid, by_owner) > 0:
            owners.append((parent, max(by_guid, by_owner)))

    for actor, class_name in others:
        unreal.log("  OTHER %s (%s)" % (actor.get_name(), class_name))

    if len(owners) != 1:
        unreal.log_warning("Report is unclear: %d parents appear to own proxies. Nothing deleted. "
                           "Neither landscape_guid nor landscape_actor may be exposed to Python in "
                           "this build - if both columns read 0 everywhere, identify the owner in the "
                           "editor outliner (expand each Landscape) before deleting anything."
                           % len(owners))
        return

    owner, owned = owners[0]
    unreal.log("owner is %s with %d proxies (expected %d)"
               % (owner.get_name(), owned, EXPECTED_PROXY_COUNT))
    if owned != EXPECTED_PROXY_COUNT:
        unreal.log_warning("Proxy count %d does not match the expected %d. Nothing deleted - "
                           "check the level before proceeding." % (owned, EXPECTED_PROXY_COUNT))
        return

    duplicates = [p for p, _ in parents if p != owner]
    if not DELETE_DUPLICATES:
        names = ", ".join(p.get_name() for p in duplicates) if duplicates else "nothing"
        unreal.log("DELETE_DUPLICATES is False. Would delete: %s" % names)
        return

    for parent in duplicates:
        unreal.log("deleting duplicate parent %s" % parent.get_name())
        unreal.EditorLevelLibrary.destroy_actor(parent)
    unreal.log("Deleted %d duplicate Landscape parents. SAVE THE LEVEL." % len(duplicates))


main()
