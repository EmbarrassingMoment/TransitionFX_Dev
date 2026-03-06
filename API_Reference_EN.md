# TransitionFX API Reference

This document provides a detailed explanation of the API for the TransitionFX plugin.

## 1. Introduction

The TransitionFX plugin mainly consists of the following two components:

*   **UTransitionManagerSubsystem**: A singleton subsystem that handles the execution, management, and ticking of transition effects.
*   **UTransitionPreset**: A data asset that defines the settings for a transition effect (Effect Class, Material, Duration, Easing Type, etc.).

## 2. Blueprint API Reference

Primary functionality is provided via `UTransitionBlueprintLibrary`, implemented as Latent Action nodes.

### Latent Action Nodes

These nodes wait for the transition to complete before proceeding.

#### Play Transition And Wait
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
Randomly selects one preset from the provided list and plays it.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Presets** | Input | Array of transition presets to choose from. |
| **Mode** | Input | Transition mode (`Forward`, `Reverse`). |
| **Play Speed** | Input | Playback speed multiplier. |
| **bInvert** | Input | Whether to invert the mask. |
| **Override Params** | Input | Structure for dynamically overriding material parameters. |
| **Completed** | Output | Executed after the transition is complete. |

### Control Nodes

#### Stop Transition
Stops the currently playing transition.

#### Force Clear
Forcefully clears all transition states and resets input. Used for emergency stops or resetting.

### Level Transition Nodes

#### Open Level With Transition
Handles the sequence of "Fade Out -> Open Level -> Fade In". The fade-in on the new level side is automatically started via `PostLoadMapWithWorld`.

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
Quickly fades the screen to black using the default `DA_FadeToBlack` preset.

| Pin Name | Type | Description |
| :--- | :--- | :--- |
| **World Context Object** | Input | The world context object. |
| **Duration** | Input | The duration of the fade (Default: 1.0). |

#### Quick Fade From Black
Quickly fades the screen from black using the default `DA_FadeToBlack` preset.

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

## 3. C++ API Reference

From C++, you control transitions via the `UTransitionManagerSubsystem`.

### Accessing the Subsystem

```cpp
UTransitionManagerSubsystem* TransitionSystem = GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>();
```

### Key Functions

#### StartTransition
Starts a transition.

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
Stops the current transition.

```cpp
void StopTransition();
```

#### ReverseTransition
Reverses the playback direction of the current transition (e.g., from Fade Out to Fade In).

```cpp
void ReverseTransition(bool bAutoStop = true);
```

#### SetPlaySpeed
Changes the playback speed multiplier dynamically.

```cpp
void SetPlaySpeed(float PlaySpeed = 1.0f);
```

#### GetCurrentProgress
Returns the current progress of the transition (0.0 to 1.0).

```cpp
float GetCurrentProgress() const;
```

#### IsTransitionPlaying
Returns true if a transition is currently active.

```cpp
bool IsTransitionPlaying() const;
```

#### IsCurrentTransitionFinished
Returns true if the current transition has finished its hold phase or completed. Useful for polling.

```cpp
bool IsCurrentTransitionFinished() const;
```

#### ForceClear
Forcefully clears any active transition and resets input. Used for emergency recovery.

```cpp
void ForceClear();
```

#### OpenLevelWithTransition
Handles the sequence of "Fade Out -> Open Level -> Fade In". Persists state across level transitions.

```cpp
void OpenLevelWithTransition(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration = 1.0f);
```

#### PrepareAutoReverseTransition
Prepares the subsystem for an auto-reverse transition on the next level load. Does NOT start any transition immediately.

```cpp
void PrepareAutoReverseTransition(UTransitionPreset* Preset, float Duration = 1.0f);
```

### Delegates / Event Dispatchers

The subsystem provides the following event dispatchers:

*   **OnTransitionStarted**: Called when a transition starts.
*   **OnTransitionCompleted**: Called when a transition completes.
*   **OnTransitionHoldStarted**: Called when a transition holds at max progress (1.0) (if `bHoldAtMax` is true).

*Note: There is no delegate named `OnTransitionStop`, but calling `StopTransition` will interrupt the transition.*

## 4. Advanced Features

### Async Loading
Using `AsyncLoadTransitionPresets`, you can asynchronously load presets using a list of Soft Object References (`TSoftObjectPtr`) and perform shader preloading.

```cpp
void AsyncLoadTransitionPresets(const TArray<TSoftObjectPtr<UTransitionPreset>>& SoftPresets, FTransitionPreloadCompleteDelegate OnComplete);
```

### Preloading
`PreloadTransitionPresets` uses loaded presets to warm up shaders in advance, preventing hitches (stuttering) during transition execution.

```cpp
void PreloadTransitionPresets(const TArray<UTransitionPreset*>& Presets);
```

## 5. Parameter Overrides

By using the `FTransitionParameters` struct, you can dynamically change preset material parameters at runtime.

```cpp
FTransitionParameters Params;
Params.ScalarParams.Add(FName("Intensity"), 2.0f);
Params.VectorParams.Add(FName("Color"), FLinearColor::Red);
// TextureParams can also be set similarly
```

This structure can be passed to `StartTransition` and various Blueprint nodes.

## 6. Manual Control & Loading Screen Workflow

TransitionFX provides the ability to manually control transition timing, which is especially useful for creating "Loading Screens" that hide the screen during level loading or heavy processing.

### Key Parameters and Functions

*   **bHoldAtMax (StartTransition)**:
    If this parameter is set to `true`, the transition effect will automatically pause (hold) when the progress reaches **1.0 (fully covered screen)**. Input remains blocked in this state.

*   **ReleaseHold**:
    Function to resume and complete a transition that is in a hold state. Calling this releases the hold and seamlessly transitions to the finish.

### Workflow Example

1.  **Start Transition**
    *   Call `StartTransition` (or the Blueprint node) and set `bHoldAtMax` to `true`.
    *   The screen becomes covered by the effect and pauses at progress 1.0.

2.  **Background Processing (Load Level / Spawn Actors)**
    *   While the screen is hidden, perform tasks you don't want the player to see, such as loading levels or spawning actors.

3.  **Release Hold (Call ReleaseHold)**
    *   Once loading is complete, call the `ReleaseHold` function.

4.  **Transition Finishes**
    *   The effect resumes and the transition finishes smoothly.

## 7. Debugging & Utilities

Functions helpful for development and debugging.

### Force Clear

*   **Blueprint Node**: `ForceClear`
    *   Instantly destroys all transition states and unblocks input. Used to recover from unexpected stuck states.

*   **Console Command**: `TransitionFX.ForceClear`
    *   **Important**: QA testers and developers can type this command in the console window (`@` or `~` key) to instantly recover from a stuck black screen.
