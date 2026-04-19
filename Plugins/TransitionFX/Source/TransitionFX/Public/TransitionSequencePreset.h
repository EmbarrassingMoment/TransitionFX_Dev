// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TransitionManagerSubsystem.h"
#include "TransitionPreset.h"
#include "TransitionSequencePreset.generated.h"

/**
 * A single transition step in a sequence.
 */
USTRUCT(BlueprintType)
struct TRANSITIONFX_API FTransitionSequenceStep
{
	GENERATED_BODY()

	/** The transition preset to play for this step. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	TObjectPtr<UTransitionPreset> Preset = nullptr;

	/** Playback mode for this step. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	ETransitionMode Mode = ETransitionMode::Forward;

	/** Playback speed multiplier for this step (ignored if bUseDurationOverride is true). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX", meta = (ClampMin = "0.01"))
	float PlaySpeed = 1.0f;

	/** Optional explicit duration override for this step. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	bool bUseDurationOverride = false;

	/** Duration in seconds when bUseDurationOverride is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX", meta = (EditCondition = "bUseDurationOverride", EditConditionHides, ClampMin = "0.01"))
	float DurationOverride = 1.0f;

	/** Whether to invert the transition mask for this step. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	bool bInvert = false;

	/** Optional per-step material parameter overrides. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	FTransitionParameters OverrideParams;
};

/**
 * Data asset that contains a list of transitions to play back-to-back.
 */
UCLASS(BlueprintType)
class TRANSITIONFX_API UTransitionSequencePreset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Ordered transition steps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	TArray<FTransitionSequenceStep> Steps;
};

