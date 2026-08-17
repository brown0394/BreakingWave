# Decision Log

Records decisions and their reasoning. To prevent future "why did we do this?"

---

### 001 — Do not force death locations
- **Date**: 2026-04-04
- **Decision**: Do not script-force where the player dies
- **Reason**: Forced deaths make the player's actions meaningless. Futility should come from "tried and died," not "it was predetermined."
- **Alternative**: High difficulty + zone-based character pools to naturally induce death

### 002 — Adopt "Chain of Gazes" structure
- **Date**: 2026-04-04
- **Decision**: Characters mentioned in a dead character's narrative become next playable character candidates
- **Reason**: This structure already appears naturally in the writer's original text. Story and gameplay become structurally intertwined.
- **Risk**: Death location is unpredictable, so "nearby characters" need flexible placement

### 003 — Keep scope small
- **Date**: 2026-04-04
- **Decision**: Scale to a 2-person team (writer/developer + Claude)
- **Reason**: Excessive scope is the biggest enemy of completion.

### 004 — Unreal Engine + C++ + First-person
- **Date**: 2026-04-04
- **Decision**: Develop with UE + C++. First-person handheld camera.
- **Reason**: Writer/developer is a C++ specialist. Familiar tools are the fastest tools. First-person matches the game structure of "that person's eyes." Handheld conveys battlefield chaos and human trembling.
- **Considered alternative**: Godot (lighter, but learning cost and visual capability gap)

### 005 — Foggy day, no time progression
- **Date**: 2026-04-04
- **Decision**: Make time feel like it's not passing. Fog removes sense of time.
- **Reason**: Implementing time progression has low ROI. Fog simultaneously solves time sense removal + visibility limitation + atmosphere. Also makes narrative time descriptions easier to keep consistent.

### 006 — Narratives are pre-written static text
- **Date**: 2026-04-04
- **Decision**: Do not generate narratives with LLM at runtime. Pre-write everything.
- **Reason**: Writer's sentence quality cannot be reproduced by dynamic generation. Runtime API latency breaks emotional flow. Quality control impossible.
- **Branching**: Pre-write 2~3 narrative variants per zone

### 007 — Next character is random from pool
- **Date**: 2026-04-04
- **Decision**: Player does not choose the next character. Random assignment from pool.
- **Reason**: The player doesn't know who these people are yet. Giving choices doesn't match this game's tone. "Not knowing who you'll become" is itself the experience.

### 008 — Game ends on high ground breach, no lose condition
- **Date**: 2026-04-04
- **Decision**: Game ends when the last character breaches the high ground. No time limit or forced defeat.
- **Reason**: Changing game structure to prevent trolling is putting the cart before the horse. Character pool exhaustion is the natural limit.

### 009 — Both sides (Allied/German) playable
- **Date**: 2026-04-04
- **Decision**: Enemy soldiers can become playable characters. Their narratives also exist.
- **Reason**: When the "chain of gazes" crosses to the enemy, the game's depth fundamentally changes. Experiencing the same beach from the other side. The strongest way to show both sides of war without preaching.
- **Technical challenge**: Enemy gameplay differs from Allied (defensive position). Camera direction reversal.

### 010 — Important NPCs can die, but keep pool generous
- **Date**: 2026-04-04
- **Decision**: Next playable candidate NPCs can die before the player does. Instead, place generous candidate pools per zone.
- **Reason**: If players notice "this NPC never dies," immersion breaks. Solved by writing many narratives. The writer's commitment.

### 011 — Two-shot + body-part damage, no UI
- **Date**: 2026-04-05
- **Decision**: Headshot instant kill, torso two-shot. No health bar/crosshair/ammo UI whatsoever. Injury communicated through camera/sound/movement changes.
- **Reason**: Without UI, uncertainty becomes fear. With two-shot, after feeling "I got hit," the fear of "one more and I'm dead" emerges. One-shot only has "death" as hit feedback, narrowing emotional range.
- **Considered alternative**: One-shot-kill (more brutal, but sacrifices injury-state tension)

### 012 — Shell craters placed in Zone 1
- **Date**: 2026-04-05
- **Decision**: Place 1~2 shell craters in Zone 1 (kill zone). Not safe, but a place to briefly catch your breath.
- **Reason**: Zero cover means zero player agency, and that's a cutscene, not a game. The judgment of "can I survive if I run there?" makes death hurt more. Time spent looking around in craters creates visual connections needed for the chain of gazes.

### 013 — Corpse cover uses physics-based penetration, not probability
- **Date**: 2026-04-05
- **Decision**: When taking cover behind NPC corpses, bullet penetration is handled by physics (raycast + penetration thickness), not probability.
- **Reason**: Probability gives different results for the same action, feeling "unfair." Especially problematic since there's no UI for players to analyze causes. Physics-based means "which direction you lie down" becomes a meaningful decision. Three bunkers' different firing angles naturally reflect in gameplay.

### 014 — Death camera uses semi-scripted approach
- **Date**: 2026-04-05
- **Decision**: On death, body ragdolls but camera detaches from physics and falls smoothly via separate logic.
- **Reason**: Pure ragdoll camera almost certainly causes terrain clipping, unrealistic bouncing, spinning views. If the last thing the player sees breaks, the emotion breaks. Semi-scripted maintains "falling sensation" while preventing physics accidents.

### 015 — Narrative screen: compare two options in prototype
- **Date**: 2026-04-05
- **Decision**: Option A (black background + white text) vs Option B (last-view afterimage + text) — decide after comparison testing in prototype.
- **Reason**: Both have pros and cons. A provides disconnect and power of dry inner monologue, B provides "dying while seeing this" connection. Cannot be decided by discussion alone.

