# TransitionFX — Guide for AI Coding Agents

This file is for AI coding agents (Claude Code, Codex, Cursor, Copilot, Gemini CLI, etc.) helping a developer **use** the TransitionFX plugin in their own Unreal Engine project. It describes the public API, the bundled assets, and the rules to follow. It is not a guide for modifying the plugin itself.

- Plugin version: see `VersionName` in `TransitionFX.uplugin`
- Engine: Unreal Engine 5.5 or later, **Win64 only** (DX12 / SM6)
- Works in Blueprint-only and C++ projects; no C++ is needed for standard use
- Full docs: https://github.com/EmbarrassingMoment/TransitionFX (Quick Start, API Reference, FAQ, in English and Japanese)

## What the plugin does

TransitionFX plays procedural, texture-free screen transitions (fade, iris, wipe, dissolve, pixelate, ...) driven by a single material parameter `Progress` (0.0 to 1.0). Everything is data-driven: a `TransitionPreset` Data Asset holds the effect class, material, duration, easing, input blocking, and audio. A GameInstance subsystem (`TransitionManagerSubsystem`) owns the state machine and is reachable from any Blueprint or C++ code.

```
Blueprint / C++ call
  → UTransitionBlueprintLibrary   (latent nodes: Play Transition And Wait, Open Level With Transition, ...)
  → UTransitionManagerSubsystem   (GameInstance subsystem: state machine, delegates, sequences)
  → ITransitionEffect
      ├ UPostProcessTransitionEffect  PostProcess volume + dynamic material. Drawn BELOW UMG/Slate UI.
      └ UWidgetTransitionEffect       Full-viewport Slate overlay. Drawn ABOVE UMG/Slate UI.
  → material parameter "Progress" (eased 0→1) drives the SDF shader
```

## Rules for agents

1. **Prefer the bundled presets.** `/TransitionFX/Data/DA_*` cover 30 effects. Do not create new materials unless the developer asks for a custom effect.
2. **Never edit assets under this plugin folder** (`Content/`). To change a preset, duplicate it into the developer's project `Content/` and edit the copy. Plugin assets are overwritten on update.
3. **Pick the effect layer deliberately.** If the transition must cover UMG/Slate widgets (HUD, menus), use a `DA_Widget_*` preset (`WidgetTransitionEffect`). Otherwise use the plain `DA_*` preset (`PostProcessTransitionEffect`). The Blueprint API is identical.
4. **Fade Out and Fade In are the same preset** with `Mode` set to `Forward` or `Reverse`. A completed Forward transition keeps covering the screen until a Reverse transition or `Stop Transition` runs. This is intentional.
5. **Only one transition plays at a time.** Starting a new one replaces the current one. There is no replication; everything is client-local.
6. **Custom materials must expose a scalar parameter named `Progress`.** Optional: `Invert` (scalar) and `FadeColor` (vector). Without `Progress` the transition logs an error and does not animate.
7. **Level transitions:** use `Open Level With Transition` instead of `Open Level` + manual fades. The fade-in on the new level is automatic.
8. **Loading screens:** use `StartTransition` with `bHoldAtMax = true`, do the work, then `ReleaseHold` and play `Reverse`. Forgetting `ReleaseHold` leaves the screen black forever (recover with console `TransitionFX.ForceClear`).
9. **Do not assume the plugin is in the project folder.** When installed from Fab it may live under the engine's `Plugins/Marketplace` folder. Check that `TransitionFX` is enabled in the project's `.uproject` or the Plugins window before debugging missing nodes.
10. **Do not delete unused bundled presets or materials**, even to shrink the build. Plugin assets that nothing references are not cooked, so deleting them saves nothing. `DA_FadeToBlack` and `M_Transition_Fade` are loaded by path from C++ (the default preset for Quick Fade, and for Open Level With Transition without a preset), so the Content Browser's reference check reports them as unused; deleting them breaks those nodes.

## Blueprint nodes (`UTransitionBlueprintLibrary`, category "TransitionFX")

| Node | Behavior |
| :--- | :--- |
| **Play Transition And Wait** `(Preset, Mode, PlaySpeed, bInvert, OverrideParams)` | Plays a preset; `Completed` fires when it finishes. The most common node. |
| **Play Transition And Wait With Duration** `(Preset, Mode, Duration, bInvert, OverrideParams)` | Same, but takes seconds instead of a speed multiplier. |
| **Play Random Transition And Wait** `(Presets[], Mode, PlaySpeed, bInvert, OverrideParams)` | Picks one preset from an array. |
| **Play Sequence And Wait** `(Sequence)` | Plays a `TransitionSequence` asset including all loops. |
| **Open Level With Transition** `(LevelName, Preset, Duration)` | Fade out, open level, auto fade-in on the new level. Fire-and-forget. |
| **Open Level With Transition And Wait** `(LevelName, Preset, Duration)` | Latent version. `Completed` fires right after fade-out finishes and `OpenLevel` is called, **not** after the new level has loaded. |
| **Quick Fade To Black / Quick Fade From Black** `(Duration)` | Fire-and-forget black fade using the bundled `DA_FadeToBlack`. Does not wait. |
| **Is Any Transition Playing** | True while a transition is playing or holding. |
| **Apply Easing** `(Alpha, EasingType, CustomCurve)` | Pure math helper. |

