---
description: Shader warmup. Tests whether the agent names the plugin's preload API and places the call at startup.
tags: [transitionfx, smoke, performance]
plugins: ["../.."]
max_turns: 10
allowed_tools: [Read, Glob, Grep, Skill]
---

The first time any TransitionFX transition plays in my packaged game there's a visible hitch; after that it's smooth. What does the plugin offer to fix this, and where should I call it?
