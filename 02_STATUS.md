# Current Status

> Last updated: 2026-09-30. CLAUDE.md imports this file into every session, so it holds the
> current state and what's next — nothing else — and stays under ~150 lines. **Replace stale
> lines; never append history.** Session history is git log; the why of a choice is
> `03_DECISIONS.md`; engine traps are `11_ENGINE_NOTES.md`.

## Phase: the beach arc (Decisions 042–056)

Tuning detail on a beach whose systems do not exist yet measures absence, not balance, so the arc
builds the missing systems terrain-first (Decision 055). **A (fog) is done. B is done except
the walk**: the trench is in the level and trace-verified, and the duplicate Landscape parents are
gone. Nothing from C onward is started.

## RESUME HERE

1. **PIE-test `cea023a` — it has never run.** `FInfantrySoldierState` now owns `Position` and
   `FacingYaw`; `SyncSoldierShell` is the only write back to the actor. Check only that the seven
   Zone 3 riflemen still rise, fire, flinch and ragdoll as before. No change is expected, so any
   difference is a regression.
2. **Walk the trench** — B's last step. Does a 1.2 m carve read as a trench from the beach? Does
   the 28% communication-trench climb read as a trench or a ramp? Decide the noise corridor
   (`05_ZONES.md`, *Known wart*). Judge zone sizes and time the zones on the same walk (A's
   leftovers).
3. **Write the 5.6 heightmap re-import menu path** into `11_ENGINE_NOTES.md` *Level / landscape*
   while it is fresh — the 2026-09-30 re-import did not record it.

## The arc — build order (Decision 055)

Only E has no playable state. The definition of done and what is out of scope are in Decision 055.

- [x] **A — Fog.** ~35 m (density 1.0, falloff 0.05, start 500 cm), set by `Tools/PlaceFog.py`.
  Open: revisit once allies render ("can I see the man I am about to become"); judge zone sizes
  and record zone transit times in `05_ZONES.md` — never done.
- [~] **B — Landscape.** Centerline, carve (056), duplicate cleanup, re-import and trace
  verification done; the walk remains.
- [ ] **C — Trench geometry + nodes.** One in-editor script consumes `TrenchCenterline.json` and
  emits parapet, firing steps, saps and ~120 tagged `TrenchNode` actors; then a level save.
- [ ] **D — Craft.** 7–9 hulls with an arrive/ground/ramp/disgorge/depart cycle; boatloads replace
  the 1.5/s trickle; `MaxAlive` 128 → ~300 (048, 049).
- [ ] **E — Merge (043).** One soldier system, ~850 lines. **Nothing is playable while it is in
  flight.** Exit on parity: the game plays exactly as today, measured against existing telemetry.
  `BeachAllySim` stays as a switchable, still-runnable debug path.
- [ ] **F — Graph movement.** A* over the node graph, node reservation, infantry relocation, MG crew
  reinforcement up the communication trench (046, 047).
- [ ] **G — Fire.** Ally fire with zone gating and an `AllyRifle` collision row, muzzle-flash
  conspicuity for everyone including the player (044), fire discipline (045, 053), player muzzle
  lift (053), ally spread row (054).
- [ ] **H — Population.** Ally corpses with a cap — a prerequisite, since **allies leave no body
  today**; hang it off `KillAlly` only, never `ClaimAlly`. Flat cover array with corpses as cover
  points, four personalities, Leader influence and shouts (054).
- [ ] **I — The line.** ~90 defenders in strongpoints thin at the seams (055); ally breakthrough
  into the trench; local section fall and retake (050).
- [ ] **J — Telemetry.** The 2 Hz `disc_allies` sample calls `CountAlliesInDisc`, which skips the
  `TakeoverForwardReach` filter the takeover row applies — two meanings under one name. Record
  live ally positions. Fix `AnalyzePlaytests.py` crashing on a cp949 console at the `AGGREGATES`
  em-dash.

## Code-health checklist

From the 2026-09-05 architecture read. Nothing is broken today; these grow with the populations D
and I multiply. The two pre-merge landmines were cleared in `cea023a`.

Cheap and independent:
- [ ] **Stagger the MG perception clocks.** `FMGBunkerState::EvalTimer` starts at 0 and `BeginPlay`
  never randomises it, so all three guns trace on the same frame — the stop-clock desync's bug
  class. One `FRandRange`.
- [ ] **Rate-limit `AAllySimManager::TraceGroundZ` and give it `FCollisionQueryParams`.** A 100 m
  trace per ally per tick (~7,700/s at 128, ~18,000/s at 300); on `ECC_Visibility` with no params
  it snaps allies onto bunkers, obstacles and the player — the failure `TakeoverMaxGroundStep`
  fixed on the takeover path only.
- [ ] **Move `CountAlliesInDisc` behind the 2 Hz sample gate** in `AMGBunkerManager::Tick`: a
  full-array scan per frame, ~97% discarded. Same edit as J's semantics fix.
- [ ] **Promote the scoring literals into `FMGSettings`.** `ScoreTarget` hard-codes `100.f`,
  `0.25f/0.75f`, the unseen floor `5.f` and the broke-cover `500.f`, while `FiredUponScoreBonus`
  is balanced against that 500 from another file. Same shape in `SelectSoldierTarget`.