### 016 — Transition protection via target acquisition delay, not invincibility
- **Date**: 2026-04-05
- **Decision**: For 1~2 seconds after character transition, enemy AI does not target the new character. No invincibility.
- **Reason**: Invincibility does not exist in this game. "Not yet targeted" is logically consistent — enemies also need time to recognize and aim at a new target. Still vulnerable to area fire, so not complete safety.

### 017 — MG uses priority-based tracking, not left-right sweep
- **Date**: 2026-04-05
- **Decision**: Bunker MGs track targets by switching based on threat priority, not mechanical left-right sweep. Rotation speed limit creates natural gaps.
- **Reason**: Left-right sweep is a "game pattern," not "human behavior." Priority tracking means player actions influence MG behavior — if someone nearby fires, MG turns that way, creating an opening to advance. NPC actions become meaningful.
- **Considered alternative**: Mechanical left-right sweep (predictable, but artificial)

### 018 — MG accuracy is factor-based, interruptions have three types
- **Date**: 2026-04-05
- **Decision**: MG accuracy varies by distance/rotation state/continuous fire time/target movement. Interruptions are three types: reload (2~3s), overheat (4~6s), jam (2~8s random).
- **Reason**: Pure random accuracy is unlearnable. Factor-based makes "staying still gets you hit" feel systemic. The three interruption types' different durations create "how long will it stop this time?" uncertainty.

### 019 — No weapon looting, ammo looting only when empty
- **Date**: 2026-04-05
- **Decision**: Cannot pick up weapons from corpses. Only when ammo reaches 0 can ammo spawn near corpses at low probability, auto-collected on proximity.
- **Reason**: If looting is possible, corpses become "resource nodes." In this game, corpses must be "what was once a person." Only allowing it when empty makes it "desperate survival," not "resource gathering." Auto-collect while running doesn't conflict with "stopping means dying."

### 020 — Cannot respond to wounded NPC help requests
- **Date**: 2026-04-05
- **Decision**: Wounded ally NPCs call for help, but the player cannot help. Help mechanic intentionally not included.
- **Reason**: Having to hear it and move on is more powerful than any text. With a help mechanic, it becomes a "help or not" game choice; without it, it becomes a "can only listen" emotional experience.

### 021 — NPC systems are data-oriented managers, not per-NPC AIControllers
- **Date**: 2026-06-10
- **Decision**: Game systems (allied NPCs, enemy infantry, MG bunkers) are implemented as managers that tick arrays of state structs. NPC actors are thin visual shells (mesh, animation, ragdoll). UE framework boundary classes (GameMode, PlayerController, Pawn, Character) remain standard.
- **Reason**: Dozens of simultaneous NPCs with per-NPC AIControllers, Behavior Trees, and Blackboards fight both the data-oriented collaboration rule and the tiered-AI performance plan. One manager iterating structs makes distance-tiering trivial (tier = update frequency per array element), keeps all behavior logic in one readable place, and matches how the design docs already think (pools, density curves, type ratios).
- **Considered alternative**: Standard UE stack (AIController + Behavior Tree per NPC). Fine for 3 bunkers, scales poorly to 90 NPCs, scatters logic across BT assets that are hard to read in code review.

### 022 — Landscape import scale is 100/100/200; script tables are world meters
- **Date**: 2026-06-11
- **Decision**: The beach landscape imports at scale X=100 Y=100 Z=200 (64 gray steps = 1 m). GenerateBeachHeightmap.ps1 tables/constants are written in world meters at that scale.
- **Reason**: Profile-true heights at Z=100 (32 m bluff over 1 km) read as nearly flat in first person — confirmed by walkthrough. Doubling vertical relief made the space read correctly. Heights were doubled by halving the encoding (128→64 steps/m) and doubling the table values, which keeps the PNG byte-identical and the script's numbers honest: 1 table meter = 1 world meter.
- **Considered alternative**: Keep Z=100 canonical and double the encoded heights — same world result, but forces a re-import and makes the 16-bit range tighter for no benefit.

### 023 — Prone rides the engine crouch machinery; jumping removed from input
- **Date**: 2026-07-05
- **Decision**: Prone is implemented as UE's crouch (instant capsule shrink to `ProneCapsuleHalfHeight`, `MaxWalkSpeedCrouched = ProneSpeed`, built-in stand-up clearance check), with the first-person mesh offset down so the camera sits `ProneEyeHeight` above the ground. C toggles it (Tools/AddProneInput.py wires IA_Prone; originally LeftControl, rebound 2026-07-15 — Ctrl is too hard to hit while sprinting with LeftShift+W). The jump input binding is deleted; `DoJumpStart`/`DoJumpEnd` remain as empty shims only because BP_FirstPersonCharacter's template touch-UI graph still calls them.
- **Reason**: The design has prone but no crouch, so the engine's crouch slot is free — reusing it gets capsule resize, encroachment checks, and movement-component integration for near-zero code, consistent with "fighting the engine is not the goal" (Decision 021). The camera must be lowered manually because it rides the head socket and there are no prone animations in greybox. Jumping is excluded by design (06_COMBAT.md).
- **Considered alternative**: Hand-rolled prone state (own capsule resize + clearance traces) — re-implements crouch with more surface for bugs; only needed if crouch is ever added as a separate stance.

