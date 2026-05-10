# Combat System

## Core Philosophy

Combat is brief and brutal. The player dies quickly, most of the time.
Controls are simple, but judgment matters.
No UI. The body tells you everything.

---

## Player Controls

### Movement
- **Running** — Default movement. Standing still means full exposure.
- **Prone** — Drop to the ground instantly. Drastically reduces hit profile. Movement speed near-zero.
- **Slide into prone** — Going prone while running carries momentum into a sliding prone. Fast emergency dodge.
- **Cover** — Press against cover near an object. Blocks hits from that direction.

### Firing
- **Hip fire** — Shoot from cover with minimal aiming. Low accuracy.
- **Aimed fire** — Narrows FOV, prevents movement. High accuracy.
- **Reload** — Manual reload. Empty mag triggers empty-chamber click. Reload motion and audio provide feedback.

### What You Don't Control
- No jumping
- No melee
- No item use
- No weapon switching (one rifle, always)

---

## Hits and Damage

### No UI Principle
No health bar, crosshair, ammo counter, or minimap on screen.
All information is conveyed through sensation.

### Damage Model: Two-Shot + Location-Based

| Location | Damage | Result |
|----------|--------|--------|
| Head | Instant death | One shot, one kill. Meaningless death. |
| Torso (1st hit) | Wounded | Enters wounded state. One more shot = death. |
| Torso (2nd hit) | Death | — |
| Limbs | Minor | Subtle feedback. Not fatal. Repeated hits can trigger wounded state. |

### Wounded State — Expressed Without UI

The player knows they've been hit through these sensations:
- Screen edges darken and redden
- Camera headbob shifts to an unsteady, lurching pattern
- Movement speed decreases
- Breathing becomes ragged; heartbeat grows louder
- Increased sway when aiming — accuracy drops
- Vision intermittently blurs

Wounded state does not recover. Once hit, that character fights wounded until death.

### Ammo — Expressed Without UI

- Reload sound and animation confirm "I reloaded"
- Pulling the trigger on an empty mag produces a dry click — "I'm out"
- Exact round count is unknown. That uncertainty creates tension.

---

## Cover System

### Protection by Cover Type

| Cover | Location | Protection Level | Notes |
|-------|----------|-----------------|-------|
| None (open ground) | Zones 0–1 | No cover | Run or go prone |
| Shell craters | Zone 1 | Partial cover | Only 1–2. Not safe — just a brief pause. |
| NPC corpses | Zones 1–3 | Penetration attenuation | Not full stop. See below. |
| Czech hedgehogs | Zone 2 | Partial block | Metal obstacles. Gaps allow hits. |
| Sand berms | Zone 3 | Good cover | Prone behind them nearly blocks all. Enemy uses them too. |
| Debris piles | Zone 3 | Partial block | Irregular shape. Limited angles of protection. |
| Trenches / foxholes | Zones 3–4 | Good cover | Enemy positions. Usable after capturing. |
| Bunker walls | Zone 4 | Full block | Concrete. No penetration. |

### Corpse Cover — Physics-Based Penetration

Not handled with probability. Handled with physics.

**How it works:**
1. Raycast from enemy fire origin to player position
2. If a corpse collider is in the path, calculate entry and exit points
3. Attenuate damage based on corpse penetration depth

**Attenuation by penetration depth:**

| Depth | Damage Multiplier | Feel |
|-------|------------------|------|
| Deep (through torso center) | 0.1–0.3× | Nearly stopped — but not completely. |
| Medium (through limbs) | 0.4–0.6× | Meaningful reduction. |
| Shallow (grazing edge) | 0.7–0.9× | Barely blocked. |
| None (corpse not in path) | 1.0× | Full damage. |

Numbers to be tuned after playtesting.

**Why angle matters:**
Three bunkers. Bullets from the left bunker may be stopped by a corpse,
but bullets from the right bunker come from a different angle and bypass it.
→ "Which way do I lie down" becomes a survival decision.

**Corpse collision implementation:**
- Full ragdoll precision collision is too expensive
- Simplified collision volumes per major region: head (capsule), torso (box), legs (capsule)
- Raycasts only occur on hit checks, not every frame
- Corpses outside fog range (far away) have collision disabled

**Corpse hit reaction:**
- Bullets hitting a corpse apply impulse to the ragdoll → corpse twitches slightly
- In first-person, watching a nearby corpse absorb a shot and move is powerful

---

## Combat Rhythm by Zone

### Zone 0 — Landing (5–15 seconds)
- The landing craft ramp drops
- Nothing to do but run
- Bad luck means death before the ramp finishes dropping

### Zone 1 — Kill Zone (10–30 seconds)
- Cover: 1–2 shell craters, fallen corpses
- Actions: running, prone, rolling into craters
- No firing possible — enemy is too far and there's no time to aim
- All three bunkers' crossing fire overlaps here. Most lethal zone.
- Brief shelter in craters lets you see the NPCs around you

### Zone 2 — Obstacle Belt (20–45 seconds)
- Cover: Czech hedgehogs, terrain beyond wire
- Actions: zigzag between obstacles, prone, push through wire
- Firing becomes possible — but the enemy is hard to see through fog
- Zone of judgment: which path do I take?

### Zone 3 — Firefight (30–60 seconds)
- Cover: sand berms, debris, corpses
- Actions: cover + hip/aimed fire. Real combat begins.
- Enemy infantry are visible. You shoot and get shot.
- Advancing by taking down enemies. A sense of capturing ground.
- **As enemy:** fire from defensive positions at shapes coming up the beach.

### Zone 4 — Breakthrough (30–90 seconds)
- Cover: trenches (once captured), bunker exterior
- Actions: capture trench → approach bunker → break through
- Heaviest resistance. Concentrated MG fire.
- **As enemy:** MG fire from inside bunker. Movement through communication trench.

---

## Enemy Gameplay Differences

Allied and German players use the same controls, but the situation is different.

| | Allied | German |
|--|--------|--------|
| Objective | Advance, survive | Defend, hold |
| Starting state | Exposed, moving | In cover, fixed position |
| Firing opportunity | Limited (from Zone 3) | Available from the start |
| Threats | Bunker MGs, infantry fire | Advancing Allies, artillery |
| Camera direction | Up the beach (advancing) | Down the beach (overlooking) |
| Form of death | Shot while advancing | Position overrun, shelled, breached |

The enemy's fear is a different kind:
Shapes emerge from the fog. You shoot and they keep coming.
A comrade in the next foxhole screams. The far end of the communication trench goes quiet.

---

## Open Questions

- [ ] Exact number of limb hits required to enter wounded state
- [ ] Exact number and placement of shell craters in Zone 1
- [ ] Czech hedgehog hit detection — gap probability vs. physics
- [ ] Enemy MG bunker fire pattern — sweep? fixed? tracking?
- [ ] Enemy infantry AI behavior
- [ ] Barbed wire crossing mechanic — time cost? increased hit risk?
- [ ] Whether artillery strike events exist (environmental threat)
- [ ] Sound design — gunfire, impacts, screams, wind, etc.
- [ ] Penetration attenuation value tuning
