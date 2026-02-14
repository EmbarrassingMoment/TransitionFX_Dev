#pragma once

#include "CoreMinimal.h"

namespace TransitionFXConfig
{
	/** Threshold for duration (in seconds) to treat as nearly zero. */
	static constexpr float MinDurationThreshold = 0.001f;

	/** Fallback play speed when duration is near zero. */
	static constexpr float FallbackPlaySpeed = 100.0f;

	/** Default path for the Fade to Black preset data asset. */
	static const TCHAR* const DefaultFadePresetPath = TEXT("/TransitionFX/Data/DA_FadeToBlack.DA_FadeToBlack");

	/** Default path for the Master Transition Material. */
	static const TCHAR* const DefaultMasterMaterialPath = TEXT("/TransitionFX/Materials/M_Transition_Master.M_Transition_Master");
}