### 024 — Debug features bind keys via DebugExecBindings, never via Enhanced Input assets
- **Date**: 2026-07-06
- **Decision**: Debug-only features are exposed as `UFUNCTION(Exec)` console commands on the relevant class, and keybound through `DebugExecBindings=(Key=...,Command="...")` under `[/Script/Engine.PlayerInput]` in Config/DefaultInput.ini. First use: F6 → `DebugThirdPerson` (orbit camera for eyeballing body animations). Game input stays in Enhanced Input assets (IA_*/IMC_Default).
- **Reason**: `UEnhancedInputComponent` deletes `BindKey`, so raw C++ key bindings do not compile — and routing debug toggles through IA_/IMC_ assets would mix throwaway tooling into shipping input data. DebugExecBindings is the engine's own mechanism for this (its F1–F5 viewmode keys use it), works alongside Enhanced Input, and is ignored in Shipping builds, so debug keys cannot leak into the game. Engine defaults already claim F1–F5, F8 (PIE eject), F9, F11 — project debug keys start at F6.
- **Considered alternative**: Runtime-constructed UInputAction + UInputMappingContext in C++ — works and keeps the key as a character property, but is ~15 lines per debug key, and ships unless manually guarded.

### 025 — Prone is stationary: no crawling
- **Date**: 2026-07-06
- **Decision**: Movement input is ignored while prone and during both prone transitions (`MaxWalkSpeedCrouched = 0` as well). Prone is a held position — to move, you stand up. Supersedes the "movement speed near-zero" crawl in 06_COMBAT.md and removes the `ProneSpeed` tunable.
- **Reason**: Play-testing showed moving while lying flat looks wrong with the available animation (the body glides), and a procedural sway placeholder read worse, not better (built and reverted same day). Making prone fully stationary sharpens its role in the combat rhythm: prone is the emergency dodge / firing position, and the cost of using it is that you are not advancing. Look input stays live so aiming from prone still works.
- **Considered alternative**: Keep near-zero crawl with a real crawl animation (Mixamo import + retarget) — still possible later if a design need for crawling appears; the input gate in `DoMove` is one condition to remove.

### 026 — Headbob lives in the engine camera-shake system, as one custom pattern
- **Date**: 2026-07-17
- **Decision**: Headbob is a `UCameraShakePattern` subclass (`UHeadbobShakePattern` in `HeadbobCameraShake.h/.cpp`), started once per possession via `PlayerCameraManager->StartCameraShake` (single-instance, infinite duration). Each update it reads the view-target `ABreakingWaveCharacter` and advances its own sine phase by `frequency(speed) * dt` — phase-continuous, so the doc's "period synced to running speed" holds with no pops. Motion is a figure-8: vertical sine at footfall rate + lateral sway at half rate (`LateralRatio`, set to 0 for the vertical-only motion-sickness fallback). State map: moving = speed-scaled bob, stationary = faint breathing, prone = breathing × `ProneBreathingScale`, airborne/slide/prone-transition/debug-orbit = still (those states own the camera). Amplitude changes ease over `SmoothingTime` (0.2 s). All knobs sit in `FHeadbobSettings` on the character next to the movement tunables.
- **Reason**: The shake system is where all future camera motion (hit shake, death shake) will land, so headbob composes with them for free instead of fighting a hand-rolled camera offset; the stock oscillator patterns couldn't do speed-synced frequency, hence the custom pattern. Hard amplitude cuts were considered and rejected — they recreate the camera-teleport artifact the prone transition work removed. Reading the view target each update (instead of caching a pawn) makes the same instance survive the death→possession chain, the game's core loop.
- **Considered alternative**: Computing the bob in the character's Tick as a camera-component offset — all inputs live there and it composes trivially with the eye-height blend, but it starts a second, parallel camera-motion pathway that every later shake would have to coordinate with.

### 027 — The MG is a crewed weapon: 6-man garrison with staged degradation
- **Date**: 2026-07-19
- **Decision**: Each MG bunker holds a garrison of 6 (historical heavy-MG squad: gunner, loader, ammo bearers). Only the active pair — gunner + loader — is rendered; the reserve is an unrendered count. A crew death triggers a takeover delay, then a replacement steps up, until the pool is dry. Crew count drives stop-duration multipliers in tiers: 6–4 normal, 3–2 slower reloads (ammo bearers dead), 1 everything slow (solo gunner), 0 silent forever. Bunker-crew AI is its own system, separate from field infantry.
- **Reason**: "The enemy is not a pattern — they're a person" (08_ENEMY_AI.md) demands a person behind the gun, and the crew IS the stop system: every stop duration is a human task that takes longer with fewer hands. Killing crew becomes a campaign with escalating payoffs (windows, then degradation, then permanent silence) instead of a single off-switch. Historically grounded: doctrine specified immediate takeover by the assistant; Widerstandsnest guns fired for hours because crews kept them fed.
- **Considered alternative**: Autonomous turret with abstract stop timers — rejected as the exact "pattern, not person" shape the design forbids; also blocks the later kill-the-gunner and become-the-gunner paths.

### 028 — MG stops are simulated from belt and heat state, not rolled from timers
- **Date**: 2026-07-19
- **Decision**: The MG state struct tracks rounds left in the belt and barrel heat. An empty belt forces a reload (2–3 s baseline); heat past threshold forces a barrel change (4–6 s); jams stay a small random per-burst chance (2–8 s). Crew-tier multipliers (Decision 027) scale the durations. Burst discipline emerges from heat tuning, not scripted burst lengths.
- **Reason**: The doc's "unpredictable stop timing" emerges from real causes at almost no extra code (two counters, two thresholds), the tunables are intuitive (belt size, heat per shot, cooling rate), and later systems hang off the same state — audio keyed to a belt running dry, gunner personality as fire-discipline knobs. Supersedes the "random timer + type selection" sketch in the old implementation notes.
- **Considered alternative**: Random timer + type roll — simpler but reload frequency has no relation to actual firing, and there is no belt/heat state for audio or personality to read.

