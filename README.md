# TransitionFX

> **Note:** 🚧 **Work In Progress** 🚧
> This project is currently under active development.

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
*   **Event System:** Access `OnTransitionStarted` and `OnTransitionCompleted` delegates for precise gameplay logic timing.
*   **Blueprint Support:** Includes a Latent Action node (`PlayTransitionAndWait`) for clean and easy scripting.

## Installation
1.  Download the plugin from the release page.
2.  Place the `TransitionFX` folder into your project's `Plugins` directory.
3.  Enable `TransitionFX` in the editor plugins window.

## Quick Start

### 1. Create a Preset
Right-click in Content Browser > `Miscellaneous` > `Data Asset`.
Select the `TransitionPreset` class and name it (e.g., `DA_FadeBlack`).
*   **Effect Class:** Select `PostProcessTransitionEffect`.
*   **Transition Material:** Select `M_Transition_Master` (or `Iris`, `Diamond`).
*   **Default Duration:** Set duration in seconds (e.g., `1.0`).
*   **Progress Curve:** (Optional) Set a float curve to control the ease-in/out of the transition.
*   **bAutoBlockInput:** Set to `True` to automatically disable player input during the transition.
*   **bTickWhenPaused:** Set to `True` to allow the transition to play even when the game is paused.
*   **Priority:** Set the rendering priority (default: 1000).

### 2. Call from Blueprint
Use the `Play Transition And Wait` node in your Level Blueprint or GameInstance.

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
| **Fade** | Standard opacity fade. Simple and lightweight. | ![Fade](https://via.placeholder.com/320x180/000000/FFFFFF?text=Fade) |
| **Iris** | Classic circular wipe closing toward the center. Aspect ratio corrected. | ![Iris](https://via.placeholder.com/320x180/000000/FFFFFF?text=Iris) |
| **Diamond** | Diamond-shaped wipe closing toward the center. Retro style. | ![Diamond](https://via.placeholder.com/320x180/000000/FFFFFF?text=Diamond) |
| **Box** | A simple square expanding from the center. Basic geometric transition. | ![Box](https://via.placeholder.com/320x180/000000/FFFFFF?text=Box) |
| **Linear Wipe** | Directional wipe (adjustable Angle). Accurately covers the screen from edge to edge. | ![Linear](https://via.placeholder.com/320x180/000000/FFFFFF?text=Linear+Wipe) |
| **Radial Wipe** | Clock-like radial wipe. Supports smooth edges and adjustable start angle. | ![Radial](https://via.placeholder.com/320x180/000000/FFFFFF?text=Radial+Wipe) |
| **Box Grid** | The screen is divided into a grid, and blocks expand from the center in a wave pattern. | ![BoxGrid](https://via.placeholder.com/320x180/000000/FFFFFF?text=Box+Grid) |
| **Polka Dots** | A wave of expanding circles (halftone pattern) covers the screen. Pop and modern look. | ![PolkaDots](https://via.placeholder.com/320x180/000000/FFFFFF?text=Polka+Dots) |
| **Blinds** | Stylish stripe/venetian blind effect. Stripes expand and merge to cover the screen. | ![Blinds](https://via.placeholder.com/320x180/000000/FFFFFF?text=Blinds) |
| **Spiral** | A hypnotic spiral effect that swirls into the center. Supports adjustable rotation spin and start angle. | ![Spiral](https://via.placeholder.com/320x180/000000/FFFFFF?text=Spiral) |
| **Random Tiles** | A stochastic transition where grid tiles appear in a random order using procedural noise. | ![RandomTiles](https://via.placeholder.com/320x180/000000/FFFFFF?text=Random+Tiles) |

## License
MIT License
