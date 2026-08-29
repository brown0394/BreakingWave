# Allied NPC AI

## Core Philosophy

Allied NPCs are not background — they're people.
They run right next to the player, and die right next to the player.
If these people move like robots, the first-person immersion breaks instantly.
And one of these people may become "me" next.

---

## The Role of Allied NPCs

1. **Density** — Creates a battlefield where dozens run and fall together
2. **MG distraction** — Their movement and fire draw MG priority, creating windows for the player
3. **Next character pool** — The next playable character is somewhere among them
4. **Soundscape** — Screams, groans, and shouting build the audio landscape of the battlefield

---

## Personality Types

If every NPC behaves identically, they look like a robot swarm.
Mix four types in deployment.

### Charger (~30%)
- Runs forward as fast as possible
- Goes prone infrequently
- Dies first. Occasionally gets the furthest by luck.
- Strongly draws MG priority — they're a moving, exposed target
- When one is nearby, it creates an advance opportunity for the player

### Cautious (~35%)
- Runs, frequently drops prone
- Hides behind craters, corpses, and obstacles
- Lies still for a while, then runs again
- Survives longest but advances slowly
- Looking around while prone — the key detail that makes them feel human

### Frozen (~15%)
- Can't move immediately after landing or after being hit
- Lies prone, paralyzed by fear
- Eventually starts moving again, or stays there until they die
- It's unrealistic for everyone to run "bravely." Frozen people make it feel real.
- This type may have the most powerful narrative

### Leader (~20%)
- Shouts and yells at others ("Move!", "This way!", "Get down!")
- Shouts while running themselves
- Influences nearby Frozen types — when a Leader gets close, Frozen types start moving sooner
- The original text's "Pull yourself together. No time to grab a walking corpse and shake it" — that person is a Leader

---

## Behavior by Zone

> **ADOPTED AS WRITTEN by Decision 044 (2026-08-22).** The zone gating below is the ally
> fire model — it is no longer aspirational. It also solves the fog problem for free: Z3
> allies sit 0–80 m from enemy infantry, so ally fire happens at fog-ish ranges by
> construction and no ally is asked to shoot 350 m through 35 m of fog.

### Zones 0–1 — Running Only
- No firing. Just running or going prone.
- Shooting a rifle here is suicide — enemy is too far and stopping to aim means death
- Chargers run, Cautious types run then drop prone, Frozen types stop, Leaders shout

### Zone 2 — Movement Focused, Rare Firing
- Extremely rare attempts to fire from behind obstacles
- Primarily focused on movement
- Path choice between obstacles differs by type — Chargers take the shortest route, Cautious take routes with more cover

### Zone 3 — Full Combat
- Firing at enemy infantry from behind sand berms
- This fire affects enemy infantry behavior (forces enemies into cover)
- This fire draws MG priority, creating windows for other allies and the player
- Leaders direct firepower ("Shoot over there!", "Keep your head down!")

### Zone 4 — Breakthrough
- The few survivors
- Active firing and advance
- A sense of breaking through together as bunkers are approached

---

## Ways of Dying

Allied deaths must be varied for the battlefield to feel real.

### Instant Death
- Falls suddenly while running. No sound.
- Most common. Most pointless.

### Screaming Death
- Gets hit, screams while falling
- Brief scream, then silence

### Wounded Crawl
- Hit and falls, then crawls
- Crawls a few meters, then stops
- Groaning and labored breathing while crawling

### Wounded Groaning (Not Dead)
- Moving on the ground, calling for help
- Voices like "Help me," "Over here," "Please"
- Continues making noise without dying — contributes to the soundscape
- **The player cannot help. No aid mechanic is intentionally included.**
- An experience of hearing it and walking past. Stronger than any text.

### Freeze After Hit
- Hit but not killed
- Freezes in place from shock
- Either starts moving again, or dies from a follow-up hit

---

## Density Management

### Fog-Based Spawn/Despawn

