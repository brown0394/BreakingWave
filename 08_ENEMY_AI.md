# Enemy AI

## Core Philosophy

The enemy is not a pattern — they're a person.
The AI should behave not like "a fair game enemy" but like "a person who is actually there."
The MG gunner shoots the most dangerous target first. Infantry shoot until they're scared, then hide.
And these people die too. These people can become the player.

---

## Bunker MG

### Overview
- 3 MG bunkers (left, center, right), all manned (Decision 033)
- Fixed placement in Zone 4. Fire coverage spans all of Zones 0–3.
- The flank guns fire enfilade — toed in 30° toward the beach center (Decision 032) so their
  slit arcs interlock: the overlap lane is the center, and each gun's own near frontage is
  covered by the opposite gun. The center gun faces straight out to sea.
- "Environmental threat" — firepower falling like rain. Doesn't precisely track individuals, but switches targets based on priority.
- The MG is a crewed weapon, not a turret (Decision 027). A person aims it, people feed it, and killing them is how it dies.

### The Crew — 6-Man Garrison (Decision 027)

Each MG bunker is manned by 6 (historical heavy-MG squad). Only the active pair is rendered —
gunner at the gun, loader beside him; the rest are an unrendered reserve inside the bunker.

- A crew death → takeover delay (the next man moves up) → the gun resumes. Silence windows
  escalate as the pool drains.
- Crew count drives stop-duration multipliers:

| Crew alive | Effect |
|-----------|--------|
| 6–4 | Normal — dedicated loader, belts prepped |
| 3–2 | Reloads slower — ammo bearers dead, someone has to fetch |
| 1 | Reload AND barrel change much slower — solo gunner does everything |
| 0 | Silent forever |

- Bunker-crew AI is its own system, separate from field infantry.

### Perception — What the Gunner Actually Sees (Decision 031)

Per potential target, a visibility score:

**visibility = exposure × attention × distance**

- **Exposure fraction** — LOS traces from the muzzle to ~3 body points (head, chest, pelvis).
  0 = in cover, partial = peeking, 1 = fully exposed. Exposure crossing from 0 to positive is
  the "just broke cover" edge trigger.
- **Off-axis attention falloff** — vision is sharpest along the current muzzle direction and
  fades toward the edges of the slit arc (hard limit). Off the gun's axis, you are genuinely
  unseen — flanking works.
- **Distance factor** — closer is easier to notice; fog caps the whole thing.

Score over threshold → target enters the gunner's **awareness set**, with a short memory after
sight is lost (he suppresses the crater you ducked into, then forgets). The priority ladder
ranks only the awareness set. Evaluation runs every 0.3–0.5 s — that tick IS the gunner's
reaction time.

### Priority-Based Targeting

The MG doesn't sweep left to right. The gunner judges and tracks the most dangerous target.

**Target Priority (top to bottom):**

1. **Enemy actively firing at me** — If bullets are hitting the MG position, respond first
2. **Enemy who just broke cover** — Detect movement change. The moment someone rises from a crater is most dangerous
3. **Exposed enemy at close range** — The closer, the more threatening
4. **Exposed enemy at long range** — Exposed but far away, lower priority
5. **Enemy in cover** — Only when no other targets. Suppression fire at cover

**Priority Switching:**
- If a higher-priority target appears, switch from current target
- On switch, rotate MG toward new target — this rotation time naturally creates a "window"
- If a target goes back into cover, priority drops and MG switches to another target

**Priority modifiers (Decision 033):**
- **Movement draws the eye** — an exposed target's priority scales with ground speed:
  a man sprinting up the beach outranks a nearer man lying still. Paired with the
  accuracy factors below, this is the run-don't-stop gamble: running attracts fire but
  is harder to hit; stopping makes you quiet, then dead.
- **The battery spreads its fire** — a target another gun is already working scores
  lower (a crude fire plan). The guns split the wave instead of converging on the same
  front-runner, so nobody crosses free just because someone else is closer.

### MG Rotation Speed Limit

- The MG can't rotate infinitely fast. Set a max rotation speed.
- Switching from a left target to a right target — during rotation, the area in between is "not aimed at"
- Firing continues during rotation, but accuracy drops significantly (see below)
- Slight random variance in rotation speed itself — transition time isn't always identical

