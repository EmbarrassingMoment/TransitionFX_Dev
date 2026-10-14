# TransitionFX API Reference

This document provides a detailed explanation of the API for the TransitionFX plugin.

## 1. Introduction

The TransitionFX plugin mainly consists of the following components:

*   **UTransitionManagerSubsystem**: A singleton subsystem that handles the execution, management, and ticking of transition effects.
*   **UTransitionPreset**: A data asset that defines the settings for a transition effect (Effect Class, Material, Duration, Easing Type, etc.). `Effect Class` selects the rendering path: `PostProcessTransitionEffect` (PostProcess volume, ordered by `Priority`) or `WidgetTransitionEffect` (full-screen Slate overlay that also covers UMG/Slate UI, ordered by `WidgetZOrder`, default 10000; requires a UI-domain material such as `MI_Widget_*`).
*   **UTransitionSequence**: A data asset that chains several presets back-to-back (see [Sequence Nodes](#sequence-nodes)).
*   **UTransitionBlueprintLibrary**: The Blueprint nodes described below (latent actions, level transitions, quick fades, helpers).
*   **UTransitionFXSettings**: Project settings under **Project Settings > Plugins > TransitionFX** (see [Object Pooling](#object-pooling)).

### When the Effect Is Removed

What happens when a transition completes depends on whether it ends with the screen covered:

*   **Ends covered** — `Forward` without Invert (a Fade Out), or `Reverse` with Invert: `OnTransitionCompleted` fires and the effect **stays on screen** at full coverage, so nothing behind it is exposed while you load or reposition things. Input stays blocked if the preset's `bAutoBlockInput` is set. The effect is removed when you start the next transition (it is replaced in the same frame), resume it with `Reverse Transition` / `Invert Transition`, call `Stop Transition` / `Force Clear`, or travel to another level.
*   **Ends revealed** — `Reverse` without Invert (a Fade In), or `Forward` with Invert: the transition stops and cleans up automatically (effect removed, input restored).

In both cases `Is Transition Playing` returns false once the transition has completed, and `Is Current Transition Finished` returns true.

## 2. Blueprint API Reference

Primary functionality is provided via `UTransitionBlueprintLibrary`, implemented as Latent Action nodes.

### Latent Action Nodes

These nodes wait for the transition to complete before proceeding.

#### Play Transition And Wait

![Blueprint screenshot of the Play Transition And Wait latent action node](images/api_play_transition_node.png)

Plays a transition using the specified preset and waits for completion.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Preset** | Input | The transition preset (`UTransitionPreset`) to use. |
| **Mode** | Input | Transition mode (`Forward`: Fade Out/0→1, `Reverse`: Fade In/1→0). |
| **Play Speed** | Input | Playback speed multiplier (Default: 1.0). |
| **bInvert** | Input | Whether to invert the mask. |
| **Override Params** | Input | Structure for dynamically overriding material parameters (`FTransitionParameters`). |
| **Completed** | Output | Executed after the transition is complete. |

#### Play Transition And Wait With Duration
Plays a transition with a specific duration (in seconds) instead of using play speed.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Preset** | Input | The transition preset (`UTransitionPreset`) to use. |
| **Mode** | Input | Transition mode (`Forward`, `Reverse`). |
| **Duration** | Input | Duration of the transition in seconds. |
| **bInvert** | Input | Whether to invert the mask. |
| **Override Params** | Input | Structure for dynamically overriding material parameters. |
| **Completed** | Output | Executed after the transition is complete. |

#### Play Random Transition And Wait
Randomly selects one preset from the provided list and plays it. If the array is empty or the selected preset is null, the node completes immediately without playing a transition.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Presets** | Input | Array of transition presets to choose from. |
| **Mode** | Input | Transition mode (`Forward`, `Reverse`). |
| **Play Speed** | Input | Playback speed multiplier. |
| **bInvert** | Input | Whether to invert the mask. |
| **Override Params** | Input | Structure for dynamically overriding material parameters. |
| **Completed** | Output | Executed after the transition is complete. |

### Subsystem Nodes

The following nodes are called directly on the `UTransitionManagerSubsystem` instance. Get the subsystem via `Get Game Instance Subsystem`.

#### Start Transition
Starts a transition with the given preset. Unlike the Latent Action nodes, this does not wait for completion. Supports `bHoldAtMax` for loading screen workflows.

On completion, a Fade Out (Forward without Invert) keeps covering the screen until the next transition, `Reverse Transition` / `Invert Transition`, or `Stop Transition`; a Fade In (Reverse without Invert) stops and cleans up automatically. See [When the Effect Is Removed](#when-the-effect-is-removed).

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **Preset** | Input | The transition preset (`UTransitionPreset`) to use. If null, `ForceClear` is called and an error is logged. |
| **Mode** | Input | Transition mode (`Forward`: Fade Out/0→1, `Reverse`: Fade In/1→0). |
| **Play Speed** | Input | Playback speed multiplier (Default: 1.0). Clamped to a minimum of 0.01. |
| **bInvert** | Input | Whether to invert the mask. |
| **bHold At Max** | Input | If true, the transition holds at progress 1.0 instead of completing. |
| **Override Params** | Input | Structure for dynamically overriding material parameters (`FTransitionParameters`). |

#### Stop Transition
Stops the currently playing transition, or removes the effect that a completed Fade Out left covering the screen. Input is restored and `OnTransitionCompleted` does not fire.

#### Force Clear
Forcefully clears all transition states and resets input, and also cancels any playing sequence. Used for emergency stops or resetting.

#### Reverse Transition
Reverses the playback direction of the current transition (e.g., from Fade Out to Fade In). Works while a transition is playing, while it holds at max progress, and after a Fade Out has completed and is still covering the screen. After a transition that ended revealed has stopped, there is nothing to reverse (a warning is logged). If `bAutoStop` is true (default), the transition is finished automatically when the reverse completes; if false, it stays active at progress 0.

#### Invert Transition
Toggles the current transition's mask inversion and replays it forward (0 → 1) with the flipped mask. Like `Reverse Transition`, it can be called on a completed Fade Out that is still covering the screen — this fades back in. If `bAutoComplete` is true (default), the replayed transition completes normally; if false, it holds at max progress (release with `Release Hold`).

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **bAutoComplete** | Input | If true (default), the replayed transition completes normally. If false, it holds at progress 1.0. |

#### Set Play Speed
Changes the playback speed multiplier dynamically. The value is clamped to a minimum of 0.01.

#### Release Hold
Releases the hold at max progress, allowing a held transition to complete. Used with the `bHoldAtMax` workflow. Releasing a held Fade Out completes it (`OnTransitionCompleted` fires) and the screen stays covered; to fade back in, call `Reverse Transition` / `Invert Transition` instead (they work directly from the hold) or start a Reverse transition. See [Manual Control & Loading Screen Workflow](#6-manual-control--loading-screen-workflow).

#### Add Progress Threshold
Registers a progress threshold value (0.0 to 1.0, clamped). When the eased progress crosses this value during a transition (in either direction), `OnProgressThresholdReached` is broadcast once. Each new transition re-arms the registered thresholds; they stay registered until `Clear Progress Thresholds` is called.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **Threshold** | Input | A value between 0.0 and 1.0. |

#### Clear Progress Thresholds
Removes all registered progress thresholds.

### Query Nodes

#### Get Current Progress
Returns the current progress of the transition (0.0 to 1.0) **before easing** — the linear time-based value. `OnTransitionProgressChanged` and progress thresholds use the eased value instead. Available on the `TransitionManagerSubsystem`.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **Return Value** | Output | The current transition progress before easing (0.0 to 1.0). |

#### Is Transition Playing
Returns true while a transition is playing or holding. Returns false once it has completed — even if a completed Fade Out is still covering the screen. Available on the `TransitionManagerSubsystem`.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **Return Value** | Output | True if a transition is active. |

#### Is Current Transition Finished
Returns true once the current transition has completed (`OnTransitionCompleted` has fired; a held transition counts only after its hold is released). It stays true until another transition starts or the current one is resumed with `Reverse Transition` / `Invert Transition`. Useful for polling. Available on the `TransitionManagerSubsystem`.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **Return Value** | Output | True if the transition has finished. |

### Level Transition Nodes

#### Open Level With Transition

![Open Level With Transition node](images/api_open_level_node.png)

Handles the sequence of "Fade Out -> Open Level -> Fade In". The screen stays covered from the end of the fade-out until the old level is torn down, and the fade-in on the new level side is automatically started via `PostLoadMapWithWorld`. If `Preset` is null, the default `DA_FadeToBlack` preset is used.

*Note: Internally, the auto fade-in uses `Forward` mode with `bInvert=true` rather than `Reverse` mode. The visual result is equivalent.*

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Level Name** | Input | The name of the level to open. |
| **Preset** | Input | The transition preset to use. |
| **Duration** | Input | The duration of the transition (applies to both fade out and fade in). Default: 1.0. |

#### Open Level With Transition And Wait
Plays a transition (Fade Out), waits for it to complete, then opens the specified level. The new level will automatically play the reverse transition (Fade In).

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Level Name** | Input | The name of the level to open. |
| **Preset** | Input | The transition preset to use. |
| **Duration** | Input | The duration of the transition (applies to both fade out and fade in). |
| **Completed** | Output | Executed after the fade-out transition completes and `OpenLevel` is called. |

### Utility Nodes

#### Quick Fade To Black

![Blueprint screenshot of the Quick Fade To Black / Quick Fade From Black nodes](images/api_quick_fade_node.png)

Quickly fades the screen to black. The screen stays black until `Quick Fade From Black` (or another transition) runs. This is a **fire-and-forget** function that does not wait for the transition to complete.

Both Quick Fade nodes play a temporary preset that borrows only the effect class and material of the default `DA_FadeToBlack` preset: the color is always black, easing is `Linear`, input is blocked, and no sound plays. Use `Play Transition And Wait` with your own preset if you need other settings.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Duration** | Input | The duration of the fade (Default: 1.0). |

#### Quick Fade From Black
Quickly fades the screen from black (a Reverse fade built the same way as `Quick Fade To Black`). This is a **fire-and-forget** function that does not wait for the transition to complete.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Duration** | Input | The duration of the fade (Default: 1.0). |

#### Is Any Transition Playing
Returns true if any transition is currently playing.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Return Value** | Output | True if a transition is active. |

#### Apply Easing
Applies an easing function to the given alpha value. A pure math node useful for custom animation curves.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **Alpha** | Input | The input alpha value (0.0 to 1.0). |
| **Easing Type** | Input | The easing type to apply (`ETransitionEasing`). |
| **Custom Curve** | Input | Optional custom curve to use if EasingType is `Custom`. |
| **Return Value** | Output | The eased alpha value. |

### System Nodes

#### Preload Transition Presets

![Preload Transition Presets node](images/api_preload_node.png)

Pre-compiles shaders for the given presets to prevent first-frame hitching. Available on the `TransitionManagerSubsystem`.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **Presets** | Input | Array of transition presets to preload. |

#### Async Load Transition Presets
Asynchronously loads presets from Soft Object References and warms up their shaders. Available on the `TransitionManagerSubsystem`.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **Soft Presets** | Input | Array of soft object references to transition presets. |
| **On Complete** | Output | Delegate fired when loading and shader warmup is complete. |

### Sequence Nodes

Play multiple transitions back-to-back using a `UTransitionSequence` data asset. Sequences cannot contain level transitions, and only one sequence can play at a time — calling `StartTransition` or `OpenLevelWithTransition` while a sequence is playing cancels it. Between steps the effect is swapped without exposing the scene; when the sequence finishes, the last step's effect follows the usual rule (see [When the Effect Is Removed](#when-the-effect-is-removed)) — it stays covering the screen if the last step ends covered.

#### Play Sequence And Wait
Latent action that plays a transition sequence and waits for it to complete (including all loops). If the sequence is null or empty, the node completes immediately with a warning.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Sequence** | Input | The transition sequence (`UTransitionSequence`) to play. |
| **Completed** | Output | Executed after the entire sequence (and all loops) finishes. |

#### Play Sequence
Starts playing a transition sequence without waiting. Available on the `TransitionManagerSubsystem`. A null or empty sequence is a no-op (with a warning log). Stops any currently playing sequence or transition first. Ignored (with a warning) while a level transition is pending.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **Sequence** | Input | The transition sequence (`UTransitionSequence`) to play. |

#### Stop Sequence
Stops the currently playing sequence (if any) and the underlying transition. Does **not** fire `OnSequenceCompleted` (cancellation is not a successful completion).

#### Is Sequence Playing
Returns true if a sequence is currently playing.

#### Get Current Sequence Step
Returns the index of the currently playing entry, or `-1` if no sequence is playing.

## 3. C++ API Reference

From C++, you control transitions via the `UTransitionManagerSubsystem`.

### Accessing the Subsystem

```cpp
UTransitionManagerSubsystem* TransitionSystem = GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>();
```

### Key Functions

#### StartTransition
Starts a transition. If `Preset` is null or its material is missing, `ForceClear` is called and an error is logged. `PlaySpeed` is clamped to a minimum of 0.01. A transition that ends revealed (e.g., a Reverse fade-in) stops and cleans up automatically; one that ends covered (e.g., a Forward fade-out) keeps its effect on screen until the next transition, `ReverseTransition` / `InvertTransition`, or `StopTransition` (see [When the Effect Is Removed](#when-the-effect-is-removed)).

```cpp
void StartTransition(
    UTransitionPreset* Preset,
    ETransitionMode Mode = ETransitionMode::Forward,
    float PlaySpeed = 1.0f,
    bool bInvert = false,
    bool bHoldAtMax = false,
    FTransitionParameters OverrideParams = FTransitionParameters()
);
```

#### StopTransition
Stops the current transition, or removes the effect that a completed transition left covering the screen. Restores input; does not broadcast `OnTransitionCompleted`.

```cpp
void StopTransition();
```

#### ReleaseHold
Releases the hold at max progress, allowing a held transition to complete. Used after `StartTransition` with `bHoldAtMax = true`. A released Fade Out keeps covering the screen after it completes.

```cpp
void ReleaseHold();
```

#### ReverseTransition
Reverses the playback direction of the current transition (e.g., from Fade Out to Fade In). Also works on a completed Fade Out that is still covering the screen; logs a warning and does nothing when no transition is left to reverse.

```cpp
void ReverseTransition(bool bAutoStop = true);
```

#### InvertTransition
Toggles the current transition's mask inversion and replays it forward (0 → 1). Also works on a completed Fade Out that is still covering the screen. If `bAutoComplete` is false, the replayed transition holds at max progress (release with `ReleaseHold`).

```cpp
void InvertTransition(bool bAutoComplete = true);
```

#### SetPlaySpeed
Changes the playback speed multiplier dynamically. The value is clamped to a minimum of 0.01.

```cpp
void SetPlaySpeed(float PlaySpeed = 1.0f);
```

#### GetCurrentProgress
Returns the current progress of the transition (0.0 to 1.0) before easing.

```cpp
float GetCurrentProgress() const;
```

#### IsTransitionPlaying
Returns true while a transition is playing or holding; false once it has completed, even if a completed Fade Out is still covering the screen.

```cpp
bool IsTransitionPlaying() const;
```

#### IsCurrentTransitionFinished
Returns true once the current transition has completed (after its hold, if any). Useful for polling.

```cpp
bool IsCurrentTransitionFinished() const;
```

#### ForceClear
Forcefully clears any active transition (including a playing sequence and an effect left covering the screen) and resets input. Used for emergency recovery.

```cpp
void ForceClear();
```

#### OpenLevelWithTransition
Handles the sequence of "Fade Out -> Open Level -> Fade In". Persists state across level transitions. Also callable from Blueprint on the subsystem.

```cpp
void OpenLevelWithTransition(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration = 1.0f);
```

#### PrepareAutoReverseTransition
Prepares the subsystem for an auto-reverse transition on the next level load. Does NOT start any transition immediately. Also callable from Blueprint on the subsystem.

```cpp
void PrepareAutoReverseTransition(UTransitionPreset* Preset, float Duration = 1.0f);
```

#### AddProgressThreshold
Registers a progress threshold (0.0 to 1.0, clamped). When the eased progress crosses this value, `OnProgressThresholdReached` is broadcast once. Each new transition re-arms the registered thresholds; they stay registered until `ClearProgressThresholds` is called.

```cpp
void AddProgressThreshold(float Threshold);
```

#### ClearProgressThresholds
Removes all registered progress thresholds.

```cpp
void ClearProgressThresholds();
```

#### Sequence Playback
Plays a `UTransitionSequence` data asset's entries back-to-back. `PlaySequence` validates the sequence (null or empty is a no-op with a warning) and stops any currently playing transition or sequence first; it is ignored while a level transition is pending. `StopSequence` cancels playback without firing `OnSequenceCompleted`.

```cpp
void PlaySequence(UTransitionSequence* Sequence);
void StopSequence();
bool IsSequencePlaying() const;
int32 GetCurrentSequenceStep() const; // -1 if no sequence is playing
```

#### GetDefaultFadePreset
Returns the default `DA_FadeToBlack` preset, loading it lazily if necessary.

```cpp
UTransitionPreset* GetDefaultFadePreset();
```

#### GetTransitionManager (Static Helper)
Retrieves the `UTransitionManagerSubsystem` from the given world context. Returns `nullptr` on failure. Available on `UTransitionBlueprintLibrary`.

```cpp
static UTransitionManagerSubsystem* GetTransitionManager(const UObject* WorldContextObject);
```

### Delegates / Event Dispatchers

The subsystem provides the following event dispatchers:

*   **OnTransitionStarted**: Called when a transition starts.
*   **OnTransitionCompleted**: Called when a transition completes.
*   **OnTransitionHoldStarted**: Called when a transition holds at max progress (1.0) (if `bHoldAtMax` is true).
*   **OnTransitionProgressChanged** `(float Progress)`: Broadcasts the eased progress value (0.0 to 1.0) every tick while a transition is active. Removes the need to poll `GetCurrentProgress()`.
*   **OnProgressThresholdReached** `(float Threshold)`: Fires once when the eased progress crosses a value registered via `AddProgressThreshold`. Thresholds are re-armed automatically when a new transition starts.
*   **OnSequenceStepChanged** `(int32 StepIndex)`: Broadcast each time a sequence advances to a new entry (including the first).
*   **OnSequenceCompleted**: Broadcast when the entire sequence finishes (after all loops, if any). Not fired when a sequence is cancelled via `StopSequence`.

*Note: There is no delegate named `OnTransitionStop`, but calling `StopTransition` will interrupt the transition.*

## 4. Advanced Features

### Async Loading
Using `AsyncLoadTransitionPresets`, you can asynchronously load presets using a list of Soft Object References (`TSoftObjectPtr`) and perform shader preloading.

```cpp
void AsyncLoadTransitionPresets(const TArray<TSoftObjectPtr<UTransitionPreset>>& SoftPresets, FTransitionPreloadCompleteDelegate OnComplete);
```

### Object Pooling
The subsystem pools transition effect instances for reuse, capped per effect class to prevent memory bloat (default: **3 instances**). The cap is configurable via **Project Settings > Plugins > TransitionFX > `MaxPoolSizePerEffectClass`**; set it to 0 to disable pooling. Excess instances are released for garbage collection.

### Preloading
`PreloadTransitionPresets` uses loaded presets to warm up shaders in advance, preventing hitches (stuttering) during transition execution.

```cpp
void PreloadTransitionPresets(const TArray<UTransitionPreset*>& Presets);
```

## 5. Parameter Overrides

By using the `FTransitionParameters` struct, you can dynamically change preset material parameters at runtime.

```cpp
FTransitionParameters Params;
Params.ScalarParams.Add(FName("SplitCount"), 12.0f);          // e.g. a Stripe Cascade parameter
Params.VectorParams.Add(FName("FadeColor"), FLinearColor::Red);
// TextureParams can also be set similarly (e.g. "MaskTexture" for Texture Mask)
```

This structure can be passed to `StartTransition` and various Blueprint nodes.

*   Parameter names must match the material. An override for a parameter the material does not expose is ignored and a warning is logged (`LogTransitionFX`).
*   To give a preset its own default color without passing overrides, enable `bOverrideTransitionColor` on the preset and set `TransitionColor`. A `FadeColor` passed in `FTransitionParameters` takes precedence over it.

## 6. Manual Control & Loading Screen Workflow

TransitionFX provides the ability to manually control transition timing, which is especially useful for creating "Loading Screens" that hide the screen during level loading or heavy processing.

### Key Parameters and Functions

*   **bHoldAtMax (StartTransition)**:
    If this parameter is set to `true`, the transition effect will automatically pause (hold) when the progress reaches **1.0 (fully covered screen)**. Input remains blocked in this state.

*   **ReleaseHold**:
    Function to complete a transition that is in a hold state. The held Fade Out completes (`OnTransitionCompleted` fires) and its effect keeps covering the screen — it does not fade back in by itself.

*   **ReverseTransition / InvertTransition**:
    Fade back in from the hold (or from a completed Fade Out). `ReverseTransition` plays the same effect backwards (1 → 0); `InvertTransition` replays it forwards with the mask flipped. Both can be called directly while holding — `ReleaseHold` is not needed first.

### Workflow Example

1.  **Start Transition**
    *   Call `StartTransition` (or the Blueprint node) and set `bHoldAtMax` to `true`.
    *   The screen becomes covered by the effect and pauses at progress 1.0 (`OnTransitionHoldStarted` fires).

2.  **Background Processing (Load Level / Spawn Actors)**
    *   While the screen is hidden, perform tasks you don't want the player to see, such as loading levels or spawning actors.

3.  **Fade Back In**
    *   Once loading is complete, call `ReverseTransition` (or `InvertTransition`). Alternatively, call `ReleaseHold` and then start a Reverse transition in the same frame (e.g., `Play Transition And Wait` with `Mode: Reverse`).

4.  **Transition Finishes**
    *   The fade-in completes, the effect is removed, and input is restored.

## 7. Debugging & Utilities

Functions helpful for development and debugging.

### Force Clear

*   **Blueprint Node**: `ForceClear`
    *   Instantly destroys all transition states and unblocks input. Used to recover from unexpected stuck states.

*   **Console Command**: `TransitionFX.ForceClear`
    *   **Important**: QA testers and developers can type this command in the console window (`@` or `~` key) to instantly recover from a stuck black screen.
