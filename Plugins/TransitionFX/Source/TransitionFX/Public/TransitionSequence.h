// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TransitionPreset.h"
#include "TransitionManagerSubsystem.h"
#include "TransitionSequence.generated.h"

/**
 * A single entry within a UTransitionSequence.
 *
 * Each entry drives one call to the underlying UTransitionManagerSubsystem::StartTransition,
 * followed by an optional DelayAfter before the next entry begins.
 */
USTRUCT(BlueprintType)
struct TRANSITIONFX_API FTransitionSequenceEntry
{
	GENERATED_BODY()

	/** The transition preset to play for this entry. Required. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	TObjectPtr<UTransitionPreset> Preset = nullptr;

	/** Playback direction for this entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	ETransitionMode Mode = ETransitionMode::Forward;

	/**
	 * Duration override in seconds. If 0 (default), the preset's DefaultDuration is used.
	 * Internally converted to PlaySpeed via TransitionFXConfig::CalculatePlaySpeed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition", meta = (ClampMin = "0.0"))
	float DurationOverride = 0.0f;

	/** Invert the transition mask for this entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	bool bInvert = false;

	/** Delay in seconds AFTER this entry completes, before starting the next. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition", meta = (ClampMin = "0.0"))
	float DelayAfter = 0.0f;

	/** Per-entry material parameter overrides (scalar / vector / texture). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	FTransitionParameters OverrideParams;
};

/**
 * DataAsset describing an ordered list of transitions to play back-to-back.
 *
 * Held-at-max and level transitions are intentionally unsupported within a sequence;
 * use UTransitionManagerSubsystem::StartTransition or OpenLevelWithTransition directly for those.
 */
UCLASS(BlueprintType)
class TRANSITIONFX_API UTransitionSequence : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Ordered list of transitions to play back-to-back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sequence")
	TArray<FTransitionSequenceEntry> Entries;

	/** If true, the sequence loops after finishing all entries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sequence")
	bool bLoop = false;

	/**
	 * Number of times to loop. 0 = infinite (only meaningful when bLoop is true).
	 * Finite values play the sequence (LoopCount + 1) times total.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sequence", meta = (ClampMin = "0", EditCondition = "bLoop"))
	int32 LoopCount = 0;
};
