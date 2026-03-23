# TransitionFX Preview Tool Manual

## 1. Overview

The TransitionFX Preview Tool is a tool for directly previewing the 24 types of SDF-based screen transitions included in the TransitionFX plugin within the editor.

- **No PIE (Play In Editor) required** -- You can instantly check effects within the editor.
- **Ideal for capturing GIFs** -- Designed to be used with external capture tools like ScreenToGif with a fixed-size viewport.
- **Playback control** -- Supports play, reverse, loop, speed adjustment, and manual scrubbing.

---

## 2. How to Open the Tool

Open it from the editor's menu bar:

```
Tools > TransitionFX > TransitionFX Preview
```

It opens as a dockable tab, so you can place it anywhere in your layout.

---

## 3. How to Use the UI

The panel consists of the following sections from top to bottom.

### 3.1 Effect Selection & Invert

| Control | Description |
|---|---|
| **Effect Dropdown** | Selects the transition effect included in the plugin. All material instances in `/TransitionFX/Materials/Instances/` are automatically listed. |
| **Invert Checkbox** | Inverts the transition mask. For example, switches an effect where the "screen is covered in black" to the direction "returning from black". |

### 3.2 Preview Viewport

The transition effect is drawn as a post-process material on a white background. The size can be changed with the **Size** dropdown at the bottom.

### 3.3 Progress Slider

| Control | Description |
|---|---|
| **Progress Slider** | Drag to manually adjust the transition progress (0% to 100%). Auto-playback is paused while dragging. |
| **Numeric Display** | The current progress (0 to 100) is displayed to the right of the slider. |
| **Reset Button** | Resets the progress to 0% and stops playback. |

### 3.4 Playback Controls

| Button | Description |
|---|---|
| **Play** | Starts forward playback. If the progress is at 100%, it restarts from 0%. |
| **Reverse** | Starts reverse playback. If the progress is at 0%, it restarts from 100%. |
| **Stop** | Stops playback. The current progress is maintained. |
| **Loop: ON / OFF** | Toggles looping. When ON, playback repeats in a ping-pong (auto-reverse) manner when reaching the end. |

### 3.5 Speed & Size

| Control | Description |
|---|---|
| **Speed** | The playback speed multiplier. Adjustable from 0.1x to 5.0x in 0.1 increments. Default is 1.0x. |
| **Size** | Selects the viewport resolution. You can choose from `480x270` (default), `640x360`, and `800x450`. |

---

## 4. GIF Capture

### 4.1 Built-in Capture (Recommended)

The Preview Tool has a built-in **Capture GIF** button that automatically records one loop of the transition and saves it as a GIF file.

1. **Select Effect** -- Choose the transition you want to capture from the dropdown.
2. **Select Size** -- Choose the resolution from the Size dropdown.
3. **Click "Capture GIF"** -- The tool will automatically play the transition forward (Progress 0% → 100%) at 30 FPS.
4. **Wait for capture to complete** -- Progress is shown next to the button (e.g., "Capturing... 15/30").
5. **Choose save location** -- A file dialog appears when capture is complete. The default filename is the effect name.

> **Note:** The saved GIF loops infinitely, making it ideal for embedding in README files and documentation. During capture, normal playback controls are disabled.

### 4.2 External Capture (Alternative)

You can also use external capture tools like ScreenToGif:

1. **Select Size** -- Choose the resolution you want to capture from the dropdown.
2. **Select Effect** -- Choose the transition you want to preview from the dropdown.
3. **Launch ScreenToGif** -- Set the capture area to match the viewport area.
4. **Adjust Speed** (Optional) -- Change the playback speed if necessary.
5. **Start Recording in ScreenToGif**
6. **Click Play Button** -- The transition will play.
7. **Stop Recording** -- Save as a GIF.

> **Tip:** Leaving Loop ON makes it easier to start and stop recording at the best timing because the ping-pong playback repeats.

---

## 5. How to Add a New Transition Material

Here are the steps to display your own custom transition materials in the Preview Tool.

### 5.1 Prerequisites

The transition material must work as a **Post Process Material**.

- Set the material's **Material Domain** to `Post Process`.
- Set the material's **Blendable Location** to `After Tonemapping`.

### 5.2 Required Parameters

The material (or its parent material) must have the following **Scalar Parameters**.

