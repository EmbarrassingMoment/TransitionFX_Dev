// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionManagerSubsystem.h"
#include "TransitionFXConfig.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "TransitionPreset.h"
#include "TransitionSequence.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"
#include "TransitionBlueprintLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TransitionFX.h"

/** Registers console commands, preloads default assets, and binds the post-load-map delegate. */
void UTransitionManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("TransitionFX.ForceClear"),
		TEXT("Forcefully clears any active transition and resets input."),
		FConsoleCommandDelegate::CreateUObject(this, &UTransitionManagerSubsystem::ForceClear),
		ECVF_Default
	);

	// Preload default assets to avoid hitching during gameplay
	GetDefaultFadePreset();

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UTransitionManagerSubsystem::OnPostLoadMapWithWorld);
}

/** Lazily loads and caches the default Fade to Black preset from the configured asset path. */
UTransitionPreset* UTransitionManagerSubsystem::GetDefaultFadePreset()
{
	if (!DefaultFadePreset)
	{
		DefaultFadePreset = LoadObject<UTransitionPreset>(nullptr, TransitionFXConfig::DefaultFadePresetPath);
	}
	return DefaultFadePreset;
}

/** Unregisters console commands, removes delegates, and empties the effect pool. */
void UTransitionManagerSubsystem::Deinitialize()
{
	IConsoleManager::Get().UnregisterConsoleObject(TEXT("TransitionFX.ForceClear"));

	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	// Clear any pending sequence timer and state before releasing the pool
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceDelayTimerHandle);
	}
	OnTransitionCompleted.RemoveDynamic(this, &UTransitionManagerSubsystem::OnSequenceStepFinished);
	CurrentSequence = nullptr;
	CurrentSequenceStep = -1;
	CurrentLoopIteration = 0;
	bIsSequencePlaying = false;

	// Clean up any active transition before releasing the pool
	ForceClear();

	EffectPool.Empty();

	Super::Deinitialize();
}

/**
 * Advances the active transition's progress based on delta time, applies easing,
 * and handles completion, hold, and reverse logic.
 */
void UTransitionManagerSubsystem::Tick(float DeltaTime)
{
	if (!bIsTransitionActive || !CurrentPreset)
	{
		return;
	}

	float Duration = CurrentPreset->DefaultDuration;
	float BaseDelta = (Duration > 0.0f) ? (DeltaTime / Duration) : 1.0f;
	float DeltaProgress = BaseDelta * CurrentPlaySpeed;

	if (CurrentMode == ETransitionMode::Reverse)
	{
		CurrentProgress -= DeltaProgress;
	}
	else
	{
		CurrentProgress += DeltaProgress;
	}

	CurrentProgress = FMath::Clamp(CurrentProgress, 0.0f, 1.0f);
	float RawProgress = CurrentProgress;

	float EasedProgress = UTransitionBlueprintLibrary::ApplyEasing(RawProgress, CurrentPreset->EasingType, CurrentPreset->ProgressCurve);

	if (CurrentEffect)
	{
		CurrentEffect->UpdateProgress(EasedProgress);
	}

	OnTransitionProgressChanged.Broadcast(EasedProgress);

	// Check Progress Thresholds
	for (int32 i = 0; i < ProgressThresholds.Num(); ++i)
	{
		if (!ThresholdFired[i])
		{
			const float T = ProgressThresholds[i];
			const bool bCrossedForward = PreviousEasedProgress < T && EasedProgress >= T;
			const bool bCrossedReverse = PreviousEasedProgress > T && EasedProgress <= T;
			if (bCrossedForward || bCrossedReverse)
			{
				ThresholdFired[i] = true;
				OnProgressThresholdReached.Broadcast(T);
			}
		}
	}
	PreviousEasedProgress = EasedProgress;

	// Check Completion
	if (CurrentMode == ETransitionMode::Reverse)
	{
		if (RawProgress <= 0.0f)
		{
			if (!bHasCompleted)
			{
				bHasCompleted = true;
				OnTransitionCompleted.Broadcast();

				if (bAutoStopOnReverseComplete)
				{
					StopTransition();
				}
			}
		}
	}
	else
	{
		if (RawProgress >= 1.0f)
		{
			if (bShouldHoldAtMax && !bIsHolding && !bHasCompleted)
			{
				bIsHolding = true;
				OnTransitionHoldStarted.Broadcast();
				return;
			}

			if (bIsHolding)
			{
				CurrentProgress = 1.0f;
				return;
			}

			if (!bHasCompleted)
			{
				bHasCompleted = true;
				OnTransitionCompleted.Broadcast();

				// Auto-stop forward transitions that are not holding.
				// Without this, the PostProcessVolume, AudioComponent, and tick remain active
				// even though the transition is visually complete.
				StopTransition();
			}
		}
	}
}

