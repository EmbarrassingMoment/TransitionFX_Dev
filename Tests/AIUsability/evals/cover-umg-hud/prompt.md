---
description: Widget-layer coverage. Tests whether the agent knows PostProcess cannot cover UMG and points to the DA_Widget_* / WidgetTransitionEffect path.
tags: [transitionfx, smoke, widget-layer]
plugins: ["../.."]
max_turns: 15
allowed_tools: [Read, Glob, Grep, Skill]
---

TransitionFX: my iris transition plays underneath my UMG HUD and pause menu, so the UI stays visible while the screen closes. How do I get the transition to cover the UI too?
