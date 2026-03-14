# Missing Data Asset Presets

The following 15 transition effects have materials and material instances but lack Data Asset presets (`UTransitionPreset`).
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

## Missing Presets

| Data Asset Name | TransitionMaterial (Material Instance) |
|---|---|
| `DA_Blinds` | `MI_Transition_Blinds` |
| `DA_Box` | `MI_Transition_Box` |
| `DA_Diamond` | `MI_Transition_Diamond` |
| `DA_FlowerIris` | `MI_Transition_FlowerIris` |
| `DA_Iris` | `MI_Transition_Iris` |
| `DA_Pixelate` | `MI_Transition_Pixelate` |
| `DA_PolkaDots` | `MI_Transition_PolkaDots` |
| `DA_RadialWipe` | `MI_Transition_RadialWipe` |
| `DA_RandomTiles` | `MI_Transition_RandomTiles` |
| `DA_Split` | `MI_Transition_Split` |
| `DA_Spiral` | `MI_Transition_Spiral` |
| `DA_TVSwitchOff` | `MI_Transition_TVSwitchOff` |
| `DA_TextureMask` | `MI_Transition_TextureMask` |
| `DA_WavyCurtain` | `MI_Transition_WavyCurtain` |
| `DA_ZoomWipe` | `MI_Transition_ZoomWipe` |

## Existing Presets (9)

- `DA_CheckerBoard` → `MI_Transition_Checkerboard`
- `DA_CrossWipe` → `MI_Transition_CrossWipe`
- `DA_Fade` → `MI_Transition_Fade`
- `DA_FadeToBlack` → `MI_Transition_Fade`
- `DA_Heart` → `MI_Transition_Heart`
- `DA_Hexagon` → `MI_Transition_Hexagon`
- `DA_LinearWipe` → `MI_Transition_LinearWipe`
- `DA_Tiles` → `MI_Transition_Tiles`
- `DA_Wind` → `MI_Transition_Wind`

## Notes

- `DA_TextureMask` requires a texture parameter — set a default mask texture in `TextureParams` if available.
- After creating all presets, run `PreloadTransitionPresets()` in your game's initialization to warm up shaders.
