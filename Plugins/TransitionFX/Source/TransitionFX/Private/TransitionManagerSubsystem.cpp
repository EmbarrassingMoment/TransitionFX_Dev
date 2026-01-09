#include "TransitionManagerSubsystem.h"
#include "TransitionPreset.h"
#include "ITransitionEffect.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Curves/CurveFloat.h"

void UTransitionManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bIsTransiting = false;
	CurrentProgress = 0.0f;
	CurrentTime = 0.0f;
	bHalfwayTriggered = false;
	CurrentEffectInterface = nullptr;
}

void UTransitionManagerSubsystem::Deinitialize()
{
	StopTransition();
	Super::Deinitialize();
}

void UTransitionManagerSubsystem::Tick(float DeltaTime)
{
	if (!bIsTransiting || !CurrentPreset || !CurrentEffectInterface)
	{
		return;
	}

	CurrentTime += DeltaTime;
	float Duration = CurrentPreset->DefaultDuration;
	if (Duration <= 0.0f)
	{
		Duration = 0.001f; // Avoid division by zero
	}

	float RawProgress = FMath::Clamp(CurrentTime / Duration, 0.0f, 1.0f);

	// Apply curve if available
	CurrentProgress = RawProgress;
	if (CurrentPreset->ProgressCurve)
	{
		CurrentProgress = CurrentPreset->ProgressCurve->GetFloatValue(RawProgress);
	}

	// Update effect
	CurrentEffectInterface->UpdateProgress(CurrentProgress);

	// Check halfway
	if (!bHalfwayTriggered && RawProgress >= CurrentPreset->HalfwayThreshold)
	{
		bHalfwayTriggered = true;
		OnTransitionHalfway.Broadcast();
	}

	// Check completion
	if (RawProgress >= 1.0f)
	{
		// Cleanup will be called by StopTransition
		OnTransitionCompleted.Broadcast();
		StopTransition();
	}
}

TStatId UTransitionManagerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTransitionManagerSubsystem, STATGROUP_Tickables);
}

bool UTransitionManagerSubsystem::IsTickable() const
{
	return bIsTransiting;
}

bool UTransitionManagerSubsystem::IsTickableWhenPaused() const
{
	if (bIsTransiting && CurrentPreset)
	{
		return CurrentPreset->bTickWhenPaused;
	}
	return false;
}

void UTransitionManagerSubsystem::StartTransition(UTransitionPreset* Preset)
{
	if (!Preset)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartTransition called with null Preset"));
		return;
	}

	if (bIsTransiting)
	{
		StopTransition();
	}

	CurrentPreset = Preset;

	if (!CurrentPreset->EffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartTransition Preset has no EffectClass"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Create the effect instance
	CurrentEffectObject = NewObject<UObject>(this, CurrentPreset->EffectClass);

	if (!CurrentEffectObject->GetClass()->ImplementsInterface(UTransitionEffect::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("EffectClass does not implement ITransitionEffect"));
		CurrentEffectObject = nullptr;
		return;
	}

	CurrentEffectInterface = Cast<ITransitionEffect>(CurrentEffectObject);
	if (!CurrentEffectInterface)
	{
		// This might happen if interface is implemented in BP but not natively castable?
		// Actually Cast<ITransitionEffect> works for native implementation.
		// For BP implementation, we might need to use Execute_Initialize via the UFunction if it was a BlueprintNativeEvent.
		// But here ITransitionEffect is a pure C++ interface (as I implemented it).
		// So the object MUST implement it natively.
		// If the user wants BP effects, I should have made it a UInterface with BlueprintNativeEvents.
		// Given the constraints and the way I wrote ITransitionEffect, it supports C++ implementation.
		// If the class is a BP class, it must inherit from a C++ base that implements the interface.
		UE_LOG(LogTemp, Error, TEXT("Could not cast to ITransitionEffect"));
		CurrentEffectObject = nullptr;
		return;
	}

	// Initialize state
	bIsTransiting = true;
	CurrentTime = 0.0f;
	CurrentProgress = 0.0f;
	bHalfwayTriggered = false;

	// Input Blocking
	if (CurrentPreset->bAutoBlockInput)
	{
		SetInputBlocking(true);
	}

	// Audio
	if (CurrentPreset->TransitionSound)
	{
		UGameplayStatics::PlaySound2D(World, CurrentPreset->TransitionSound);
	}

	// Init Effect
	CurrentEffectInterface->Initialize(World);

	// Initial update
	CurrentEffectInterface->UpdateProgress(0.0f);

	OnTransitionStarted.Broadcast();
}

void UTransitionManagerSubsystem::StopTransition()
{
	if (bIsTransiting)
	{
		if (CurrentPreset && CurrentPreset->bAutoBlockInput)
		{
			SetInputBlocking(false);
		}

		// Always cleanup the effect when stopping
		if (CurrentEffectInterface)
		{
			CurrentEffectInterface->Cleanup();
		}
	}

	bIsTransiting = false;
	CurrentPreset = nullptr;
	CurrentEffectObject = nullptr;
	CurrentEffectInterface = nullptr;
	CurrentTime = 0.0f;
	CurrentProgress = 0.0f;
	bHalfwayTriggered = false;
}

bool UTransitionManagerSubsystem::IsTransitionPlaying() const
{
	return bIsTransiting;
}

void UTransitionManagerSubsystem::SetInputBlocking(bool bBlock)
{
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			if (bBlock)
			{
				PC->SetCinematicMode(true, true, true, true, true);
			}
			else
			{
				PC->SetCinematicMode(false, true, true, true, true);
			}
		}
	}
}
