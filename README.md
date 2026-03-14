# TransitionFX

<!-- IMAGE: hero_banner.gif - Hero banner showing multiple transition effects in action (montage/GIF) -->

## Description
TransitionFX is a lightweight and advanced procedural screen transition system for Unreal Engine 5.
It renders high-quality transitions based on SDF (Signed Distance Field) math without using textures, and can be implemented from Blueprints with just a single node.

## Features
*   **UE 5.5+ Native:** Optimized for the latest Unreal Engine features.
*   **Procedural Rendering:** Texture-less SDF-based rendering ensures no degradation at any resolution and automatically corrects aspect ratio distortion.
*   **Design-First Workflow:**
    *   **Data Asset Driven:** Manage transition patterns, duration, and curves as reusable "Presets".
    *   **Auto Input Blocking:** Automatically handles player input blocking during transitions.
    *   **Pause Support:** Works smoothly even when the game is paused.
*   **Versatile Control:**
    *   **Forward / Reverse:** Control "Fade Out" and "Fade In" with a single preset using Transition Modes.
    *   **Speed Control:** Dynamic playback speed adjustment via `SetPlaySpeed`.
*   **Audio Integration:** Synchronize Sound Effects (SFX) with your transitions. The system manages the audio lifecycle, ensuring sounds play on start and stop automatically if the transition is cancelled.
*   **Event System:** Access `OnTransitionStarted`, `OnTransitionCompleted`, and `OnTransitionHoldStarted` delegates for precise gameplay logic timing.
*   **Blueprint Support:** Includes a Latent Action node (`PlayTransitionAndWait`) for clean and easy scripting.

## Installation
1.  Download the plugin from the release page.
2.  Place the `TransitionFX` folder into your project's `Plugins` directory.
3.  Enable `TransitionFX` in the editor plugins window.

<!-- IMAGE: install_enable_plugin.png - Screenshot of the Plugins window with TransitionFX enabled -->

## Quick Start

### 1. Create a Preset
Right-click in Content Browser > `Miscellaneous` > `Data Asset`.
Select the `TransitionPreset` class and name it (e.g., `DA_FadeBlack`).

<!-- IMAGE: quickstart_create_data_asset.png - Screenshot of Content Browser showing Data Asset creation flow -->

*   **Effect Class:** Select `PostProcessTransitionEffect`.
*   **Transition Material:** Select `M_Transition_Master` (or `M_Transition_Iris`, `M_Transition_Diamond`, etc.).
*   **Default Duration:** Set duration in seconds (e.g., `1.0`).
*   **Progress Curve:** (Optional) Set a float curve to control the ease-in/out of the transition.
*   **bAutoBlockInput:** Set to `True` to automatically disable player input during the transition.
*   **bTickWhenPaused:** Set to `True` to allow the transition to play even when the game is paused.
*   **Priority:** Set the rendering priority (default: 1000).
*   **Audio:** (Optional) Assign a Sound asset to play. Includes Volume and Pitch controls.

<!-- IMAGE: quickstart_preset_settings.png - Screenshot of the TransitionPreset detail panel showing all properties -->

### 2. Call from Blueprint
Use the `Play Transition And Wait` node in your Level Blueprint or GameInstance.

<!-- IMAGE: quickstart_bp_play_node.png - Screenshot of the Play Transition And Wait Blueprint node -->

*   **Fade Out (Forward):**
    `Play Transition And Wait` (Preset: `DA_FadeBlack`, Mode: `Forward`, Speed: `1.0`)
    *(Screen stays black after completion)*

*   **Fade In (Reverse):**
    `Play Transition And Wait` (Preset: `DA_FadeBlack`, Mode: `Reverse`, Speed: `1.0`)
    *(Effect is removed automatically upon completion)*

### 3. Events
You can bind to the following events in the `TransitionManagerSubsystem`:
*   **OnTransitionStarted:** Fired when the transition begins.
*   **OnTransitionCompleted:** Fired when the transition finishes.
*   **OnTransitionHoldStarted:** Fired when the transition holds at max progress (1.0) (if `bHoldAtMax` is true).

