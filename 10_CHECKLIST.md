# Development Checklist

Follow this order when coding.
Check each item when it's done, and update 02_STATUS.md.
Legend: `[x]` done · `[~]` partially done · `[ ]` not started.
"Relevant docs" per step = what to read before working on that step (in addition to the always-read docs in CLAUDE.md).

---

## Step 1 — Grey Box Beach
> Goal: Confirm the sense of space. "Can I actually run through this beach."

- [x] Create UE project (first-person template base)
- [~] Build beach terrain with BSP or basic geometry
  - [x] Heightmap-generated grey landscape imported
  - [ ] Slopes and boundaries for Zones 0–4 confirmed/adjusted (deferred with the walkthrough)
  - [x] Shell craters (3 deep + 10 shallow, raised rims, baked into heightmap)
  - [x] Czech hedgehog placement (Zone 2) — Tools/PlaceBeachObstacles.py
  - [x] Sand berms (baked into heightmap), debris piles (Zone 3, placement tool)
  - [x] Exterior of 3 bunkers (Zone 4) — Tools/PlaceHeroPieces.py
  - [x] 3 landing craft with ramps (Zone 0) — Tools/PlaceHeroPieces.py
- [ ] Fog setup (limit visibility to 30–40m) — DEFERRED until after more system work (2026-07-04)
- [ ] Walk through it yourself and feel whether the zone sizes are right — DEFERRED with fog

**When done**: Judge whether zone sizes feel appropriate. Adjust if too short or too long.
**Relevant docs**: 05_ZONES.md

---

## Step 2 — First-Person Movement
> Goal: Confirm the feel of running and going prone.

- [x] Create player character C++ class
- [x] Implement running
- [x] Implement prone (instant drop to ground) — C toggle (rebound from LeftControl 2026-07-15), stationary (Decision 025), transitions feel-checked 2026-07-08
- [x] Debug third-person view toggle (dev tool, not a feature) — F6, verified 2026-07-08
- [x] Sprint body animation reads as running — BS_Idle_Walk_Run rebuilt all-rifle with foot-true rates (Tools/RebuildLocomotionAsRifle.py), feel-checked PASSED 2026-07-18
- [x] Implement slide-into-prone (momentum slide) — built as a ballistic dive + short skid, feel-check closed as good-enough 2026-07-17
- [x] Implement headbob (primarily vertical bounce, small amplitude) — Decision 026, feel-checked PASSED 2026-07-18
- [ ] Run through each zone yourself and measure time — DEFERRED with fog until after base systems (2026-07-18); do together with the fog walkthrough
  - [ ] Zone 0 transit time: ___ seconds
  - [ ] Zone 1 transit time: ___ seconds
  - [ ] Zone 2 transit time: ___ seconds
  - [ ] Zone 3 transit time: ___ seconds
  - [ ] Zone 4 transit time: ___ seconds

**When done**: Record zone transit times in 05_ZONES.md.
**Relevant docs**: 04_PRINCIPLES.md, 07_CAMERA.md (headbob section)

---

## Step 3 — One MG
> Goal: Feel "run and die."

- [ ] Place one MG Actor in a bunker — Tools/PlaceMGCrew.py written 2026-07-19, run it in-editor
- [x] Implement MG targeting system (manager + state struct, Decision 021) — built 2026-07-19 w/ crewed gun + ally sim (Decisions 027–031)
  - [x] Priority-based target selection
  - [x] Rotation speed limit
  - [x] Factor-based accuracy
- [x] Implement MG stops — simulated from belt/heat, crew-tier multipliers (Decision 028)
  - [x] Reload (2–3 seconds)
  - [x] Overheat (4–6 seconds)
  - [x] Jam (2–8 seconds random)
- [ ] Run the beach yourself and test
  - [ ] How often do you get hit while running?
  - [ ] How long can you survive hiding in a crater?
  - [ ] Can you advance during MG stop windows?
  - [ ] Difficulty feel: too easy? impossible?

**When done**: Record accuracy, rotation speed, and stop frequency values in 08_ENEMY_AI.md.
**Relevant docs**: 08_ENEMY_AI.md, 06_COMBAT.md

---

## Step 4 — Getting Hit and Dying
> Goal: Feel "get hit and die."

> **Built 2026-08-16 (Decisions 039–040), NOT yet play-tested.** The damage *state* and the
> death camera *geometry* landed with the transition pass; the persistent wounded
> presentation and the death-camera filters were deliberately deferred to a later polish
> pass, because neither changes how often or how legibly the death→takeover seam fires.

- [x] Implement hit detection — mesh-authoritative: bounds broadphase + `LineTraceComponent`
  against the physics asset. The capsule is no longer a combat volume
- [x] Location-based damage (headshot = instant, body = two-shot). Limb tier still open
- [~] Wounded state presentation — DEFERRED as a block (Decision 039)
  - [ ] Red vignette (post-process)
  - [ ] Headbob stagger pattern
  - [ ] Reduced movement speed
  - [ ] Breathing/heartbeat audio
  - [ ] Increased aim sway
