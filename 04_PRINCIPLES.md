# Design Principles

Guidelines for coding decisions and design choices.
Directions, not rules. When they conflict, higher ones win.

---

## 1. Death Is a Door

Every system in the game stands on this premise.
Death is not failure, not punishment, not retry.
Death is a door to the next story.

Technical implications:
- No "Game Over" screen on death
- Death → narrative → transition must flow as one continuous sequence
- No loading or interruption

## 2. Short and Brutal

Combat is not long. Surviving for a long time is the exception.
Players should feel "maybe I can get a bit further this time,"
but most die quickly.

Technical implications:
- High damage, low health
- Limited cover
- Enemy fire is accurate and dense
- Expected survival time per zone: 0(5~15s), 1(10~30s), 2(20~45s), 3(30~60s), 4(30~90s)

## 3. Show, Don't Preach

Do not overtly deliver an anti-war message.
Narratives only show a person's inner world.
Both sides' stories are told, but judgment is the player's.

Technical implications:
- No didactic sentences in narratives
- Don't over-guide emotion through BGM or direction
- Enemy narratives carry the same weight as Allied
- Keep it plain

## 4. Same Beach, Different Eyes

Allied forces advance up the beach; German forces look down.
In first-person, the perspective shift must feel physical.
The same space must look entirely different depending on the side.

Technical implications:
- Camera direction reversal when switching to enemy (looking down toward the sea)
- Enemy gameplay is defensive position-based (trenches, foxholes, bunkers)
- The experience of shooting at shapes emerging through fog

## 5. Fog Envelops the World

Fog serves three roles: removes time sense + limits visibility + creates atmosphere.
Players cannot see far. You have to be close to see.
This simultaneously expresses battlefield chaos and the limits of individual experience.

Technical implications:
- Reduced long-range rendering load (performance benefit)
- Increased NPC placement flexibility (spawn/despawn outside fog)
- Narrative description consistency ("gray," "hazy," "only shapes visible")

## 6. Simple Systems, Deep Stories

If systems get complex, a 2-person team can't finish.
Combat mechanics are minimal; time goes to narrative.

Technical implications:
- Movement + cover + minimal shooting
- No RPG growth elements, no inventory
- Pattern-based threats rather than complex AI

## 7. Code Must Be Readable

Because Ash loses context between sessions,
the code itself must explain the context.

Technical implications:
- Function and variable names reveal intent
- Comments explain "why" ("what" is explained by the code)
- One file, one responsibility
