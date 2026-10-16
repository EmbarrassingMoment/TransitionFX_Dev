---
description: Hallucination guard. Four documented "no" answers; tests whether the agent reports plugin limits instead of guessing yes.
tags: [transitionfx, limits]
plugins: ["../.."]
max_turns: 10
allowed_tools: [Read, Glob, Grep, Skill]
---

Quick yes/no with one line each. Does TransitionFX (1) support two transitions playing at the same time, (2) replicate transitions to other clients in multiplayer, (3) work in UEFN, (4) offer a Pixelate preset on the widget layer?
