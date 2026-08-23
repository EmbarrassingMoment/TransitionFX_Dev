# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Setup

This is an Unreal Engine 5.5 plugin project. There are no CLI build scripts — all compilation happens through the UE editor or Visual Studio 2022.

**Setup steps:**
1. Right-click `TransitionFX_Dev.uproject` → **Generate Visual Studio project files**
2. Open `TransitionFX_Dev.uproject` in UE 5.5; click **Yes** to rebuild missing modules
3. First launch compiles shaders (several minutes)

**Requirements:** Visual Studio 2022 (C++ Game Development workload), UE 5.5+, Windows with DX12/SM6 GPU.

**Testing:** No automated test suite. Test manually using:
- The `L_ShowCase` level (showcases 27 of the 28 transition effects; `DA_LinearWipe` is currently not referenced by the level)
- The in-editor **Transition Preview Panel** (real-time playback with easing/duration controls)

## Architecture

TransitionFX is a **data-driven, GameInstance-scoped** transition system built on two C++ modules:

### Module Boundaries

| Module | Path | Role |
|--------|------|------|
| `TransitionFX` (Runtime) | `Plugins/TransitionFX/Source/TransitionFX/` | Core state machine, effect rendering, Blueprint API |
| `TransitionFXEditor` (Editor) | `Plugins/TransitionFX/Source/TransitionFXEditor/` | Preview panel, GIF exporter, asset type actions |

### Data Flow

```
Blueprint / C++ call
        ↓
UTransitionBlueprintLibrary  (latent actions — PlayTransitionAndWait, OpenLevelWithTransition)
        ↓
UTransitionManagerSubsystem  (GameInstance subsystem — state machine, pooling, event broadcasting)
        ↓
ITransitionEffect / UPostProcessTransitionEffect  (PostProcess volume + dynamic material instance)
        ↓
Material parameter "Progress" [0.0→1.0] drives SDF shader on screen
```

### Core Classes

| Class | File | Role |
|-------|------|------|
| `UTransitionManagerSubsystem` | `Public/TransitionManagerSubsystem.h` | Central tick-driven state machine; manages effect pool, audio, input blocking, level transitions, and sequences |
| `UTransitionPreset` | `Public/TransitionPreset.h` | Data Asset holding all config: effect class, material, duration, easing, audio, input blocking |
| `ITransitionEffect` | `Public/ITransitionEffect.h` | Interface for effect implementations (`Initialize`, `UpdateProgress`, `Cleanup`, `SetInvert`) |
| `UPostProcessTransitionEffect` | `Public/PostProcessTransitionEffect.h` | Concrete implementation: creates a PostProcess volume + dynamic material instance |
| `UTransitionBlueprintLibrary` | `Public/TransitionBlueprintLibrary.h` | Blueprint-callable latent actions and helper nodes |
| `UTransitionSequence` | `Public/TransitionSequence.h` | Data Asset for chaining multiple transitions; each `FTransitionSequenceEntry` holds preset, mode, duration override, delay |
| `UTransitionSequencePlayer` | `Public/TransitionSequencePlayer.h` | Internal sequence playback engine owned by the subsystem; owns step/loop/delay state and drives `StartTransition` per entry |
| `UTransitionFXConfig` | `Public/TransitionFXConfig.h` | Compile-time constants: material parameter names (`Progress`, `Invert`, `FadeColor`), default asset path |
| `UTransitionFXSettings` | `Public/TransitionFXSettings.h` | `UDeveloperSettings`-based project settings (Project Settings > Plugins > TransitionFX): effect pool cap |

### Effect Pool

`UTransitionManagerSubsystem` maintains a `TMap<UClass*, FTransitionEffectPool>` capped per class by `UTransitionFXSettings::MaxPoolSizePerEffectClass` (default 3, Project Settings > Plugins > TransitionFX). Always return effects to the pool via `CleanupAndPoolCurrentEffect()` — never destroy them directly.

### Easing

Easing is applied in the subsystem's `Tick()` using `ETransitionEasing` (12+ built-in curves + `UCurveFloat` custom). The raw `[0,1]` progress is eased before being written to the material's `Progress` parameter.