| Parameter Name | Type | Value Range | Description |
|---|---|---|---|
| `Progress` | Scalar | 0.0 to 1.0 | Transition progress. 0.0 is the starting state, 1.0 is the completed state. |
| `Invert` | Scalar | 0.0 or 1.0 | Mask inversion flag. Switched in the material using an If node (threshold 0.5). |

### 5.3 Steps to Create a Material Instance

1. **Create a Parent Material** (or use the existing `M_Transition_Master` as a parent)
   - Create a material in `Content/TransitionFX/Materials/`.
   - Material Domain: `Post Process`.
   - Add `Progress` and `Invert` Scalar Parameters.

2. **Create a Material Instance**
   - Create a Material Instance from the parent material.

3. **Rename According to the Naming Convention**

   ```
   MI_Transition_<EffectName>
   ```

   Example: `MI_Transition_MyCustomWipe`

   > It will be displayed in the Preview Tool dropdown with the `MI_Transition_` prefix automatically removed (e.g., `MyCustomWipe`).

4. **Save in the Designated Folder**

   ```
   /TransitionFX/Materials/Instances/
   ```

   The Preview Tool automatically detects `MaterialInstanceConstant` assets in this folder upon startup.

5. **Reopen the Preview Tool** -- The new effect will appear in the dropdown.

### 5.4 Reference List of Existing Materials

The plugin includes the following 24 transition types. Use them as a reference when creating new ones.

| Effect Name | Material Instance |
|---|---|
| Blinds | `MI_Transition_Blinds` |
| Box | `MI_Transition_Box` |
| Checkerboard | `MI_Transition_Checkerboard` |
| CrossWipe | `MI_Transition_CrossWipe` |
| Diamond | `MI_Transition_Diamond` |
| Fade | `MI_Transition_Fade` |
| FlowerIris | `MI_Transition_FlowerIris` |
| Heart | `MI_Transition_Heart` |
| Hexagon | `MI_Transition_Hexagon` |
| Iris | `MI_Transition_Iris` |
| LinearWipe | `MI_Transition_LinearWipe` |
| Pixelate | `MI_Transition_Pixelate` |
| PolkaDots | `MI_Transition_PolkaDots` |
| RadialWipe | `MI_Transition_RadialWipe` |
| RandomTiles | `MI_Transition_RandomTiles` |
| Split | `MI_Transition_Split` |
| Spiral | `MI_Transition_Spiral` |
| TVSwitchOff | `MI_Transition_TVSwitchOff` |
| TextureMask | `MI_Transition_TextureMask` |
| Tiles | `MI_Transition_Tiles` |
| Triangle | `MI_Transition_Triangle` |
| WavyCurtain | `MI_Transition_WavyCurtain` |
| Wind | `MI_Transition_Wind` |
| ZoomWipe | `MI_Transition_ZoomWipe` |

---

## 6. Runtime Usage (Preset Integration)

To use the materials you checked in the Preview Tool as in-game transitions, create a **Transition Preset**.

### 6.1 Creating a Transition Preset

1. Right-click in the Content Browser > Select **Miscellaneous > Data Asset**.
2. Select `TransitionPreset` as the class.
3. Configure the following properties:

| Property | Description |
|---|---|
| `TransitionMaterial` | The material instance to use (e.g., `MI_Transition_MyCustomWipe`) |
| `DefaultDuration` | Transition playback duration (seconds). Default: 1.0 |
| `EasingType` | Type of easing (Linear, EaseInSine, EaseOutBounce, etc.) |
| `Priority` | PostProcess priority. Default: 1000.0 |
| `bAutoBlockInput` | Whether to block player input during the transition |
| `bTickWhenPaused` | Whether to advance the transition even when the game is paused |
| `TransitionSound` | Sound to play when the transition occurs (optional) |

### 6.2 Calling from Blueprint

```
Play Transition And Wait
├── Preset: The created TransitionPreset asset
├── Mode: TransitionIn / TransitionOut / TransitionInOut
├── Speed: Playback speed multiplier (Default: 1.0)
└── Invert: Inversion flag
```

Other useful Blueprint functions:

- `Quick Fade To Black` / `Quick Fade From Black` -- Simple fades
- `Open Level With Transition` -- Switch levels with a transition
- `Is Any Transition Playing` -- Check if a transition is currently playing
