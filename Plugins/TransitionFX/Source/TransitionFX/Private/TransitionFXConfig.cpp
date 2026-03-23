// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

/**
 * Defines the exported configuration constants for the TransitionFX plugin,
 * including default asset paths and material parameter names.
 */

#include "TransitionFXConfig.h"

namespace TransitionFXConfig
{
	const TCHAR* const DefaultFadePresetPath = TEXT("/TransitionFX/Data/DA_FadeToBlack.DA_FadeToBlack");

	const FName ProgressParamName(TEXT("Progress"));
	const FName InvertParamName(TEXT("Invert"));
	const FName ColorParamName(TEXT("Color"));
}