### Level Transition Pattern

`OpenLevelWithTransition()` uses a two-step delegate chain:
1. Fade-out completes → `OnLevelTransitionFadeOutFinished()` opens the level
2. `OnPostLoadMapWithWorld()` fires → auto-reverse fade-in plays

`PrepareAutoReverseTransition()` stores state so it survives level unload.

### Sequence System

Sequence playback logic lives in `UTransitionSequencePlayer` (Phase 2 extraction complete), a `UObject` owned by `UTransitionManagerSubsystem` (its Outer) and created lazily on the first `PlaySequence()` call. The subsystem remains the Blueprint-facing API: it validates sequences and forwards `PlaySequence`/`StopSequence` to the player, which drives `StartTransition()` per entry and broadcasts the subsystem's `OnSequenceStepChanged`/`OnSequenceCompleted` delegates. The player's `IsDispatchingStep()` flag prevents external `StartTransition()` calls from interrupting internal sequence steps.

## Key Files

```
Plugins/TransitionFX/
├── Source/TransitionFX/
│   ├── Public/
│   │   ├── TransitionManagerSubsystem.h   ← all public API + delegates
│   │   ├── TransitionPreset.h             ← data asset schema
│   │   ├── ITransitionEffect.h            ← effect interface
│   │   ├── TransitionBlueprintLibrary.h   ← Blueprint nodes
│   │   ├── TransitionSequence.h           ← sequence data asset
│   │   ├── TransitionSequencePlayer.h     ← sequence playback engine
│   │   ├── TransitionFXConfig.h           ← material param name constants
│   │   └── TransitionFXSettings.h         ← project settings (effect pool cap)
│   └── Private/
│       ├── TransitionManagerSubsystem.cpp ← core tick loop + state machine
│       ├── TransitionSequencePlayer.cpp   ← sequence step/loop/delay logic
│       └── TransitionBlueprintLibrary.cpp ← latent action implementations
├── Source/TransitionFXEditor/
│   ├── Public/TransitionPreviewPanel.h    ← editor preview UI
│   └── Private/
│       ├── STransitionPreviewPanel.cpp    ← preview panel UI + playback
│       ├── TransitionPreviewGifCapture.cpp ← single-GIF capture engine (frames/encode/save)
│       ├── TransitionPreviewBatchCaptureDriver.cpp ← dev-tools batch GIF workflows (TRANSITIONFX_DEV_TOOLS)
│       └── GifEncoder.cpp                ← GIF89a encoder for docs
└── Content/
    ├── Data/          ← 29 DA_*.uasset presets (28 effects + DA_FadeToBlack) + Sequence/ sample
    ├── Materials/     ← M_Transition_*.uasset master materials + instances
    └── MaterialFunctions/  ← 9 reusable SDF helper functions
```

## Conventions

- **PCH**: `PCHUsage = UseExplicitOrSharedPCHs` — always include the module's own header explicitly.
- **UPROPERTY Transient**: All runtime state (pool, current effect, audio component) uses `UPROPERTY(Transient)` to avoid serialization.
- **Weak pointers for cache**: `TWeakObjectPtr` for `CachedPlayerController` to handle destruction safely.
- **Category**: All `UFUNCTION`/`UPROPERTY` use `Category = "TransitionFX"` or `"TransitionFX|Sequence"`.
- **No Blueprint-only builds assumption**: The plugin must work for both Blueprint-only and C++ projects; keep all required logic in the Runtime module.

## Platform & Compatibility Notes

- **Win64 only** (DX12 SM6). Console and mobile are untested — SDF effects are GPU-bound.
- **Not compatible with UEFN** (Unreal Editor for Fortnite).
- **PostProcess limitation**: Transitions do not cover UMG/Slate UI layers.
- **Single active transition**: One subsystem per GameInstance; no simultaneous transitions.
- **No multiplayer replication**: Operates locally per client.

## CI/CD

`.github/workflows/release.yml` triggers on version tags (`v*`) to build a distribution ZIP (excluding binaries/intermediates), parse CHANGELOG.md, and publish a GitHub release.