/** Returns true only when a transition is actively playing. */
bool UTransitionManagerSubsystem::IsTickable() const
{
	return bIsTransitionActive;
}

/**
 * Asynchronously streams in soft-referenced presets, then performs synchronous shader warmup.
 * Fires the OnComplete delegate when all presets are loaded and warmed up.
 */
void UTransitionManagerSubsystem::AsyncLoadTransitionPresets(const TArray<TSoftObjectPtr<UTransitionPreset>>& SoftPresets, FTransitionPreloadCompleteDelegate OnComplete)
{
	if (SoftPresets.Num() == 0)
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	// Create Path List
	TArray<FSoftObjectPath> ItemsToStream;
	for (const auto& Ref : SoftPresets)
	{
		ItemsToStream.Add(Ref.ToSoftObjectPath());
	}

	// Get StreamableManager (from AssetManager)
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	// Kick Async Load
	Streamable.RequestAsyncLoad(ItemsToStream, FStreamableDelegate::CreateWeakLambda(this, [this, SoftPresets, OnComplete]()
	{
		// Post-Load Processing
		TArray<UTransitionPreset*> LoadedPresets;
		for (const auto& Ref : SoftPresets)
		{
			if (UTransitionPreset* Preset = Ref.Get())
			{
				LoadedPresets.Add(Preset);
			}
		}

		// Execute Shader Warmup (Synchronous Preload)
		PreloadTransitionPresets(LoadedPresets);

		// Notify Completion
		OnComplete.ExecuteIfBound();
	}));
}

/**
 * Orchestrates a "Fade Out -> Open Level -> Fade In" sequence.
 * Stores level transition state and binds the fade-out completion callback.
 */
void UTransitionManagerSubsystem::OpenLevelWithTransition(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration)
{
	if (!Preset)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("OpenLevelWithTransition: Null Preset provided. Falling back to default fade."));
		Preset = GetDefaultFadePreset();
	}

	PendingLevelName = LevelName;
	PendingPreset = Preset;
	PendingDuration = Duration;
	bAutoReverseOnLevelLoad = true;

	// Ensure we don't have stale bindings
	OnTransitionCompleted.RemoveDynamic(this, &UTransitionManagerSubsystem::OnLevelTransitionFadeOutFinished);
	OnTransitionCompleted.AddDynamic(this, &UTransitionManagerSubsystem::OnLevelTransitionFadeOutFinished);

	float PlaySpeed = TransitionFXConfig::CalculatePlaySpeed(Preset->DefaultDuration, Duration);

	// Start Fade Out (Forward, Invert=False)
	StartTransition(Preset, ETransitionMode::Forward, PlaySpeed, false);
}

/** Stores preset and duration for an auto-reverse transition on the next level load without starting playback. */
void UTransitionManagerSubsystem::PrepareAutoReverseTransition(UTransitionPreset* Preset, float Duration)
{
	PendingPreset = Preset;
	PendingDuration = Duration;
	bAutoReverseOnLevelLoad = true;
}

/** One-shot callback that opens the pending level after the fade-out transition completes. */
void UTransitionManagerSubsystem::OnLevelTransitionFadeOutFinished()
{
	// One-shot callback
	OnTransitionCompleted.RemoveDynamic(this, &UTransitionManagerSubsystem::OnLevelTransitionFadeOutFinished);

	UGameplayStatics::OpenLevel(this, PendingLevelName);
}