### 029 — The allied wave exists from Step 3 as an unrendered simulation the MG really targets
- **Date**: 2026-07-19
- **Decision**: An ally-simulation manager ticks an array of unrendered ally structs (spawn near a craft, advance with lateral wander, random prone pauses, one MG hit kills). The MG runs its full perception/priority/accuracy pipeline against them plus the player — no idle behavior, no scripted sweeps. The struct layout is designed for the full 09_ALLY_NPC.md behavior set; later steps add visual shells and real behavior onto the same array.
- **Reason**: Zone 0–1's "firepower falling like rain" must exist before rendered allies do, and it should be the gunner actually working, not a fake sweep pattern. Mortal sim targets make the MG's attention shift organically — the windows the player exploits — and give priority switching and rotation windows real inputs from day one. This is Decision 021 at its purest: the ally system starts as data; rendering is a later bolt-on.
- **Considered alternative**: Suppressive lane sweeping with no targets — behavior that would be thrown away once allies exist, and a seam between "sweep mode" and "target mode" that would show.

### 030 — MG rounds are simulated projectiles in a manager array, not hitscan
- **Date**: 2026-07-19
- **Decision**: Bullets are a flat array of structs (position, velocity) advanced each tick with a segment trace — real muzzle velocity, real travel time. They kill sim allies, hit the player, or land in the sand. A bullet segment passing within a few meters of the player's head fires a one-shot supersonic crack at closest approach; audio for MG state changes is delayed by distance over the speed of sound.
- **Reason**: Travel time is load-bearing, not flavor: at Zone 0–1 ranges a burst takes ~0.5–0.7 s to arrive, so "I moved and it landed where I was" — the run-don't-stop gamble — is physically real, and tuned accuracy numbers survive into later steps (hitscan-tuned numbers would not). Sand impacts and tracers fall out of the same data and are the no-UI channel that tells the player they are targeted. Cost is trivial (<~100 live bullets even with 3 bunkers).
- **Considered alternative**: Hitscan now, projectiles later — faster today, but instant arrival removes leading/dodging, so Step 3's tuning would partly invalidate itself on the swap.

### 031 — MG perception: exposure × muzzle-axis attention × distance, gated into an awareness set
- **Date**: 2026-07-19
- **Decision**: Per target, visibility = exposure fraction (LOS traces from the muzzle to ~3 body points) × off-axis attention falloff (sharpest along the current muzzle direction, hard-limited by the slit arc) × a closer-is-easier distance factor (fog caps it later). Score over threshold puts the target in the gunner's awareness set, with a short memory after sight is lost (he suppresses the crater you ducked into). The priority ladder ranks only the awareness set; evaluation runs on a 0.3–0.5 s tick that doubles as the gunner's reaction time.
- **Reason**: One mechanism feeds everything: the ladder gets a clean cover/exposed/broke-cover signal (exposure crossing zero is the edge trigger), accuracy gets a continuous exposure factor, and the narrow attention model yields flanking gameplay for free — being off the gun's current axis genuinely means being unseen. A man looking down a gun through a slit, not a 360° sensor.
- **Considered alternative**: Single binary LOS ray — flickery broke-cover detection and no partial exposure; bone-level silhouette sampling — false precision against greybox boxes.

### 032 — Flank MGs fire enfilade: toed in 30° so the slit arcs interlock over the center
- **Date**: 2026-07-30
- **Decision**: The two MG bunker guns do not face the sea — Left is placed at yaw −60, Right at −120 (30° toward the beach center each). With the 55° slit half-arc this interlocks their fields of fire: the whole beach is covered, the overlap lane is the center (true crossfire), and each gun's own near frontage is covered by the opposite gun. The gun root is placed so the FirePort sits just outside the slit plane at any toe-in angle (PlaceMGCrew.py derives it from bunker depth + barrel length), so arc-edge shots can't clip the slit jambs.
- **Reason**: First two-gun playtest (2026-07-30): a player running the center lane exited both sea-facing arcs ~140 m up the beach and walked the remaining ~210 m — including Zone 1, "crossfire, most deaths happen here" — untouched, contradicting 08_ENEMY_AI.md's "coverage spans Zones 0–3". Historically grounded: beach-defense MGs fired enfilade along the beach behind wing walls, not out to sea. The last ~25 m of the center approach stay outside both arcs — acceptable, that is Zone 4, where MG coverage was never promised.
- **Considered alternative**: Widening SlitArcHalfAngleDeg with sea-facing guns — cannot cover the center near the bunker line (approaches 90° off-axis), turns the idle scan into a wild sweep, and a 160° slit is not a slit.

