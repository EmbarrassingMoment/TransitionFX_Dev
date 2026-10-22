// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

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
	UTransitionPreset();

	/**
	 * The class of the transition effect to spawn. Defaults to PostProcessTransitionEffect;
	 * use WidgetTransitionEffect (with an MI_Widget_* material) to also cover UMG/Slate UI.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX", meta = (MustImplement = "/Script/TransitionFX.TransitionEffect"))
	TSubclassOf<UObject> EffectClass;

	/** The material to use for this transition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX", meta = (ForceShowPluginContent))
	TObjectPtr<UMaterialInterface> TransitionMaterial;

	/**
	 * When true, applies TransitionColor to the material's color parameter at the
	 * start of the transition (e.g., fade-to-white) without requiring a call-site
	 * parameter override. An explicit color supplied via FTransitionParameters at
	 * the call site always takes precedence.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX")
	bool bOverrideTransitionColor = false;

	/** Default transition color applied when bOverrideTransitionColor is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX", meta = (EditCondition = "bOverrideTransitionColor"))
	FLinearColor TransitionColor = FLinearColor::Black;

	/** Default duration of the transition in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX", meta = (ClampMin = "0.01"))
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

	/**
	 * Z-order of the full-screen overlay when EffectClass is a widget-layer effect
	 * (e.g. WidgetTransitionEffect). UMG's AddToViewport uses ZOrder + 10 internally,
	 * so keep this well above your UI to ensure the transition covers it.
	 * Ignored by PostProcess-based effects.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX|Widget")
	int32 WidgetZOrder = 10000;

	/** The sound to play (Optional). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> TransitionSound;

	/** Volume of the transition sound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float SoundVolume;

	/** Pitch of the transition sound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio", meta = (ClampMin = "0.0"))
	float SoundPitch;

	/** Returns true if the preset can be played: both TransitionMaterial and EffectClass are set. */
	bool IsPlayable() const
	{
		return TransitionMaterial != nullptr && EffectClass != nullptr;
	}

#if WITH_EDITOR
	/**
	 * Editor data validation (Validate Assets, and on save when the Data Validation plugin is enabled).
	 * Errors: no material or effect class, no "Progress" parameter, or a material domain the built-in effect
	 * class cannot render (PostProcess / UI). Warnings: no "Invert" parameter, no "FadeColor" parameter while
	 * bOverrideTransitionColor is set, and Custom easing without a ProgressCurve.
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