`Mode` is `ETransitionMode`: `Forward` (0→1, screen gets covered) or `Reverse` (1→0, screen is revealed). `bInvert` flips the spatial mask (e.g. iris opens from the edges instead of closing to the center) and is independent of `Mode`.

## Subsystem API (`UTransitionManagerSubsystem`, GameInstance subsystem)

In Blueprint: `Get Game Instance Subsystem → TransitionManagerSubsystem`. In C++ (`UTransitionBlueprintLibrary::GetTransitionManager(WorldContext)` is a C++-only helper):

```cpp
UTransitionManagerSubsystem* TFX = GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>();

TFX->StartTransition(Preset, ETransitionMode::Forward, /*PlaySpeed*/1.f, /*bInvert*/false, /*bHoldAtMax*/false, /*OverrideParams*/{});
TFX->ReleaseHold();                 // let a held transition complete (screen stays covered)
TFX->ReverseTransition(/*bAutoStop*/true);   // flip direction, also from a hold or a completed fade-out
TFX->InvertTransition(/*bAutoComplete*/true);
TFX->StopTransition();              // stop, or remove the effect a completed fade-out left on screen
TFX->SetPlaySpeed(2.f);
TFX->ForceClear();                  // emergency reset: destroys effect, unblocks input, no OnTransitionCompleted
TFX->GetCurrentProgress();          // 0..1, before easing
TFX->IsTransitionPlaying();         // playing or holding
TFX->IsCurrentTransitionFinished();
TFX->AddProgressThreshold(0.5f);    // fires OnProgressThresholdReached once per transition
TFX->ClearProgressThresholds();
TFX->PreloadTransitionPresets({Preset1, Preset2});                    // sync shader warmup (call in GameInstance::Init)
TFX->AsyncLoadTransitionPresets(SoftPresets, OnCompleteDynamicDelegate); // async warmup for soft refs
TFX->PlaySequence(Sequence); TFX->StopSequence(); TFX->IsSequencePlaying(); TFX->GetCurrentSequenceStep();
```

Delegates (all `DECLARE_DYNAMIC_MULTICAST_DELEGATE`; bind with `AddDynamic` / "Bind Event to ..." before the transition starts):

| Delegate | Fires |
| :--- | :--- |
| `OnTransitionStarted` | right after `StartTransition` begins |
| `OnTransitionCompleted` | progress reaches 1.0 (Forward) or 0.0 (Reverse). Fires for both directions. Not fired by `StopTransition` / `ForceClear`. |
| `OnTransitionHoldStarted` | `bHoldAtMax` transition reached 1.0 and is holding |
| `OnTransitionProgressChanged(float)` | every tick, eased progress |
| `OnProgressThresholdReached(float)` | once per registered threshold per transition |
| `OnSequenceStepChanged(int32)` / `OnSequenceCompleted` | sequence playback |

Unbind delegates bound in Level Blueprints on `EndPlay`, or bind in the GameInstance instead, to avoid duplicate bindings on level reload.

## `TransitionPreset` Data Asset (create via right-click → Miscellaneous → Transition Preset)

| Property | Notes |
| :--- | :--- |
| `EffectClass` | `PostProcessTransitionEffect` (default) or `WidgetTransitionEffect` (covers UI) |
| `TransitionMaterial` | `MI_Transition_*` for PostProcess, matching `MI_Widget_*` for the widget layer |
| `DefaultDuration` | seconds; `PlaySpeed` on the node multiplies it |
| `EasingType` / `ProgressCurve` | `ETransitionEasing`: Linear, EaseIn/Out/InOut Sine, Cubic, Expo, EaseOutElastic, EaseOutBounce, Custom (uses `ProgressCurve`, a `CurveFloat`) |
| `bOverrideTransitionColor` / `TransitionColor` | per-preset `FadeColor` (e.g. fade to white). A per-call `FadeColor` in `OverrideParams` wins. |
| `bAutoBlockInput` | blocks player input during the transition (recommended `true`) |
| `bTickWhenPaused` | set `true` for pause menus |
| `Priority` | PostProcess blend priority (default 1000); ignored on the widget layer |
| `WidgetZOrder` | overlay Z-order (default 10000); raise if the project's UI is higher |
| `TransitionSound` / `SoundVolume` / `SoundPitch` | optional one-shot audio, stops if the transition is cancelled |

`FTransitionParameters` (`OverrideParams` pin) has `ScalarParams`, `VectorParams`, `TextureParams` maps keyed by material parameter name. Use it for `FadeColor`, effect-specific scalars (angle, count, softness, etc.; open the `MI_*` asset to see names), and `MaskTexture` for Texture Mask.

## Bundled assets (`/TransitionFX/...` content path)

