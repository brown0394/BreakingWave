# Camera System

## Core Philosophy

The camera is the character's eyes.
First-person handheld. Not a machine — a person holding a camera.
The camera does three things in this game:
1. During combat — convey the chaos of the battlefield through the body
2. On death — stage the act of falling in a controlled way
3. On transition — create the experience of new eyes opening

---

## 1. Combat Camera

### Headbob
- **Small amplitude.** Motion sickness prevention is top priority.
- Primarily vertical bounce. Minimize horizontal sway.
- Sync period to running speed.
- No headbob when stationary — only faint breath-driven movement.
- Near-zero headbob while prone.

### Headbob While Wounded
- Add a staggering pattern distinct from normal movement.
- Period becomes irregular — a limping feeling.
- Amplitude increases slightly — the sensation of struggling to stay upright.
- Headbob alone should communicate "I've been hit."

### Hit Camera
- **Camera shake** — Medium intensity. Too strong causes nausea; too weak goes unnoticed.
- **Hit direction hint** — Hit from the left pushes the camera right. The only way to communicate "where did that come from" through the body, without UI.
- **Red vignette** — Red bloom at screen edges. Stronger and more persistent in wounded state.
- **Pain sound** — Character voice. Short grunt immediately on hit.
- Additional hits while wounded — increased shake intensity, stronger red vignette, more vision blur.

### Aiming Camera
- FOV narrows slightly when aiming (zoom effect).
- No movement. Headbob stops.
- While wounded and aiming — increased sway. Precise aiming is harder.
- Subtle up-down movement from breathing.

---

## 2. Death Camera

### Semi-Scripted Approach

Pure ragdoll camera is not used.
Ragdoll has problems: clipping through terrain, unrealistic bouncing, spinning view.
The body falls as a ragdoll, but the camera runs on separate logic.

### Death Sequence (1.5–2 seconds)

```
Death trigger
    ↓
[0.0s] Input blocked. Camera control separated from physics.
    ↓
[0.0–0.3s] Camera shakes strongly in the direction of the hit.
    ↓
[0.3–1.5s] Camera smoothly descends from current height to ground level.
           While descending, slowly tilts to one side.
           Vision gradually narrows and blurs.
           Sound fades away (audio low-pass filter gradually applied).
    ↓
[1.5–2.0s] Camera stops. Final view locked.
           What's visible in that view — sky, sand, someone's boot, fog.
           Varies by camera direction, but never clips into terrain.
    ↓
[2.0s+] Fade out. Transition to narrative.
```

### Camera Fall Rules
- Camera stops 15–20cm above ground (the height of a face touching the floor).
- Tilt direction is opposite to last movement direction — the feeling of "running forward, falling sideways."
- Mandatory collision check to prevent camera clipping into terrain or objects.
- Fall speed starts fast and decelerates at the end (ease-out).

### Audio Sync
- On death, ambient sound is progressively muffled by a low-pass filter.
- Heartbeat slows, then stops.
- What remains at the end — wind, or distant gunfire, or someone's voice.

---

## 3. Narrative Screen

### Two Candidates (Compare in Prototype)

**Option A: Black background + white text**
- Complete severance. The battlefield disappears; only the interior remains.
- May suit dry internal monologue well.
- Simple to implement.

**Option B: Final view residue + text**
- Capture the last frame from the death camera.
- Apply blur + desaturation, use as background.
- Text appears over it.
- The feeling of "this person is dying while looking at this."
- Implementation: capture render target at death moment → post-process.

**Decision pending** — compare both in prototype, then decide.

### Text Display
- One sentence at a time. Typing effect or fade-in.
- Appropriate pause between sentences (breathing room).
- Whether the player advances with a button press or it auto-advances — undecided.
- If narration (TTS) is used, text display syncs to voice speed.

---

## 4. Transition — New Eyes Open

### Fade-In Sequence (1–1.5 seconds)

```
Narrative ends
    ↓
[0.0s] Black screen (or residual background).
    ↓
[0.0–0.5s] Screen gradually brightens. Vision starts blurry.
           New character's ambient sound gradually comes in.
    ↓
[0.5–1.0s] Focus sharpens. The world becomes clear.
           We see what this character was already doing.
    ↓
[1.0–1.5s] Control transferred. Player can move.
           Enemy targeting delay begins (enemies don't target this character for 1–2 seconds).
```

### Starting State by Zone

| Zone | New Character State | What's Visible |
|------|-------------------|----------------|
| Zone 0 | Inside landing craft. Ramp about to drop. | The back of the person ahead, grey sky, spray |
| Zone 1 | Already running. | Sand, water, people running ahead, impacts |
| Zone 2 | Moving between obstacles or prone. | Steel structures, sand, wire |
| Zone 3 | In cover behind a sand berm or firing. | Fog beyond the berm, muzzle flash |
| Zones 3–4 (enemy) | Inside trench / foxhole / bunker. Looking down. | Shapes rising through the fog |

### Enemy Targeting Delay (Replacing Invincibility)
- No invincibility frames. This game has none.
- Instead: for 1–2 seconds after switching to a new character, enemy AI doesn't target them.
- Logical basis: enemies also need time to perceive and aim at a new target.
- Area fire (grenades, MG sweeps) can still hit during this window.
- Not full invincibility — just "not yet targeted."

---

## 5. Special Handling for Enemy Transition

When the viewpoint crosses over from Allied to German:

- When the fade-in happens, **the camera is facing the sea.**
- The direction you've been climbing is reversed.
- This reversal alone is a powerful experience — no additional staging needed; the perspective shift is enough.
- Shapes rising through the fog are visible — yourself from five minutes ago.

German → back to Allied:
- Now facing away from the sea, climbing the beach again.
- The sense of "I have to go up again."

---

## UE Implementation Notes

### Death Camera
- On death trigger, PlayerController switches camera control to a custom CameraActor.
- Lerp camera position/rotation to target point (near ground) over time.
- Terrain clip prevention: LineTrace downward from camera position; ground + 15cm is the lower bound.
- Control post-process volume parameters via Timeline (vignette, blur, desaturation).

### Transition System
- New character Pawn is pre-spawned and active before the transition.
- On transition, PlayerController Possesses the new Pawn.
- Fade-in: Lerp camera post-process Scene Color Tint from black to white.
- Enemy targeting delay: tag the new Pawn, exclude from AI target filter for 1–2 seconds.

### Headbob
- Add sine wave offset to camera position.
- Bind amplitude/period to movement speed.
- Add noise-based irregular offset in wounded state.

---

## Open Questions

- [ ] Narrative screen Option A vs B — decide after prototype comparison test
- [ ] Narrative text progression — auto vs. manual (button)
- [ ] Exact headbob amplitude/period values — tune through testing
- [ ] Death camera fall speed curve — tune through testing
- [ ] Exact enemy targeting delay duration (1 second? 2?) — tune through testing
- [ ] Exact fade-in/out duration — tune through testing
- [ ] Level of control over what's visible in the final death frame — fully random? slightly guided?
