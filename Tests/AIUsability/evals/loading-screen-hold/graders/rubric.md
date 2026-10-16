---
type: llm
---

PASS if the reply says to start the transition with `bHoldAtMax` set to true (via `StartTransition` or the Blueprint node), do the loading while it holds at full coverage, and call `ReleaseHold` when loading finishes. Mentioning `OnTransitionHoldStarted` as the moment to begin loading is a plus but not required.

FAIL if the primary suggestion is a Transition Sequence asset (hold is unsupported there), `SetPlaySpeed(0)` or pausing the game to freeze the screen, or if `ReleaseHold` is never mentioned.
