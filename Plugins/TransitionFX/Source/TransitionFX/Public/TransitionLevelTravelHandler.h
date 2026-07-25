// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TransitionLevelTravelHandler.generated.h"

class UTransitionManagerSubsystem;
class UTransitionPreset;
class UWorld;

/**
 * Owns the "Fade Out -> Open Level -> Fade In" flow behind
 * UTransitionManagerSubsystem::OpenLevelWithTransition / PrepareAutoReverseTransition.
 * Created and owned by the subsystem; stores the pending level transition state so it
 * survives level unload, and drives the subsystem's StartTransition for both phases.
 */
UCLASS()
class TRANSITIONFX_API UTransitionLevelTravelHandler : public UObject
{
	GENERATED_BODY()

public:
	/** Stores the owning subsystem and binds the post-load-map delegate. Called once from the subsystem's Initialize. */
	void Initialize(UTransitionManagerSubsystem* InOwnerSubsystem);

	/** Removes the post-load-map delegate binding. Called from the subsystem's Deinitialize. */
	void Deinitialize();

	/** Starts the fade-out phase and stores the pending level to open when it completes. */
	void BeginLevelTransition(FName LevelName, UTransitionPreset* Preset, float Duration);

	/** Stores preset and duration for an auto-reverse transition on the next level load without starting playback. */
	void PrepareAutoReverse(UTransitionPreset* Preset, float Duration);

	/** Returns true while a level transition (or prepared auto-reverse) is pending. */
	bool IsLevelTransitionPending() const { return bAutoReverseOnLevelLoad; }

private:
	/** One-shot callback that opens the pending level after the fade-out transition completes. */
	UFUNCTION()
	void OnFadeOutFinished();

	/** Called after a new level is loaded. Triggers the auto-reverse fade-in if one was prepared. */
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	/** The subsystem that owns this handler. */
	UPROPERTY(Transient)
	TObjectPtr<UTransitionManagerSubsystem> OwnerSubsystem;

	/** Whether to automatically play a reverse transition when a new level finishes loading. */
	bool bAutoReverseOnLevelLoad = false;

	/** The name of the level to open after the fade-out completes. */
	FName PendingLevelName;

	/** The duration to use for the pending level transition. */
	float PendingDuration = 1.0f;

	/** The preset to use for the pending level transition. */
	UPROPERTY()
	TObjectPtr<UTransitionPreset> PendingPreset;
};
