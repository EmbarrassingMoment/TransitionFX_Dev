# TransitionFX Roadmap

> Planned features for future releases. Priorities may shift based on community feedback. Feature requests are welcome on [GitHub Issues](https://github.com/EmbarrassingMoment/TransitionFX_Dev/issues/new/choose).

---

## Planned

### New Effects
- [ ] New transition effects are planned — specific effects are to be determined based on user feedback and creative exploration

### Feature Extensions
- [ ] **Widget-Layer Variants for the Remaining Effects** `High` — Extend `WidgetTransitionEffect` support beyond the 8 effects shipped in v1.5.0. Pixelate and Slice are excluded because their materials cannot be reproduced on the widget layer
- [ ] **Origin Point Override** `Medium` — Allow center-based transitions (Iris, Diamond, Tiles, etc.) to expand from a custom screen-space coordinate
- [ ] **Simultaneous Transitions** `Low` — Support for layering multiple independent transitions with a multi-slot manager

### Improvements & Optimization
- [ ] **Preset Validation in Editor** `High` — Warn if a preset has no material or is missing the required `Progress` parameter
- [ ] **Editor Preset Thumbnails** `Medium` — Auto-generate static thumbnails for TransitionPreset assets in the Content Browser
- [ ] **Blueprint Preset Picker Widget** `Medium` — A visual dropdown showing available presets with mini-previews
- [ ] **Shader Complexity Tiers** `Low` — Simplified material variants for performance-sensitive platforms

### Documentation & Tutorials
- [ ] **Material Parameter Reference** `High` — Dedicated doc listing every built-in material's adjustable parameters
- [ ] **Video Tutorial: Getting Started** `Medium` — Installation, preset creation, and first transition walkthrough
- [ ] **Video Tutorial: Level Transition Workflow** `Medium` — Demonstrating `OpenLevelWithTransition` and the hold-at-max loading screen pattern
- [ ] **Custom Effect Authoring Guide** `Medium` — Step-by-step guide for creating new SDF materials and wiring them via `ITransitionEffect`
- [ ] **Common-Pattern Blueprint Examples** `Medium` — Pre-configured Blueprint examples for common patterns (pause menu, level select, cutscene transitions)

---

## Shipped

| Item | Released in |
| :--- | :--- |
| **UMG Widget-Layer Transitions** — A full-screen Slate overlay path (`WidgetTransitionEffect`) that also covers UMG/Slate UI, available for 8 effects | v1.5.0 |
| **Configurable Pool Size** — The effect pool cap is exposed as `MaxPoolSizePerEffectClass` under **Project Settings > Plugins > TransitionFX** | v1.4.0 |
| **Transition Color per Preset** — `bOverrideTransitionColor` / `TransitionColor` on presets (e.g., fade-to-white) without call-site overrides | v1.3.0 |
| **Transition Chaining / Sequencing** — A DataAsset-based sequence of presets played back-to-back with optional looping | v1.2.0 |
| **OnTransitionProgress Delegate** — `OnTransitionProgressChanged` broadcasts progress each tick, plus threshold callbacks via `AddProgressThreshold` | v1.1.0 |
| **Sample Project** — Downloadable sample project with the `L_ShowCase` and `L_WidgetLayerSample` levels | [Releases page](https://github.com/EmbarrassingMoment/TransitionFX_Dev/releases) |

See [CHANGELOG.md](../CHANGELOG.md) for the full release history.
