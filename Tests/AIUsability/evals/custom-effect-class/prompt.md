---
description: Extension point. Tests whether the agent lists the full ITransitionEffect contract and the preset hook-up.
tags: [transitionfx, cpp, extension]
plugins: ["../.."]
max_turns: 15
allowed_tools: [Read, Glob, Grep, Skill]
---

I want to write my own effect class for TransitionFX that renders differently from the built-in PostProcess one. Which interface do I implement, which methods are required, and how do I hook the class into a preset?
