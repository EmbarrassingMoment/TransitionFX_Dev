#include "TransitionManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "Sound/SoundBase.h"

void UTransitionManagerSubsystem::Tick(float DeltaTime)
{
	if (!bIsTransitionActive || !CurrentPreset)
	{
		return;
	}

	float Duration = CurrentPreset->DefaultDuration;
	float DeltaProgress = (Duration > 0.0f) ? (DeltaTime / Duration) : 1.0f;

	if (bIsReversing)
	{
		CurrentProgressValue -= DeltaProgress;
	}
	else
	{
		CurrentProgressValue += DeltaProgress;
	}

	CurrentProgressValue = FMath::Clamp(CurrentProgressValue, 0.0f, 1.0f);
	float RawProgress = CurrentProgressValue;

	float EasedProgress = RawProgress;
	if (CurrentPreset->ProgressCurve)
	{
		EasedProgress = CurrentPreset->ProgressCurve->GetFloatValue(RawProgress);
	}

	if (CurrentEffect)
	{
		CurrentEffect->UpdateProgress(EasedProgress);
	}

	// Check Halfway
	if (!bHasReachedHalfway && RawProgress >= CurrentPreset->HalfwayThreshold)
	{
		bHasReachedHalfway = true;
		OnTransitionHalfway.Broadcast();
	}

	// Check Completion
	if (bIsReversing)
	{
		if (RawProgress <= 0.0f)
		{
			OnTransitionCompleted.Broadcast();
			StopTransition();
		}
	}
	else
	{
		if (RawProgress >= 1.0f)
		{
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

bool UTransitionManagerSubsystem::IsTickableWhenPaused() const
{
	return bIsTransitionActive && CurrentPreset && CurrentPreset->bTickWhenPaused;
}

TStatId UTransitionManagerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTransitionManagerSubsystem, STATGROUP_Tickables);
}

void UTransitionManagerSubsystem::StartTransition(UTransitionPreset* Preset)
{
	if (!Preset)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartTransition called with null preset."));
		return;
	}

	// Stop any existing transition
	if (bIsTransitionActive)
	{
		StopTransition();
	}

	CurrentPreset = Preset;
	CurrentProgressValue = 0.0f;
	bIsTransitionActive = true;
	bIsReversing = false;
	bHasReachedHalfway = false;
	bHasCompleted = false;

	// Create Effect
	if (Preset->EffectClass)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			UObject* NewEffectObj = NewObject<UObject>(this, Preset->EffectClass);
			if (NewEffectObj && NewEffectObj->Implements<UTransitionEffect>())
			{
				CurrentEffect = NewEffectObj;
				CurrentEffect->Initialize(World, Preset);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create transition effect instance."));
			}
		}
	}

	// Block Input
	if (CurrentPreset->bAutoBlockInput)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			PC->SetCinematicMode(true, true, true, true, true);
		}
	}

	// Play Sound
	if (CurrentPreset->TransitionSound)
	{
		UGameplayStatics::PlaySound2D(this, CurrentPreset->TransitionSound);
	}

	OnTransitionStarted.Broadcast();
}

void UTransitionManagerSubsystem::ReverseTransition()
{
	if (!CurrentPreset)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReverseTransition called with null preset."));
		return;
	}

	bIsReversing = true;
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
		CurrentEffect = nullptr;
	}

	// Restore Input
	if (CurrentPreset && CurrentPreset->bAutoBlockInput)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			PC->SetCinematicMode(false, true, true, true, true);
		}
	}

	bIsTransitionActive = false;
	CurrentPreset = nullptr;
}

bool UTransitionManagerSubsystem::IsTransitionPlaying() const
{
	return bIsTransitionActive;
}
