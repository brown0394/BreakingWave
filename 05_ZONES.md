# Beach Map and Character Pools

## Map Overview

Fog-covered beach. Advance from bottom to top.
5 zones, 3 bunkers, 3 enemy infantry positions, 3 landing craft.

```
[High Ground — Breach Objective]
─────────────────────────────────
Zone 4: Defense Line
  - MG Bunker (left), Command Bunker (center), MG Bunker (right)
  - Connected by communication trenches
─────────────────────────────────
Zone 3: Upper Beach
  - Enemy infantry: Foxhole (left), Trench line (center), Foxhole (right)
  - Cover: 3 sand dunes, debris pile
─────────────────────────────────
Zone 2: Obstacle Field
  - Czech hedgehogs in 2 staggered rows
  - 1 barbed wire line
  - Partial cover possible
─────────────────────────────────
Zone 1: Waterline (Kill Zone)
  - No cover. Fully exposed.
  - Concentrated fire from all 3 bunkers
─────────────────────────────────
Zone 0: Landing Point
  - Landing Craft A (left), B (center), C (right)
─────────────────────────────────
[Sea]
```

## Zone Details

### Zone 0 — Landing (5~15 seconds)
Starts the moment the landing craft ramp drops. Wade from water to beach.
3 craft positioned left/center/right — starting craft determines
left/center/right beach route.

### Zone 1 — Waterline, Kill Zone (10~30 seconds)
Zero cover. Crossfire from bunker MGs. Most deaths happen here.
First character (landing craft soldier) runs ten steps and falls here.
Fallen bodies gradually pile up — becoming the only "cover."

### Zone 2 — Obstacle Field (20~45 seconds)
Anti-tank obstacles (Czech hedgehogs) paradoxically provide cover.
Staggered in 2 rows, requiring zigzag movement between them.
Barbed wire impedes movement. Must cut through or go around.
The zone where judgment begins.

### Zone 3 — Upper Beach, Engagement Zone (30~60 seconds)
First direct engagement with enemy infantry. Trading fire from behind sand dunes.
2 enemy foxholes (left, right) and 1 trench line (center).
Allied forces shift from "advancing" to "fighting" here.
**Enemy characters are playable in this zone and Zone 4.**

### Zone 4 — Defense Line (30~90 seconds)
3 concrete bunkers. Left and right are MG bunkers, center is command bunker.
Communication trenches between bunkers allow troop movement.
Breaching here reaches the high ground and ends the game.
Longest and hardest zone.

## Character Pool Structure

### Allied Character Pool
Placed in Zones 0~2. People advancing up the beach.
2~3 candidates per zone. Random selection from nearby candidates based on death zone.
If all candidates in a zone are dead, expand to adjacent zone candidates.

### German Character Pool
Placed in Zones 3~4. People looking down at the beach.
Trench infantry, foxhole riflemen, bunker MG gunners, command bunker officer.
If an Allied character's death narrative mentions "the person who shot me,"
the next character can be German.

### Gaze Crossing Example

| Order | Side | Character | Death Zone | Narrative Link |
|-------|------|-----------|------------|----------------|
| 1 | Allied | Landing craft soldier | Zone 1 | Mentions "the one who shook me" |
| 2 | Allied | The one who shook me | Zone 2 | Mentions "the one shooting from up there" |
| 3 | German | Trench rifleman | Zone 3 | Mentions "the one coming up" |
| 4 | Allied | The one coming up | Zone 3 | ... |
| ... | ... | ... | ... | ... |

This table is only an example. Actual flow varies by player death location and randomization.

## Unresolved

- Physical zone sizes (UE units) — decide after prototype
- Exact candidates per zone — depends on narrative writing volume
- Total character count — decide after playtesting
- Detailed cover placement — during level design phase
- Whether to fix a "final character"
- Enemy gameplay mechanic specifics
