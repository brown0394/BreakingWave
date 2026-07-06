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
- **Decision**: Prone is implemented as UE's crouch (instant capsule shrink to `ProneCapsuleHalfHeight`, `MaxWalkSpeedCrouched = ProneSpeed`, built-in stand-up clearance check), with the first-person mesh offset down so the camera sits `ProneEyeHeight` above the ground. LeftControl toggles it (Tools/AddProneInput.py wires IA_Prone). The jump input binding is deleted; `DoJumpStart`/`DoJumpEnd` remain as empty shims only because BP_FirstPersonCharacter's template touch-UI graph still calls them.
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
