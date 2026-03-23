# Missing Data Asset Presets

The following transition effects have materials and material instances but lack Data Asset presets (`UTransitionPreset`).
These must be created in the Unreal Editor.

## How to Create a Data Asset Preset

1. In the Content Browser, navigate to `Plugins/TransitionFX Content/Data/`
2. Right-click → **Miscellaneous** → **Data Asset**
3. Select `TransitionPreset` as the class
4. Configure with the settings below

## Default Settings for All Presets

| Property | Value |
|---|---|
| EffectClass | `PostProcessTransitionEffect` |
| DefaultDuration | `1.0` |
| EasingType | `Linear` |
| bAutoBlockInput | `true` |
| bTickWhenPaused | `false` |
| Priority | `1000.0` |

## Progress: 24 / 25 data assets created (96%)

- Transition Presets: **24/25** complete

---

## Still Missing Presets

| Data Asset Name | TransitionMaterial (Material Instance) | Status |
|---|---|---|
| `DA_WavyCurtain` | `MI_Transition_WavyCurtain` | Missing |

---

## Created Presets (Complete)

| Data Asset Name | TransitionMaterial (Material Instance) | Status |
|---|---|---|
| `DA_Blinds` | `MI_Transition_Blinds` | Done |
| `DA_Box` | `MI_Transition_Box` | Done |
| `DA_CheckerBoard` | `MI_Transition_Checkerboard` | Done |
| `DA_CrossWipe` | `MI_Transition_CrossWipe` | Done |
| `DA_Diamond` | `MI_Transition_Diamond` | Done |
| `DA_Fade` | `MI_Transition_Fade` | Done |
| `DA_FadeToBlack` | `MI_Transition_Fade` | Done |
| `DA_FlowerIris` | `MI_Transition_FlowerIris` | Done |
| `DA_Heart` | `MI_Transition_Heart` | Done |
| `DA_Hexagon` | `MI_Transition_Hexagon` | Done |
| `DA_Iris` | `MI_Transition_Iris` | Done |
| `DA_LinearWipe` | `MI_Transition_LinearWipe` | Done |
| `DA_Pixelate` | `MI_Transition_Pixelate` | Done |
| `DA_PolkaDots` | `MI_Transition_PolkaDots` | Done |
| `DA_RadialWipe` | `MI_Transition_RadialWipe` | Done |
| `DA_RandomTiles` | `MI_Transition_RandomTiles` | Done |
| `DA_Spiral` | `MI_Transition_Spiral` | Done |
| `DA_Split` | `MI_Transition_Split` | Done |
| `DA_TVSwitchOff` | `MI_Transition_TVSwitchOff` | Done |
| `DA_TextureMask` | `MI_Transition_TextureMask` | Done |
| `DA_Tiles` | `MI_Transition_Tiles` | Done |
| `DA_Triangle` | `MI_Transition_Triangle` | Done |
| `DA_Wind` | `MI_Transition_Wind` | Done |
| `DA_ZoomWipe` | `MI_Transition_ZoomWipe` | Done |

## Notes

- `DA_TextureMask` requires a texture parameter — set a default mask texture in `TextureParams` if available.
- After creating all presets, run `PreloadTransitionPresets()` in your game's initialization to warm up shaders.