> **STRUCK by Decision 048 (2026-08-29). There is no density director.** Fog-edge top-up
> spawning is cancelled: allies enter the world only at landing craft, and the numbers below
> are an **outcome we observe**, never a target the game maintains around the player. The
> reason is `01_SOUL.md`'s "the player is not a hero" — a population that exists because the
> player is near is a world that revolves around him. What replaces it: **7–9 craft ~75 m
> apart** instead of 3 at 280 m, men arriving in **boatloads** of ~25–30 rather than a
> trickle, and `MaxAlive` ~300. Despawn is gone too (Decision 050): men who reach the
> defense line fight there instead of walking off the map.

Player visibility is limited to 30–40m by fog.
No need to have 90 NPCs on the full beach at once.

- **Active NPCs within visible range (30–40m)**: 8–12 — *observed target, not maintained*
- ~~Spawn at fog edge, despawn at fog edge~~ — struck, Decision 048
- ~~Spawn/despawn only outside player's field of view~~ — struck, Decision 048

### Density Reduction Curve

As zones progress, fewer living allies remain nearby. **These are outcomes to measure, not
rules to enforce** (Decision 048). Boatload arrival produces the shape for free: a dispersing
cluster of ~30 gives 8–12 in a 35 m disc at the waterline, thinning with distance and
attrition toward the top.

| Zone | Nearby Active Allies | Feel |
|------|---------------------|------|
| 0–1 | 8–12 | Many people running together |
| 2 | 5–8 | Fewer, but still present |
| 3 | 3–5 | Almost alone |
| 4 | 1–3 | A surviving few |

This density reduction itself communicates the brutality of the battlefield.

---

## Corpse Management

### Corpse Count Limits

Manage the physics, collision, and rendering cost of ragdoll corpses.

| Range | Handling | Cap |
|-------|----------|-----|
| Close range (0–20m) | Full physics + collision (used for cover system) | ~15 |
| Mid range (20–40m) | Physics disabled, collision only | ~30 |
| Far range (40m+) | Static mesh replacement or fade out | No limit (lightweight) |
| Rear (zones already passed) | Safe to remove | — |

- Close-range corpses are never removed — disappearing in front of you breaks immersion
- First-person, advancing forward means rarely looking back → rear corpses can safely be removed
- Exact numbers adjusted after profiling

### Corpse Function

Corpses are not just debris — they're gameplay elements:
- **Cover** — See corpse penetration physics in 06_COMBAT.md
- **Ammo spawn location** — See ammo looting below
- **Visual battlefield depiction** — Accumulating corpses show the state of the beach

---

## Ammo Looting

### Condition: Only When Ammo Is Completely Gone

- Looting is unavailable while ammo remains. Corpses are just corpses.
- When ammo reaches 0:
  1. Character mutters: "Damn, I'm out" (tone varies by character)
  2. Ammo may spawn near a nearby corpse (low probability)
  3. Approaching spawned ammo auto-collects it (no separate interaction key)

### Design Intent

- Looting should feel like "desperate survival," not "resource collection"
- Corpses must not feel like "resource nodes"
- Ammo on every corpse makes them resource nodes → spawn at low probability only
- The gamble: "will there be ammo if I make it to that corpse?"

### Looting Method

- **Collect while running**: automatically pick up by running over ammo
- **Collect from cover**: reach-out animation from the adjacent corpse while in cover
- No traditional looting with stop-and-press-key
- Doesn't conflict with "stop and die" — collection works in motion

### Amount Collected

- One magazine's worth. Not a full top-off.
- Just enough to fire a few more shots.
- Coexistence of "finally got some" relief + "this will run out fast" tension

### The Experience of No Ammo

- No ammo means no firing. All you can do is run.
- This is an experience in itself: "I have a gun but can't shoot"
- Most characters die before running out of ammo anyway
- Running dry is "both reward and curse" for a character who's survived long

---

## Visual Distinction

- The next playable character must not look visually special
- All share the same uniform, the same helmet
- Only subtle differences: presence of a pack, equipment placement, slight body type variation
- Absolutely no indicator marking "this person is the next character"
- Not knowing who's next is the core of this game

---

## Performance Optimization — Tiered AI

Can't run full AI on dozens simultaneously.
Tier AI complexity based on distance from player.

