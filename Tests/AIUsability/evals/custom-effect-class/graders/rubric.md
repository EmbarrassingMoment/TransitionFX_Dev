---
type: llm
---

PASS if the reply says to implement `ITransitionEffect` on a `UObject`-derived class, lists all five pure-virtual methods (`Initialize`, `UpdateProgress`, `Cleanup`, `SetInvert`, `SetParameters`), and says to set the preset's `EffectClass` to the new class (noting that the property requires a class implementing the interface).

FAIL if more than one of the five methods is missing, if methods that do not exist on the interface are invented (for example `Tick`, `OnStart`, `Render`), or if the hook-up is described as subclassing `UTransitionPreset` or `UTransitionManagerSubsystem` instead of setting `EffectClass`.
