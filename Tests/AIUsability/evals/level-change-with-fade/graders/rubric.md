---
type: llm
---

PASS if the reply recommends the plugin's `Open Level With Transition` node (or its `Open Level With Transition And Wait` variant) as the primary solution, names its inputs (level name, transition preset, duration), and states or clearly implies that the fade-in on the new level happens automatically so the user does not wire a second reverse transition in the new level.

FAIL if the primary recommendation is the engine's plain `Open Level` node plus a separately wired `Play Transition And Wait` in Reverse mode on the new level's BeginPlay, if the reply invents a node that does not exist in the plugin (for example "Fade Out And Load Level", "Transition To Level", "Open Level With Fade"), or if it says the plugin cannot do this.
