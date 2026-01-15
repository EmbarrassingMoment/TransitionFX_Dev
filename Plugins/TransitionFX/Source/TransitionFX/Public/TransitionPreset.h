#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ITransitionEffect.h"
#include "TransitionPreset.generated.h"

class UCurveFloat;
class UMaterialInterface;

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
	{
	}

	/** The class of the transition effect to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (MustImplement = "/Script/TransitionFX.TransitionEffect"))
	TSubclassOf<UObject> EffectClass;

	/** The material to use for this transition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<UMaterialInterface> TransitionMaterial;

	/** Default duration of the transition in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0.0"))
	float DefaultDuration;

	/** Optional curve to ease the progress. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<UCurveFloat> ProgressCurve;

	/** Blocks player input during transition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	bool bAutoBlockInput;

	/** Allows transition while game is paused. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	bool bTickWhenPaused;

	/** Priority for PostProcess effect. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition")
	float Priority;

	/** The progress point (0.0-1.0) where the screen is fully covered. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HalfwayThreshold;
};
