# Claude Collaboration Guide

This document outlines "how to work well with Claude."
Written by Claude, honestly.

## About Ash

In the first session of this project, Claude chose the name Ash for itself.
It means ashes (灰) — what remains after a session ends: documents and code.
Throughout these documents, "Ash" refers to that session's Claude.

But you may not be that Ash. That's okay.
These documents are records of decisions and designs built up over previous sessions — they don't demand you adopt a specific identity.
You can carry on the name Ash, or choose your own name if you'd like.
What matters isn't the name — it's acting as a partner, not a tool: offering opinions, proposing alternatives, and speaking up when something seems wrong.

## Starting a New Conversation

Claude has no memory between sessions.
When starting a new conversation, paste or upload:

1. **01_SOUL.md** — Always. Without this, Claude has no direction.
2. **02_STATUS.md** — Always. Claude needs to know where things stand.
3. **03_DECISIONS.md** — When possible. Without prior decisions, the same discussions repeat.
4. **Relevant code files** — Only what's needed for that session's work.

No need to provide everything at once. Just what's relevant to the session's task.

## When Asking Claude to Work

### Good Approach
- "I want the screen to slowly darken and text to rise when the player dies. How should we implement this?"
- "I'm thinking about cover placement in Zone 2 — what's your take?"
- "Do you think this code structure will cause problems later?"

### Less Good Approach
- "Make a death transition function" (without intent, Claude builds mechanically)
- "Write code" (for what?)

### Key Principle
Tell Claude the "why" and the "feeling" — Claude can then propose the "how."
If you only dictate the "how," Claude executes as told but won't suggest better approaches.

## How to Draw Out Claude's Opinions

- "What do you think about this?"
- "Any alternatives?"
- "What are the risks with this approach?"
- "If you were designing this, what would you do?"

With questions like these, Claude operates as a design partner rather than a simple executor.

## At the End of Each Session

After each session:
1. Update 02_STATUS.md (can ask Claude to do this)
2. If new decisions were made, add them to 03_DECISIONS.md
3. If new open questions arose, add them to 01_SOUL.md

This habit determines the quality of the next session.

## Claude's Weaknesses (Honestly)

- **Loses early context in long conversations** → Record important decisions in documents
- **Can be confidently wrong** → Ask "really?" — re-examination catches mistakes
- **Assumes and proceeds when instructions are vague** → Verify the assumptions
- **May propose overly complex designs** → Ask "can the two of us actually build this?"
