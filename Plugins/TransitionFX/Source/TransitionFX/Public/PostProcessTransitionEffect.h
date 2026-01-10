#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ITransitionEffect.h"
#include "PostProcessTransitionEffect.generated.h"

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
	virtual void Initialize(UWorld* World) override;
	virtual void UpdateProgress(float Progress) override;
	virtual void Cleanup() override;

	/**
	 * Updates custom material parameters. Override this in subclasses to add extra parameters.
	 * @param MID The dynamic material instance.
	 * @param Progress The current transition progress.
	 */
	virtual void UpdateMaterialParameters(UMaterialInstanceDynamic* MID, float Progress);

protected:
	/** The source material for the transition. */
	UPROPERTY(EditDefaultsOnly, Category = "Transition")
	TObjectPtr<UMaterialInterface> TransitionMaterial;

	/** The dynamic material instance created at runtime. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	/** The spawned post process volume. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<APostProcessVolume> SpawnedVolume;
};
