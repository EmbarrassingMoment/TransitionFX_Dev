---
type: llm
---

PASS if the reply says that Reverse reverses the playback direction (progress runs from 1 to 0, "Fade In (1 to 0)") while bInvert flips which area of the screen the mask covers (the shape reveals instead of covers) with progress still running 0 to 1, and notes that Forward + bInvert therefore gives a fade-in look by a different mechanism (for example the shape opens outward instead of the closing animation being played backwards).

FAIL if the reply says the two are identical or interchangeable in all cases, describes bInvert as reversing time or playback, or describes Reverse as inverting the mask.
