# Missing Documentation Images

All documentation files contain `<!-- IMAGE: ... -->` placeholder comments for images that need to be captured.
These images should be placed in `docs/images/` and the HTML comments should be replaced with markdown image syntax.

## Progress: 23 / 40 images captured (57%)

- Effect Preview GIFs: **23/24** complete (Triangle missing)
- README Hero & Overview: **0/4**
- Quick Start Guide: **0/8**
- API Reference: **0/4**

---

## Still Missing Images

### README (EN/JP) — Hero & Overview
| Filename | Description | Status |
|---|---|---|
| `hero_banner.gif` | Montage/GIF showing multiple transition effects in action | Missing |
| `install_enable_plugin.png` | Plugins window with TransitionFX enabled | Missing |
| `easing_curves.png` | Chart comparing all easing types (Linear, Sine, Cubic, Expo, Bounce, Elastic) | Missing |
| `performance_preload_bp.png` | PreloadTransitionPresets node in GameInstance Init Blueprint | Missing |

### Quick Start Guide (EN/JP)
| Filename | Description | Status |
|---|---|---|
| `quickstart_create_data_asset.png` | Content Browser — Data Asset creation flow | Missing |
| `quickstart_preset_settings.png` | TransitionPreset detail panel showing all properties | Missing |
| `quickstart_bp_play_node.png` | Play Transition And Wait Blueprint node | Missing |
| `quickstart_preload_bp.png` | PreloadTransitionPresets node in GameInstance Init | Missing |
| `quickstart_fadeout_fadein_bp.png` | Fade Out → Processing → Fade In Blueprint graph | Missing |
| `quickstart_open_level_bp.png` | Open Level With Transition And Wait node | Missing |
| `quickstart_hold_workflow_bp.png` | bHoldAtMax + ReleaseHold loading screen Blueprint workflow | Missing |
| `quickstart_forceclear_console.png` | Console with TransitionFX.ForceClear command | Missing |

### API Reference (EN/JP)
| Filename | Description | Status |
|---|---|---|
| `api_play_transition_node.png` | Play Transition And Wait latent action node | Missing |
| `api_open_level_node.png` | Open Level With Transition node | Missing |
| `api_quick_fade_node.png` | Quick Fade To Black / Quick Fade From Black nodes | Missing |
| `api_preload_node.png` | Preload Transition Presets node | Missing |

### Effect Preview GIFs (missing)
| Filename | Effect | Status |
|---|---|---|
| `effect_triangle.gif` | Triangle | Missing |

---

## Captured Images (Complete)

### Effect Preview GIFs — 23/23 done
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
