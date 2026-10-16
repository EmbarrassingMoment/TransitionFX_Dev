---
type: llm
---

PASS if the reply tells the user to switch to a widget-layer preset (a `DA_Widget_*` asset such as `DA_Widget_Iris`) or to set the preset's `EffectClass` to `WidgetTransitionEffect` with an `MI_Widget_*` material, and mentions `WidgetZOrder` as the knob to raise if the UI sits at a higher Z-order.

FAIL if the reply claims the PostProcess effect can be made to cover UMG by raising `Priority` or a PostProcess setting, if hiding or collapsing the widgets is presented as the only or primary fix, if it says the plugin cannot cover UI at all, or if it recommends a Pixelate or Slice widget-layer preset (those do not exist).
