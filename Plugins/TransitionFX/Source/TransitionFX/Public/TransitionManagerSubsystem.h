#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TransitionPreset.h"
#include "ITransitionEffect.h"
#include "TransitionManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionHalfway);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionCompleted);

/**
 * Manager subsystem for transition effects.
 */
UCLASS()
class TRANSITIONFX_API UTransitionManagerSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual TStatId GetStatId() const override;

	/** Starts a transition with the given preset. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void StartTransition(UTransitionPreset* Preset);

	/** Reverses the current transition (e.g. Fade In). */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void ReverseTransition();

	/** Stops the current transition. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void StopTransition();

	/** Returns true if a transition is currently playing. */
	UFUNCTION(BlueprintPure, Category = "Transition")
	bool IsTransitionPlaying() const;

public:
	/** Triggered when a transition starts. */
	UPROPERTY(BlueprintAssignable, Category = "Transition")
	FOnTransitionStarted OnTransitionStarted;

	/** Triggered when a transition reaches the halfway threshold. */
	UPROPERTY(BlueprintAssignable, Category = "Transition")
	FOnTransitionHalfway OnTransitionHalfway;

	/** Triggered when a transition completes. */
	UPROPERTY(BlueprintAssignable, Category = "Transition")
	FOnTransitionCompleted OnTransitionCompleted;

private:
	/** The current transition preset. */
	UPROPERTY(Transient)
	TObjectPtr<UTransitionPreset> CurrentPreset;

	/** The current active effect instance. */
	UPROPERTY(Transient)
	TScriptInterface<ITransitionEffect> CurrentEffect;

	/** Current progress value (0.0 to 1.0). */
	float CurrentProgressValue;

	/** Whether a transition is currently active. */
	bool bIsTransitionActive;

	/** Whether the halfway point has been reached for the current transition. */
	bool bHasReachedHalfway;

	/** Whether the transition is reversing. */
	bool bIsReversing;

	/** Whether the transition has broadcast completion (for forward playback). */
	bool bHasCompleted;
};
