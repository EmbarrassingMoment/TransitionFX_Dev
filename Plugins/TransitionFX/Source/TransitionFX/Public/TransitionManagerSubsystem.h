#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TransitionPreset.h"
#include "TransitionManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionHalfway);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionCompleted);

/**
 * The core logic manager for transitions.
 */
UCLASS()
class TRANSITIONFX_API UTransitionManagerSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End USubsystem Interface

	// Begin FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	// End FTickableGameObject Interface

	/** Starts a transition with the given preset. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void StartTransition(UTransitionPreset* Preset);

	/** Stops the current transition immediately. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void StopTransition();

	/** Returns true if a transition is currently playing. */
	UFUNCTION(BlueprintPure, Category = "TransitionFX")
	bool IsTransitionPlaying() const;

	/** Delegate triggered when a transition starts. */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionStarted OnTransitionStarted;

	/** Delegate triggered when a transition reaches the halfway threshold. */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionHalfway OnTransitionHalfway;

	/** Delegate triggered when a transition completes. */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionCompleted OnTransitionCompleted;

private:
	/** The current active preset. */
	UPROPERTY()
	TObjectPtr<UTransitionPreset> CurrentPreset;

	/** The instantiated effect object. */
	UPROPERTY()
	TObjectPtr<UObject> CurrentEffectObject;

	/** Interface pointer to the current effect. Not a UPROPERTY as it's just a cast of CurrentEffectObject. */
	ITransitionEffect* CurrentEffectInterface;

	/** Current progress of the transition (0.0 to 1.0 based on duration). */
	float CurrentProgress;

	/** Current time elapsed since start of transition. */
	float CurrentTime;

	/** Whether the transition is currently active. */
	bool bIsTransiting;

	/** Whether we have already triggered the halfway delegate for the current transition. */
	bool bHalfwayTriggered;

	/** Helper to handle input blocking. */
	void SetInputBlocking(bool bBlock);
};
