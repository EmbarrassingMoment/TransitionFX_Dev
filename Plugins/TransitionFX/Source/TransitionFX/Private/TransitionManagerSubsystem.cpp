// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionManagerSubsystem.h"
#include "TransitionFXConfig.h"
#include "TransitionFXSettings.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "TransitionPreset.h"
#include "TransitionSequence.h"
#include "TransitionSequencePlayer.h"
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
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &UTransitionManagerSubsystem::OnWorldCleanup);
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
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);

	// Clear any pending sequence timer and state before releasing the pool
	if (SequencePlayer)
	{
		SequencePlayer->Reset();
	}

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

				// A completion handler may already have started, reversed, or stopped a
				// transition; only finish this one if it is still the completed transition.
				// Defer cleanup to FinishSequence / hot-swap when a sequence is playing,
				// so the effect stays rendering between steps and no background frame is exposed.
				if (bHasCompleted && bIsTransitionActive && bAutoStopOnReverseComplete && !IsSequencePlaying())
				{
					FinishCompletedTransition();
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

				// Finish forward transitions that are not holding: one that ends revealed is
				// stopped, one that ends covering the screen (a fade-out) keeps its effect on
				// screen without ticking. A completion handler may already have started,
				// reversed, or stopped a transition, in which case there is nothing to finish.
				// During a sequence, StartTransition for the next step hot-swaps the effect,
				// and FinishSequence finishes the final step, so skip this here to keep the
				// rendered frame covered (no background flash between steps).
				if (bHasCompleted && bIsTransitionActive && !IsSequencePlaying())
				{
					FinishCompletedTransition();
				}
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
	if (IsSequencePlaying())
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("OpenLevelWithTransition called while a sequence is playing. Cancelling sequence."));
		StopSequence();
	}

	if (!Preset)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("OpenLevelWithTransition: Null Preset provided. Falling back to default fade."));
		Preset = GetDefaultFadePreset();
	}

	// Without a playable preset the fade-out could never start (or complete), so the level
	// would never open. Open it right away instead of failing the level change.
	if (!Preset || !Preset->IsPlayable())
	{
		UE_LOG(LogTransitionFX, Error, TEXT("OpenLevelWithTransition: No playable preset (the default fade preset could not be loaded, or the preset has no TransitionMaterial or EffectClass). Opening '%s' without a transition."), *LevelName.ToString());
		UGameplayStatics::OpenLevel(this, LevelName);
		return;
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

/** Returns a used effect object to the pool, capping at the configured max pool size to prevent memory bloat. */
void UTransitionManagerSubsystem::ReturnEffectToPool(UObject* EffectObj)
{
	if (!EffectObj)
	{
		return;
	}

	FTransitionEffectPool& Pool = EffectPool.FindOrAdd(EffectObj->GetClass());

	// Cap the pool size to prevent memory bloat (configurable in Project Settings > Plugins > TransitionFX)
	const int32 MaxPoolSize = GetDefault<UTransitionFXSettings>()->MaxPoolSizePerEffectClass;
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
	if (SequencePlayer)
	{
		SequencePlayer->Reset();
	}

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
	bIsCoveringScreen = false;
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
	if (!Preset || !Preset->IsPlayable())
	{
		if (!Preset || !Preset->TransitionMaterial)
		{
			UE_LOG(LogTransitionFX, Error, TEXT("TransitionFX: Invalid Preset or Material is missing!"));
		}
		else
		{
			// Without an effect class nothing would be rendered, yet the transition would still
			// tick, block input and fire its events.
			UE_LOG(LogTransitionFX, Error, TEXT("TransitionFX: Preset '%s' has no EffectClass. Set it to PostProcessTransitionEffect or WidgetTransitionEffect."), *Preset->GetName());
		}
		ForceClear();
		return;
	}

	// Apply the preset's default transition color unless the caller already
	// supplied an explicit color override (call-site overrides always win).
	if (Preset->bOverrideTransitionColor && !OverrideParams.VectorParams.Contains(TransitionFXConfig::ColorParamName))
	{
		OverrideParams.VectorParams.Add(TransitionFXConfig::ColorParamName, Preset->TransitionColor);
	}

	// An external StartTransition call interrupts any running sequence.
	// Internal per-step dispatches from the sequence player set its dispatching
	// flag to bypass this guard.
	const bool bIsDispatchingSequenceStep = SequencePlayer && SequencePlayer->IsDispatchingStep();
	if (IsSequencePlaying() && !bIsDispatchingSequenceStep)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("StartTransition called while a sequence is playing. Cancelling sequence."));
		StopSequence();
	}

	// Hot-swap path: during a sequence step dispatch, keep the previous effect
	// rendering until the new one is initialized so no frame exposes the
	// background between steps.
	const bool bHotSwap = bIsDispatchingSequenceStep && bIsTransitionActive && CurrentEffect;

	// Effect object pending cleanup once the new effect is in place (hot-swap only).
	TScriptInterface<ITransitionEffect> PendingOldEffect;

	if (bHotSwap)
	{
		// Audio switches immediately; visual continuity is what matters.
		StopAndClearAudio();
	}
	else
	{
		// Stop any existing transition (tears down the effect volume). This also removes an
		// effect kept covering the screen; the new effect is created in this same frame, so
		// no frame exposes the background.
		if (bIsTransitionActive || bIsCoveringScreen)
		{
			StopTransition();
		}

		// Ensure previous audio is stopped.
		StopAndClearAudio();
	}

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
			// In the hot-swap path, prefer reusing the existing instance when its class
			// matches. Re-running Initialize swaps the DynamicMaterial on the already-spawned
			// PostProcessVolume, producing a seamless frame-to-frame handoff.
			const bool bCanReuseExistingEffect = bHotSwap
				&& CurrentEffect
				&& CurrentEffect.GetObject()
				&& CurrentEffect.GetObject()->GetClass() == Preset->EffectClass;

			if (bCanReuseExistingEffect)
			{
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
				// Hot-swap with differing classes: hold on to the old effect until the new
				// one is fully initialized, then tear down the old one so both rendered
				// simultaneously for a single frame rather than leaving a gap.
				if (bHotSwap)
				{
					PendingOldEffect = CurrentEffect;
					CurrentEffect = nullptr;
				}

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

				// New effect is in place; safe to dispose of the previous one.
				if (PendingOldEffect)
				{
					PendingOldEffect->Cleanup();
					if (UObject* OldEffectObj = PendingOldEffect.GetObject())
					{
						ReturnEffectToPool(OldEffectObj);
					}
				}
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

	// Also resumes a completed transition whose effect is still covering the screen.
	CurrentMode = ETransitionMode::Reverse;
	bAutoStopOnReverseComplete = bAutoStop;
	bHasCompleted = false;
	bIsCoveringScreen = false;
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

		// Apply the restart progress right away. Otherwise, when called after this frame's
		// Tick (e.g. from an OnTransitionCompleted handler), the flipped mask would render
		// for one frame at the old progress (1.0) and briefly expose the scene.
		CurrentEffect->UpdateProgress(UTransitionBlueprintLibrary::ApplyEasing(0.0f, CurrentPreset->EasingType, CurrentPreset->ProgressCurve));
	}

	// Reactivate transition in forward mode with inverted mask
	CurrentMode = ETransitionMode::Forward;
	CurrentProgress = 0.0f;
	bShouldHoldAtMax = !bAutoComplete;
	bIsHolding = false;
	bHasCompleted = false;
	bIsCoveringScreen = false;
	bIsTransitionActive = true;
}

