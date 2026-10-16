---
description: Level change with automatic fade-in. Tests whether the agent finds the single plugin node instead of hand-wiring Open Level plus a manual reverse fade.
tags: [transitionfx, smoke, blueprint]
plugins: ["../.."]
max_turns: 15
allowed_tools: [Read, Glob, Grep, Skill]
---

I'm using the TransitionFX plugin in my UE5 project. From a Blueprint button click I want to fade the screen out, open the level named `Lobby`, and have the screen fade back in automatically once the new level is up. Which TransitionFX node(s) do I use and what do I plug into the pins? Keep it short.