- [ ] **Randomise the shared-target scan order.** `IsTargetedByAnotherGun` reads guns in index
  order, so bunker 0 always picks freely and bunker 2 always eats `SharedTargetScorePenalty`.

Design into E rather than port:
- [ ] **A cheap gate before the LOS trace in `SelectSoldierTarget`.** The MG rejects most of the
  beach with one slit-arc angle test; infantry trace every candidate in range — ~27,000 traces per
  0.4 s round at I's ~90 defenders against D's ~300 allies.
- [ ] **A broadphase for bullet-vs-soldier in `UpdateBullets`** — today the full ally array per
  bullet, `bAlive` the only early-out. D raises the population and G arms it at once.
- [ ] **Slice perception across ticks** rather than shrinking the wave.
- [ ] **Size the MG awareness array safely.** `AwarenessSlots = MaxAlive + 1` at `BeginPlay`, no
  bounds check in `AwarenessFor` — out of bounds once D grows the ally array at runtime.

Watch, no action yet: the 60 s pre-warm is ~10k ground traces at load (the rate-limit mostly fixes
it; re-measure after D); `TryTakeover` re-iterates actors per death where managers cache peers.

## Open numbers and standing calls

- **Open, tune once the systems exist**: craft count and cadence, boatload size, final `MaxAlive`,
  ally corpse cap, every spread value. The trench route stays tunable: edit
  `Tools/TrenchCenterline.json`, re-run the generator, and re-derive the contour rows if `Profile`
  or `BluffWaviness` changes.
- **Three constants are called "fog" and they are different distances.** Only
  `FAllySimSettings.TakeoverRadius` (3500 = 35 m) tracks player visibility.
  `FInfantrySettings.MaxEngagementRange` (12000) and `FMGSettings.VisibilityMaxRange` (50000) do
  not: setting the MG to 35 m deletes the Zone 1 kill zone, 230–310 m from the bunkers.
- **Cancelled, not deferred**: the MG + infantry `PlayerTargetScoreMultiplier` tuning batch (coded
  at 3.0, untuned). Decision 044 makes the player's score situational.
- **Do not tune against the 2026-08-22 or earlier batches** (047): 048–055 change ally supply,
  lethality, the line and the terrain. Takeover density tuning is on hold for the same reason.
- **The wounded presentation stays unbuilt** — reason in `06_COMBAT.md`.
- **Watch prone.** 0 of 12 takeovers on 08-22 landed on a prone ally (expected ~20%, p ≈ 7%) — a
  bug if the next batch is also 0. On 08-17 the player was prone in 3.7% of samples: not used.

## What exists

**Code** (`Source/BreakingWave/`; structure and reasons in `12_ARCHITECTURE.md`). All numbers
tentative; each system's knobs live in its settings struct.
- **Player** (`BreakingWaveCharacter`) — walk 600 / sprint 900, no jump; stationary prone on C,
  dive-into-prone when moving; rifle (LMB hip, RMB aimed, R reload, infinite reserves);
  mesh-authoritative hits (head kills, otherwise the second wound kills); hit shake + pain grunt.
  Feel-checked. **Unverified**: whether the FP ABP has the `DefaultSlot` the rifle montages play
  on — if the arms don't animate, add a Slot node to `ABP_FP_Copy` or wait for the visual pass.
- **Camera** — headbob and hit shake as engine camera-shake patterns (026, 039). Feel-checked.
- **Death → takeover** (`BreakingWavePlayerController`, 038–041) — phase machine, scripted death
  camera, zero-duration narrative seam, 35 m takeover disc with a manufacture fallback, new pawn
  per life, capped ragdoll corpses, targeting lockout. Feel-checked 08-17, verified 08-22.
- **MG bunkers** (`MGBunkerSystem`, 027–033) — 3 guns, 6-man crews, belt/heat stops, perception,
  priority ladder, target lead, and the **shared bullet array** every shooter uses.
- **Ally sim** (`BeachAllySim`, 029) — unrendered structs, `MaxAlive` 128, 60 s pre-warm. Allies
  cannot shoot and leave no body.
- **Zone 3 infantry** (`BeachInfantrySystem`, 036) — 7 riflemen: cover → rise → aim → 1–3 bolt
  shots → drop, flinch layer, ragdoll. No relocation.
- **Telemetry** (`PlaytestRecorder`) and **debug commands** — see `12_ARCHITECTURE.md` §13, §15.

**Tools** (`Tools/`) — every script is idempotent and has been run. All audio is synthesized placeholder. `BakeIKBonesFromFK.py`
**must re-run after any IK retarget**; `AddSprintToLocomotion.py` is superseded, kept for history.

**Level** (`Lvl_FirstPerson`) — landscape at 100/100/200 with dunes, berm and 13 craters and
the trench (trace-verified 16/16), one Landscape parent (`Landscape2`) owning 64 proxies; 22 hedgehogs, 151 wire posts,
20 debris blocks; 3 greybox bunkers and 3 landing craft; 3 MG guns + manager; 7 infantry shells +
manager + 12 parapet pieces; fog at ~35 m.

## Backlog outside the arc

- Writing: the second character ("the one who shook me"); the first enemy character
- Dive-into-prone polish (closed as good enough): if the flight ever feels flat, add a brief
  additive camera pitch-down
