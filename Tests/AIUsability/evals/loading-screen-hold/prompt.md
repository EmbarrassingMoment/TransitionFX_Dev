---
description: Hold-at-max loading screen. Tests whether the agent uses bHoldAtMax / ReleaseHold rather than a workaround.
tags: [transitionfx, smoke, loading]
plugins: ["../.."]
max_turns: 15
allowed_tools: [Read, Glob, Grep, Skill]
---

With TransitionFX I need the screen to go fully black and *stay* black while I stream in a big level, then finish the transition once loading is done. How do I set that up?
