# TransitionFX

> **Note:** 🚧 **Work In Progress** 🚧
> This project is currently under active development.

**Status:** 🚧 Under Development

## Description
TransitionFX is a lightweight and advanced procedural screen transition system for Unreal Engine 5.
It renders high-quality transitions based on SDF (Signed Distance Field) math without using textures, and can be implemented from Blueprints with just a single node.

## Features
*   **UE 5.5+ Native:** Optimized for the latest Unreal Engine features.
*   **Procedural Rendering:** Texture-less SDF-based rendering ensures no degradation at any resolution and automatically corrects aspect ratio distortion.
*   **Design-First Workflow:**
    *   **Data Asset Driven:** Manage transition patterns, duration, curves, and sounds as reusable "Presets".
    *   **Auto Input Blocking:** Automatically handles player input blocking during transitions.
    *   **Pause Support:** Works smoothly even when the game is paused.
*   **Versatile Control:**
    *   **Forward / Reverse:** Control "Fade Out" and "Fade In" with a single preset using Transition Modes.
    *   **Speed Control:** Dynamic playback speed adjustment.
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
*   **bAutoBlockInput:** Set to `True`.

### 2. Call from Blueprint
Use the `Play Transition And Wait` node in your Level Blueprint or GameInstance.

*   **Fade Out (Forward):**
    `Play Transition And Wait` (Preset: `DA_FadeBlack`, Mode: `Forward`, Speed: `1.0`)
    *(Screen stays black after completion)*

*   **Fade In (Reverse):**
    `Play Transition And Wait` (Preset: `DA_FadeBlack`, Mode: `Reverse`, Speed: `1.0`)
    *(Effect is removed automatically upon completion)*

## Built-in Effects
*   **Fade:** Simple opacity fade.
*   **Iris:** Circular wipe closing toward the center (Aspect Ratio Corrected).
*   **Diamond:** Diamond-shaped wipe closing toward the center (Aspect Ratio Corrected).
*   *(More coming soon...)*

## License
MIT License
