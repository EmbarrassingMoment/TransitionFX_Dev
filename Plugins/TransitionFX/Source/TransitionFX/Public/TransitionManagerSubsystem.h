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

UENUM(BlueprintType)
enum class ETransitionMode : uint8
{
	Forward UMETA(DisplayName = "Fade Out (0 to 1)"),
	Reverse UMETA(DisplayName = "Fade In (1 to 0)")
};

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
	void StartTransition(UTransitionPreset* Preset, ETransitionMode Mode = ETransitionMode::Forward, float PlaySpeed = 1.0f);

	/** Sets the playback speed multiplier. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void SetPlaySpeed(float NewSpeed);

	/** Reverses the current transition. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void ReverseTransition(bool bAutoStop = true);

	/** Stops the current transition. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void StopTransition();

	/** Returns true if a transition is currently playing. */
	UFUNCTION(BlueprintPure, Category = "Transition")
	bool IsTransitionPlaying() const;

	/** Returns true if the current transition has finished its hold phase or completed. */
	UFUNCTION(BlueprintPure, Category = "Transition")
	bool IsCurrentTransitionFinished() const;

	/** Returns the current progress of the transition (0.0 to 1.0). */
	UFUNCTION(BlueprintPure, Category = "Transition")
	float GetCurrentProgress() const;

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

	/** Current transition mode. */
	ETransitionMode CurrentMode;

	/** Current normalized progress of the transition (0.0 to 1.0). */
	float CurrentProgress;

	/** Whether a transition is currently active. */
	bool bIsTransitionActive;

	/** Whether to automatically stop the transition when reverse completes. */
	bool bAutoStopOnReverseComplete;

	/** Whether the halfway point has been reached for the current transition. */
	bool bHasReachedHalfway;

	/** Whether the completion event has been triggered for the current transition. */
	bool bHasCompleted;

	/** Current playback speed multiplier. */
	float CurrentPlaySpeed;
};
