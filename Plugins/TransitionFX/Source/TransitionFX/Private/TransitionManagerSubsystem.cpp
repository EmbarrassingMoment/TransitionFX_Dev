#include "TransitionManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "HAL/IConsoleManager.h"

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
		DefaultFadePreset = LoadObject<UTransitionPreset>(nullptr, TEXT("/TransitionFX/Data/DA_FadeToBlack.DA_FadeToBlack"));
	}
	return DefaultFadePreset;
}

UMaterialInterface* UTransitionManagerSubsystem::GetDefaultMasterMaterial()
{
	if (!DefaultMasterMaterial)
	{
		DefaultMasterMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/TransitionFX/Materials/M_Transition_Master.M_Transition_Master"));
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

	float EasedProgress = RawProgress;
	if (CurrentPreset->ProgressCurve)
	{
		EasedProgress = CurrentPreset->ProgressCurve->GetFloatValue(RawProgress);
	}

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

				StopTransition();
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

void UTransitionManagerSubsystem::ForceClear()
{
	UE_LOG(LogTemp, Warning, TEXT("TransitionFX: Force Clear Executed."));

	// Cleanup Effect
	if (CurrentEffect)
	{
		CurrentEffect->Cleanup();

		// Return to pool
		if (UObject* EffectObj = CurrentEffect.GetObject())
		{
			FTransitionEffectPool& Pool = EffectPool.FindOrAdd(EffectObj->GetClass());
			Pool.Effects.Add(EffectObj);
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

	// TODO: Reset Audio Volume to 1.0f

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
		UE_LOG(LogTemp, Error, TEXT("TransitionFX: Invalid Preset or Material is missing!"));
		ForceClear();
		return;
	}

	// Stop any existing transition
	if (bIsTransitionActive)
	{
		StopTransition();
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
				UE_LOG(LogTemp, Error, TEXT("Failed to create or retrieve transition effect instance."));
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
		UE_LOG(LogTemp, Warning, TEXT("ReverseTransition called with null preset."));
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

	// Cleanup Effect
	if (CurrentEffect)
	{
		CurrentEffect->Cleanup();

		// Return to pool
		if (UObject* EffectObj = CurrentEffect.GetObject())
		{
			FTransitionEffectPool& Pool = EffectPool.FindOrAdd(EffectObj->GetClass());
			Pool.Effects.Add(EffectObj);
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
