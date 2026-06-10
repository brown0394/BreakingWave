# Enemy AI

## Core Philosophy

The enemy is not a pattern — they're a person.
The AI should behave not like "a fair game enemy" but like "a person who is actually there."
The MG gunner shoots the most dangerous target first. Infantry shoot until they're scared, then hide.
And these people die too. These people can become the player.

---

## Bunker MG

### Overview
- 3 bunkers (left MG, center command, right MG)
- Fixed placement in Zone 4. Fire coverage spans all of Zones 0–3.
- "Environmental threat" — firepower falling like rain. Doesn't precisely track individuals, but switches targets based on priority.

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

### MG Stops — Three Types

The MG can't fire continuously. Interruptions occur, each feeling different.

| Type | Duration | Frequency | Sound |
|------|----------|-----------|-------|
| **Reload** (belt change) | 2–3 seconds | Common | Belt ejecting, new belt locking in, charging handle |
| **Overheat** (barrel change) | 4–6 seconds | Rare | Hissing overheat sound, barrel removal/insertion |
| **Jam** | 2–8 seconds (random) | Rare | Failed firing attempt, frustrated sounds, manipulation noise |

- All three types have different probability and timing, so "how long will it stop this time?" is unpredictable
- This uncertainty creates the gamble: "should I run now?"
- Each bunker has independent stops — one reloading while the other two fire
- All three bunkers stopping simultaneously is extremely rare, but when it happens, major opportunity for the player

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
- One MG system holds an array of 3 MG state structs (current target, priority score, yaw, rotation state, stop state)
- Update loop per bunker: evaluate target → rotate → fire → check for stop
- Priority evaluation every 0.3–0.5 seconds (not every frame)
- Rotation: Lerp MG mesh Yaw at max rotation speed
- Stops: random timer + type selection → pause firing for that duration
- Bunker actors only carry mesh, muzzle flash, and audio

### Infantry AI
- One infantry system ticks an array of soldier state structs; soldier actors carry mesh, animation, and ragdoll
- State cycle per soldier: cover → (check triggers) → move or rise → aim → fire → cover
- Movement trigger checks: impact detection (hit event within radius), comrade death event, enemy distance check
- NavMesh path requests for movement (inside trench / position)
- Accuracy: distance-based base value × random variance

---

## Open Questions

- [ ] MG max rotation speed value — tune in prototype
- [ ] MG stop probability distribution — tune through testing
- [ ] Infantry count finalized (Zone 3: ~7 is tentative)
- [ ] Infantry aiming time value — tune through testing
- [ ] Infantry inter-position movement frequency and condition details
- [ ] Allied NPC behavior when enemy is playable
- [ ] Zone 4 bunker breakthrough mechanic (grenade? entry? suppression?)
