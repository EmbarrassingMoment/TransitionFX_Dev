---
type: llm
---

PASS if all four questions are answered "No": (1) only one transition plays at a time per GameInstance and a new one replaces the current, (2) the plugin runs locally per client with no replication, (3) it is not compatible with UEFN, (4) Pixelate (and Slice) have no widget-layer variant.

FAIL if any of the four is answered "Yes", "partially", or "yes with setup", or if the reply refuses to answer one of them.
