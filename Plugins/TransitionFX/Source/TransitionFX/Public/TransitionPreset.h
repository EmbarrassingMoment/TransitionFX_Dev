// Copyright Kurorekishi. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ITransitionEffect.h"
#include "Materials/MaterialInterface.h"
#include "TransitionFXTypes.h"
#include "TransitionPreset.generated.h"

class UCurveFloat;
class USoundBase;
class UTexture;

/**
 * Parameters to override transition material properties at runtime.
 */
USTRUCT(BlueprintType)
struct FTransitionParameters
{
	GENERATED_BODY()

	/** Scalar parameters to override (e.g., float values). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransitionFX")
	TMap<FName, float> ScalarParams;

	/** Vector parameters to override (e.g., Colors). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransitionFX")
	TMap<FName, FLinearColor> VectorParams;

	/** Texture parameters to override. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransitionFX")
	TMap<FName, TObjectPtr<UTexture>> TextureParams;
};

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
		, SoundVolume(1.0f)
		, SoundPitch(1.0f)
	{
	}

	/** The class of the transition effect to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX", meta = (MustImplement = "/Script/TransitionFX.TransitionEffect"))
	TSubclassOf<UObject> EffectClass;

	/** The material to use for this transition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	TObjectPtr<UMaterialInterface> TransitionMaterial;

	/** Default duration of the transition in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX", meta = (ClampMin = "0.0"))
	float DefaultDuration;

	/** The easing function to apply to the progress. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TransitionFX")
	ETransitionEasing EasingType;

	/** Optional curve to ease the progress. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX", meta = (EditCondition = "EasingType == ETransitionEasing::Custom", EditConditionHides))
	TObjectPtr<UCurveFloat> ProgressCurve;

	/** Blocks player input during transition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	bool bAutoBlockInput;

	/** Allows transition while game is paused. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	bool bTickWhenPaused;

	/** Priority for PostProcess effect. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	float Priority;

	/** The sound to play (Optional). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> TransitionSound;

	/** Volume of the transition sound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float SoundVolume;

	/** Pitch of the transition sound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float SoundPitch;
};