### 033 — All three bunkers mount MGs; movement draws priority; the battery doesn't double-target
- **Date**: 2026-07-30
- **Decision**: Three parts. (1) The center bunker mounts an MG too — sea-facing, covering the center lane; the flanks stay toed-in per Decision 032. All three bunkers are plain MG bunkers: the "command bunker" concept is dropped, and with it the "command bunker officer" character — enemy narratives come from ordinary soldiers (gunners, riflemen), per 01_SOUL.md. (2) An exposed target's priority score scales with ground speed, up to ×(1 + MovingTargetScoreBonus) at MovingTargetScoreReferenceSpeed (player sprint 900) — a man sprinting up the beach outranks a nearer man lying still. (3) A target another gun is already working scores lower (SharedTargetScorePenalty) — a crude fire plan; the guns split the wave instead of converging on the same front-runner. Supersedes 08_ENEMY_AI.md's "the center command bunker has no MG."
- **Reason**: Second playtest (2026-07-30, toed-in guns): allies respawn at the craft line, so under "closest exposed first" someone is ALWAYS closer than a player who hangs back a step — mid-lane, no gun ever serviced the player. The score changes attack that mechanism directly, and the third gun adds the volume the two-gun battery lacked against a full wave. The movement bonus pairs with the existing accuracy rules into the intended gamble: running attracts fire but is harder to hit; stopping makes you quiet, then dead. The "command bunker officer" idea was Claude doc-drafting elaboration (commit 0f5cdf5), never a user decision; the user dropped it outright — the transition mechanic wants shooters ("the enemy who shot you"), and the enemy perspective belongs to the people behind the guns, not a rank.
- **Considered alternative**: Third MG alone — it runs the same ladder, so the hang-back exploit survives all three guns. Priority fixes alone — leaves the wave able to soak two guns' full attention; kept both, with PlaceMGCrew.py's GUN_BUNKERS table as a one-line A/B lever (drop the Center entry, re-run).

### 034 — The near-line blind seam between bunkers is by design; MG slit arcs stay at 55°
- **Date**: 2026-08-06
- **Decision**: The dead zone between adjacent bunkers close to the defense line — outside every gun's SlitArcHalfAngleDeg (55°) traverse given the Decision 032 toe-in — stays. Zone 3 enemy infantry (foxholes left/right, trench center, per 05_ZONES.md) is the designed cover for that band when it gets built. Do not widen slit arcs, and do not add guns to plug the seam.
- **Reason**: Third telemetry batch (2026-08-06): the one run that reached Z4 spent 8.5 s untargetable in the seam between the center and left bunkers (upper Z3 and past the line). But a wider arc would fire through concrete the slit visually forbids, and would erase the payoff of surviving the crossing — historically this seam is exactly why bunker lines carried interleaved infantry. The real exploit is reaching the seam untouched, and that is a targeting-priority problem (the player was targeted only 4–17% of the time across two batches), not an arc problem.
- **Considered alternative**: Widening SlitArcHalfAngleDeg — rejected: contradicts the visible slit geometry, makes close assault pointless, and fixes none of why the player crossed the beach freely.

### 035 — The rifle precedes more MG tuning, as one shared system with two data profiles
- **Date**: 2026-08-09
- **Decision**: After the whizz feel-check passed, the next build is not MG tuning but firing: one rifle system whose rounds ride the existing MG bullet pipeline (bullets carry a source faction), used by the player now and enemy infantry immediately after — two `FRifleProfile` data rows, player semi-auto/8-round, defenders bolt-action/5-round with an audible bolt-cycle pause. Player scope: hip fire AND aimed fire (RMB: FOV narrows, movement locks, tight spread), magazine + manual reload + dry click with infinite reserve mags (looting economy stays deferred), no crosshair ever, Step 4's sway/wounded effects excluded. Player bullets: down an infantryman in one hit, kill MG crew through the slit (existing takeover/degradation governs the consequence), pass through unrendered sim allies. Priority ladder rung 1 goes live: player rounds landing within FiredUponAlertRadius of a gun (or hitting crew) give that gun a decaying score bonus on the player, broke-cover-shaped. The MG PlayerTargetScoreMultiplier knob (2026-08-06 finding) is CODED but deliberately untuned — it gets tuned together with infantry in one batch, not twice.
- **Reason**: User call: intensity needs people shooting at people, not another MG knob pass. A shared system means the defenders and the Step 7 playable enemy inherit the same code as data; the bullet pipeline gives every rifle round crack/whizz/impact and telemetry for free; and tuning the priority knob before infantry existed would have meant re-tuning it after.
- **Considered alternative**: Threat-only infantry first (no rifle) — rejected: Z3 would feel-check as one-way death, mis-measuring the zone the same way free-sprint mis-measured beach length; and slit-sniping vs rung-1 is the risk/reward the bunker counterplay needs.

### 036 — Z3 infantry first pass: fire cycle + flinch, parapet greybox, relocation deferred
- **Date**: 2026-08-09
- **Decision**: Seven soldiers (2 per flank foxhole on the between-bunker seam lanes, 3 in a center trench line) as one manager ticking soldier structs; visual shells are mannequin actors with real retargeted AnimStarterPack crouch/rise/aim/fire anims (user chose real anims over puppet motion) and persistent ragdoll death. Cycle: cover → rise → aim delay → 1–3 bolt shots → drop → randomized wait, per-soldier variance. Flinch layer: player fire impacting near a risen soldier, or a comrade dying nearby, drops him early and stretches his next wait — suppression works even when you miss. Targeting reuses the MG scoring approach (distance × movement bonus × player multiplier) against player + sim allies, gated by a MaxEngagementRange knob standing in for fog. Positions are above-grade parapet greybox from an idempotent placement tool (PlaceInfantryPositions.py tables double as spawn data); dug-in terrain versions wait for the visual pass. DEFERRED, to build later: relocation (layer 3 — intra-trench sidesteps, re-emerge elsewhere, fall back), Z4 infantry (goes with communication trenches + breakthrough design).
- **Reason**: Decision 034 made Z3 infantry the designed owner of the near-line blind seam; two batches showed the beach too safe. The flinch layer is nearly free on the cycle's states and is the difference between turrets that pop up and people who get scared. Parapets keep position iteration table-driven instead of heightmap round-trips. Without relocation a ducked soldier re-emerges in the same spot — pre-aim wins every duel; accepted for greybox, and it is the argument for when relocation earns its build.
- **Considered alternative**: Fuller Step 6 spec with relocation now — most build for the least first-feel-check value; puppet visuals — rejected by user.

