#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ITransitionEffect.h"
#include "TransitionManagerSubsystem.generated.h"

class UTransitionPreset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionHalfway);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionCompleted);

/**
 * Subsystem to manage transition effects.
 */
UCLASS()
class TRANSITIONFX_API UTransitionManagerSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// Begin FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual TStatId GetStatId() const override;
	// End FTickableGameObject Interface

	/**
	 * Starts a transition using the specified preset.
	 * @param Preset The preset defining the transition.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void StartTransition(UTransitionPreset* Preset);

	/**
	 * Stops the current transition immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void StopTransition();

	/**
	 * Checks if a transition is currently playing.
	 * @return True if a transition is active.
	 */
	UFUNCTION(BlueprintPure, Category = "TransitionFX")
	bool IsTransitionPlaying() const;

	/** Called when a transition starts. */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionStarted OnTransitionStarted;

	/** Called when the transition progress passes the halfway threshold. */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionHalfway OnTransitionHalfway;

	/** Called when the transition completes. */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionCompleted OnTransitionCompleted;

private:
	/** The current transition preset being used. */
	UPROPERTY()
	TObjectPtr<UTransitionPreset> CurrentPreset;

	/** The current active transition effect instance. */
	UPROPERTY()
	TScriptInterface<ITransitionEffect> CurrentEffect;

	/** Current elapsed time of the transition. */
	float CurrentTime;

	/** Whether a transition is currently active. */
	bool bIsTransitioning;

	/** Whether the halfway point has been triggered for the current transition. */
	bool bHasTriggeredHalfway;
};