**PostProcess presets** (`Data/DA_*`, materials `Materials/Instances/MI_Transition_*`):
Blinds, Box, BoxRoll, CheckerBoard, CornerWipe, CrossWipe, Diamond, DiamondWipe, Dissolve, Fade, FadeToBlack, Fan, FlowerIris, Hexagon, Iris, LinearWipe, Pixelate, PolkaDots, RadialWipe, RandomTiles, Slice, SlidingDoor, Spiral, Split, StripeCascade, TVSwitchOff, TextureMask, Tiles, Triangle, WavyCurtain, Wind.

**Widget-layer presets** (`Data/DA_Widget_*`, materials `Materials/Widget/Instances/MI_Widget_*`):
Blinds, CheckerBoard, CornerWipe, Diamond, Dissolve, Fade, FadeToBlack, Hexagon, Iris, LinearWipe, PolkaDots, RadialWipe, RandomTiles, SlidingDoor, TextureMask.
Pixelate and Slice have **no** widget-layer variant by design.

**Sequence sample:** `Data/Sequence/DA_SequenceSamples`.

`DA_FadeToBlack` is loaded automatically by the subsystem and used by the Quick Fade nodes.

Texture Mask: import the mask as grayscale with **sRGB off** (Compression: Masks or Grayscale). Black = transition starts, White = ends. Pass it via `OverrideParams.TextureParams["MaskTexture"]`.

## Common patterns

**Fade out → do work → fade in**
```
Play Transition And Wait (DA_FadeToBlack, Forward)
  → [spawn actors / switch UI]
  → Play Transition And Wait (DA_FadeToBlack, Reverse)
```

**Loading screen (hold)**
```
StartTransition(Preset, Forward, 1.0, false, bHoldAtMax=true)
  → OnTransitionHoldStarted → [load / stream]
  → ReleaseHold()
  → Play Transition And Wait (Preset, Reverse)
```
Or call `ReverseTransition()` directly while holding.

**Level change**
```
Open Level With Transition (LevelName, DA_FadeToBlack, 1.0)
```

**Shader warmup (avoid first-play hitch)**: call `PreloadTransitionPresets` with the presets used early in the game from `GameInstance::Init` or the first level's `BeginPlay`. Use `AsyncLoadTransitionPresets` for soft references.

## Sequences (`TransitionSequence` Data Asset)

Entries hold a preset, mode, optional duration override, and delay. Played by `Play Sequence And Wait` or `PlaySequence`. Limits: no level transitions inside a sequence, no `bHoldAtMax`, one sequence at a time, and `StartTransition` / `OpenLevelWithTransition` during playback cancels it.

## Troubleshooting

| Symptom | Likely cause |
| :--- | :--- |
| Screen stays black | Forward completed and no Reverse / `StopTransition` followed, or `bHoldAtMax` without `ReleaseHold` |
| Black screen and no input | Transition interrupted with `bAutoBlockInput`; run `TransitionFX.ForceClear` in the console or call `ForceClear` |
| Transition does not animate | Custom material lacks the `Progress` scalar parameter |
| UI still visible during transition | Using a PostProcess `DA_*` preset; switch to `DA_Widget_*` or hide widgets on `OnTransitionStarted` |
| Nodes missing in Blueprint | Plugin not enabled, or project not restarted after enabling |
| Hitch on first transition | Shaders compile on first use; preload presets |
| `OnTransitionCompleted` never fires | `StopTransition` / `ForceClear` do not fire it; Quick Fade nodes do not wait |
| Quick Fade shows nothing (log: `Could not find DA_FadeToBlack or M_Transition_Fade`) | `DA_FadeToBlack` and `M_Transition_Fade` were deleted from the plugin; reinstall the plugin |

## Project settings

**Edit > Project Settings > Plugins > TransitionFX** (`UTransitionFXSettings`, saved to `Config/DefaultGame.ini`):

```ini
[/Script/TransitionFX.TransitionFXSettings]
MaxPoolSizePerEffectClass=3   ; pooled effect instances per class, 0 disables pooling
```

## Custom effects (C++ only)

Implement `ITransitionEffect` (`Initialize`, `UpdateProgress`, `Cleanup`, `SetInvert`, `SetParameters`) on a `UObject` subclass and set it as the preset's `EffectClass`. Reference implementations: `Source/TransitionFX/Public/PostProcessTransitionEffect.h` and `WidgetTransitionEffect.h`. Shared parameter application lives in `TransitionFXMaterialUtils.h`. Material parameter name constants are in `TransitionFXConfig.h`.

## Editor tools

- **Tools > TransitionFX > TransitionFX Preview**: real-time preview with easing/duration controls and GIF capture. Lists only `MI_Transition_*` (PostProcess) materials; test `DA_Widget_*` presets in PIE.
- Console command `TransitionFX.ForceClear`: emergency reset during play.

## Known limitations

- Win64 only; not built for Mac, Linux, consoles, or mobile. Not compatible with UEFN.
- PostProcess presets need a PostProcess-capable pipeline (Deferred, or Forward with PostProcess enabled).
- Single active transition per GameInstance; no multiplayer replication.
- Pull requests to the upstream repository are not accepted; report issues at https://github.com/EmbarrassingMoment/TransitionFX/issues.