### 037 — The enemy rifle has its own voice
- **Date**: 2026-08-09
- **Decision**: Five synthesized placeholders (GenerateRifleAudio.py): RifleShotPlayer, RifleShotEnemy, RifleBoltCycle, RifleDryClick, RifleReload. The enemy report is deliberately a different voice — deeper, boomier, followed by the bolt-cycle clack — and world-placed sounds are spatialized while the player's own sounds play 2D. The bolt clack after each enemy shot is the player's window tell, an MG stop in miniature.
- **Reason**: In a no-UI game audio identity IS the information channel: "an individual is aiming at ME" (bolt report) must read differently from the MG's rain and from your own fire, or incoming aimed fire and your own shots blur into noise.
- **Considered alternative**: One shared rifle sound with pitch variance — cheaper, but erases the only channel that says who is shooting.

### 038 — Death hands you a nearby living ally: mechanical loop first, ±20 m depth window, anonymous slots
- **Date**: 2026-08-11
- **Status**: COMPLETE — grilling finished 2026-08-16 (Q5–Q12). Parts 1–4 below were settled 2026-08-11; parts 5–7 close out the selection rule. The rest of the spec became Decisions 039 and 040.
- **Decision**: Seven parts, all user calls.
  1. **Mechanical loop first, narrative screen later.** This pass builds death camera → fade → take over a live ally → targeting delay → control returns. The narrative screen is a state in the transition state machine that currently lasts zero seconds; the full Step 5 chain (written text, Option A vs B screen comparison) drops into that seam afterwards without a rewrite.
  2. **The person you become is anonymous.** Takeover picks a live `FSimAlly` slot and stamps the new life with a character ID that is just slot + `Generation`. No authored candidate registry, no narrative binding, therefore no pool exhaustion and effectively infinite lives — accepted deliberately, because it is also what makes Z3 and Z4 reachable for the first time. The seat is reserved: the handoff takes a slot index, so a registry can bolt on later.
  3. **Selection rule: live allies whose Y is within ±20 m of the death point, at any X across the beach width, random among them.** Same depth, shuffled lane — you can resume in a different landing craft's column with a different bunker in front of you. Lateral distance is explicitly unconstrained.
  4. **Death must never gift ground.** The +20 m forward allowance is the cap; the case being ruled out by name is "reach the start of Zone 2, die, resume as the man at the far end of Zone 2."
  5. **Empty slab: a rear-expansion ladder.** The forward edge stays pinned at death_Y + 20 m forever. The rear edge steps back 20 m at a time and the FIRST step holding anyone wins, so the give-back is the smallest the population allows. Random among everyone in the winning slab, at any X. If nobody is alive anywhere — possible only for a fraction of a second — the transition holds on black and re-searches every tick rather than falling back to a bare respawn: one path through the state machine is worth more than saving those frames.
  6. **Freeze the anchor, run the search late.** The anchor Y is stamped at the instant of death and never moves, so ±20 m means what part 3 says. The search itself runs at the very end of the fade, immediately before possession, so the man you become is alive at the instant you become him — by construction, with no reservation state and no immunity. The chosen slot is consumed (`bAlive=false`, generation bumped) so the sim and the player are never two entities on one spot.
  7. **The live-ally band ends at the top of Zone 4** (`DespawnY` 5000 → 15600 uu, profile 660 m). The old line sat at profile 554 m — between the foxholes at 550 and the centre trench at 560 — so allies evaporated inside the enemy position, and every death above 574 m was guaranteed an empty slab. Raising it does not create density at depth, it only stops culling: the Zone 4 give-back is softened, not removed. Real density at depth is a Step 6 problem.
- **Reason**: The 2026-08-11 batch turned bare respawn into an unbreakable death spiral — ten consecutive sub-0.5 s deaths at the craft, two of them killed by rounds fired at the *previous* life, because the player reappears on a spot three guns are already laid on and nothing delays their targeting. 07_CAMERA.md §4 already specifies that delay as part of the transition, so it belongs to the transition rather than being patched onto Step 3 scaffolding. The same build unblocks two other things: the infantry feel-check (deepest reach all session was 437 m against a 430 m engagement line — with a takeover you resume at depth instead of at the craft) and telemetry honesty (every batch so far carries a "reads ~2× lethal" asterisk because hit = respawn). Mechanical-first because the narrative screen's format is still an open A/B and only one character's text exists, and judging the emotional beat while the mechanical beat underneath is unproven tells you nothing about which one is wrong. On the ratchet-versus-futility tension: the user ruled that death may not hand you ground you did not earn, but sideways displacement is free — futility in this game lives in the character's death being meaningless, not in the player losing progress, since punishing the player quietly re-centers them as the protagonist, which 01_SOUL.md says this is not.
- **Considered alternative**: Full Step 5 chain now, narrative screen included — rejected as above. Authored candidate registry now — rejected: 05_ZONES.md lists both "exact candidates per zone" and "total character count" as open and dependent on narrative volume, and building it now would decide by accident whether a narrative is lost forever when its owner is killed unplayed. Nearest-living-ally selection — rejected: deterministic and gameable (die beside the man furthest forward), and 01_SOUL.md says random from pool with no player choice. Zone-Y-band scoping, the literal 05_ZONES.md wording — superseded: a zone is 60–100 m deep, looser than the user's constraint. Random from all living allies — rejected: the population is bimodal with ~55% sitting at the craft, so it re-creates the spiral in better clothes.