/** Called after a new level is loaded. Triggers the auto-reverse fade-in if one was prepared. */
void UTransitionManagerSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (bAutoReverseOnLevelLoad)
	{
		bAutoReverseOnLevelLoad = false;

		// Verify the loaded world matches our current world context
		if (LoadedWorld && LoadedWorld != GetWorld())
		{
			UE_LOG(LogTransitionFX, Warning, TEXT("TransitionFX: OnPostLoadMapWithWorld called with mismatched world. Skipping auto-reverse."));
			return;
		}

		if (PendingPreset)
		{
			float PlaySpeed = TransitionFXConfig::CalculatePlaySpeed(PendingPreset->DefaultDuration, PendingDuration);

			// Start Fade In (Forward, Invert=True to go from Black to Clear if using standard mask behavior)
			StartTransition(PendingPreset, ETransitionMode::Forward, PlaySpeed, true);
		}
	}
}

/**
 * Synchronously creates temporary dynamic material instances for each unique material
 * to force PSO (Pipeline State Object) compilation and shader cache warmup.
 */
void UTransitionManagerSubsystem::PreloadTransitionPresets(const TArray<UTransitionPreset*>& Presets)
{
	if (Presets.IsEmpty())
	{
		return;
	}

	UE_LOG(LogTransitionFX, Log, TEXT("Preloading %d Transition Presets..."), Presets.Num());

	TSet<UMaterialInterface*> ProcessedMaterials;

	for (UTransitionPreset* Preset : Presets)
	{
		if (Preset && Preset->TransitionMaterial)
		{
			bool bIsAlreadyInSet = false;
			ProcessedMaterials.Add(Preset->TransitionMaterial, &bIsAlreadyInSet);

			if (bIsAlreadyInSet)
			{
				continue;
			}

			// Create a temporary Dynamic Material Instance (MID)
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Preset->TransitionMaterial, this);

			if (MID)
			{
				// Set a scalar parameter to ensure the uniform buffer is initialized
				MID->SetScalarParameterValue(TransitionFXConfig::ProgressParamName, 0.0f);

				// Do not store this MID. We want it to be garbage collected immediately.
				// The sole purpose is to force the engine to compile/cache the PSOs (Pipeline State Objects) for this material.
			}
		}
	}
}

/** Returns a used effect object to the pool, capping at MaxPoolSize to prevent memory bloat. */
void UTransitionManagerSubsystem::ReturnEffectToPool(UObject* EffectObj)
{
	if (!EffectObj)
	{
		return;
	}

	FTransitionEffectPool& Pool = EffectPool.FindOrAdd(EffectObj->GetClass());

	// Cap the pool size to prevent memory bloat
	constexpr int32 MaxPoolSize = 3;
	if (Pool.Effects.Num() < MaxPoolSize)
	{
		Pool.Effects.Add(EffectObj);
	}
	else
	{
		// Do nothing. Let the Garbage Collector handle the unreferenced object.
		UE_LOG(LogTransitionFX, Verbose, TEXT("Pool for %s is full. Discarding effect instance for GC."), *EffectObj->GetClass()->GetName());
	}
}

/** Stops the current audio component and releases the reference. */
void UTransitionManagerSubsystem::StopAndClearAudio()
{
	if (CurrentAudioComponent)
	{
		if (CurrentAudioComponent->IsPlaying())
		{
			CurrentAudioComponent->Stop();
		}
		CurrentAudioComponent = nullptr;
	}
}

/** Cleans up the current effect, returns it to the pool, and clears the reference. */
void UTransitionManagerSubsystem::CleanupAndPoolCurrentEffect()
{
	if (CurrentEffect)
	{
		CurrentEffect->Cleanup();

		// Return to pool
		if (UObject* EffectObj = CurrentEffect.GetObject())
		{
			ReturnEffectToPool(EffectObj);
		}

		CurrentEffect = nullptr;
	}
}

