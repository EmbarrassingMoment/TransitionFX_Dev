#include "TransitionManagerSubsystem.h"
#include "TransitionFXConfig.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "TransitionPreset.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "HAL/IConsoleManager.h"
#include "TransitionBlueprintLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TransitionFX.h"

void UTransitionManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("TransitionFX.Clear"),
		TEXT("Forcefully clears any active transition and resets input."),
		FConsoleCommandDelegate::CreateUObject(this, &UTransitionManagerSubsystem::ForceClear),
		ECVF_Default
	);

	// Preload default assets to avoid hitching during gameplay
	GetDefaultFadePreset();
	GetDefaultMasterMaterial();
}

UTransitionPreset* UTransitionManagerSubsystem::GetDefaultFadePreset()
{
	if (!DefaultFadePreset)
	{
		DefaultFadePreset = LoadObject<UTransitionPreset>(nullptr, TransitionFXConfig::DefaultFadePresetPath);
	}
	return DefaultFadePreset;
}

UMaterialInterface* UTransitionManagerSubsystem::GetDefaultMasterMaterial()
{
	if (!DefaultMasterMaterial)
	{
		DefaultMasterMaterial = LoadObject<UMaterialInterface>(nullptr, TransitionFXConfig::DefaultMasterMaterialPath);
	}
	return DefaultMasterMaterial;
}

void UTransitionManagerSubsystem::Deinitialize()
{
	IConsoleManager::Get().UnregisterConsoleObject(TEXT("TransitionFX.Clear"));

	EffectPool.Empty();

	Super::Deinitialize();
}

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
			}
		}
	}
}

bool UTransitionManagerSubsystem::IsTickable() const
{
	return bIsTransitionActive;
}

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
				static const FName ProgressParamName(TEXT("Progress"));
				MID->SetScalarParameterValue(ProgressParamName, 0.0f);

				// Do not store this MID. We want it to be garbage collected immediately.
				// The sole purpose is to force the engine to compile/cache the PSOs (Pipeline State Objects) for this material.
			}
		}
	}
}

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

void UTransitionManagerSubsystem::ForceClear()
{
	UE_LOG(LogTransitionFX, Warning, TEXT("TransitionFX: Force Clear Executed."));

	// Cleanup Effect
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

	// Reset Input
	APlayerController* PC = CachedPlayerController.Get();
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			CachedPlayerController = PC;
		}
	}

	if (PC)
	{
		// Force disable cinematic mode
		PC->SetCinematicMode(false, true, true, true, true);
	}

	// Stop Audio
	if (CurrentAudioComponent)
	{
		if (CurrentAudioComponent->IsPlaying())
		{
			CurrentAudioComponent->Stop();
		}
		CurrentAudioComponent = nullptr;
	}

	// Reset Flags
	bIsTransitionActive = false;
	bHasCompleted = false;
	bAutoStopOnReverseComplete = false;
	bShouldHoldAtMax = false;
	bIsHolding = false;
	CurrentPreset = nullptr;
	CurrentProgress = 0.0f;
}

bool UTransitionManagerSubsystem::IsCurrentTransitionFinished() const
{
	return bHasCompleted;
}

float UTransitionManagerSubsystem::GetCurrentProgress() const
{
	return CurrentProgress;
}

bool UTransitionManagerSubsystem::IsTickableWhenPaused() const
{
	return bIsTransitionActive && CurrentPreset && CurrentPreset->bTickWhenPaused;
}

TStatId UTransitionManagerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTransitionManagerSubsystem, STATGROUP_Tickables);
}

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
	if (CurrentAudioComponent)
	{
		if (CurrentAudioComponent->IsPlaying())
		{
			CurrentAudioComponent->Stop();
		}
		CurrentAudioComponent = nullptr;
	}

	CurrentPreset = Preset;
	CurrentMode = Mode;
	CurrentPlaySpeed = FMath::Max(0.01f, PlaySpeed);
	bShouldHoldAtMax = bHoldAtMax;
	bIsHolding = false;
	bIsTransitionActive = true;
	bAutoStopOnReverseComplete = true;
	bHasCompleted = false;

	if (CurrentMode == ETransitionMode::Forward)
	{
		CurrentProgress = 0.0f;
	}
	else
	{
		CurrentProgress = 1.0f;
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
		// Refresh cache if invalid (e.g. level change, or first run)
		if (!CachedPlayerController.IsValid())
		{
			CachedPlayerController = UGameplayStatics::GetPlayerController(this, 0);
		}

		if (APlayerController* PC = CachedPlayerController.Get())
		{
			PC->SetCinematicMode(true, true, true, true, true);
		}
	}

	OnTransitionStarted.Broadcast();
}

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

void UTransitionManagerSubsystem::SetPlaySpeed(float NewSpeed)
{
	CurrentPlaySpeed = FMath::Max(0.01f, NewSpeed);
}

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

void UTransitionManagerSubsystem::StopTransition()
{
	if (!bIsTransitionActive)
	{
		return;
	}

	// Stop Audio
	if (CurrentAudioComponent)
	{
		if (CurrentAudioComponent->IsPlaying())
		{
			CurrentAudioComponent->Stop();
		}
		CurrentAudioComponent = nullptr;
	}

	// Cleanup Effect
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
	bShouldHoldAtMax = false;
	bIsHolding = false;
	CurrentPreset = nullptr;
}

bool UTransitionManagerSubsystem::IsTransitionPlaying() const
{
	return bIsTransitionActive;
}
