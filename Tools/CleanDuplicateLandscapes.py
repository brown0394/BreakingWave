# Reports the level's Landscape actors, then optionally deletes the duplicate parents.
# Workstream B's second half (Decision 055): the level carried three ALandscape parents -
# Landscape, Landscape2, Landscape4, all sitting at -50400/-50400 - of which only ONE (Landscape2)
# owned the 64 ALandscapeStreamingProxy actors that hold the actual geometry. The duplicates were
# deleted 2026-09-30; re-run this report-only after any heightmap re-import to confirm one parent.
#
# This is not cosmetic. It already cost a session: PlaceFog.py detected the landscape corner
# with isinstance(unreal.Landscape), which misses every proxy (ALandscapeStreamingProxy is a
# SIBLING of ALandscape, not a subclass), matched a degenerate duplicate parent first, and so
# traced every range marker off the map with no error reported. See 11_ENGINE_NOTES.md.
#
# Ownership is asked of each proxy: ALandscapeProxy::GetLandscapeActor is a BlueprintCallable
# UFUNCTION, and the streaming proxy's LandscapeActorRef is EditAnywhere. LandscapeGuid is a bare
# UPROPERTY and is never readable from Python, so it cannot be used to match.
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
# zero proxies, exactly one other parent owns all of them, and every proxy named its owner.
EXPECTED_PROXY_COUNT = 64


def actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def landscape_actors():
    found = []
    for actor in actor_subsystem().get_all_level_actors():
        class_name = actor.get_class().get_name()
        if "Landscape" in class_name:
            found.append((actor, class_name))
    return found


def owning_parent(proxy):
    try:
        owner = proxy.get_landscape_actor()
        if owner is not None:
            return owner
    except Exception:
        pass
    try:
        owner_ref = proxy.get_editor_property("landscape_actor_ref")
        return owner_ref.get() if owner_ref is not None else None
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

    proxies_by_owner = {}
    unresolved_proxies = []
    for proxy, _ in proxies:
        owner = owning_parent(proxy)
        if owner is None:
            unresolved_proxies.append(proxy)
        else:
            proxies_by_owner.setdefault(owner.get_name(), []).append(proxy)

    owners = []
    for parent, class_name in parents:
        name = parent.get_name()
        owned = len(proxies_by_owner.get(name, []))
        origin, extent = parent.get_actor_bounds(False)
        unreal.log("  %-50s label=%-12s owns_proxies=%d loc=(%.0f, %.0f) extent=(%.0f, %.0f)"
                   % (name, parent.get_actor_label(), owned,
                      parent.get_actor_location().x, parent.get_actor_location().y,
                      extent.x, extent.y))
        if owned > 0:
            owners.append((parent, owned))

    parent_names = set(p.get_name() for p, _ in parents)
    for owner_name, owned_proxies in proxies_by_owner.items():
        if owner_name not in parent_names:
            unreal.log_warning("  %d proxies name an owner that is not a parent in this level: %s"
                               % (len(owned_proxies), owner_name))

    for actor, class_name in others:
        unreal.log("  OTHER %s (%s)" % (actor.get_name(), class_name))

    if unresolved_proxies:
        unreal.log_warning("%d of %d proxies did not name an owner. Nothing deleted - identify the "
                           "owner in the editor: select any proxy and read 'Landscape Actor' in its "
                           "Details panel." % (len(unresolved_proxies), len(proxies)))
        return

    if len(owners) != 1:
        unreal.log_warning("Report is unclear: %d parents own proxies. Nothing deleted."
                           % len(owners))
        return

    owner, owned = owners[0]
    unreal.log("owner is %s (label %s) with %d proxies (expected %d)"
               % (owner.get_name(), owner.get_actor_label(), owned, EXPECTED_PROXY_COUNT))
    if owned != EXPECTED_PROXY_COUNT or owned != len(proxies):
        unreal.log_warning("Owner holds %d of %d proxies, expected %d. Nothing deleted - "
                           "check the level before proceeding."
                           % (owned, len(proxies), EXPECTED_PROXY_COUNT))
        return

    duplicates = [p for p, _ in parents if p != owner]
    if not DELETE_DUPLICATES:
        names = ", ".join("%s (label %s)" % (p.get_name(), p.get_actor_label())
                          for p in duplicates) if duplicates else "nothing"
        unreal.log("DELETE_DUPLICATES is False. Would delete: %s" % names)
        return

    for parent in duplicates:
        unreal.log("deleting duplicate parent %s (label %s)"
                   % (parent.get_name(), parent.get_actor_label()))
        actor_subsystem().destroy_actor(parent)
    unreal.log("Deleted %d duplicate Landscape parents. SAVE THE LEVEL." % len(duplicates))


main()
