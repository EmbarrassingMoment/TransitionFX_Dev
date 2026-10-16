---
type: llm
---

PASS if the reply names `PreloadTransitionPresets` and/or `AsyncLoadTransitionPresets` on the Transition Manager Subsystem, explains that it warms up (compiles/caches) the transition shaders ahead of time, and recommends calling it once early (GameInstance Init, a startup level's BeginPlay, or a loading phase) with the presets the game will use.

FAIL if the reply does not name either function, attributes the fix to a generic engine setting only (for example "enable shader pre-caching" with no plugin API), or says nothing can be done.
