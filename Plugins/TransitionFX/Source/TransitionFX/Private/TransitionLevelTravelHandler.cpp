// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionLevelTravelHandler.h"
#include "TransitionManagerSubsystem.h"
#include "TransitionFXConfig.h"
#include "TransitionPreset.h"
#include "TransitionFX.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UTransitionLevelTravelHandler::Initialize(UTransitionManagerSubsystem* InOwnerSubsystem)
{
	OwnerSubsystem = InOwnerSubsystem;

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UTransitionLevelTravelHandler::OnPostLoadMapWithWorld);
}

void UTransitionLevelTravelHandler::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
}

void UTransitionLevelTravelHandler::BeginLevelTransition(FName LevelName, UTransitionPreset* Preset, float Duration)
{
	PendingLevelName = LevelName;
	PendingPreset = Preset;
	PendingDuration = Duration;
	bAutoReverseOnLevelLoad = true;

	// Ensure we don't have stale bindings
	OwnerSubsystem->OnTransitionCompleted.RemoveDynamic(this, &UTransitionLevelTravelHandler::OnFadeOutFinished);
	OwnerSubsystem->OnTransitionCompleted.AddDynamic(this, &UTransitionLevelTravelHandler::OnFadeOutFinished);

	float PlaySpeed = TransitionFXConfig::CalculatePlaySpeed(Preset->DefaultDuration, Duration);

	// Start Fade Out (Forward, Invert=False)
	OwnerSubsystem->StartTransition(Preset, ETransitionMode::Forward, PlaySpeed, false);
}

void UTransitionLevelTravelHandler::PrepareAutoReverse(UTransitionPreset* Preset, float Duration)
{
	PendingPreset = Preset;
	PendingDuration = Duration;
	bAutoReverseOnLevelLoad = true;
}

void UTransitionLevelTravelHandler::OnFadeOutFinished()
{
	// One-shot callback
	OwnerSubsystem->OnTransitionCompleted.RemoveDynamic(this, &UTransitionLevelTravelHandler::OnFadeOutFinished);

	UGameplayStatics::OpenLevel(OwnerSubsystem, PendingLevelName);
}

void UTransitionLevelTravelHandler::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (bAutoReverseOnLevelLoad)
	{
		bAutoReverseOnLevelLoad = false;

		// Verify the loaded world matches our current world context
		if (LoadedWorld && LoadedWorld != OwnerSubsystem->GetWorld())
		{
			UE_LOG(LogTransitionFX, Warning, TEXT("TransitionFX: OnPostLoadMapWithWorld called with mismatched world. Skipping auto-reverse."));
			return;
		}

		if (PendingPreset)
		{
			float PlaySpeed = TransitionFXConfig::CalculatePlaySpeed(PendingPreset->DefaultDuration, PendingDuration);

			// Start Fade In (Forward, Invert=True to go from Black to Clear if using standard mask behavior)
			OwnerSubsystem->StartTransition(PendingPreset, ETransitionMode::Forward, PlaySpeed, true);
		}
	}
}
