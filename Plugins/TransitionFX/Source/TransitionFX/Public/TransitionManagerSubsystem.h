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

/** Delegate broadcast each tick with the current eased progress value (0.0 to 1.0). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTransitionProgressChanged, float, Progress);

/** Delegate broadcast once when the eased progress crosses a registered threshold. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProgressThresholdReached, float, Threshold);

/** Delegate called when asynchronous preset preloading completes. */
DECLARE_DYNAMIC_DELEGATE(FTransitionPreloadCompleteDelegate);

/** Delegate broadcast when a sequence completes all of its entries (and loops, if any). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSequenceCompleted);

/** Delegate broadcast each time a sequence advances to a new entry (including the first). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSequenceStepChanged, int32, StepIndex);

class APlayerController;
class UAudioComponent;
class UTransitionSequence;

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

	/** Inverts the current transition's mask and replays forward (0 to 1). */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void InvertTransition(bool bAutoComplete = true);

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

	/**
	 * Registers a progress threshold. When the eased progress crosses this value,
	 * OnProgressThresholdReached is broadcast once. Thresholds are reset each time
	 * a new transition starts.
	 *
	 * @param Threshold A value between 0.0 and 1.0.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void AddProgressThreshold(float Threshold);

	/** Removes all registered progress thresholds. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX")
	void ClearProgressThresholds();

	// --- Sequence API (Phase 1 implementation — moves to UTransitionSequencePlayer in future refactor) ---

	/** Starts playing a transition sequence. Stops any currently playing sequence or transition. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX|Sequence")
	void PlaySequence(UTransitionSequence* Sequence);

	/** Stops the currently playing sequence (if any) and the underlying transition. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX|Sequence")
	void StopSequence();

	/** Returns true if a sequence is currently playing. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TransitionFX|Sequence")
	bool IsSequencePlaying() const;

	/** Returns the index of the currently playing entry, or -1 if no sequence is playing. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TransitionFX|Sequence")
	int32 GetCurrentSequenceStep() const;

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

	/** Triggered each tick with the current eased progress (0.0 to 1.0). */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnTransitionProgressChanged OnTransitionProgressChanged;

	/** Triggered once when the eased progress crosses a registered threshold value. */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX")
	FOnProgressThresholdReached OnProgressThresholdReached;

	/** Broadcast when the entire sequence finishes (after all loops if bLoop is set). */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX|Sequence")
	FOnSequenceCompleted OnSequenceCompleted;

	/** Broadcast each time the sequence advances to a new entry (including the first). */
	UPROPERTY(BlueprintAssignable, Category = "TransitionFX|Sequence")
	FOnSequenceStepChanged OnSequenceStepChanged;

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

	/** Whether the current transition's mask is inverted. */
	bool bIsInverted = false;

	/** Whether the transition should hold when it reaches max progress (1.0). */
	bool bShouldHoldAtMax = false;

	/** Whether the transition is currently holding at max progress. */
	bool bIsHolding = false;

	/** Current playback speed multiplier. */
	float CurrentPlaySpeed = 1.0f;

	/** Registered progress threshold values (0.0 to 1.0). */
	TArray<float> ProgressThresholds;

	/** Tracks whether each threshold has already been fired for the current transition. */
	TArray<bool> ThresholdFired;

	/** The eased progress from the previous tick, used for threshold crossing detection. */
	float PreviousEasedProgress = 0.0f;

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

	// --- Sequence State (Phase 1 implementation — TODO: extract into UTransitionSequencePlayer in Phase 2) ---

	/** The currently playing sequence, or null when no sequence is active. */
	UPROPERTY(Transient)
	TObjectPtr<UTransitionSequence> CurrentSequence = nullptr;

	/** Index of the entry currently playing, or -1 when no sequence is active. */
	int32 CurrentSequenceStep = -1;

	/** Number of completed loop iterations (0 == on the first pass). */
	int32 CurrentLoopIteration = 0;

	/** True while a sequence is in progress. */
	bool bIsSequencePlaying = false;

	/**
	 * Scope-limited flag set by StartSequenceStep while dispatching the per-entry
	 * StartTransition call. Lets StartTransition distinguish internal sequence-driven
	 * calls from external callers that should interrupt the sequence.
	 */
	bool bIsDispatchingSequenceStep = false;

	/** Timer handle for DelayAfter between entries. */
	FTimerHandle SequenceDelayTimerHandle;

	/** Begins the entry at StepIndex, or finishes/loops the sequence if out of range. Moves to UTransitionSequencePlayer in future refactor. */
	void StartSequenceStep(int32 StepIndex);

	/** Bound one-shot to OnTransitionCompleted for each entry. Advances to the next step (with optional DelayAfter). Moves to UTransitionSequencePlayer in future refactor. */
	UFUNCTION()
	void OnSequenceStepFinished();

	/** Resets sequence state and broadcasts OnSequenceCompleted. Does not stop the final transition (assumed already finished). Moves to UTransitionSequencePlayer in future refactor. */
	void FinishSequence();
};
