// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
struct FTransitionParameters;

/**
 * Material helpers shared by the transition effect implementations.
 */
namespace TransitionFXMaterialUtils
{
	/**
	 * Applies scalar, vector, and texture parameter overrides to a dynamic material instance.
	 * Parameters that do not exist on the material are skipped with a warning.
	 * @param MID The dynamic material instance to modify. Null is ignored.
	 * @param Params The parameter overrides to apply.
	 */
	TRANSITIONFX_API void ApplyParameters(UMaterialInstanceDynamic* MID, const FTransitionParameters& Params);
}
