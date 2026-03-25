// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TransitionPreset.h"
#include "ITransitionEffect.h"
#include "TransitionManagerSubsystem.generated.h"

/** Delegate broadcast when a transition begins playing. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionStarted);

/** Delegate broadcast when a transition finishes (forward reaches 1.0 or reverse reaches 0.0). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionCompleted);

/** Delegate broadcast when a transition enters the hold state at max progress. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionHoldStarted);

/** Delegate called when asynchronous preset preloading completes. */
DECLARE_DYNAMIC_DELEGATE(FTransitionPreloadCompleteDelegate);

class APlayerController;
class UAudioComponent;

/** Pool for transition effects. */
USTRUCT()
struct FTransitionEffectPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UObject>> Effects;
};

/**
 * Direction mode for transition playback.
 */
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

	/** Updates active transition progress each frame. */
	virtual void Tick(float DeltaTime) override;

	/** Returns true when a transition is active and needs ticking. */
	virtual bool IsTickable() const override;

	/** Returns true if the current transition should tick while the game is paused. */
	virtual bool IsTickableWhenPaused() const override;

	/** Returns the stat ID for profiling. */
	virtual TStatId GetStatId() const override;

	// USubsystem Interface

	/** Initializes the subsystem, registers console commands, and preloads default assets. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Cleans up console commands, delegates, and the effect pool. */
	virtual void Deinitialize() override;

	/** Starts a transition with the given preset. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (PlaySpeed = "1.0"))
	void StartTransition(UTransitionPreset* Preset, ETransitionMode Mode = ETransitionMode::Forward, float PlaySpeed = 1.0f, bool bInvert = false, bool bHoldAtMax = false, FTransitionParameters OverrideParams = FTransitionParameters());

	/** Releases the hold at max progress, allowing the transition to complete. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void ReleaseHold();

	/** Sets the playback speed multiplier. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (PlaySpeed = "1.0"))
	void SetPlaySpeed(float PlaySpeed = 1.0f);

	/** Reverses the current transition. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void ReverseTransition(bool bAutoStop = true);

	/** Stops the current transition. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void StopTransition();

	/** Returns true if a transition is currently playing. */
	UFUNCTION(BlueprintPure, Category = "TransitionFX")
	bool IsTransitionPlaying() const;

	/** Returns true if the current transition has finished its hold phase or completed. */
	UFUNCTION(BlueprintPure, Category = "TransitionFX")
	bool IsCurrentTransitionFinished() const;

	/** Returns the current progress of the transition (0.0 to 1.0). */
	UFUNCTION(BlueprintPure, Category = "TransitionFX")
	float GetCurrentProgress() const;

	/** Forcefully clears any active transition and resets input. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void ForceClear();

	/** Returns the default Fade preset (DA_FadeToBlack), loading it if necessary. */
	UTransitionPreset* GetDefaultFadePreset();

	/** Preloads a list of transition presets to warm up shaders. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX|System")
	void PreloadTransitionPresets(const TArray<UTransitionPreset*>& Presets);

	/** Asynchronously loads a list of transition presets and warms up their shaders. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX|System")
	void AsyncLoadTransitionPresets(const TArray<TSoftObjectPtr<UTransitionPreset>>& SoftPresets, FTransitionPreloadCompleteDelegate OnComplete);

	/**
	 * Handles the sequence of "Fade Out -> Open Level -> Fade In".
	 * Persists state across level transitions.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (WorldContext = "WorldContextObject"))
	void OpenLevelWithTransition(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration = 1.0f);

	/**
	 * Prepares the subsystem for an auto-reverse transition on the next level load.
	 * Does NOT start any transition immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void PrepareAutoReverseTransition(UTransitionPreset* Preset, float Duration = 1.0f);

public:
	/** Triggered when a transition starts. */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionStarted OnTransitionStarted;

	/** Triggered when a transition completes. */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionCompleted OnTransitionCompleted;

	/** Triggered when a transition holds at max progress (1.0). */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionHoldStarted OnTransitionHoldStarted;

private:
	/** Stops and releases the current audio component. */
	void StopAndClearAudio();

	/** Cleans up the current effect and returns it to the object pool. */
	void CleanupAndPoolCurrentEffect();

	/** Returns the cached player controller, refreshing the cache if stale. */
	APlayerController* GetOrCachePlayerController();

	/** Internal helper to return an effect to the pool with size checks. */
	void ReturnEffectToPool(UObject* EffectObj);

	/** Pool of available transition effects. */
	UPROPERTY(Transient)
	TMap<UClass*, FTransitionEffectPool> EffectPool;

	/** The current transition preset. */
	UPROPERTY(Transient)
	TObjectPtr<UTransitionPreset> CurrentPreset;

	/** The current active effect instance. */
	UPROPERTY(Transient)
	TScriptInterface<ITransitionEffect> CurrentEffect;

	/** The current active audio component. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentAudioComponent;

	/** Current transition mode. */
	ETransitionMode CurrentMode = ETransitionMode::Forward;

	/** Current normalized progress of the transition (0.0 to 1.0). */
	float CurrentProgress = 0.0f;

	/** Whether a transition is currently active. */
	bool bIsTransitionActive = false;

	/** Whether to automatically stop the transition when reverse completes. */
	bool bAutoStopOnReverseComplete = false;

	/** Whether the completion event has been triggered for the current transition. */
	bool bHasCompleted = false;

	/** Whether the transition should hold when it reaches max progress (1.0). */
	bool bShouldHoldAtMax = false;

	/** Whether the transition is currently holding at max progress. */
	bool bIsHolding = false;

	/** Current playback speed multiplier. */
	float CurrentPlaySpeed = 1.0f;

	/** Cached player controller to avoid redundant lookups. */
	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	/** Cached default fade preset to avoid runtime loading. */
	UPROPERTY(Transient)
	TObjectPtr<UTransitionPreset> DefaultFadePreset;

	// Level Transition State

	/** Whether to automatically play a reverse transition when a new level finishes loading. */
	bool bAutoReverseOnLevelLoad = false;

	/** The name of the level to open after the fade-out completes. */
	FName PendingLevelName;

	/** The duration to use for the pending level transition. */
	float PendingDuration = 1.0f;

	/** The preset to use for the pending level transition. */
	UPROPERTY()
	TObjectPtr<UTransitionPreset> PendingPreset;

	/** Callback fired when the fade-out phase of a level transition completes. Opens the pending level. */
	UFUNCTION()
	void OnLevelTransitionFadeOutFinished();

	/** Callback fired after a new level is loaded. Triggers the auto-reverse fade-in if configured. */
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);
};
