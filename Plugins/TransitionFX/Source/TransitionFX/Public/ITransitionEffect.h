#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ITransitionEffect.generated.h"

// This class does not need to be modified.
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

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/**
	 * Initialize the transition effect.
	 * @param World The world context.
	 */
	virtual void Initialize(UWorld* World) = 0;

	/**
	 * Update the transition progress.
	 * @param Progress Current progress from 0.0 to 1.0.
	 */
	virtual void UpdateProgress(float Progress) = 0;

	/**
	 * Cleanup the transition effect.
	 */
	virtual void Cleanup() = 0;

	/**
	 * Returns the progress point (0.0-1.0) where the transition is at its peak/halfway.
	 * Default is 0.5f.
	 */
	virtual float GetHalfwayPoint() const { return 0.5f; }
};
