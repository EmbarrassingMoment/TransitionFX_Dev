// Copyright Kurorekishi. All Rights Reserved.

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
	virtual void Initialize(UWorld* World, UTransitionPreset* Preset) override;
	virtual void UpdateProgress(float Progress) override;
	virtual void Cleanup() override;
	virtual void SetInvert(bool bInvert) override;
	virtual void SetParameters(const FTransitionParameters& Params) override;

	/**
	 * Updates custom material parameters. Override this in subclasses to add extra parameters.
	 * @param MID The dynamic material instance.
	 * @param Progress The current transition progress.
	 */
	virtual void UpdateMaterialParameters(UMaterialInstanceDynamic* MID, float Progress);

protected:
	/** The dynamic material instance created at runtime. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	/** The spawned post process volume. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<APostProcessVolume> SpawnedVolume;
};
