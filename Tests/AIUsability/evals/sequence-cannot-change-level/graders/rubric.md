---
type: llm
---

PASS if the reply answers "no" (sequences cannot contain a level transition; entries only hold preset, mode, duration override, invert, delay and parameter overrides) and recommends `OpenLevelWithTransition` or `Open Level With Transition And Wait` for the level change, optionally chaining a sequence before or after it.

FAIL if the reply says yes, invents a sequence entry type or field for loading a level, or recommends putting `Open Level` inside the sequence via an event.
