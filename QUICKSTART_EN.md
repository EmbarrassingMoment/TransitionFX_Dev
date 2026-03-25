# TransitionFX Quick Start Guide

---

## Table of Contents

1. [Preset Creation](#step-1-preset-creation)
2. [Initialization](#step-2-initialization)
3. [How to Call from BP](#step-3-how-to-call-from-bp)
4. [Application to Loading Screens](#step-4-application-to-loading-screens)
5. [Debugging Methods](#step-5-debugging-methods)
6. [Utilizing Events (Delegates)](#step-6-utilizing-events-delegates)

---

## Step 1: Preset Creation

In TransitionFX, transition effect settings are managed as **DataAssets (Transition Presets)**.
First, create this asset.

### Creation Steps

1. Right-click in the Content Browser → select `Miscellaneous` → `Data Asset`
2. Select `TransitionPreset` from the class list
3. Set the asset name (e.g., `DA_FadeToBlack`, `DA_Iris`, etc.)

<!-- IMAGE: quickstart_create_data_asset.png - Screenshot of Content Browser showing the Data Asset creation flow -->

### Settings

**Basic Settings (Required)**

| Property | Description | Recommended Value |
| :--- | :--- | :--- |
| `Effect Class` | The class of the effect to use | `PostProcessTransitionEffect` |
| `Transition Material` | The material used for the transition | `M_Transition_Master`, `M_Transition_Iris`, `M_Transition_Diamond`, etc. |
| `Default Duration` | The length of the transition (seconds) | `1.0` |

**Behavior Settings**

| Property | Description | Recommended Value |
| :--- | :--- | :--- |
| `Easing Type` | The easing function applied to the progress | `Linear` (sufficient for a simple fade) |
| `Progress Curve` | FloatCurve for custom easing (only visible when `Custom` is selected) | Arbitrary |
| `bAutoBlockInput` | Automatically blocks player input during the transition | `True` recommended |
| `bTickWhenPaused` | Allows the transition to update even when paused | `True` if used on a pause screen |
| `Priority` | PostProcess rendering priority | `1000` (default) |

**Audio Settings (Optional)**

| Property | Description |
| :--- | :--- |
| `Transition Sound` | The Sound asset to play when the transition starts |
| `Sound Volume` | Volume (default: `1.0`) |
| `Sound Pitch` | Pitch (default: `1.0`) |

![Screenshot of the TransitionPreset detail panel showing all properties](docs/images/quickstart_preset_settings.png)

### Example Setting: Simple Fade Out

```
DA_FadeToBlack
├─ Effect Class        : PostProcessTransitionEffect
├─ Transition Material : M_Transition_Master
├─ Default Duration    : 1.0
├─ Easing Type         : EaseInOutSine
├─ bAutoBlockInput     : True
└─ bTickWhenPaused     : False
```

> **💡 Tip**
> The material requires a scalar parameter named `Progress`.
> If this parameter does not exist, the transition will not animate (a warning log will be output).

---

## Step 2: Initialization

Since `UTransitionManagerSubsystem` operates as a **GameInstance Subsystem**, it is automatically initialized when the engine starts. All console command registration and default asset loading are handled automatically, so no special setup is required.

The only initialization that should be performed **manually is shader preloading**.

### Why Preloading is Necessary

Unreal Engine compiles shaders **the first time they are used**. If nothing is done, a momentary stutter (hitch) may occur when a transition is played for the first time during the game. By calling `PreloadTransitionPresets`, you can complete this compilation when the game starts.

### Implementation Method

**Blueprint**

Call the node in the `Init` event of the `GameInstance`, or in `BeginPlay` of the first level.

![Blueprint screenshot of PreloadTransitionPresets node in GameInstance Init](docs/images/quickstart_preload_bp.png)

```
Event Init (GameInstance)
└─ Get Game Instance Subsystem (TransitionManagerSubsystem)
    └─ Preload Transition Presets
        └─ Presets: [DA_FadeToBlack, DA_Iris, ...]
```

**C++**

```cpp
void UMyGameInstance::Init()
{
    Super::Init();

    UTransitionManagerSubsystem* TransitionSystem = GetSubsystem<UTransitionManagerSubsystem>();
    if (TransitionSystem)
    {
        TArray<UTransitionPreset*> Presets = { FadePreset, IrisPreset };
        TransitionSystem->PreloadTransitionPresets(Presets);
    }
}
```

### When Saving Memory is Desired: Asynchronous Preloading

If you manage presets via soft references, or if you want to prepare shaders in the background during a loading screen, use `AsyncLoadTransitionPresets`.

```
Loading Screen BeginPlay
└─ Async Load Transition Presets
    ├─ Soft Presets: [DA_FadeToBlack (Soft), DA_Iris (Soft), ...]
    └─ On Complete → Transition to the main game (Open Level, etc.)
```

```cpp
TArray<TSoftObjectPtr<UTransitionPreset>> SoftPresets = { SoftFadePreset, SoftIrisPreset };

TransitionSystem->AsyncLoadTransitionPresets(SoftPresets,
    FTransitionPreloadCompleteDelegate::CreateLambda([]()
    {
        // Shaders are ready → Proceed to main game
    })
);
```

### What is Handled Automatically (No Action Required)

| Action | Timing |
| :--- | :--- |
| Registration of `TransitionFX.ForceClear` console command | Subsystem initialization |
| Loading of the default fade preset `DA_FadeToBlack` (and its referenced material) | Subsystem initialization |
| Preparation for automatic fade-in after a level transition | Automatically bound to `PostLoadMapWithWorld` |

> **💡 Tip**
> It is recommended to limit the presets passed to preloading to those that are "guaranteed to be used early in the game."
> Passing all presets will increase the load at startup. If memory is a concern, use asynchronous preloading and design it so that they are loaded when needed.

---

## Step 3: How to Call from BP

The basic usage is simply calling the **`Play Transition And Wait`** node.
Both fade-out and fade-in use the same preset, switched via the `Mode` pin.

### Basic Pattern: Fade Out → Processing → Fade In

![Blueprint graph showing Fade Out → Processing → Fade In pattern](docs/images/quickstart_fadeout_fadein_bp.png)

This is the most common configuration. Darken the screen, perform the processing, and return with a fade-in.

```
[Any Event]
    │
    ▼
Play Transition And Wait
├─ Preset    : DA_FadeToBlack
├─ Mode      : Forward (Fade Out)
├─ Play Speed: 1.0
└─ bInvert   : False
    │
    ▼ (Completed)
[Any Processing: Actor spawning, UI switching, etc.]
    │
    ▼
Play Transition And Wait
├─ Preset    : DA_FadeToBlack
├─ Mode      : Reverse (Fade In)
├─ Play Speed: 1.0
└─ bInvert   : False
    │
    ▼ (Completed)
[Post-Processing]
```

### When Specifying Playback Time in Seconds

If you want to specify an exact number of seconds instead of `Play Speed`, use **`Play Transition And Wait With Duration`**.

```
Play Transition And Wait With Duration
├─ Preset   : DA_FadeToBlack
├─ Mode     : Forward
└─ Duration : 2.0
```

### When Using Random Effects

If you want to play a random effect from multiple presets, use **`Play Random Transition And Wait`**.

```
Play Random Transition And Wait
├─ Presets   : [DA_Iris, DA_Diamond, DA_Spiral]
├─ Mode      : Forward
└─ Play Speed: 1.0
```

### When Making Level Transitions Seamless

![Open Level With Transition And Wait node](docs/images/quickstart_open_level_bp.png)

If you want to complete Fade Out → Open Level → Fade In using a single node, use **`Open Level With Transition And Wait`**.

```
Open Level With Transition And Wait
├─ Level Name: MainLevel
├─ Preset    : DA_FadeToBlack
└─ Duration  : 1.0
    │
    ▼ (Completed)
[Write any post-processing in the caller, if necessary]
```

> **⚠️ Warning**
> The `Completed` pin fires **immediately after the fade-out completes and `OpenLevel` is called**.
> It does not wait for the loading of the new level to finish. The fade-in on the new level side is automatically started via `PostLoadMapWithWorld`.

### When You Want to Fade Quickly Without a Preset

If you want to easily fade without creating a DataAsset, you can use **`Quick Fade To Black`** / **`Quick Fade From Black`**.

```
Quick Fade To Black   ── Duration: 1.0
Quick Fade From Black ── Duration: 1.0
```

> **💡 Tip**
> `Quick Fade` internally uses the default `DA_FadeToBlack`.
> If an effect other than a black fade is required, please create a dedicated DataAsset and use `Play Transition And Wait`.

> **⚠️ Warning**
> `Quick Fade To Black` and `Quick Fade From Black` are **fire-and-forget** functions. They start the transition but **do not wait for completion**. If you need to execute logic after the fade finishes, use `Play Transition And Wait` instead, or bind to the `OnTransitionCompleted` delegate.

### When Dynamically Changing Material Parameters

By passing an `FTransitionParameters` struct to the `Override Params` pin, you can overwrite the material parameters of the preset at runtime.

```
[Make TransitionParameters]
├─ Scalar Params : { "Intensity" → 2.0 }
└─ Vector Params : { "Color" → LinearColor(Red) }
    │
    ▼
Play Transition And Wait
└─ Override Params: ↑
```

---

## Step 4: Application to Loading Screens

This is a workflow for when you want to perform **heavy processing that you do not want the player to see**, such as level loading or spawning a large number of actors, while keeping the screen hidden. This can be achieved by combining `bHoldAtMax` and `Release Hold`.

### Mechanism

Normally, a transition completes automatically when progress reaches 1.0, but by setting `bHoldAtMax = true`, you can **pause (hold) it at 1.0 progress (when the screen is completely covered)**. Input remains blocked during the hold.

Once preparations are complete, calling `Release Hold` transitions the Forward transition to the completed state. After that, fade-in is performed by manually calling `Play Transition And Wait` (Mode: Reverse).

### Implementation Workflow

<!-- IMAGE: quickstart_hold_workflow_bp.png - Blueprint workflow showing bHoldAtMax + ReleaseHold loading screen pattern -->

```
[Load Start Event]
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    │
    ▼
Start Transition
├─ Preset     : DA_FadeToBlack
├─ Mode       : Forward
├─ Play Speed : 1.0
└─ bHoldAtMax : True  ← This is the key
    │
    ▼ (OnTransitionHoldStarted fires)
[Background Processing]
├─ Level streaming load
├─ Actor spawning
└─ Data initialization, etc.
    │
    ▼ (Processing complete)
Release Hold
    │
    ▼
Play Transition And Wait
├─ Preset : DA_FadeToBlack
└─ Mode   : Reverse  ← Fade in is called manually
    │
    ▼ (Completed)
```

### Detecting Holds with Events

The timing when the hold state is entered can be retrieved with the `OnTransitionHoldStarted` delegate. Use this when you want to implement it in an event-driven manner rather than through polling.

```
[Pre-bind in BeginPlay, etc.]
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    └─ Bind Event to OnTransitionHoldStarted
        └─ [Custom Event that starts background processing]
```

### C++ Implementation Example

```cpp
// 1. Bind before StartTransition
TransitionSystem->OnTransitionHoldStarted.AddDynamic(this, &UMyClass::OnHoldStarted);

// 2. Start transition with hold
TransitionSystem->StartTransition(FadePreset, ETransitionMode::Forward, 1.0f, false, true);

// 3. Background processing after hold → ReleaseHold → Fade In
void UMyClass::OnHoldStarted()
{
    // Background processing...

    TransitionSystem->ReleaseHold();
    TransitionSystem->StartTransition(FadePreset, ETransitionMode::Reverse, 1.0f, false, false);
}
```

> **⚠️ Warning**
> If you forget to call `Release Hold`, the screen will remain dark forever.
> If it gets stuck by any chance, you can forcefully recover it by executing `TransitionFX.ForceClear` in the console (`@` or `~` key).

---

## Step 5: Debugging Methods

### Stack Symptoms and Causes

| Symptom | Main Cause |
| :--- | :--- |
| Screen remains pitch black and nothing happens | Forgot to call `Release Hold` while `bHoldAtMax = true` |
| Screen is dark and accepts no input | The transition was interrupted while `bAutoBlockInput = true` |
| Transition stops halfway and does not proceed | `Stop Transition` was called unintentionally, or `Play Speed` is extremely low |

### Recovery Method 1: Console Command (For Development / QA)

During game execution, open the console window (`@` or `~` key) and enter the following command:

<!-- IMAGE: quickstart_forceclear_console.png - Screenshot of console with TransitionFX.ForceClear command -->

```
TransitionFX.ForceClear
```

This will reset all of the following:

- Destruction of the current transition effect
- Unblocking of input (CinematicMode)
- Stopping of currently playing sounds
- Resetting of all transition flags (`bIsTransitionActive`, `bIsHolding`, etc.)

> **💡 Tip**
> Be sure to make this command known to QA testers.
> They can recover on their own if they get stuck with a dark screen, which improves the quality of bug reports.

### Recovery Method 2: Integrating `Force Clear` BP Node into Debug UI

In environments where the console cannot be used (consumer builds, debugging on consoles, etc.), it is convenient to set up the `Force Clear` node in the debug UI/inputs.

**Resetting via Specific Key Input**

```
Event [Cheat Key / Debug Input]
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    │
    ▼
Force Clear
```

**Assigning to a Button in a Debug Widget**

```
[Button: "Force Clear Transition"] OnClicked
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    │
    ▼
Force Clear
```

> **⚠️ Warning**
> `Force Clear` immediately destroys the state without waiting for the transition to complete. `OnTransitionCompleted` will not fire.
> If the game logic assumes `OnTransitionCompleted` will fire, you must manually call the subsequent processing after `Force Clear`.

### Difference Between `Force Clear` and `Stop Transition`

| | `Stop Transition` | `Force Clear` |
| :--- | :--- | :--- |
| Effect Post-processing | Cleans up normally | Destroys immediately |
| Unblocking Input | Follows `bAutoBlockInput` setting | Unblocks forcefully |
| Stopping Sound | Stops | Stops |
| Firing `OnTransitionCompleted` | No | No |
| Primary Use | Normal stop processing | Emergency reset / Debugging |

---

## Step 6: Utilizing Events (Delegates)

The `TransitionManagerSubsystem` provides 3 delegates, allowing you to execute game logic corresponding to the timing of transitions.

| Delegate | Firing Timing |
| :--- | :--- |
| `OnTransitionStarted` | Immediately after `StartTransition` is called and the transition has begun |
| `OnTransitionCompleted` | When the transition progress reaches the completion value (Forward: `1.0` / Reverse: `0.0`) |
| `OnTransitionHoldStarted` | When `bHoldAtMax = true` and the progress reaches `1.0`, entering a hold |

### Binding Method

Delegates should be bound **before the transition occurs**, such as in `GameInstance`'s `Init` or a level's `BeginPlay`.

**Blueprint**

```
Event BeginPlay (or GameInstance Init)
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    ├─ Bind Event to OnTransitionStarted     → [Custom Event: On Started]
    ├─ Bind Event to OnTransitionCompleted   → [Custom Event: On Completed]
    └─ Bind Event to OnTransitionHoldStarted → [Custom Event: On Hold]
```

**C++**

```cpp
TransitionSystem->OnTransitionStarted.AddDynamic(this, &UMyClass::OnTransitionStarted);
TransitionSystem->OnTransitionCompleted.AddDynamic(this, &UMyClass::OnTransitionCompleted);
TransitionSystem->OnTransitionHoldStarted.AddDynamic(this, &UMyClass::OnTransitionHoldStarted);
```

### Use Case Examples

**OnTransitionStarted: Hiding HUD exactly when a transition starts**

Used to prevent the HUD from remaining visible during the transition animation.

```
OnTransitionStarted
    │
    ▼
Set Visibility (HUD Widget)
└─ Visibility: Hidden
```

**OnTransitionCompleted: Switching the level's state after a fade-out is complete**

This is equivalent to the `Completed` pin of `Play Transition And Wait` (Mode: Forward), but using a delegate allows you to separate the caller from the processing.

```
OnTransitionCompleted
    │
    ▼
Branch (Check if current game state is "FadingOut")
├─ True  → Spawn New Level Actors / Switch Game State
│               │
│               ▼
│           Play Transition And Wait (Mode: Reverse)
└─ False → Do nothing
```

**OnTransitionHoldStarted: Displaying a loading progress UI during a hold**

This is an example of displaying a loading spinner at the exact moment the screen is covered with `bHoldAtMax = true`.

```
OnTransitionHoldStarted
    │
    ▼
Set Visibility (Loading Spinner)  ── Visible
    │
    ▼
[Start asynchronous loading process]
    │
    ▼ (Loading complete)
Set Visibility (Loading Spinner)  ── Hidden
    │
    ▼
Release Hold
    │
    ▼
Play Transition And Wait (Mode: Reverse)
```

### Notes

> **⚠️ `OnTransitionCompleted` fires upon completion of both Forward and Reverse.**
> If multiple transitions are executed in sequence, you need to use game states, etc. to distinguish which completion you are responding to.

> **⚠️ Always `Remove` bound delegates when you are finished using them.**
> Especially when binding in a Level Blueprint, duplicate bindings can occur each time the level is reloaded. It is recommended to design it so that bindings are done on the `GameInstance` side, or to explicitly unbind them with `Remove Dynamic`.