### Accuracy — Factor-Based

Not pure random — situational factors influence accuracy.

**Factors that reduce accuracy:**
- Greater distance to target
- MG is mid-rotation (turning while firing is inaccurate)
- Longer continuous firing duration (recoil buildup, gunner fatigue)
- Target is moving
- Fog limiting visibility

**Factors that increase accuracy:**
- Target is stationary (hold still and die)
- MG stationary, aiming before firing
- Closer target
- Early in a continuous burst (before recoil builds up)

→ "Staying still lets the MG acquire you and get accurate" = stop and die
→ "Running makes you harder to hit but exposes you" = must run, but it's dangerous

### MG Stops — Simulated, Not Rolled (Decision 028)

The MG can't fire continuously. Stops come from real state the struct tracks — rounds left in
the belt and barrel heat — not from random timers.

| Type | Cause | Duration (full crew) | Sound |
|------|-------|---------------------|-------|
| **Reload** (belt change) | Belt runs empty | 2–3 seconds | Belt ejecting, new belt locking in, charging handle |
| **Overheat** (barrel change) | Heat crosses threshold | 4–6 seconds | Hissing overheat sound, barrel removal/insertion |
| **Jam** | Small random chance per burst | 2–8 seconds (random) | Failed firing attempt, frustrated sounds, manipulation noise |

- Firing decrements the belt and builds heat; pauses cool the barrel. Burst discipline
  emerges from heat tuning, not scripted burst lengths.
- Durations scale with the crew-tier multipliers (Decision 027) — fewer hands, longer silences.
- "How long will it stop this time?" stays unpredictable because the causes interleave —
  this uncertainty creates the gamble: "should I run now?"
- Each bunker has independent belts, heat, and crew — one reloading while the other two fire
- All three bunkers stopping simultaneously is extremely rare, but when it happens, major opportunity for the player
- Sound design note (later pass): state-change audio arrives delayed by distance ÷ speed of
  sound; a bullet passing near the player's head snaps a supersonic **crack**, the muzzle
  **thump** arrives later — the crack–thump gap reads distance with no UI (Decision 030)

### Bunker Independence

The 3 bunkers operate independently:
- Each makes its own priority judgment
- Each has its own rotation state
- Each has its own stop timing
- Their fire overlaps, but they may shoot the same target or different targets simultaneously

The combinations that arise from this independence create a different experience every run.

---

## Infantry

### Overview
- Zone 3: 2 soldiers each in left and right foxholes, 3 in the central trench line. ~7 total.
- Zone 4: Additional infantry around bunkers and communication trenches.
- "Personal threat" — unlike the MG, these are individuals who recognize and aim at the player.
- Enemies the player can shoot and eliminate. Threats where "something I can do" exists.

### Behavior Cycle

```
In cover (inside trench / foxhole)
    ↓ Target detected or timer elapsed
Rise from cover (upper body exposed)
    ↓ Aiming time (doesn't fire instantly)
Fire (1–3 shots)
    ↓
Drop back into cover
    ↓ Wait (random duration)
Repeat
```

- Random variance in rise timing, shot count, and wait duration
- Each soldier has different cycle timing → natural crossfire
- Does not move and fire simultaneously — doing both at once looks robotic

### Position Movement

Infantry don't stay in one spot. They relocate periodically.

**Movement range:**
- Intra-trench movement: 2–3 meters sideways. Natural and frequent.
- Inter-position movement: Rare. Leaving cover is dangerous for infantry too. Only triggers in special conditions.

**Movement triggers:**
- **Concentrated fire** — If bullets are hitting their position, they move sideways
- **Hit after firing** — If they rise to shoot and get hit, they re-emerge from a different spot
- **Comrade killed** — If an adjacent soldier dies, they move to fill the gap and cover the angle
- **Enemy approach** — Fall back if Allies get too close

**Movement rules:**
- No firing while moving
- Move → arrive at new position → take cover → rise and fire (order maintained)
- Low stance while moving (reduced hit profile, but not immune)
- Movement generates sound (equipment rattling, footsteps) — hints to the player that "they moved"

### Infantry Accuracy

Graduated by distance. Lower overall than MG, but threatening up close.

