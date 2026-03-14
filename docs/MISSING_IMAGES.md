# Missing Documentation Images

All documentation files contain `<!-- IMAGE: ... -->` placeholder comments for images that need to be captured.
These images should be placed in `docs/images/` and the HTML comments should be replaced with markdown image syntax.

## Required Images

### README (EN/JP) — Hero & Overview
| Filename | Description |
|---|---|
| `hero_banner.gif` | Montage/GIF showing multiple transition effects in action |
| `install_enable_plugin.png` | Plugins window with TransitionFX enabled |
| `easing_curves.png` | Chart comparing all easing types (Linear, Sine, Cubic, Expo, Bounce, Elastic) |
| `performance_preload_bp.png` | PreloadTransitionPresets node in GameInstance Init Blueprint |

### Quick Start Guide (EN/JP)
| Filename | Description |
|---|---|
| `quickstart_create_data_asset.png` | Content Browser — Data Asset creation flow |
| `quickstart_preset_settings.png` | TransitionPreset detail panel showing all properties |
| `quickstart_bp_play_node.png` | Play Transition And Wait Blueprint node |
| `quickstart_preload_bp.png` | PreloadTransitionPresets node in GameInstance Init |
| `quickstart_fadeout_fadein_bp.png` | Fade Out → Processing → Fade In Blueprint graph |
| `quickstart_open_level_bp.png` | Open Level With Transition And Wait node |
| `quickstart_hold_workflow_bp.png` | bHoldAtMax + ReleaseHold loading screen Blueprint workflow |
| `quickstart_forceclear_console.png` | Console with TransitionFX.ForceClear command |

### API Reference (EN/JP)
| Filename | Description |
|---|---|
| `api_play_transition_node.png` | Play Transition And Wait latent action node |
| `api_open_level_node.png` | Open Level With Transition node |
| `api_quick_fade_node.png` | Quick Fade To Black / Quick Fade From Black nodes |
| `api_preload_node.png` | Preload Transition Presets node |

### Effect Preview GIFs (23 effects)
| Filename | Effect |
|---|---|
| `effect_fade.gif` | Fade |
| `effect_iris.gif` | Iris |
| `effect_heart_iris.gif` | Heart Iris |
| `effect_flower_iris.gif` | Flower Iris |
| `effect_diamond.gif` | Diamond |
| `effect_box.gif` | Box |
| `effect_linear_wipe.gif` | Linear Wipe |
| `effect_split.gif` | Split |
| `effect_wavy_curtain.gif` | Wavy Curtain |
| `effect_radial_wipe.gif` | Radial Wipe |
| `effect_tiles.gif` | Tiles |
| `effect_polka_dots.gif` | Polka Dots |
| `effect_blinds.gif` | Blinds |
| `effect_spiral.gif` | Spiral |
| `effect_random_tiles.gif` | Random Tiles |
| `effect_wind.gif` | Wind |
| `effect_cross_wipe.gif` | Cross Wipe |
| `effect_zoom_wipe.gif` | Zoom Wipe |
| `effect_texture_mask.gif` | Texture Mask |
| `effect_tv_switch_off.gif` | TV Switch Off |
| `effect_hexagon.gif` | Hexagon |
| `effect_checkerboard.gif` | Checkerboard |
| `effect_pixelate.gif` | Pixelate |

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
