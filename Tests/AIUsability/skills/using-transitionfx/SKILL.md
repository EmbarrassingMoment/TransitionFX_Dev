---
name: using-transitionfx
description: Integrates, configures and debugs the TransitionFX Unreal Engine 5 plugin (procedural SDF screen transitions driven by UTransitionPreset data assets, Blueprint latent nodes and UTransitionManagerSubsystem). Use when the user mentions TransitionFX, a DA_* or DA_Widget_* transition preset, Play Transition And Wait, Open Level With Transition, Transition Sequence assets, fade/iris/wipe screen transitions in UE5, covering UMG with a transition, or a screen stuck black after a transition.
---

# Using TransitionFX

TransitionFX is a data-driven, GameInstance-scoped screen-transition system for Unreal Engine 5.5+ (Win64, DX12/SM6).
A `UTransitionPreset` data asset (effect class + material + duration + easing + audio + input blocking) is played by
`UTransitionManagerSubsystem`, which drives the material parameter `Progress` from 0 to 1 (eased) each tick.
Blueprint users mostly call the latent nodes in `UTransitionBlueprintLibrary`; C++ users call the subsystem directly.

Answer from the bundled documentation, not from memory. Every public symbol below exists in the headers under
`reference/`; if a name you want to use is not in them, it does not exist in the plugin.

## Where to look

| Question | Read |
| :--- | :--- |
| Overview, feature list, built-in effect list, widget-layer variant table, limitations | [README.md](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/README.md) |
| Exact Blueprint node pins, C++ signatures, delegates, parameter overrides, hold/loading-screen workflow, Force Clear | [docs/API_Reference_EN.md](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/docs/API_Reference_EN.md) |
| Step-by-step setup: creating presets, preloading, calling from Blueprint, loading screens, debugging, events | [docs/QUICKSTART_EN.md](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/docs/QUICKSTART_EN.md) |
| Troubleshooting and behaviour questions (nothing shows, pause, UMG not covered, pooling, seamless travel) | [docs/FAQ_EN.md](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/docs/FAQ_EN.md) |
| Editor Transition Preview Panel: opening it, controls, GIF export | [docs/TransitionFX_PreviewTool_Manual.md](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/docs/TransitionFX_PreviewTool_Manual.md) |
| Sample project `L_ShowCase` level (Japanese): preset list, controls, how it is wired | [docs/SHOWCASE_LEVEL.md](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/docs/SHOWCASE_LEVEL.md) |
| Sample project `L_WidgetLayerSample` level (Japanese): PostProcess vs widget-layer comparison | [docs/WIDGET_LAYER_SAMPLE.md](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/docs/WIDGET_LAYER_SAMPLE.md) |
| What is planned vs already shipped (do not promise planned items as available) | [docs/ROADMAP_EN.md](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/docs/ROADMAP_EN.md) |
| Which version added or changed a behaviour | [CHANGELOG.md](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/CHANGELOG.md) |

Public C++ headers (authoritative for names, signatures, defaults and `Category` paths), under
`${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/`:

| Header | Contents |
| :--- | :--- |
| [TransitionManagerSubsystem.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionManagerSubsystem.h) | `UTransitionManagerSubsystem` (all runtime API), `ETransitionMode`, every delegate |
| [TransitionBlueprintLibrary.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionBlueprintLibrary.h) | Latent nodes (`PlayTransitionAndWait`, `OpenLevelWithTransitionAndWait`, `PlaySequenceAndWait`, ...) and `GetTransitionManager` |
| [TransitionPreset.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionPreset.h) | `UTransitionPreset` properties and `FTransitionParameters` |
| [TransitionSequence.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionSequence.h) | `UTransitionSequence`, `FTransitionSequenceEntry` |
| [TransitionSequencePlayer.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionSequencePlayer.h) | Internal sequence engine (not a user-facing API) |
| [ITransitionEffect.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/ITransitionEffect.h) | Interface custom effects implement |
| [PostProcessTransitionEffect.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/PostProcessTransitionEffect.h) | Default effect class (PostProcess volume, below UMG) |
| [WidgetTransitionEffect.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/WidgetTransitionEffect.h) | Widget-layer effect class (full-viewport Slate overlay, above UMG) |
| [TransitionFXTypes.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionFXTypes.h) | `ETransitionEasing` |
| [TransitionFXConfig.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionFXConfig.h) | Material parameter names, `CalculatePlaySpeed` |
| [TransitionFXSettings.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionFXSettings.h) | Project setting `MaxPoolSizePerEffectClass` |
| [TransitionFXMaterialUtils.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionFXMaterialUtils.h) | Shared parameter-override helper |
| [TransitionFX.h](${CLAUDE_PLUGIN_ROOT}/skills/using-transitionfx/reference/Plugins/TransitionFX/Source/TransitionFX/Public/TransitionFX.h) | Module and `LogTransitionFX` log category |

## Facts that answers most often get wrong

**Getting the subsystem.** Blueprint: `Get Game Instance Subsystem (TransitionManagerSubsystem)`.
C++: `UTransitionBlueprintLibrary::GetTransitionManager(WorldContextObject)` or
`GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>()`. Add `"TransitionFX"` to
`PublicDependencyModuleNames` in the game module's `Build.cs`.

**Starting a transition without waiting** (C++ and Blueprint share this signature):

```cpp
void StartTransition(UTransitionPreset* Preset,
                     ETransitionMode Mode = ETransitionMode::Forward,
                     float PlaySpeed = 1.0f,
                     bool bInvert = false,
                     bool bHoldAtMax = false,
                     FTransitionParameters OverrideParams = FTransitionParameters());
```

