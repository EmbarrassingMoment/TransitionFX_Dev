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
	UTransitionPreset()
		: DefaultDuration(1.0f)
		, bAutoBlockInput(true)
		, bTickWhenPaused(false)
		, Priority(1000.0f)
		, HalfwayThreshold(0.5f)
	{}

	/** The class of the effect to instantiate. Must implement ITransitionEffect. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (MustImplement = "TransitionEffect"))
	TSubclassOf<UObject> EffectClass;

	/** Duration of the transition in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	float DefaultDuration;

	/** Optional curve to ease the transition progress. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<UCurveFloat> ProgressCurve;

	/** Whether to block player input during the transition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	bool bAutoBlockInput;

	/** Whether the transition should update while the game is paused. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	bool bTickWhenPaused;

	/** Priority for PostProcess effects or UI Z-Order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	float Priority;

	/** The progress point (0.0-1.0) where the screen is considered fully covered. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HalfwayThreshold;

	/** Optional sound to play when transition starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> TransitionSound;
};
