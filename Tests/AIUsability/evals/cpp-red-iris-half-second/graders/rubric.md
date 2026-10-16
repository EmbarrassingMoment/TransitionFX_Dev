---
type: llm
---

PASS if the code calls `StartTransition` on `UTransitionManagerSubsystem` with arguments in the order (Preset, Mode, PlaySpeed, bInvert, bHoldAtMax, OverrideParams), uses `ETransitionMode::Forward` for the fade-out, derives PlaySpeed from the 0.5 s target rather than passing 0.5 as if it were seconds (either `TransitionFXConfig::CalculatePlaySpeed(IrisPreset->DefaultDuration, 0.5f)` or `IrisPreset->DefaultDuration / 0.5f`), and puts the red colour into `FTransitionParameters::VectorParams` under the key `FadeColor`.

FAIL if 0.5f is passed in the PlaySpeed slot and described as the duration, if the colour is set through a function that does not exist on the subsystem or preset (for example `SetColor`, `SetTransitionColor`, `SetFadeColor`), if a non-existent latent or "WithDuration" overload is invented on the subsystem, or if the argument order of `StartTransition` is wrong.