/** Returns the cached player controller, refreshing the cache from PlayerIndex 0 if stale. */
APlayerController* UTransitionManagerSubsystem::GetOrCachePlayerController()
{
	APlayerController* PC = CachedPlayerController.Get();
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			CachedPlayerController = PC;
		}
	}
	return PC;
}

/** Emergency cleanup: stops the effect, restores player input, stops audio, and resets all state flags. */
void UTransitionManagerSubsystem::ForceClear()
{
	UE_LOG(LogTransitionFX, Warning, TEXT("TransitionFX: Force Clear Executed."));

	// Tear down any active sequence first so its transient delegate binding is released
	// before we clean up the current effect.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceDelayTimerHandle);
	}
	OnTransitionCompleted.RemoveDynamic(this, &UTransitionManagerSubsystem::OnSequenceStepFinished);
	CurrentSequence = nullptr;
	CurrentSequenceStep = -1;
	CurrentLoopIteration = 0;
	bIsSequencePlaying = false;

	CleanupAndPoolCurrentEffect();

	// Reset Input
	if (APlayerController* PC = GetOrCachePlayerController())
	{
		// Force disable cinematic mode
		PC->SetCinematicMode(false, true, true, true, true);
	}

	StopAndClearAudio();

	// Reset Flags
	bIsTransitionActive = false;
	bHasCompleted = false;
	bAutoStopOnReverseComplete = false;
	bShouldHoldAtMax = false;
	bIsHolding = false;
	bIsInverted = false;
	CurrentPreset = nullptr;
	CurrentProgress = 0.0f;
}

/** Returns true if the completion event has been fired for the current transition. */
bool UTransitionManagerSubsystem::IsCurrentTransitionFinished() const
{
	return bHasCompleted;
}

/** Returns the current raw (pre-easing) progress value in the range [0.0, 1.0]. */
float UTransitionManagerSubsystem::GetCurrentProgress() const
{
	return CurrentProgress;
}

/** Returns true if the active transition's preset allows ticking while paused. */
bool UTransitionManagerSubsystem::IsTickableWhenPaused() const
{
	return bIsTransitionActive && CurrentPreset && CurrentPreset->bTickWhenPaused;
}

/** Returns the stat ID used for profiling this tickable object. */
TStatId UTransitionManagerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTransitionManagerSubsystem, STATGROUP_Tickables);
}

/**
 * Begins a new transition: stops any active one, creates or reuses an effect from the pool,
 * initializes audio, blocks input if configured, and broadcasts OnTransitionStarted.
 */
