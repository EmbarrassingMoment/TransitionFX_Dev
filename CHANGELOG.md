# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [1.0.0] - 2026-02-18

### Added

**Core System**
- `UTransitionManagerSubsystem` — GameInstance subsystem managing the full transition lifecycle
- `UTransitionPreset` — Data asset for configuring transition effects (duration, easing, audio, material, input blocking)
- `ITransitionEffect` — Interface for implementing custom transition effects
- `UPostProcessTransitionEffect` — Concrete post-process volume based transition effect
- `UTransitionBlueprintLibrary` — Blueprint function library with latent action support

**Built-in Transition Effects (22 effects)**
- Fade, Iris, Flower Iris, Diamond, Box
- Linear Wipe, Split, Wavy Curtain, Radial Wipe, Cross Wipe
- Tiles, Polka Dots, Blinds, Spiral, Random Tiles, Wind
- Texture Mask, TV Switch Off, Hexagon, Triangle, Checkerboard, Pixelate

**Easing System**
- Linear, Sine (In/Out/InOut), Cubic (In/Out/InOut), Expo (In/Out/InOut)
- Elastic Out, Bounce Out
- Custom FloatCurve support

**Blueprint API**
- `PlayTransitionAndWait` — Latent action node (Forward / Reverse exec pins)
- `PlayRandomTransitionAndWait` — Randomly selects a preset from an array
- `PlayTransitionAndWaitWithDuration` — Duration override at call site
- `QuickFadeToBlack` / `QuickFadeFromBlack` — Convenience single-node fades
- `StopTransition`, `ForceClear`, `ReverseTransition`, `SetPlaySpeed`, `ReleaseHold`
- `IsAnyTransitionPlaying`, `IsTransitionPlaying`, `GetCurrentProgress`, `IsCurrentTransitionFinished`
- `OpenLevelWithTransition`, `OpenLevelWithTransitionAndWait` — Level transition nodes with auto fade-in
- `ApplyEasing` — Pure math node for easing calculations

**C++ API**
- `StartTransition` with `FTransitionParameters` for runtime material parameter overrides
- `PreloadTransitionPresets` — Synchronous shader warmup to prevent first-frame hitching
- `AsyncLoadTransitionPresets` — Async asset loading with automatic shader warmup and completion callback
- `OpenLevelWithTransition` — Seamless level transitions with auto-reverse fade-in
- `ReleaseHold` — Releases held transitions for loading screen workflows
- Object pooling for transition effect instances (capped at 3 per class)
- `TransitionFX.ForceClear` console command for emergency recovery

**Event System**
- `OnTransitionStarted` — Fired when transition begins
- `OnTransitionCompleted` — Fired when transition finishes
- `OnTransitionHoldStarted` — Fired when transition reaches max and holds

**Other Features**
- Auto input blocking via `SetCinematicMode` during transitions
- Pause-aware ticking (`bTickWhenPaused`)
- Hold-at-max support for loading screen workflows
- Audio SFX synchronization with volume and pitch control
- Aspect ratio correction for all SDF-based effects
- Soft object reference support for on-demand asset loading

**Documentation**
- README in English and Japanese
- API Reference in English and Japanese (`API_Reference_EN.md`, `API_Reference_JP.md`)
- Quick Start Guide in English and Japanese (`QUICKSTART_EN.md`, `QUICKSTART_JP.md`)
- Performance rationale document (`PERFORMANCE_RATIONALE.md`)