`PlaySpeed` is a multiplier, not seconds. For an exact duration use the Blueprint node
`Play Transition And Wait With Duration`, or in C++ convert with
`TransitionFXConfig::CalculatePlaySpeed(Preset->DefaultDuration, DurationSeconds)` (equivalent to
`DefaultDuration / DurationSeconds`).

**Modes.** `ETransitionMode::Forward` = "Fade Out (0 to 1)", `Reverse` = "Fade In (1 to 0)". `Reverse` reverses
playback direction; `bInvert` flips which area the mask covers (shape reveals instead of covers) while progress
still runs 0 to 1. `Forward + bInvert` is therefore another way to get a fade-in look.

**Waiting in Blueprint.** `Play Transition And Wait`, `Play Transition And Wait With Duration`,
`Play Random Transition And Wait`, `Play Sequence And Wait`, `Open Level With Transition And Wait` are latent nodes
with a `Completed` pin. On the three `Play Transition...` nodes the `Mode` enum is expanded into exec pins.

**Level change with fade.** `OpenLevelWithTransition(WorldContextObject, LevelName, Preset, Duration)` fades out,
opens the level, and automatically plays the reverse (fade-in) once the new level has loaded. Do not add a manual
fade-in in the new level. `PrepareAutoReverseTransition` only arms that fade-in for the next load.

**Loading screen / hold.** `StartTransition(..., bHoldAtMax = true)` pauses at progress 1.0 with input still
blocked; `OnTransitionHoldStarted` fires; call `ReleaseHold()` to finish. Not available inside a sequence.

**Overriding material parameters.** Fill `FTransitionParameters` (`ScalarParams`, `VectorParams`, `TextureParams`,
all `TMap<FName, ...>`) and pass it as `OverrideParams`. The colour parameter is named `FadeColor`
(`TransitionFXConfig::ColorParamName`); progress is `Progress`, invert is `Invert`. A preset can also set
`bOverrideTransitionColor` + `TransitionColor`; a call-site `FadeColor` override wins.

**Covering UMG/Slate UI.** `PostProcessTransitionEffect` (the default `EffectClass`) renders below UMG and cannot be
made to cover it with `Priority`. Use a widget-layer preset (`DA_Widget_Fade`, `DA_Widget_FadeToBlack`,
`DA_Widget_Iris`, `DA_Widget_LinearWipe`, `DA_Widget_Dissolve`, `DA_Widget_RadialWipe`, `DA_Widget_CheckerBoard`,
`DA_Widget_Blinds`, `DA_Widget_TextureMask`) or set `EffectClass = WidgetTransitionEffect` with an `MI_Widget_*`
material; raise `WidgetZOrder` (default 10000) if the UI uses a higher Z-order. Pixelate and Slice have no
widget-layer variant. Widget-layer materials do not appear in the editor Transition Preview Panel.

**Sequences.** `UTransitionSequence` holds ordered `Entries` (`Preset`, `Mode`, `DurationOverride` 0 = preset
default, `bInvert`, `DelayAfter`, `OverrideParams`) plus `bLoop` / `LoopCount` (0 = infinite). Play with
`Play Sequence And Wait` or `PlaySequence`. Level transitions and `bHoldAtMax` are not supported inside a sequence;
only one sequence plays at a time; `StartTransition` / `OpenLevelWithTransition` cancel a running sequence.

**Events** (all `BlueprintAssignable` on the subsystem): `OnTransitionStarted`, `OnTransitionCompleted`,
`OnTransitionHoldStarted`, `OnTransitionProgressChanged(float Progress)`, `OnProgressThresholdReached(float Threshold)`
(register with `AddProgressThreshold`), `OnSequenceStepChanged(int32 StepIndex)`, `OnSequenceCompleted`.
There is no `OnTransitionStopped`; `StopTransition` and `ForceClear` do not fire `OnTransitionCompleted`.

**First-play hitch.** Call `PreloadTransitionPresets(Presets)` (hard references) or
`AsyncLoadTransitionPresets(SoftPresets, OnComplete)` (soft references, `FTransitionPreloadCompleteDelegate`,
bind with `BindDynamic` to a `UFUNCTION`) once at startup to warm up shaders.

**Stuck black screen.** Causes: `bHoldAtMax` without `ReleaseHold`, or an interrupted transition with
`bAutoBlockInput`. Recover with the `ForceClear` node or the console command `TransitionFX.ForceClear`.
`ForceClear` destroys state immediately and unblocks input; `StopTransition` cleans up normally and respects
`bAutoBlockInput`.

**Custom effects.** Implement `ITransitionEffect` on a `UObject`: `Initialize(UWorld*, UTransitionPreset*)`,
`UpdateProgress(float)`, `Cleanup()`, `SetInvert(bool)`, `SetParameters(const FTransitionParameters&)`. Set the
preset's `EffectClass` to the class (`MustImplement` TransitionEffect). Instances are pooled per class
(`MaxPoolSizePerEffectClass`, default 3, Project Settings > Plugins > TransitionFX).

**Hard limits.** One transition at a time per GameInstance (a new one replaces the current). No multiplayer
replication (client-local). Not compatible with UEFN. Win64 only. The subsystem survives `ServerTravel`, but the
automatic fade-in only follows `OpenLevel`; after `ServerTravel` start the Reverse transition manually (see FAQ).

## How to answer

1. Classify the request: Blueprint wiring, C++ integration, preset/material setup, widget-layer coverage, sequence,
   level change, performance, debugging, or capability question.
2. Read the matching reference file from the table above. For code, open the header and copy the exact signature.
3. Prefer the plugin's own node or function over a workaround (for example `OpenLevelWithTransition` over
   `Open Level` + manual fades; `ReleaseHold` over `SetPlaySpeed(0)`).
4. State plugin limits plainly when the request hits one (simultaneous transitions, replication, UEFN, Pixelate or
   Slice on the widget layer, level change inside a sequence).