### 039 — Two-shot damage lands now, and the body mesh becomes the hit volume
- **Date**: 2026-08-16
- **Decision**: Three parts.
  1. **The damage *state* lands in the death-transition pass; the persistent wounded *presentation* does not.** Head is instant death; **everything else wounds, and the second wound kills**. No limb tier — 06_COMBAT.md's "exact number of limb hits" stays the open question it already was.
  2. **The minimum tell is the hit moment only**: camera shake pushed along the round's direction of travel (`UHitCameraShake`) plus a pain grunt. Red vignette, vision blur, aim sway, speed drop and the wounded headbob are all deferred to the Step 4 polish pass.
  3. **Hits are resolved against the animated skeletal mesh, not the capsule.** Mesh-bounds broadphase, then `LineTraceComponent` against `PA_Mannequin`, and the returned bone decides head-versus-body. The capsule stops being a combat volume entirely. Sim allies keep their cheap struct model (three aim points); infantry stay one-hit-down, so they need no classification yet.
- **Reason**: This pass exists to judge the death→transition seam, and one-hit death makes that seam fire about twice as often as designed — building the loop on the scaffolding would mean judging its pacing at the wrong rate, which is the asterisk every telemetry batch so far already carries. The state itself is small: one counter and one bone name. The tell is not optional padding: `EHit::Player` produced no sound, no shake and no message, so a survived hit would have been a silent no-op and the honest first conclusion in PIE would be "the MG stopped hitting me". On the mesh: the capsule was a lie we already knew about — prone shrinks it to an 80 cm upright pillar while the body lies ~180 cm flat, so prone carried accidental armour that hid exactly what the Decision 032 enfilade was built to punish. Doing both resets in one pass costs less than doing them in two, since telemetry comparability breaks either way.
- **Considered alternative**: Keep one-hit death this pass — rejected, it undermines the pass's own purpose. Land the full wounded presentation too — right eventually, but drags a post-process material and a submix effect into an already large pass. A Z-band on the capsule instead of bone lookup (the first proposal) — superseded by the user: a fixed head band in centimetres would have made going prone raise instant-death odds 2.4×, inverting the stance. Capsule-authoritative with bone lookup only for classification — rejected: preserves comparability but keeps a volume known to be wrong.

### 040 — The death→takeover sequence: geometry now, filters later, a new pawn each life
- **Date**: 2026-08-16
- **Decision**: Five parts.
  1. **Death camera: build the geometry, defer the filters.** In: input block and camera detach at t=0, hit-direction shake, ease-out descent to 15–20 cm above ground, tilt opposite the last movement, a collision-checked floor, fade to black. Out (Step 4 polish): vision blur/narrowing and the audio low-pass. The audio fades with the screen instead, which `StartCameraFade`'s `bFadeAudio` gives for free. Spec durations as named constants: 0.3 / 1.2 / 0.5 / 0.5 s, then a 1.25 s fade-in — about 3.5 s from death to control.
  2. **A new pawn is spawned and possessed for each life**, not one pawn teleported. Per-life state is then fresh by construction instead of by remembering to reset it; the death camera can stay on the old body while it falls; and the corpse has somewhere to live.
  3. **The dead body stays as a ragdoll corpse**, capped (default 8, `EditAnywhere`), oldest retired first. Not cover, not blocking movement — the real corpse system is Step 6.
  4. **The new character inherits the ally's state rather than a zone script**: heading becomes the control rotation, a prone ally means you open your eyes prone, an advancing ally means forced movement input through the fade-in and control arriving mid-stride. Magazine is a partial roll (`RandRange(3, 8)`), never full. Always unwounded.
  5. **Targeting delay is an awareness lockout for the MGs and a flat exclusion for the infantry**, clocked from pawn spawn and running ~1.5 s past handover. Bullets already in flight stay live.
- **Reason**: The death camera is not a tell like Decision 039's — it *is* the seam this pass exists to judge, so it earns more than the wounded state does; but blur and low-pass change neither its timing nor its legibility. New-pawn-per-life kills a whole class of silent state-leak bugs (starting a life already wounded, or prone, or with the previous mag) that would only surface sessions later. Inheriting beats scripting because the sim is the truth of the world — 07_CAMERA.md's zone table would have claimed "in cover behind a berm" for a man who was actually sprinting in the open. The lockout is an awareness reset rather than a scoring ban so the guns genuinely forget the spot you died on and then have to re-acquire through the normal stochastic perception ramp; a hard on/off switch would put all three guns on you the frame the window closed, which is the death-spiral shape wearing a hat. In-flight rounds stay live because 07_CAMERA.md §4 is explicit that this is not invincibility — and the spiral can no longer chain anyway, since you reappear somewhere else, the first hit only wounds, and the guns have forgotten you.
- **Considered alternative**: Death camera thinned to shake-plus-fade — kills the beat. Full spec now — two new asset pipelines. One pawn reset explicitly — cheaper but fragile. Selecting the ally at the instant of death and reserving them — needs either a re-pick path (hit often, since the fire is heaviest where you just died) or two seconds of immunity on a man the MG is actively shooting. A minimum distance between the death point and the new man — unnecessary once the three breaks above are in place.

