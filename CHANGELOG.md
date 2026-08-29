# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [Unreleased]

### Added

**Built-in Effects (+2, total 30)**
- **Stripe Cascade** — The screen is divided into stripes that wipe closed one after another with a staggered delay, cascading across the screen. Stripe count, direction (4-way), stagger delay, and edge softness are adjustable (`SplitCount`, `Direction`, `StripeDelay`, `Smoothness`). Adds the `DA_StripeCascade` preset, `M_Transition_StripeCascade` master material, `MI_Transition_StripeCascade` instance, the `MF_StripePos` material function, and README (EN/JA) Built-in Effects table entries.
- **Box Roll** — Boxes roll in from the right edge and stack up row by row until they cover the screen. Row count, roll speed, and per-column/per-row stagger are adjustable (`Rows`, `Speed`, `DTCol`, `DTRow`). Anti-aliasing is analytic (1.5 px from `ViewSize`), and the whole animation is driven by the standard `Progress` parameter. Adds the `DA_BoxRoll` preset, `M_Transition_BoxRoll` master material, `MI_Transition_BoxRoll` instance, the `MF_Rotate2D` / `MF_SdBox2D` / `MF_BoxDist` / `MF_RowDist` material functions, a showcase level entry, preview GIF, and README (EN/JA) Built-in Effects table entries.

- **Project Settings page (`UTransitionFXSettings`)** — New developer settings class exposing plugin configuration under **Project Settings > Plugins > TransitionFX** (saved to `DefaultGame.ini`). This page is the future home for additional plugin-wide options.
- **Configurable effect pool size** — The per-effect-class pool cap (previously hardcoded at 3) is now exposed as `MaxPoolSizePerEffectClass` in the settings page. Set to 0 to disable pooling entirely.

### Changed

