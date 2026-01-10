#include "TransitionManagerSubsystem.h"
#include "TransitionPreset.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Curves/CurveFloat.h"

void UTransitionManagerSubsystem::Tick(float DeltaTime)
{
	if (!bIsTransitioning || !CurrentPreset || !CurrentEffect)
	{
		return;
	}

	CurrentTime += DeltaTime;

	const float Duration = FMath::Max(CurrentPreset->DefaultDuration, KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp(CurrentTime / Duration, 0.0f, 1.0f);

	float Progress = Alpha;
	if (CurrentPreset->ProgressCurve)
	{
		Progress = CurrentPreset->ProgressCurve->GetFloatValue(Alpha);
	}

	// Update the effect
	CurrentEffect->UpdateProgress(Progress);

	// Check for halfway point
	if (!bHasTriggeredHalfway && Progress >= CurrentPreset->HalfwayThreshold)
	{
		bHasTriggeredHalfway = true;
		OnTransitionHalfway.Broadcast();
	}

	// Check for completion
	if (Alpha >= 1.0f)
	{
		StopTransition();
		OnTransitionCompleted.Broadcast();
	}
}

bool UTransitionManagerSubsystem::IsTickable() const
{
	return bIsTransitioning;
}

bool UTransitionManagerSubsystem::IsTickableWhenPaused() const
{
	return bIsTransitioning && CurrentPreset && CurrentPreset->bTickWhenPaused;
}

TStatId UTransitionManagerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTransitionManagerSubsystem, STATGROUP_Tickables);
}

void UTransitionManagerSubsystem::StartTransition(UTransitionPreset* Preset)
{
	// If already transitioning, stop the previous one? Or ignore?
	// Usually better to stop previous one first to cleanup.
	if (bIsTransitioning)
	{
		StopTransition();
	}

	if (!Preset || !Preset->EffectClass)
	{
		return;
	}

	CurrentPreset = Preset;
	CurrentTime = 0.0f;
	bIsTransitioning = true;
	bHasTriggeredHalfway = false;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Instantiate the effect
	UClass* EffectClass = Preset->EffectClass;
	UObject* NewEffectObj = NewObject<UObject>(this, EffectClass);

	if (NewEffectObj->Implements<UITransitionEffect>())
	{
		CurrentEffect = NewEffectObj;
		CurrentEffect->Initialize(World);
	}
	else
	{
		// Should not happen if TSubclassOf is correct
		bIsTransitioning = false;
		CurrentPreset = nullptr;
		return;
	}

	// Block Input
	if (CurrentPreset->bAutoBlockInput)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (PC)
		{
			PC->SetCinematicMode(true, true, true, true, true);
		}
	}

	// Play Sound
	if (CurrentPreset->TransitionSound)
	{
		UGameplayStatics::PlaySound2D(World, CurrentPreset->TransitionSound);
	}

	OnTransitionStarted.Broadcast();
}

void UTransitionManagerSubsystem::StopTransition()
{
	if (CurrentEffect)
	{
		CurrentEffect->Cleanup();
		CurrentEffect = nullptr;
	}

	if (bIsTransitioning && CurrentPreset && CurrentPreset->bAutoBlockInput)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			if (PC)
			{
				PC->SetCinematicMode(false, true, true, true, true);
			}
		}
	}

	bIsTransitioning = false;
	CurrentPreset = nullptr;
	bHasTriggeredHalfway = false;
	CurrentTime = 0.0f;
}

bool UTransitionManagerSubsystem::IsTransitionPlaying() const
{
	return bIsTransitioning;
}
