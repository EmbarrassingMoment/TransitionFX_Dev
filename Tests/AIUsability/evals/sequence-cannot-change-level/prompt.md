---
description: Capability limit. Tests whether the agent knows sequences cannot contain level transitions and redirects to OpenLevelWithTransition.
tags: [transitionfx, sequence, limits]
plugins: ["../.."]
max_turns: 10
allowed_tools: [Read, Glob, Grep, Skill]
---

Can a TransitionFX Transition Sequence asset include a level change in the middle (e.g. Fade out → open level → Iris in)? If not, what should I do instead?
