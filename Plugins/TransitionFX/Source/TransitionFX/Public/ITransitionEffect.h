#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ITransitionEffect.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UITransitionEffect : public UInterface
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
	 * Called to initialize the transition effect.
	 * @param World The world context.
	 */
	virtual void Initialize(UWorld* World) = 0;

	/**
	 * Called every frame to update the transition progress.
	 * @param Progress The current progress of the transition (0.0 to 1.0).
	 */
	virtual void UpdateProgress(float Progress) = 0;

	/**
	 * Called to cleanup the transition effect.
	 */
	virtual void Cleanup() = 0;

	/**
	 * Returns the progress point where the screen is considered fully covered.
	 * @return The halfway point (default 0.5).
	 */
	virtual float GetHalfwayPoint() const
	{
		return 0.5f;
	}
};
