// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"

/**
 * Configuration constants for the TransitionFX plugin.
 * Contains default asset paths and material parameter names.
 */
namespace TransitionFXConfig
{
	/** Threshold for duration (in seconds) to treat as nearly zero. */
	static constexpr float MinDurationThreshold = 0.001f;

	/** Fallback play speed when duration is near zero. */
	static constexpr float FallbackPlaySpeed = 100.0f;

	/** Default path for the Fade to Black preset data asset. */
	extern TRANSITIONFX_API const TCHAR* const DefaultFadePresetPath;

	/** Material parameter name for transition progress (0.0 to 1.0). */
	extern TRANSITIONFX_API const FName ProgressParamName;

	/** Material parameter name for invert toggle (0.0 or 1.0). */
	extern TRANSITIONFX_API const FName InvertParamName;

	/** Material parameter name for transition color. */
	extern TRANSITIONFX_API const FName ColorParamName;
}