- [x] Hit camera — `UHitCameraShake`
  - [x] Camera shake
  - [x] Hit direction hint (push along the round's travel)
  - [x] Pain sound — Tools/GeneratePlayerPainAudio.py (**tool written, not yet run**)
- [~] Death camera (semi-scripted)
  - [x] Camera fall (ease-out) — scripted `ACameraActor`, never the ragdoll
  - [x] Tilt
  - [ ] Vision blur + narrowing — deferred (needs a post-process material)
  - [~] Audio low-pass filter — deferred; audio fades with the screen instead
    (`StartCameraFade` `bFadeAudio`)
  - [x] Terrain clip prevention (LineTrace lower bound)
  - [x] Fade out
- [ ] Play it yourself and check
  - [ ] Can you tell a survived hit from a miss on the shake and grunt alone?
  - [ ] Is the death moment emotional?
  - [ ] Does the camera clip into terrain?
  - [ ] Is prone still worth doing now that it is a real body on the ground?

**When done**: Record death camera fall values and wounded state values in 07_CAMERA.md.
**Relevant docs**: 07_CAMERA.md, 06_COMBAT.md

---

## Step 5 — Narrative + Transition
> Goal: One full loop. Proof that this game exists.

> **Mechanical half BUILT 2026-08-16 (Decisions 038–040), NOT yet play-tested.** The loop —
> death camera → fade → take over a nearby living ally → targeting delay → control — exists,
> with the narrative screen as a state of zero duration between FadeOut and the takeover.
> The narrative half (screen Option A vs B, text display, inserting the written narrative)
> drops into that seam without reshaping anything around it.

- [ ] Implement narrative screen
  - [ ] Option A: black background + white text
  - [ ] Option B: final view residue + text
  - [ ] Text display (one sentence at a time, fade-in or typing)
  - [ ] Compare both options → choose one
- [x] Spawn next character Pawn — a new pawn per life, possessed on takeover (Decision 040)
- [x] Implement transition system
  - [x] Death → [narrative, zero duration] → fade in → possess new character
  - [x] Starting state — inherited from the sim ally (heading, stance, motion), not scripted
    per zone. The Zone 0 "inside the landing craft" row of 07_CAMERA.md's table is NOT
    delivered by this pass; it needs Step 6's real allies
  - [x] Enemy targeting delay — awareness lockout (MG) + flat exclusion (infantry)
- [ ] Insert first character narrative text (your original writing)
- [ ] Play it yourself
  - [ ] Does the flow of run → die → new eyes open feel smooth? (no story in the seam yet)
  - [ ] Is ~3.5 s death-to-control the right leash at this frequency?
  - [ ] Does the emotion carry through?

**When this step is done, the core loop is running.**
This is where you'll know "does this game work."

**Relevant docs**: 01_SOUL.md, 07_CAMERA.md

---

## Step 6 — NPCs and Expansion
> Goal: Fill the battlefield.

- [ ] Allied NPC system
  - [ ] 4 types (Charger/Cautious/Frozen/Leader)
  - [ ] Fog-based spawn/despawn
  - [ ] Density reduction curve
  - [ ] Varied death animations (instant, screaming, crawling, groaning)
- [ ] Corpse system
  - [ ] Distance-tiered handling (full physics / collision only / static)
  - [ ] Corpse cover — physics-based penetration
  - [ ] Corpse count cap management
- [ ] Expand to 3 MGs + verify independent behavior
- [~] Enemy infantry AI (Zone 3) — pulled forward 2026-08-09 (Decisions 035–036)
  - [x] Behavior cycle (cover → rise → fire → cover) + flinch layer
  - [ ] Position movement (fire concentration, comrade death, enemy approach triggers) — deferred, see 02_STATUS.md
- [~] Ammo system
  - [x] Reload mechanic (2026-08-09, infinite reserves for now)
  - [x] Empty magazine feedback (dry click)
  - [ ] Looting when out of ammo (low probability spawn, auto-collect)
- [~] Firing system — shared rifle system 2026-08-09 (Decision 035)
  - [x] Hip fire
  - [x] Aimed fire (FOV narrows, movement locked)
  - [ ] Sway increase while wounded (waits for Step 4's wounded state)

**Relevant docs**: 08_ENEMY_AI.md, 09_ALLY_NPC.md, 06_COMBAT.md

---

## Step 7 — Enemy Playable
> Goal: The viewpoint crosses over to the enemy side.

- [ ] Implement enemy character transition
  - [ ] Camera direction reversal (facing the sea, looking down)
  - [ ] Enemy starting state (trench / foxhole / bunker)
- [ ] Enemy gameplay
  - [ ] Combat from defensive positions
  - [ ] Experience of shooting at Allied troops coming up the beach
- [ ] Insert enemy narrative text
- [ ] Test Allied → German → Allied transitions

**Relevant docs**: 01_SOUL.md, 07_CAMERA.md, 05_ZONES.md

---

## Step 8 — Polish
> Goal: Finished.

- [ ] Sound (gunfire, impacts, MG stop audio, ambient sound, NPC voices)
- [ ] Visual presentation (fog quality, lighting, particles)
- [ ] Insert full narrative and test flow
- [ ] Difficulty balancing (repeated full playthrough testing)
- [ ] Ending (game ends when high ground is breached)
- [ ] Handling when character pool is exhausted
- [ ] Performance optimization

---

## Document Usage Guide

Document routing lives in CLAUDE.md ("When to Read Each Document").
Read only the docs relevant to the task — the full set dilutes focus.
Exception: design discussions ("is this direction right?") warrant reading
01_SOUL.md + 03_DECISIONS.md + relevant docs broadly.