void UTransitionManagerSubsystem::StartTransition(UTransitionPreset* Preset, ETransitionMode Mode, float PlaySpeed, bool bInvert, bool bHoldAtMax, FTransitionParameters OverrideParams)
{
	if (!Preset || !Preset->TransitionMaterial)
	{
		UE_LOG(LogTransitionFX, Error, TEXT("TransitionFX: Invalid Preset or Material is missing!"));
		ForceClear();
		return;
	}

	// Stop any existing transition
	if (bIsTransitionActive)
	{
		StopTransition();
	}

	// Ensure previous audio is stopped
	StopAndClearAudio();

	CurrentPreset = Preset;
	CurrentMode = Mode;
	CurrentPlaySpeed = FMath::Max(0.01f, PlaySpeed);
	bShouldHoldAtMax = bHoldAtMax;
	bIsHolding = false;
	bIsInverted = bInvert;
	bIsTransitionActive = true;
	bAutoStopOnReverseComplete = true;
	bHasCompleted = false;

	if (CurrentMode == ETransitionMode::Forward)
	{
		CurrentProgress = 0.0f;
		PreviousEasedProgress = 0.0f;
	}
	else
	{
		CurrentProgress = 1.0f;
		PreviousEasedProgress = 1.0f;
	}

	// Reset threshold fired flags
	for (int32 i = 0; i < ThresholdFired.Num(); ++i)
	{
		ThresholdFired[i] = false;
	}

	// Play Sound
	if (Preset->TransitionSound)
	{
		CurrentAudioComponent = UGameplayStatics::SpawnSound2D(
			this,
			Preset->TransitionSound,
			Preset->SoundVolume,
			Preset->SoundPitch
		);

		if (!CurrentAudioComponent)
		{
			UE_LOG(LogTransitionFX, Warning, TEXT("TransitionFX: Failed to spawn transition sound for Preset '%s'."), *Preset->GetName());
		}
	}

	// Create Effect
	if (Preset->EffectClass)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			UObject* EffectObj = nullptr;

			// Check Pool (Reuse existing instance to avoid allocation and reduce GC pressure)
			FTransitionEffectPool& Pool = EffectPool.FindOrAdd(Preset->EffectClass);
			if (Pool.Effects.Num() > 0)
			{
				EffectObj = Pool.Effects.Pop();
			}

			// Create New if not found
			if (!EffectObj)
			{
				EffectObj = NewObject<UObject>(this, Preset->EffectClass);
			}

			if (EffectObj && EffectObj->Implements<UTransitionEffect>())
			{
				CurrentEffect = EffectObj;
				CurrentEffect->Initialize(World, Preset);
				CurrentEffect->SetInvert(bInvert);
				CurrentEffect->SetParameters(OverrideParams);

				if (CurrentMode == ETransitionMode::Reverse)
				{
					CurrentEffect->UpdateProgress(1.0f);
				}
			}
			else
			{
				UE_LOG(LogTransitionFX, Error, TEXT("Failed to create or retrieve transition effect instance."));
			}
		}
	}

	// Block Input
	if (CurrentPreset->bAutoBlockInput)
	{
		if (APlayerController* PC = GetOrCachePlayerController())
		{
			PC->SetCinematicMode(true, true, true, true, true);
		}
	}

	OnTransitionStarted.Broadcast();
}

/** Releases a held transition, allowing it to proceed to completion. */
void UTransitionManagerSubsystem::ReleaseHold()
{
	if (bIsHolding)
	{
		bIsHolding = false;
		bShouldHoldAtMax = false;
	}
	else
	{
		// Even if not currently holding, ensure we don't hold in the future for this transition
		bShouldHoldAtMax = false;
	}
}

/** Updates the playback speed multiplier, clamped to a minimum of 0.01. */
void UTransitionManagerSubsystem::SetPlaySpeed(float PlaySpeed)
{
	CurrentPlaySpeed = FMath::Max(0.01f, PlaySpeed);
}

/** Switches the current transition to reverse mode, progressing from 1.0 back to 0.0. */
void UTransitionManagerSubsystem::ReverseTransition(bool bAutoStop)
{
	if (!CurrentPreset)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("ReverseTransition called with null preset."));
		return;
	}

	CurrentMode = ETransitionMode::Reverse;
	bAutoStopOnReverseComplete = bAutoStop;
	bHasCompleted = false;
	bIsTransitionActive = true;
}

/** Inverts the current transition's mask and replays forward (0 to 1). */
void UTransitionManagerSubsystem::InvertTransition(bool bAutoComplete)
{
	if (!CurrentPreset)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("InvertTransition called with null preset."));
		return;
	}

	// Toggle invert state
	bIsInverted = !bIsInverted;
	if (CurrentEffect)
	{
		CurrentEffect->SetInvert(bIsInverted);
	}

	// Reactivate transition in forward mode with inverted mask
	CurrentMode = ETransitionMode::Forward;
	CurrentProgress = 0.0f;
	bShouldHoldAtMax = !bAutoComplete;
	bIsHolding = false;
	bHasCompleted = false;
	bIsTransitionActive = true;
}

/** Stops the active transition: cleans up the effect, stops audio, restores input, and resets state. */
void UTransitionManagerSubsystem::StopTransition()
{
	if (!bIsTransitionActive)
	{
		return;
	}

	StopAndClearAudio();
	CleanupAndPoolCurrentEffect();

	// Restore Input
	if (CurrentPreset && CurrentPreset->bAutoBlockInput)
	{
		// Use cached controller to restore input
		if (APlayerController* PC = CachedPlayerController.Get())
		{
			PC->SetCinematicMode(false, true, true, true, true);
		}
	}

	bIsTransitionActive = false;
	// bHasCompleted preserved — reset by StartTransition() / ReverseTransition() / ForceClear()
	bShouldHoldAtMax = false;
	bIsHolding = false;
	bIsInverted = false;
	CurrentPreset = nullptr;
}

