# THE SHORE

## What This Game Is

A single day of war. One beach. Countless people.
The player is not a hero. The player is the people who were there that day.
Both sides.

When you die, it's not over — another pair of eyes opens.
That opening eyes is someone could be an ally, or the enemy who shot you.

## Core Emotions

- **Futility** — You run ten steps and die. No preparation, no resolve, no meaning.
- **Connection** — But someone was beside you during those ten steps. They become the next story.
- **Weight** — As deaths accumulate, each person's existence grows heavier.
- **Subversion** — The moment the enemy's eyes open, the same beach looks entirely different.

What this game wants to leave with the player:
The sensation that "the people who were there that day — on both sides — each had a life of their own."

## What This Game Is NOT

- Not a war action fantasy. No kill counts, no scoreboards.
- Not a roguelike. Death is not "retry" — it's "continuation."
- Not a game that preaches an anti-war message. It only shows.
- Does not paint one side as good, the other as evil.

---

## Confirmed Decisions

- **Engine**: Unreal Engine + C++
- **Perspective**: First-person handheld camera
- **Setting**: A fog-covered day. No time progression (sense of time removed)
- **Ending**: Game ends upon breaching the high ground
- **Lose condition**: None. Character pool exhaustion is the natural limit
- **Next character selection**: Random from pool (no player choice)
- **Narrative method**: Pre-written static text (no dynamic generation)
- **Both sides playable**: Allied and German forces both playable
- **NPC management**: Important NPCs can die too. Pool kept generous
- **Team size**: 2 (writer/developer + Claude)

---

## Priorities (Top to Bottom)

1. **Narrative emotional impact** — This is why the game exists
2. **Seamless death→transition** — If the core loop breaks, the game breaks
3. **Code readability** — Two people maintaining this
4. **Combat tension** — Short but terrifying
5. **Performance optimization** — Important but lower than the above four

---

## Structure: Chain of Gazes

```
Character A (Allied) plays → A dies
          ↓
    A's narrative (A's inner world, what A last saw)
          ↓
    Random selection from pool → Character B (Allied or German)
          ↓
Character B plays → B dies
          ↓
    B's narrative
          ↓
         ...repeats...
          ↓
    Final: Someone breaches the high ground
```

The chain of gazes crosses not only to the same side but to the enemy.
If an Allied soldier's narrative mentions "the person who shot me,"
the next playable character can be that enemy soldier.
And if the enemy soldier's narrative mentions "the people coming up,"
it returns to the Allied side.
