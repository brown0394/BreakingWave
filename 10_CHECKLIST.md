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
  - [ ] Slopes and boundaries for Zones 0–4 confirmed/adjusted
  - [ ] 1–2 shell craters (Zone 1)
  - [ ] Czech hedgehog placement (Zone 2)
  - [ ] Sand berms, debris piles (Zone 3)
  - [ ] Exterior of 3 bunkers (Zone 4)
  - [ ] 3 landing craft with ramps (Zone 0)
- [ ] Fog setup (limit visibility to 30–40m)
- [ ] Walk through it yourself and feel whether the zone sizes are right

**When done**: Judge whether zone sizes feel appropriate. Adjust if too short or too long.
**Relevant docs**: 05_ZONES.md

---

## Step 2 — First-Person Movement
> Goal: Confirm the feel of running and going prone.

- [x] Create player character C++ class
- [x] Implement running
- [ ] Implement prone (instant drop to ground)
- [ ] Implement slide-into-prone (momentum slide)
- [ ] Implement headbob (primarily vertical bounce, small amplitude)
- [ ] Run through each zone yourself and measure time
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

- [ ] Place one MG Actor in a bunker
- [ ] Implement MG targeting system (manager + state struct, Decision 021)
  - [ ] Priority-based target selection
  - [ ] Rotation speed limit
  - [ ] Factor-based accuracy
- [ ] Implement MG stops
  - [ ] Reload (2–3 seconds)
  - [ ] Overheat (4–6 seconds)
  - [ ] Jam (2–8 seconds random)
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

- [ ] Implement hit detection (raycast-based)
- [ ] Location-based damage (headshot = instant, body = two-shot)
- [ ] Wounded state presentation
  - [ ] Red vignette (post-process)
  - [ ] Headbob stagger pattern
  - [ ] Reduced movement speed
  - [ ] Breathing/heartbeat audio
  - [ ] Increased aim sway
- [ ] Hit camera
  - [ ] Camera shake
  - [ ] Hit direction hint (push)
  - [ ] Pain sound
- [ ] Death camera (semi-scripted)
  - [ ] Camera fall (1.5–2 seconds, ease-out)
  - [ ] Tilt
  - [ ] Vision blur + narrowing
  - [ ] Audio low-pass filter
  - [ ] Terrain clip prevention (LineTrace lower bound)
  - [ ] Fade out
- [ ] Play it yourself and check
  - [ ] Does the wounded state feel real?
  - [ ] Is the death moment emotional?
  - [ ] Does the camera clip into terrain?

**When done**: Record death camera fall values and wounded state values in 07_CAMERA.md.
**Relevant docs**: 07_CAMERA.md, 06_COMBAT.md

---

## Step 5 — Narrative + Transition
> Goal: One full loop. Proof that this game exists.

- [ ] Implement narrative screen
  - [ ] Option A: black background + white text
  - [ ] Option B: final view residue + text
  - [ ] Text display (one sentence at a time, fade-in or typing)
  - [ ] Compare both options → choose one
- [ ] Spawn second character Pawn
- [ ] Implement transition system
  - [ ] Death → narrative → fade in → possess new character
  - [ ] Starting state per zone (already running, already prone, etc.)
  - [ ] Enemy targeting delay (1–2 seconds)
- [ ] Insert first character narrative text (your original writing)
- [ ] Play it yourself
  - [ ] Does the flow of run → die → story → new eyes open feel smooth?
  - [ ] Is the transition from narrative seamless?
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
- [ ] Enemy infantry AI (Zone 3)
  - [ ] Behavior cycle (cover → rise → fire → cover)
  - [ ] Position movement (fire concentration, comrade death, enemy approach triggers)
- [ ] Ammo system
  - [ ] Reload mechanic
  - [ ] Empty magazine feedback
  - [ ] Looting when out of ammo (low probability spawn, auto-collect)
- [ ] Firing system
  - [ ] Hip fire
  - [ ] Aimed fire
  - [ ] FOV reduction when aiming, sway increase while wounded

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
