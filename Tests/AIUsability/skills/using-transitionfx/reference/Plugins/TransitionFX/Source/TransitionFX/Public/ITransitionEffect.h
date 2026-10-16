// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ITransitionEffect.generated.h"

class UTransitionPreset;
struct FTransitionParameters;

/** UInterface boilerplate for ITransitionEffect. */
UINTERFACE(MinimalAPI, BlueprintType)
class UTransitionEffect : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for transition effects.
 */
class TRANSITIONFX_API ITransitionEffect
{
	GENERATED_BODY()

public:
	/**
	 * Initializes the transition effect.
	 * @param World The world context.
	 * @param Preset The preset containing settings for this effect.
	 */
	virtual void Initialize(UWorld* World, UTransitionPreset* Preset) = 0;

	/**
	 * Updates the progress of the transition.
	 * @param Progress The current progress of the transition (0.0 to 1.0).
	 */
	virtual void UpdateProgress(float Progress) = 0;

	/**
	 * Cleans up the transition effect.
	 */
	virtual void Cleanup() = 0;

	/** Sets whether to invert the transition mask (0=Normal, 1=Inverted). */
	virtual void SetInvert(bool bInvert) = 0;

	/**
	 * Sets the custom parameters for the transition material.
	 * @param Params The parameters to apply.
	 */
	virtual void SetParameters(const FTransitionParameters& Params) = 0;
};