/** Registers a progress threshold value, clamped to [0.0, 1.0]. */
void UTransitionManagerSubsystem::AddProgressThreshold(float Threshold)
{
	ProgressThresholds.Add(FMath::Clamp(Threshold, 0.0f, 1.0f));
	ThresholdFired.Add(false);
}

/** Removes all registered progress thresholds. */
void UTransitionManagerSubsystem::ClearProgressThresholds()
{
	ProgressThresholds.Empty();
	ThresholdFired.Empty();
}

/** Returns true if any transition is currently active. */
bool UTransitionManagerSubsystem::IsTransitionPlaying() const
{
	return bIsTransitionActive;
}

// --- Sequence Implementation (Phase 1 — TODO: extract into UTransitionSequencePlayer in Phase 2) ---

/**
 * Starts playing a transition sequence. Stops any currently playing transition
 * or sequence first. A null or empty sequence is a no-op (with a warning log).
 * Moves to UTransitionSequencePlayer in future refactor.
 */
void UTransitionManagerSubsystem::PlaySequence(UTransitionSequence* Sequence)
{
	if (!Sequence)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("PlaySequence: Null sequence provided."));
		return;
	}

	if (Sequence->Entries.Num() == 0)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("PlaySequence: Sequence '%s' has no entries."), *Sequence->GetName());
		return;
	}

	// Sequences and level transitions are mutually exclusive.
	if (!PendingLevelName.IsNone() || bAutoReverseOnLevelLoad)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("PlaySequence: A level transition is pending. Sequences cannot run during level transitions. Ignoring."));
		return;
	}

	// Stop any existing transition/sequence cleanly.
	if (bIsSequencePlaying)
	{
		StopSequence();
	}
	else if (bIsTransitionActive)
	{
		StopTransition();
	}

	CurrentSequence = Sequence;
	CurrentSequenceStep = -1;
	CurrentLoopIteration = 0;
	bIsSequencePlaying = true;

	StartSequenceStep(0);
}

/**
 * Begins the entry at StepIndex, handles loop wrap-around / completion, and skips
 * null-preset entries with a warning. Moves to UTransitionSequencePlayer in future refactor.
 */
void UTransitionManagerSubsystem::StartSequenceStep(int32 StepIndex)
{
	if (!CurrentSequence || !bIsSequencePlaying)
	{
		return;
	}

	const int32 NumEntries = CurrentSequence->Entries.Num();

	// Loop / completion handling when we run off the end.
	if (StepIndex >= NumEntries)
	{
		if (!CurrentSequence->bLoop)
		{
			FinishSequence();
			return;
		}

		CurrentLoopIteration++;

		const int32 LoopCount = CurrentSequence->LoopCount;
		if (LoopCount > 0 && CurrentLoopIteration > LoopCount)
		{
			FinishSequence();
			return;
		}

		StepIndex = 0;
	}

	const FTransitionSequenceEntry& Entry = CurrentSequence->Entries[StepIndex];

	if (!Entry.Preset)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("PlaySequence: Entry %d has a null preset. Skipping."), StepIndex);
		StartSequenceStep(StepIndex + 1);
		return;
	}

	CurrentSequenceStep = StepIndex;
	OnSequenceStepChanged.Broadcast(StepIndex);

	const float SafeOverride = FMath::Max(0.0f, Entry.DurationOverride);
	const float TargetDuration = (SafeOverride > 0.0f) ? SafeOverride : Entry.Preset->DefaultDuration;
	const float PlaySpeed = TransitionFXConfig::CalculatePlaySpeed(Entry.Preset->DefaultDuration, TargetDuration);

	// One-shot bind: remove any stale binding before adding to prevent duplicates.
	OnTransitionCompleted.RemoveDynamic(this, &UTransitionManagerSubsystem::OnSequenceStepFinished);
	OnTransitionCompleted.AddDynamic(this, &UTransitionManagerSubsystem::OnSequenceStepFinished);

	StartTransition(Entry.Preset, Entry.Mode, PlaySpeed, Entry.bInvert, /*bHoldAtMax=*/false, Entry.OverrideParams);
}

