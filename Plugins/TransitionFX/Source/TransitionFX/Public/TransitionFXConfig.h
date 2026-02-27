// Copyright Kurorekishi. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace TransitionFXConfig
{
	/** Threshold for duration (in seconds) to treat as nearly zero. */
	static constexpr float MinDurationThreshold = 0.001f;

	/** Fallback play speed when duration is near zero. */
	static constexpr float FallbackPlaySpeed = 100.0f;

	/** Default path for the Fade to Black preset data asset. */
	extern TRANSITIONFX_API const TCHAR* const DefaultFadePresetPath;

	/** Default path for the Master Transition Material. */
	extern TRANSITIONFX_API const TCHAR* const DefaultMasterMaterialPath;

	extern TRANSITIONFX_API const FName ProgressParamName;
	extern TRANSITIONFX_API const FName InvertParamName;
	extern TRANSITIONFX_API const FName ColorParamName;
}
