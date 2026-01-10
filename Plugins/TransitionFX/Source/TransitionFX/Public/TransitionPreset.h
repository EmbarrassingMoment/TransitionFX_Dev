#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ITransitionEffect.h"
#include "TransitionPreset.generated.h"

class UCurveFloat;
class USoundBase;

/**
 * DataAsset to hold transition settings.
 */
UCLASS(BlueprintType)
class TRANSITIONFX_API UTransitionPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The class of the transition effect to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	TSubclassOf<UITransitionEffect> EffectClass;

	/** Duration of the transition in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	float DefaultDuration = 1.0f;

	/** Optional curve to ease the transition progress. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<UCurveFloat> ProgressCurve;

	/** Blocks player input during transition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	bool bAutoBlockInput = true;

	/** Allows transition to update while game is paused. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	bool bTickWhenPaused = false;

	/** Priority for PostProcess effects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	float Priority = 1000.0f;

	/** The progress point (0.0-1.0) where the screen is fully covered and the halfway delegate is triggered. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	float HalfwayThreshold = 0.5f;

	/** Optional sound to play at the start of the transition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> TransitionSound;
};