/**
 * Stops the active transition (or removes an effect kept covering the screen): cleans up
 * the effect, stops audio, restores input, and resets state.
 */
void UTransitionManagerSubsystem::StopTransition()
{
	if (!bIsTransitionActive && !bIsCoveringScreen)
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
	bIsCoveringScreen = false;
	bIsInverted = false;
	CurrentPreset = nullptr;
}

/** Keeps a completed transition's effect on screen if it ends covering the screen; otherwise stops it. */
void UTransitionManagerSubsystem::FinishCompletedTransition()
{
	if (!bIsTransitionActive)
	{
		return;
	}

	if (!bHasCompleted || !EndsCoveringScreen())
	{
		StopTransition();
		return;
	}

	// Stopping now would remove the effect and expose the scene right after a fade-out.
	// Stop ticking instead, but keep the effect (at full coverage), the preset, input
	// blocking and the transition sound until the next StartTransition replaces the effect
	// in the same frame, ReverseTransition / InvertTransition resume from it, or
	// StopTransition / ForceClear / level travel remove it.
	bIsTransitionActive = false;
	bIsCoveringScreen = true;
	bShouldHoldAtMax = false;
	bIsHolding = false;
}

/** Forward without Invert ends at full coverage, and so does Reverse with Invert. */
bool UTransitionManagerSubsystem::EndsCoveringScreen() const
{
	return (CurrentMode == ETransitionMode::Forward) != bIsInverted;
}

/**
 * Level travel tears down the world that owns a PostProcess effect's volume and clears all
 * viewport widgets, so a kept covering effect can no longer be shown. Release it here so the
 * effect does not keep references into the old world.
 */
void UTransitionManagerSubsystem::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (bIsCoveringScreen && World && World == GetWorld())
	{
		StopTransition();
	}
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

// --- Sequence API (playback delegated to UTransitionSequencePlayer) ---

/**
 * Starts playing a transition sequence. Validates the sequence, stops any
 * currently playing transition or sequence, then forwards playback to the
 * lazily created UTransitionSequencePlayer. A null or empty sequence is a
 * no-op (with a warning log).
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

	// Sequences and level transitions are mutually exclusive: refuse if a level
	// transition (or auto-reverse plan) is pending. bAutoReverseOnLevelLoad is
	// the authoritative in-flight flag — it is cleared in OnPostLoadMapWithWorld.
	if (bAutoReverseOnLevelLoad)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("PlaySequence: A level transition is pending. Sequences cannot run during level transitions. Ignoring."));
		return;
	}

	// Stop any existing transition/sequence cleanly (including an effect kept covering the screen).
	if (IsSequencePlaying())
	{
		StopSequence();
	}
	else if (bIsTransitionActive || bIsCoveringScreen)
	{
		StopTransition();
	}

	if (!SequencePlayer)
	{
		SequencePlayer = NewObject<UTransitionSequencePlayer>(this);
	}

	SequencePlayer->Play(Sequence);
}

/** Stops the currently playing sequence (if any) and the underlying transition. */
void UTransitionManagerSubsystem::StopSequence()
{
	if (SequencePlayer)
	{
		SequencePlayer->Stop();
	}
}

/** Returns true while a sequence is in progress. */
bool UTransitionManagerSubsystem::IsSequencePlaying() const
{
	return SequencePlayer && SequencePlayer->IsPlaying();
}

/** Returns the index of the currently playing entry, or -1. */
int32 UTransitionManagerSubsystem::GetCurrentSequenceStep() const
{
	return SequencePlayer ? SequencePlayer->GetCurrentStep() : INDEX_NONE;
}