- **Preview panel batch capture extracted into `FTransitionPreviewBatchCaptureDriver`** — The dev-tools batch GIF workflows (all-effects batch and per-easing batch, `TRANSITIONFX_DEV_TOOLS`) moved out of `STransitionPreviewPanel` into a dedicated driver that steers the panel through callbacks and advances on the capture engine's `OnWriteFinished`. The panel drops to ~725 lines and no longer contains any batch state. Editor-only, no behavior change.
- **Preview panel GIF capture extracted into `FTransitionPreviewGifCapture`** — The editor preview panel's single-GIF capture pipeline (fixed-step frame grabbing, save-path resolution, GIF encoding, notifications) moved out of `STransitionPreviewPanel` into a dedicated capture engine. The panel now decides the destination up front (batch modes pass an explicit path; interactive capture gets the save dialog) and the batch drivers advance via an `OnWriteFinished` delegate instead of being called from inside the capture finalizer. Editor-only, no behavior change.
- **Sequence playback extracted into `UTransitionSequencePlayer`** — The Phase 2 refactor planned since v1.2.0: sequence step/loop/delay logic moved out of `UTransitionManagerSubsystem` into a dedicated internal `UTransitionSequencePlayer` object (created lazily on first `PlaySequence` call). No public API change — `PlaySequence`, `StopSequence`, `IsSequencePlaying`, `GetCurrentSequenceStep`, and the sequence delegates remain on the subsystem and behave identically.
- Sample project build settings updated for Unreal Engine 5.8. `TransitionFX_Dev.Target.cs` and `TransitionFX_DevEditor.Target.cs` now use `DefaultBuildSettings = BuildSettingsVersion.V7` and `IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8`, clearing UBT's backward-compatibility `[Upgrade]` notices. The new defaults (strict MSVC inline conformance, `Precise` floating-point semantics for Editor targets, and undefined-identifier / return-type / dangling-reference / unreachable-code diagnostics promoted to errors) are accepted without overrides — both plugin modules rebuild warning-free under them. Note that these UBT values require Unreal Engine 5.7 or newer; the `TransitionFX` plugin itself is unaffected and still targets 5.5+. The sample project's `EngineAssociation` is now `5.8` accordingly, so project file generation and builds resolve to a UE 5.8 install (older engines' UBT cannot parse the V7 target settings).

### Fixed

- Fixed a misspelled function output on `MF_CorrectAspectRatio` and `MF_GetMaxScreenRadius`: `AspectRaito` is now `AspectRatio`. The 15 master materials that consume this output connect by pin, so they are unaffected; only the pin label shown in the material editor changes.
- Fixed a misspelled scalar parameter on `M_Transition_Pixelate`: `MaxResoluution` is now `MaxResolution`. The shipped `MI_Transition_Pixelate` instance and `DA_Pixelate` preset never overrode this parameter, so the built-in effect is unaffected. If your project overrides it by name — in a custom material instance or via `FTransitionParameters` — update the name to `MaxResolution`.
- Fixed a potential freeze / stack overflow when playing a looping sequence whose entries all have a null preset (e.g. a freshly created `UTransitionSequence` with `bLoop` enabled and presets not yet assigned). Null-preset entries are now skipped iteratively instead of recursively, and a sequence in which no entry is playable finishes immediately with a warning (broadcasting `OnSequenceCompleted`) instead of cycling forever. Also guarded sequence playback against the `Entries` array being emptied at runtime while a step delay is pending.

## [1.3.0] - 2026-07-19

### Added

**Built-in Effects (+2, total 28)**
- **Fan** — A pinwheel-style wipe where angular blades sweep out from the center and rotate to cover the screen, like an opening folding fan. Adds the `DA_Fan` preset, `M_Transition_Fan` master material, `MI_Transition_Fan` instance, preview GIF, and README (EN/JA) Built-in Effects table entry.
- **Slice** — The screen is cut into strips that slide off-screen in alternating directions with staggered timing. Slice count, direction, softness, and stagger are adjustable. Adds the `DA_Slice` preset, `M_Transition_Slice` master material, `MI_Transition_Slice` instance, preview GIF, and README (EN/JA) Built-in Effects table entry.

- **Transition Color per Preset** — `UTransitionPreset` now exposes a `bOverrideTransitionColor` toggle and a `TransitionColor` property. When enabled, the color is applied to the material's color parameter at the start of the transition (e.g., fade-to-white) without requiring a parameter override at the call site. An explicit color supplied via `FTransitionParameters` at the call site still takes precedence.
- `DA_SequenceSamples` — Sample `UTransitionSequence` data asset demonstrating the sequence system out of the box (`Content/Data/Sequence/`).

### Changed

- Changed the project's default map to an empty map to speed up editor and packaged startup.
- `SetParameters` now logs a warning (`LogTransitionFX`) when an override targets a scalar, vector, or texture parameter that the material does not expose, instead of silently ignoring it. This surfaces cases such as a color override having no visible effect because the material lacks that parameter.
- Bumped plugin `VersionName` in `TransitionFX.uplugin` to match the release version.
- Release ZIP no longer includes local development tooling configuration (`.claude/`).

### Fixed

- Fixed the transition color override having no effect. The material color parameter is named `FadeColor`, but `ColorParamName` was set to `Color`, so `SetVectorParameterValue` was silently ignored. This also affected `QuickFadeToBlack`/`QuickFadeFromBlack` and the new per-preset `TransitionColor`. Updated the constant and the documentation (FAQ, Quick Start, API Reference) to use `FadeColor`.
- Added a dedicated factory and asset type actions for `UTransitionSequence`. The asset now appears as a top-level "Transition Sequence" entry in the Content Browser right-click menu, matching the existing `Transition Preset` workflow. Previously, sequences could only be created via the generic `Miscellaneous → Data Asset` class picker.

---

## [1.2.0] - 2026-05-03

### Added

**Built-in Effects (+1, total 26)**
- **Diamond Band Wipe** — A dynamic transition where a diamond-shaped band expands from the center, splitting apart to reveal the underlying scene.

**Sequence System**
- `UTransitionSequence` data asset for chaining multiple transitions back-to-back.
- `PlaySequence` / `StopSequence` / `IsSequencePlaying` / `GetCurrentSequenceStep` API on `UTransitionManagerSubsystem`.
- `PlaySequenceAndWait` latent Blueprint node.
- `OnSequenceCompleted` and `OnSequenceStepChanged` delegates.

### Fixed

- Fixed a one-frame background flash between sequence steps caused by the previous step's PostProcessVolume being destroyed before the next step's effect was initialized. During a sequence, the effect now hot-swaps across steps and is only torn down when the full sequence completes.
- Fixed an issue where an incorrect material instance was set for DA_FadeToBlack.
- Refactoring to a more efficient node configuration for M_Transition_Diamond.

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
