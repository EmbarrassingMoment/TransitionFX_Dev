# TransitionFX FAQ

Frequently asked questions about the TransitionFX plugin.

---

## Table of Contents

- [Troubleshooting](#troubleshooting)
- [Specifications & Behavior](#specifications--behavior)
- [Advanced Usage & Best Practices](#advanced-usage--best-practices)

---

## Troubleshooting

### Q: The transition doesn't appear at all. What could be the cause?

Check the following in order:

1. **Preset is not set:** Ensure the `Preset` pin of the node is connected. If `nullptr` is passed, an error is logged and `ForceClear` is called internally.
2. **Material is not assigned:** Open your `TransitionPreset` data asset and confirm that `Transition Material` is set. If the material is missing, the transition will not render.
3. **`Progress` parameter is missing from the material:** All transition materials require a scalar parameter named `Progress`. If it does not exist, the effect will not animate (a warning is output to the log).
4. **Effect Class is not set:** Confirm that `Effect Class` is set to `PostProcessTransitionEffect` (or your custom effect class).
5. **Post Process settings conflict:** If your project already uses a Post Process Volume with extreme settings, it may visually interfere. Check the `Priority` value in the preset (default: 1000).

### Q: The transition freezes when the game is paused.

Set `bTickWhenPaused` to `true` in your `TransitionPreset`. By default, this is `false`, meaning the transition stops ticking when the game is paused. If you use transitions on a pause menu, you must enable this flag.

### Q: I get an error about a missing Material Instance.

This typically occurs when:
- The material asset referenced by the preset has been **moved or renamed** in the Content Browser. Re-assign the material in the preset.
- The plugin content has not been properly copied to the project. Ensure the `Plugins/TransitionFX/Content/Materials/` directory exists and contains the `M_Transition_*.uasset` files.
- If using the `TextureMask` effect, confirm that your mask texture has **sRGB unchecked** and Compression set to **Masks (no sRGB)** or **Grayscale**.

---

## Specifications & Behavior

### Q: How does auto input blocking work during transitions?

When `bAutoBlockInput` is `true` in the preset (the default), the subsystem calls `SetCinematicMode(true)` on the player controller when the transition starts, which disables player input and hides the cursor. When the transition completes (or `ForceClear` is called), cinematic mode is released. This means:
- Input is blocked automatically — no additional Blueprint logic is needed.
- If the transition is interrupted via `StopTransition`, input is also unblocked.
- If the transition is interrupted via `ForceClear`, input is forcefully unblocked regardless of settings.

### Q: Does TransitionFX work with ServerTravel / seamless travel?

TransitionFX runs as a **GameInstance Subsystem**, so the subsystem itself survives level transitions including `ServerTravel`. However, the automatic fade-in mechanism (`PostLoadMapWithWorld`) is designed for `OpenLevel`-based transitions. For `ServerTravel`, you will need to manually trigger the fade-in on the client side after the travel completes. Bind to `OnPostLoadMapWithWorld` or use a game-specific callback to call `StartTransition` in Reverse mode.

### Q: How does the object pooling system work?

The subsystem maintains an internal pool of transition effect instances (`TMap<UClass*, FTransitionEffectPool>`). When a transition ends, the effect object is returned to the pool instead of being destroyed. The next time the same effect class is requested, a pooled instance is reused.

- **Pool cap:** Maximum **3 instances per effect class**.
- **Excess handling:** Instances beyond the cap are dereferenced and handled by Unreal's Garbage Collector.
- **No user configuration required** — pooling is fully automatic.

### Q: Can I change volume and pitch dynamically during playback?

No. The `SoundVolume` and `SoundPitch` values in the `TransitionPreset` are applied at the moment the sound starts playing. There is no API to change them while a transition is in progress. If you need dynamic audio control, manage your audio separately using the `OnTransitionStarted` delegate to trigger your own audio logic.

### Q: Why does TransitionFX use a GameInstance Subsystem instead of a PlayerController component?

GameInstance Subsystems persist across level transitions. This is essential because a typical transition sequence — Fade Out, Open Level, Fade In — spans two different levels. A PlayerController component would be destroyed when the old level unloads, losing the transition state. The GameInstance Subsystem maintains all state (pending preset, auto-reverse flag, etc.) seamlessly across the level boundary.

### Q: What modules does TransitionFX depend on?

The runtime module (`TransitionFX`) depends on:
- `Core`, `CoreUObject`, `Engine`, `InputCore` — Standard engine modules
- `UMG`, `Slate`, `SlateCore` — UI framework (for input blocking via CinematicMode)
- `RenderCore` — Rendering utilities (for material/post-process operations)
- `DeveloperSettings` — Configuration support

The editor module (`TransitionFXEditor`) additionally depends on `UnrealEd`, `EditorStyle`, and related editor modules.

No third-party dependencies are required.

### Q: What is the Latent Action pattern?

Latent Actions are a UE Blueprint feature that allows a node to **pause execution** until an asynchronous operation completes. In TransitionFX, `Play Transition And Wait` is a Latent Action — it starts the transition and holds the execution pin until the transition finishes, then fires the `Completed` pin. This eliminates the need for manual timer checks, polling, or callback binding for basic use cases.

### Q: Can I use TransitionFX in a commercial project?

Yes. TransitionFX is released under the **MIT License**, which permits commercial use, modification, and distribution with no restrictions beyond including the license notice. See the `LICENSE` file in the repository root.

---

## Advanced Usage & Best Practices

### Q: How do I override material parameters at runtime for each effect?

Use the `Override Params` pin on `Play Transition And Wait` (or `StartTransition`). Create an `FTransitionParameters` struct and populate its maps:

**Common parameter names by effect:**

| Effect | Parameter Name | Type | Description |
| :--- | :--- | :--- | :--- |
| All | `Color` | Vector (LinearColor) | Transition color (default: black) |
| Linear Wipe | `Angle` | Scalar (float) | Wipe angle in degrees |
| Split | `Angle` | Scalar (float) | Split direction angle |
| Tiles / Polka Dots / Blinds / Checkerboard | `TileCount` | Scalar (float) | Number of tiles / dots / stripes |
| Spiral | `Spin` | Scalar (float) | Rotation intensity |
| Flower Iris | `PetalCount` | Scalar (float) | Number of petals |
| Random Tiles | `Seed` | Scalar (float) | Random seed value |
| Wavy Curtain | `WaveFrequency` | Scalar (float) | Wave frequency |
| Texture Mask | `MaskTexture` | Texture | Custom grayscale mask |

> **Note:** Parameter names depend on the material implementation. Open the master material in the Material Editor to see the exact parameter names available.

### Q: Can I set different sounds for each transition?

Yes. Each `TransitionPreset` data asset has its own `Transition Sound`, `Sound Volume`, and `Sound Pitch` properties. Simply assign different sound assets to different presets. When a transition plays, it uses the sound configured in the active preset.

### Q: How do I play a random transition from a list?

Use the `Play Random Transition And Wait` node. Pass an array of `TransitionPreset` references to the `Presets` pin. The node will randomly select one and play it.

```
Play Random Transition And Wait
├─ Presets   : [DA_Iris, DA_Diamond, DA_Spiral, DA_Hexagon]
├─ Mode      : Forward
└─ Play Speed: 1.0
```

If you need weighted randomness or to avoid repeating the last effect, manage the selection in your own Blueprint logic and use `Play Transition And Wait` with the chosen preset.

### Q: How do I change playback speed dynamically?

Call `SetPlaySpeed` on the `TransitionManagerSubsystem` at any time during a transition. The value is clamped to a minimum of 0.01.

```
Get Game Instance Subsystem (TransitionManagerSubsystem)
    └─ Set Play Speed
        └─ Play Speed: 2.0  (double speed)
```

Use cases:
- Speed up transitions when the player presses a "skip" button.
- Slow down a transition for a dramatic reveal.
- Synchronize transition speed with gameplay events.
