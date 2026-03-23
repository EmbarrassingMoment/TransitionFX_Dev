# Missing Documentation Images

All documentation files contain `<!-- IMAGE: ... -->` placeholder comments for images that need to be captured.
These images should be placed in `docs/images/` and the HTML comments should be replaced with markdown image syntax.

## Progress: 47 / 51 images captured (92%)

- Effect Preview GIFs: **24/24** complete
- README Hero & Overview: **2/3**
- Easing Preview GIFs: **12/12** complete
- Quick Start Guide: **5/8**
- API Reference: **4/4**

---

## Still Missing Images

### README (EN/JP) — Hero & Overview
| Filename | Description | Status |
|---|---|---|
| `hero_banner.gif` | Montage/GIF showing multiple transition effects in action | Missing |
| `install_enable_plugin.png` | Plugins window with TransitionFX enabled | Done |
| `performance_preload_bp.png` | PreloadTransitionPresets node in GameInstance Init Blueprint | Done |

### README (EN/JP) — Easing Preview GIFs
| Filename | Description | Status |
|---|---|---|
| `easing_linear.gif` | Iris with Linear easing | Done |
| `easing_ease_in_sine.gif` | Iris with EaseInSine easing | Done |
| `easing_ease_out_sine.gif` | Iris with EaseOutSine easing | Done |
| `easing_ease_in_out_sine.gif` | Iris with EaseInOutSine easing | Done |
| `easing_ease_in_cubic.gif` | Iris with EaseInCubic easing | Done |
| `easing_ease_out_cubic.gif` | Iris with EaseOutCubic easing | Done |
| `easing_ease_in_out_cubic.gif` | Iris with EaseInOutCubic easing | Done |
| `easing_ease_in_expo.gif` | Iris with EaseInExpo easing | Done |
| `easing_ease_out_expo.gif` | Iris with EaseOutExpo easing | Done |
| `easing_ease_in_out_expo.gif` | Iris with EaseInOutExpo easing | Done |
| `easing_ease_out_elastic.gif` | Iris with EaseOutElastic easing | Done |
| `easing_ease_out_bounce.gif` | Iris with EaseOutBounce easing | Done |

### Quick Start Guide (EN/JP)
| Filename | Description | Status |
|---|---|---|
| `quickstart_create_data_asset.png` | Content Browser — Data Asset creation flow | Missing |
| `quickstart_preset_settings.png` | TransitionPreset detail panel showing all properties | Done |
| `quickstart_bp_play_node.png` | Play Transition And Wait Blueprint node | Done |
| `quickstart_preload_bp.png` | PreloadTransitionPresets node in GameInstance Init | Done |
| `quickstart_fadeout_fadein_bp.png` | Fade Out → Processing → Fade In Blueprint graph | Done |
| `quickstart_open_level_bp.png` | Open Level With Transition And Wait node | Done |
| `quickstart_hold_workflow_bp.png` | bHoldAtMax + ReleaseHold loading screen Blueprint workflow | Missing |
| `quickstart_forceclear_console.png` | Console with TransitionFX.ForceClear command | Missing |

### API Reference (EN/JP)
| Filename | Description | Status |
|---|---|---|
| `api_play_transition_node.png` | Play Transition And Wait latent action node | Done |
| `api_open_level_node.png` | Open Level With Transition node | Done |
| `api_quick_fade_node.png` | Quick Fade To Black / Quick Fade From Black nodes | Done |
| `api_preload_node.png` | Preload Transition Presets node | Done |

---

## Captured Images (Complete)

### Effect Preview GIFs — 24/24 done
| Filename | Effect | Status |
|---|---|---|
| `effect_fade.gif` | Fade | Done |
| `effect_iris.gif` | Iris | Done |
| `effect_heart_iris.gif` | Heart Iris | Done |
| `effect_flower_iris.gif` | Flower Iris | Done |
| `effect_diamond.gif` | Diamond | Done |
| `effect_box.gif` | Box | Done |
| `effect_linear_wipe.gif` | Linear Wipe | Done |
| `effect_split.gif` | Split | Done |
| `effect_wavy_curtain.gif` | Wavy Curtain | Done |
| `effect_radial_wipe.gif` | Radial Wipe | Done |
| `effect_tiles.gif` | Tiles | Done |
| `effect_polka_dots.gif` | Polka Dots | Done |
| `effect_blinds.gif` | Blinds | Done |
| `effect_spiral.gif` | Spiral | Done |
| `effect_random_tiles.gif` | Random Tiles | Done |
| `effect_wind.gif` | Wind | Done |
| `effect_cross_wipe.gif` | Cross Wipe | Done |
| `effect_zoom_wipe.gif` | Zoom Wipe | Done |
| `effect_texture_mask.gif` | Texture Mask | Done |
| `effect_triangle.gif` | Triangle | Done |
| `effect_tv_switch_off.gif` | TV Switch Off | Done |
| `effect_hexagon.gif` | Hexagon | Done |
| `effect_checkerboard.gif` | Checkerboard | Done |
| `effect_pixelate.gif` | Pixelate | Done |

---

## How to Replace Placeholders

After capturing images, replace each HTML comment with markdown syntax:

```markdown
<!-- IMAGE: effect_fade.gif -->
↓
![Fade Effect](docs/images/effect_fade.gif)
```

## Capture Tips

- Use **Unreal Editor's High Resolution Screenshot** tool or **Movie Render Queue** for GIFs
- Keep GIFs under 5MB each (use optimization tools like `gifsicle`)
- Use consistent resolution (e.g., 640x360 or 800x450)
- Record at 30 FPS for smooth playback
- Ensure dark background for contrast
