# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [Unreleased]

### Added

- `UTransitionSequence` data asset for chaining multiple transitions back-to-back.
- `PlaySequence` / `StopSequence` / `IsSequencePlaying` / `GetCurrentSequenceStep` API on `UTransitionManagerSubsystem`.
- `PlaySequenceAndWait` latent Blueprint node.
- `OnSequenceCompleted` and `OnSequenceStepChanged` delegates.

---

## [1.1.0] - 2026-04-09

### Added

**Built-in Effects (+3, total 25)**
- **Sliding Doors** — Two panels slide from opposite sides and meet at the center, like elevator or airlock doors
- **Dissolve** — Classic transition where the screen dissolves like mist or sand using procedural noise
- **Corner Wipe** — A directional wipe that starts from a specified corner and expands diagonally across the entire screen; origin corner (0–3) is dynamically selectable

**Material Functions**
- `MF_StarSDF` — Star signed-distance-field material function for star-shaped effects

**Event System**
- `OnTransitionProgressChanged` — Broadcasts the eased progress value (0.0 to 1.0) each tick while a transition is active
- `OnProgressThresholdReached` — Fires once when the eased progress crosses a registered threshold value

**C++ / Blueprint API**
- `AddProgressThreshold(float Threshold)` — Registers a progress threshold (0.0 to 1.0) for one-shot callback notification
- `ClearProgressThresholds()` — Removes all registered progress thresholds

### Changed

- Optimized FString allocation in `STransitionPreviewPanel` by reserving capacity before PascalCase-to-snake_case conversion loop

### Documentation

- Added UEFN (Unreal Editor for Fortnite) compatibility FAQ to README (EN/JP)
- Updated API Reference (EN/JP) with `OnTransitionProgressChanged`, `OnProgressThresholdReached`, `AddProgressThreshold`, and `ClearProgressThresholds`
- Updated Quick Start guides (EN/JP) with new event system documentation (5 delegates)
- Added effect preview GIFs for Dissolve, Sliding Doors, and Corner Wipe
- Added Corner Wipe to the Built-in Effects table in README (EN/JP)
- Marked "OnTransitionProgress Delegate" as completed in the Roadmap

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
- `InvertTransition` — Inverts the transition mask and replays forward
- `StopTransition`, `ForceClear`, `ReverseTransition`, `SetPlaySpeed`, `ReleaseHold`
- `IsAnyTransitionPlaying`, `IsTransitionPlaying`, `GetCurrentProgress`, `IsCurrentTransitionFinished`
- `PrepareAutoReverseTransition` — Pre-configures auto-reverse for the next level load without starting a transition
- `OpenLevelWithTransition`, `OpenLevelWithTransitionAndWait` — Level transition nodes with auto fade-in
- `ApplyEasing` — Pure math node for easing calculations

**C++ API**
- `StartTransition` with `FTransitionParameters` for runtime material parameter overrides
- `PreloadTransitionPresets` — Synchronous shader warmup to prevent first-frame hitching
- `AsyncLoadTransitionPresets` — Async asset loading with automatic shader warmup and completion callback
- `OpenLevelWithTransition` — Seamless level transitions with auto-reverse fade-in
- `InvertTransition` — Inverts the current transition mask and replays forward
- `ReleaseHold` — Releases held transitions for loading screen workflows
- `PrepareAutoReverseTransition` — Pre-configures auto-reverse for the next level load without starting a transition immediately
- `GetDefaultFadePreset` — Returns the cached default fade preset (`DA_FadeToBlack`)
- Object pooling for transition effect instances (capped at 3 per class)
- `TransitionFX.ForceClear` console command for emergency recovery

**Event System**
- `OnTransitionStarted` — Fired when transition begins
- `OnTransitionCompleted` — Fired when transition finishes
- `OnTransitionHoldStarted` — Fired when transition reaches max and holds

**Editor Module**
- `TransitionFXEditor` module — Editor extension for previewing and exporting transition effects
- `STransitionPreviewPanel` — Real-time preview panel with playback controls (play, pause, loop, reverse, speed adjustment)
- `TransitionPreviewViewport` — Dedicated viewport for effect preview rendering
- `FGifEncoder` — GIF89a encoder with LZW compression and median-cut color quantization
- `AssetTypeActions_TransitionPreset` — Content browser integration with custom icon and color
- `TransitionPresetFactory` — Asset factory for creating new transition presets
- Batch export for all effects and all easing types as GIF files
- Resolution options for capture (480×270, 640×360, 800×450)

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
- FAQ in English and Japanese (`FAQ_EN.md`, `FAQ_JP.md`)
- Contributing guidelines (`CONTRIBUTING.md`)
- Preview Tool Manual (`TransitionFX_PreviewTool_Manual.md`)