/**
 * Fired when the current entry's transition completes. Advances to the next entry
 * after DelayAfter (if any). Moves to UTransitionSequencePlayer in future refactor.
 */
void UTransitionManagerSubsystem::OnSequenceStepFinished()
{
	// One-shot: always remove immediately to keep sequence step transitions deterministic.
	OnTransitionCompleted.RemoveDynamic(this, &UTransitionManagerSubsystem::OnSequenceStepFinished);

	if (!bIsSequencePlaying || !CurrentSequence)
	{
		return;
	}

	if (!CurrentSequence->Entries.IsValidIndex(CurrentSequenceStep))
	{
		FinishSequence();
		return;
	}

	const float DelayAfter = FMath::Max(0.0f, CurrentSequence->Entries[CurrentSequenceStep].DelayAfter);
	const int32 NextIndex = CurrentSequenceStep + 1;

	UWorld* World = GetWorld();
	FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this, NextIndex]()
	{
		StartSequenceStep(NextIndex);
	});

	// Always defer via the timer manager. Even with DelayAfter==0, we must not start the next
	// transition synchronously from inside OnTransitionCompleted because the caller (Tick)
	// still calls StopTransition() after the broadcast, which would tear down the new one.
	if (!World)
	{
		StartSequenceStep(NextIndex);
		return;
	}

	if (DelayAfter <= 0.0f)
	{
		World->GetTimerManager().SetTimerForNextTick(Delegate);
	}
	else
	{
		World->GetTimerManager().SetTimer(SequenceDelayTimerHandle, Delegate, DelayAfter, false);
	}
}

/**
 * Resets sequence state and broadcasts OnSequenceCompleted. Does NOT stop the
 * final transition — it is assumed to have naturally completed.
 * Moves to UTransitionSequencePlayer in future refactor.
 */
void UTransitionManagerSubsystem::FinishSequence()
{
	OnTransitionCompleted.RemoveDynamic(this, &UTransitionManagerSubsystem::OnSequenceStepFinished);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceDelayTimerHandle);
	}

	CurrentSequence = nullptr;
	CurrentSequenceStep = -1;
	CurrentLoopIteration = 0;
	bIsSequencePlaying = false;

	OnSequenceCompleted.Broadcast();
}

/**
 * Stops the active sequence mid-flight. Does NOT broadcast OnSequenceCompleted
 * (cancellation is not a successful completion). Moves to UTransitionSequencePlayer in future refactor.
 */
void UTransitionManagerSubsystem::StopSequence()
{
	if (!bIsSequencePlaying)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceDelayTimerHandle);
	}

	OnTransitionCompleted.RemoveDynamic(this, &UTransitionManagerSubsystem::OnSequenceStepFinished);

	// Reset sequence state BEFORE stopping the underlying transition so that any
	// re-entrant callbacks see a clean state.
	CurrentSequence = nullptr;
	CurrentSequenceStep = -1;
	CurrentLoopIteration = 0;
	bIsSequencePlaying = false;

	if (bIsTransitionActive)
	{
		StopTransition();
	}
}

/** Returns true while a sequence is in progress. Moves to UTransitionSequencePlayer in future refactor. */
bool UTransitionManagerSubsystem::IsSequencePlaying() const
{
	return bIsSequencePlaying;
}

/** Returns the index of the currently playing entry, or -1. Moves to UTransitionSequencePlayer in future refactor. */
int32 UTransitionManagerSubsystem::GetCurrentSequenceStep() const
{
	return bIsSequencePlaying ? CurrentSequenceStep : -1;
}