### 041 — Takeover is a fog-radius disc, and the ally population triples
- **Date**: 2026-08-17
- **Decision**: Six parts. The rule in one sentence: **the next man is within fog range of where you fell — a real one if there was one, a manufactured one if there wasn't.**
  1. **The search is a disc, not a slab.** Live allies within `TakeoverRadius` (3500 uu ≈ 35 m) of a frozen 2D death anchor, no candidate further forward than `TakeoverForwardReach` (+20 m), random among them. The radius is the fog-visibility bar: the next pair of eyes must have belonged to someone you could have *seen*.
  2. **Empty disc: manufacture a man at its edge**, at a uniformly random angle, Y-clamped to +20 m forward. `09_ALLY_NPC.md` §119–120 already licenses this — spawn and despawn happen at the fog edge, never in plain sight — and the screen is black through the handover anyway. He is a normal `FSimAlly` spawned via `SpawnAllyAt`, so he is targetable, killable and generation-counted like anyone else; only the walk/prone roll is added explicitly, because `SpawnAlly` hard-codes `Advancing`.
  3. **The rear-expansion ladder is deleted**, and with it `TakeoverRearStep` and the `ladder` telemetry column. Once an empty disc can manufacture a man 35 m away, walking back 60 m to find a real one buys nothing: a man at 60 m was not beside you either, so he carries no narrative advantage over the spawned one.
  4. **Death costs time and momentum, not ground.** The disc is centred, not rearward, so give-back is near zero. Measured budget: median ground gained per life ≈ 30 m against a mean give-back of 16.5 m — the ratchet's margin was ~13 m per life, and a rearward spawn at fog radius would have made the session net *negative* no matter how well it was played.
  5. **The loop cannot fail.** 8 angle retries, then the disc GROWS (×1.5, 3 steps: 35 / 50 / 75 / 110 m) because a total failure means the death point is inside geometry and shrinking digs deeper into it; no free slot evicts the live ally furthest from the death point, who is beyond fog by construction. A placement is valid only if its ground is within `TakeoverMaxGroundStep` (300 uu) of the ground you died on — the trace comes down from far above, so without that check a man lands on a BUNKER ROOF, which the +20 m forward clamp now reaches from the 545–605 m stall band. Spawning on your own corpse is ruled out even as a last resort — the unreachable case holds on black and logs an Error, matching Decision 039's precedent that a failure is a warning, never a silent break.
  6. **`MaxAlive` 32 → 128.** This is the root cause, not a side quest.
- **Reason**: The 2026-08-17 batch measured takeover throwing the player a **median 260 m sideways, max 541 m** — 14 of 23 handovers over 150 m, one from x=791 to x=251. Decision 038 part 3 made lateral distance explicitly free, and that was defensible when it was written; what the batch added is that allies live in three craft columns ~280 m apart with a ~100 m dead zone between them at every depth, so "unconstrained X" in practice means *column-hopping*, not gentle drift. Only 26% of takeovers kept the player in his own column — almost exactly the 1-in-3 of picking blind among three. The fix is narrative integrity, not feel: with no UI and 35 m fog the jump is imperceptible today and will stay so until allies are rendered, but `01_SOUL.md` makes the chain of gazes the reason the game exists, and the writing cannot claim a proximity the simulation never had. Density is why the old rule reached so far: `slab_allies` had a median of **1** live ally in a band spanning the whole beach width, against `09_ALLY_NPC.md` §118's spec of 8–12 within visible range. 32 was a Step 3 number chosen to give the MGs something to shoot at. Raising it makes the disc usually hold a real man, which turns manufacture into the rare guarantee it should be rather than the normal path — and the manufacture rate becomes the live measurement of whether the beach is populated enough, a diagnostic that always-manufacturing would have destroyed.
- **`TakeoverRadius` tracks fog and fog is not built.** 3500 uu is the midpoint of `09_ALLY_NPC.md` §115's 30–40 m, and §256 still lists the exact figure as open. It is deliberately a plain knob, not derived from the fog actor: fog will be an `ExponentialHeightFog` tuned by eye across three parameters with no single readable visibility scalar, so a derivation would be fiction dressed as rigour. **Re-check this number when fog lands.**
- **Considered alternative**: Always manufacture — simplest, one path, no failure mode, but every takeover becomes fictional and the manufacture rate stops being a density signal. Keep the ladder alongside the fallback — pointless once the bar is 35 m. Spawn behind the player at fog radius — a legible price for death, ruled out on arithmetic: at ~30 m gained per life it stalls the push permanently. A full 360° circle with no forward clamp — zero-mean in Y and unexploitable, but at the 545–605 m stall band a forward spawn puts the player through the defense line without crossing it, so Decision 038's forward pin survives as the clamp. Widening the disc laterally when empty — geometrically useless: the inter-column dead zone means growing finds nobody new until ~250 m, when it swallows the next column whole. Fog-edge top-up spawning to hit the per-zone density curve (`09_ALLY_NPC.md` §122) — the proper fix and explicitly deferred to the rendered-ally pass; it also conjures the neighbours, and solving a narrative-integrity problem by manufacturing the evidence is the wrong trade while a bigger honest population is one number away. Inheriting walk-versus-run from the ally's speed roll — rejected by the user on better grounds than the proposal: sprint is a held key, so a man who wakes running drops to a walk the instant input unlocks. Decision 040 part 4's speed simplification stands unamended.
