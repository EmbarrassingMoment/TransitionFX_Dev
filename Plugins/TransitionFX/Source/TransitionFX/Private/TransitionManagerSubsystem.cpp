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

	float EasedProgress = CurrentProgressValue;
	if (CurrentPreset->ProgressCurve)
	{
		EasedProgress = CurrentPreset->ProgressCurve->GetFloatValue(CurrentProgressValue);
	}

	if (CurrentEffect)
	{
		CurrentEffect->UpdateProgress(EasedProgress);
	}

	// Check Halfway
	// We only check halfway on forward progress as per typical transition logic
	if (!bIsReversing && !bHasReachedHalfway && CurrentProgressValue >= CurrentPreset->HalfwayThreshold)
	{
		bHasReachedHalfway = true;
		OnTransitionHalfway.Broadcast();
	}

	// Check Completion
	if (bIsReversing)
	{
		if (CurrentProgressValue <= 0.0f)
		{
			StopTransition();
			OnTransitionCompleted.Broadcast();
		}
	}
	else
	{
		// Forward completion: Hold at 1.0 and broadcast once.
		if (CurrentProgressValue >= 1.0f && !bHasCompleted)
		{
			bHasCompleted = true;
			OnTransitionCompleted.Broadcast();
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
	bHasReachedHalfway = false;
	bIsReversing = false;
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
		UE_LOG(LogTemp, Warning, TEXT("ReverseTransition called with no active preset."));
		return;
	}

	bIsReversing = true;
	bIsTransitionActive = true; // Ensure active if it was holding at 1.0
	// We do NOT reset bHasCompleted here, as it tracks forward completion.
	// We might want to reset bHasReachedHalfway if we want it to trigger again on next forward?
	// But usually Reverse is the end of the sequence.
}

void UTransitionManagerSubsystem::StopTransition()
{
	if (!bIsTransitionActive && !CurrentPreset)
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
	bIsReversing = false;
	bHasCompleted = false;
}

bool UTransitionManagerSubsystem::IsTransitionPlaying() const
{
	return bIsTransitionActive;
}