## API Reference
The `TransitionManagerSubsystem` provides several callable functions for advanced control:

*   **StopTransition():** Instantly stops the current transition.
*   **ReverseTransition(bool bAutoStop):** Reverses the playback direction (e.g., from Fade Out to Fade In).
*   **SetPlaySpeed(float NewSpeed):** Changes the playback speed multiplier dynamically.
*   **GetCurrentProgress():** Returns the current progress (0.0 to 1.0).
*   **IsTransitionPlaying():** Returns true if a transition is currently active.
*   **IsCurrentTransitionFinished():** Returns true if the transition has reached its end state (useful for polling).

## Built-in Effects

| Effect Name | Description | Preview |
| :--- | :--- | :--- |
| **Fade** | Standard opacity fade. Simple and lightweight. | <!-- IMAGE: effect_fade.gif --> |
| **Iris** | Classic circular wipe closing toward the center. Aspect ratio corrected. | <!-- IMAGE: effect_iris.gif --> |
| **Heart Iris** | An iris wipe in the shape of a heart using procedural SDF. | <!-- IMAGE: effect_heart_iris.gif --> |
| **Flower Iris** | An iris wipe in the shape of a flower with rounded petals. The number of petals and the flower's shape (sharpness) are adjustable. | <!-- IMAGE: effect_flower_iris.gif --> |
| **Diamond** | Diamond-shaped wipe closing toward the center. Retro style. | <!-- IMAGE: effect_diamond.gif --> |
| **Box** | A simple square expanding from the center. Basic geometric transition. | <!-- IMAGE: effect_box.gif --> |
| **Linear Wipe** | Directional wipe (adjustable Angle). Accurately covers the screen from edge to edge. | <!-- IMAGE: effect_linear_wipe.gif --> |
| **Split** | A stylish wipe that splits the screen in half from the center and opens outward. Supports adjustable split angles (horizontal, vertical, diagonal). | <!-- IMAGE: effect_split.gif --> |
| **Wavy Curtain** | A directional wipe similar to Linear Wipe, but with an animated wavy boundary like a curtain. | <!-- IMAGE: effect_wavy_curtain.gif --> |
| **Radial Wipe** | Clock-like radial wipe. Supports smooth edges and adjustable start angle. | <!-- IMAGE: effect_radial_wipe.gif --> |
| **Tiles** | The screen is divided into a grid, and blocks expand outward from the center like a wave. | <!-- IMAGE: effect_tiles.gif --> |
| **Polka Dots** | A wave of expanding circles (halftone pattern) covers the screen. Pop and modern look. | <!-- IMAGE: effect_polka_dots.gif --> |
| **Blinds** | Stylish stripe/venetian blind effect. Stripes expand and merge to cover the screen. | <!-- IMAGE: effect_blinds.gif --> |
| **Spiral** | A hypnotic spiral effect that swirls into the center. Supports adjustable rotation spin and start angle. | <!-- IMAGE: effect_spiral.gif --> |
| **Random Tiles** | A stochastic transition where grid tiles appear in a random order using procedural noise. | <!-- IMAGE: effect_random_tiles.gif --> |
| **Wind** | A directional wipe with streak noise, simulating wind blowing the image away. | <!-- IMAGE: effect_wind.gif --> |
| **Cross Wipe** | A cross shape expands from the center, pushing the image into the four corners until it vanishes. | <!-- IMAGE: effect_cross_wipe.gif --> |
| **Zoom Wipe** | A directional wipe that distorts and zooms the scene inward as it fades out. | <!-- IMAGE: effect_zoom_wipe.gif --> |
| **Texture Mask** | Uses a grayscale texture to determine the transition order (Black=Start, White=End). Supports custom mask textures via Parameter Overrides. | <!-- IMAGE: effect_texture_mask.gif --> |
| **TV Switch Off** | A retro CRT TV turn-off effect. Collapses vertically into a line, then horizontally into a point. | <!-- IMAGE: effect_tv_switch_off.gif --> |
| **Hexagon** | A sci-fi style honeycomb wipe. A wave of hexagonal cells smoothly shrinks into their centers. | <!-- IMAGE: effect_hexagon.gif --> |
| **Checkerboard** | A checkerboard pattern that tiles the screen and expands to cover it. Classic retro feel. | <!-- IMAGE: effect_checkerboard.gif --> |
| **Pixelate** | A pixelation effect that progressively reduces the screen resolution until it fades out. | <!-- IMAGE: effect_pixelate.gif --> |