| Distance | AI Level | Content |
|----------|----------|---------|
| Close (0–15m) | Full AI | Detailed behavior, firing, voice, looking around, expressions |
| Mid (15–40m) | Simple AI | Movement and prone only. No firing. Basic hit reaction. |
| Far (40m+) | Animation only | Running silhouette, falling motion. No AI. |

- Fog hides the transition, so tier switches are naturally concealed
- Tier transitions must be smooth — sudden behavior changes break immersion

---

## UE Implementation Notes

Data-oriented per Decision 021: one ally NPC system ticks an array of NPC state structs (type, position, behavior state, AI tier); NPC actors are visual shells (mesh, animation, ragdoll).

### NPC Spawning
- NPC pooling system — pre-create and reuse
- ~~Spawn/despawn at fog boundary, only outside camera frustum~~ — struck by Decision 048.
  Allies enter only at landing craft, in boatloads, and leave only by dying (Decision 050)
- Assign type (Charger/Cautious/Frozen/Leader) randomly on spawn (weighted ratios)
- **Corpses are load-bearing** (Decision 054): a dying ally must leave a real body, because
  bodies register as cover points. Today `KillAlly` sets `bAlive = false` and the man simply
  vanishes — that is now a prerequisite, not a polish item

### Corpse Transition
- Activate ragdoll on NPC death
- After a set time, halt physics simulation (Sleep)
- At mid-range and beyond, replace ragdoll with static mesh

### Ammo System
- Player ammo == 0 event → search nearby corpses → probability roll → spawn
- Spawned ammo has small collision volume — auto-collected on player entry
- On collection: play reach-out animation (while in cover) or crouch animation (while moving)

### Tiered AI
- Tier = update frequency per array element in the system loop: close = every frame, mid = every 0.5s, far = animation only
- Link mesh LOD to the same tiers

---

## Open Questions

- [ ] Finalize type ratios (current 30/35/15/20 is tentative)
- **SETTLED — personality types land in full**, including Leader influence on Frozen, Leader
  shouts, and Cautious cover-seeking against a flat cover array that holds static points
  **and corpses**. Decision 054, 2026-08-29. Static cover is only ~35 positions beach-wide
  against ~105 Cautious men at `MaxAlive` 300, so reservation is mandatory and most Cautious
  men will find nothing and go prone instead — corpses are the only cover that scales
- [ ] Finalize fog visibility distance (30m? 40m?) — **now load-bearing beyond fog itself**:
  Decision 041 ties `FAllySimSettings.TakeoverRadius` (currently 3500 uu, the midpoint of
  the range above) to this number, because the man you take over must be one you could have
  seen. Update the knob when this settles
- [ ] Active NPC count cap — target is **~300** under Decision 048; confirm by profiling.
  `MaxAlive` 128 already **binds before a single death** (transit ~120 s at ~325 uu/s wants
  180 men), and Decision 042 bounds cost by *shells* (~25–30), not population. Fog-edge
  top-up spawning is **struck** (Decision 048) and the per-zone curve is now an observed
  outcome of boatload arrival
- [ ] Ally corpse cap — **now load-bearing**, not cosmetic: corpses register as cover points
  (Decision 054) and allies currently leave no body at all. At 0.90 deaths/s the cap recycles
  fast, so it trades cover availability against memory directly
- [ ] Corpse count cap — decide after profiling
- [ ] Ammo spawn probability value — tune through testing
- [ ] Wounded NPC voice lines and count
- [ ] Concrete values for Leader's influence on Frozen type
- **SETTLED — allied NPC firing accuracy**: allies get their own spread row in the shared
  model (Decision 043's "sides differ in data, never in code"), roughly `SpreadNearDeg` 2° /
  `SpreadFarDeg` 9° against the enemy's 1°/5°, plus a longer aim cycle — about **0.5–1% per
  shot** at Zone 3 range. Decision 054, 2026-08-29. At the enemy's own accuracy (~6–7% at
  50 m) ~30 firing allies would kill ~1 defender per second and clear the beach in a minute.
  The payoff of ally fire stays conspicuity (Decision 044); kills are a real but rare bonus
