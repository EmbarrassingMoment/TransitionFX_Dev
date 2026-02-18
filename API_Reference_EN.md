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