| Distance | Accuracy | Feel |
|----------|----------|------|
| Zones 1–2 (long range) | Very low | Almost no threat. Only the MG is dangerous. |
| Zone 3 (medium range) | Moderate | Real threat begins. Fight from cover. |
| Zone 3 (close) | High | Very dangerous. Take them out fast or find cover. |
| Zone 4 (close range) | High | Breakthrough zone. Intense firefight. |

### Infantry Death

- If the player scores a hit, they go down
- Threat removed from that position → advance possible → feeling of "capturing"
- Dead enemy infantry may be the next playable character
- The person the player shot could be whose inner voice they read later

---

## Difficulty Curve

### Threat Composition by Zone

| Zone | Primary Threat | Secondary Threat | Player Actions | Emotion |
|------|---------------|-----------------|---------------|---------|
| 0 | MG (long range) | — | Running | Fear, helplessness |
| 1 | MG (crossfire) | — | Running, prone, craters | Despair, gambling |
| 2 | MG (attenuating) | — | Obstacle movement, judgment | Tension, calculation |
| 3 | Infantry (direct combat) | MG (background) | Cover, shoot, advance | Active, engaged |
| 4 | MG (close) + Infantry | Bunker face-to-face | Breakthrough | Climax, breakthrough |

### Emotional Arc

```
Helplessness → Despair → Starting to judge → Active combat → Breakthrough
  (Zone 0)     (Zone 1)      (Zone 2)           (Zone 3)       (Zone 4)
```

At the start: a body thrown in front of bullets.
At the end: a person who breaks through a bunker.
This transformation is the arc of the whole game.

---

## Allied NPC Behavior (Brief)

The Allied NPCs running alongside the player also need behavior.

- Default: run up the beach, go prone, die
- Some fire — this fire draws MG priority → creates windows for the player
- Die frequently — corpses accumulate
- The next playable character is somewhere among these NPCs
- Must not be too smart — these are people each trying to survive in the chaos of battle

Detailed Allied NPC AI is covered in 09_ALLY_NPC.md.

---

## UE Implementation Notes

Data-oriented per Decision 021: managers tick arrays of state structs; actors are visual shells.

### MG AI
- One MG system holds an array of 3 MG state structs (crew count + takeover timer, belt rounds,
  barrel heat, stop state, current target, aim yaw/pitch, rotation state, per-target awareness)
- Update loop per bunker: perceive (0.3–0.5 s tick) → pick target from awareness set → rotate → fire → belt/heat bookkeeping
- Rotation: Lerp aim yaw at max rotation speed with slight random variance; accuracy penalized mid-rotation
- Stops: belt empty → reload; heat threshold → barrel change; random per-burst jam. Durations × crew-tier multiplier (Decisions 027/028)
- Bullets: flat array of projectile structs, segment trace per tick, real travel time (Decision 030) —
  they kill sim allies, hit the player, or land in the sand
- Bunker actors only carry gun mesh, crew visual shells, muzzle flash, and audio
- The allied wave is an unrendered simulation from Step 3 (Decision 029): ally structs advance,
  wander, pause prone, and die to MG fire — the MG's perception/priority pipeline runs against
  them plus the player, so the gun is never idle and its attention shifts organically
- Dev-only exec commands: `MGNoDamage` (player immune, for observation), `MGKillCrew` (test
  takeover windows and degradation before the player can shoot), `MGDebug` (state readout)

### Infantry AI
- One infantry system ticks an array of soldier state structs; soldier actors carry mesh, animation, and ragdoll
- State cycle per soldier: cover → (check triggers) → move or rise → aim → fire → cover
- Movement trigger checks: impact detection (hit event within radius), comrade death event, enemy distance check
- NavMesh path requests for movement (inside trench / position)
- Accuracy: distance-based base value × random variance

---

## Open Questions

- [ ] MG max rotation speed value — tune in prototype
- [ ] Belt size / heat-per-shot / cooling rate / jam chance / crew-tier multipliers — tune through testing
- [ ] Infantry count finalized (Zone 3: ~7 is tentative)
- [ ] Infantry aiming time value — tune through testing
- [ ] Infantry inter-position movement frequency and condition details
- [ ] Allied NPC behavior when enemy is playable
- [ ] Zone 4 bunker breakthrough mechanic (grenade? entry? suppression?)