> **Tip for Texture Masks:**
> When importing your mask textures, ensure you uncheck **sRGB** and set Compression Settings to **Masks (no sRGB)** or **Grayscale** for accurate value reading.

## Transition Timing & Easing
Control how the transition progresses over time using the `EasingType` property in your Transition Preset.

| Easing Type | Description |
| :--- | :--- |
| **Linear** | Constant speed (Default). Good for simple fades. |
| **Sine / Cubic / Expo** | Smooth acceleration and deceleration. (In, Out, InOut variants available). |
| **Bounce / Elastic** | Adds a bouncing or elastic effect at the end of the transition. |
| **Custom** | Allows you to supply your own `FloatCurve` asset. |

*Note: The `Transition Curve` slot will only appear when `Custom` is selected.*

<!-- IMAGE: easing_curves.png - Chart comparing all easing types (Linear, Sine, Cubic, Expo, Bounce, Elastic) -->

See [easings.net](https://easings.net/) for visualization of these curves.

## Performance Tips

### Shader Preloading (Warmup)
To prevent frame drops (hitching) when a transition plays for the first time, you can pre-compile the shaders using the Preload API.

**Problem:** Unreal Engine compiles shaders on-demand, which can cause a slight stutter (hitch) the first time a transition effect plays.
**Solution:** The `PreloadTransitionPresets` function creates a temporary dynamic material instance to force the engine to prepare the shaders *before* gameplay starts.

**How to use:**
Call `PreloadTransitionPresets` in a safe place like **GameInstance Init** or **Level BeginPlay**.
Pass an array of your most commonly used Transition Presets to this function.

<!-- IMAGE: performance_preload_bp.png - Blueprint screenshot of PreloadTransitionPresets node in GameInstance Init -->

```cpp
// C++ Example
TArray<UTransitionPreset*> MyPresets = { FadePreset, WipePreset };
TransitionSubsystem->PreloadTransitionPresets(MyPresets);
```
*This creates dummy materials for a single frame to ensure the GPU is ready.*

**API Reference:**
*   **Function:** `TransitionManagerSubsystem->PreloadTransitionPresets(TArray<UTransitionPreset*> Presets)`

### Asynchronous Loading (Soft References)
If you want to load transition assets on-demand (e.g., during a loading screen) to save memory, use the Async API.
It loads the assets in the background, then automatically runs the shader warmup, and finally fires a callback event.

**How to use:**
1. Pass an array of **Soft Object References** to `AsyncLoadTransitionPresets`.
2. The system will load them in the background and warm up the shaders.
3. The `OnComplete` event fires when everything is ready.

**Blueprint Usage:**
Pass an array of Soft Object References. Connect your logic (e.g., Open Level) to the 'On Complete' delegate pin.

```cpp
// C++ Example
TArray<TSoftObjectPtr<UTransitionPreset>> SoftPresets = { ... };

TransitionSubsystem->AsyncLoadTransitionPresets(SoftPresets, FTransitionPreloadCompleteDelegate::CreateLambda([]()
{
    UE_LOG(LogTransitionFX, Log, TEXT("Assets loaded and shaders ready!"));
}));
```

**API Reference:**
*   **Function:** `AsyncLoadTransitionPresets(TArray<TSoftObjectPtr<UTransitionPreset>> Presets, FTransitionPreloadCompleteDelegate OnComplete)`

## License
MIT License
