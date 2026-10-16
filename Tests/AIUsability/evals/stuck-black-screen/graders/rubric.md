---
type: llm
---

PASS if the reply offers `ForceClear` (the Blueprint/C++ function and/or the console command `TransitionFX.ForceClear`) as the recovery, and names at least one documented cause: a transition started with `bHoldAtMax` whose `ReleaseHold` was never called, or a transition interrupted while `bAutoBlockInput` was enabled.

FAIL if the only recovery offered is stopping/restarting PIE, if `ForceClear` is not mentioned, or if the reply invents a console command or function other than `ForceClear` / `TransitionFX.ForceClear` / `StopTransition`.
