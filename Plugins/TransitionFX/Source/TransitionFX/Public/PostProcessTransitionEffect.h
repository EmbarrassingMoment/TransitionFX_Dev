// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ITransitionEffect.h"
#include "PostProcessTransitionEffect.generated.h"

class UTransitionPreset;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class APostProcessVolume;

/**
 * Concrete transition effect that uses a Post Process Volume and Material.
 */
UCLASS(Blueprintable, BlueprintType)
class TRANSITIONFX_API UPostProcessTransitionEffect : public UObject, public ITransitionEffect
{
	GENERATED_BODY()

public:
	// ITransitionEffect Interface

	/** Creates or reuses a dynamic material and post-process volume for the given preset. */
	virtual void Initialize(UWorld* World, UTransitionPreset* Preset) override;

	/** Updates the material's Progress parameter and calls UpdateMaterialParameters. */
	virtual void UpdateProgress(float Progress) override;

	/** Disables the post-process volume without destroying it, allowing reuse. */
	virtual void Cleanup() override;

	/** Sets the material's Invert parameter (1.0 for inverted, 0.0 for normal). */
	virtual void SetInvert(bool bInvert) override;

	/** Applies scalar, vector, and texture parameter overrides to the dynamic material. */
	virtual void SetParameters(const FTransitionParameters& Params) override;

	/**
	 * Updates custom material parameters. Override this in subclasses to add extra parameters.
	 * @param MID The dynamic material instance.
	 * @param Progress The current transition progress.
	 */
	virtual void UpdateMaterialParameters(UMaterialInstanceDynamic* MID, float Progress);

protected:
	/** The dynamic material instance created at runtime. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "TransitionFX")
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	/** The spawned post process volume. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "TransitionFX")
	TObjectPtr<APostProcessVolume> SpawnedVolume;
};
