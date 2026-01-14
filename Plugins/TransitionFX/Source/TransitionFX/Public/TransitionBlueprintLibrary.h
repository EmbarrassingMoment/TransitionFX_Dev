#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/LatentActionManager.h"
#include "TransitionManagerSubsystem.h"
#include "TransitionBlueprintLibrary.generated.h"

UCLASS()
class TRANSITIONFX_API UTransitionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Plays a transition and waits for it to complete.
	 * For Forward transitions (Fade Out), it waits until progress reaches 1.0 (screen fully obscured).
	 * For Reverse transitions (Fade In), it waits until progress reaches 0.0 (screen fully clear) or the transition is stopped.
	 *
	 * @param WorldContextObject	Context object to find the subsystem.
	 * @param Preset				The transition preset to use.
	 * @param Mode					Whether to play Forward (Fade Out) or Reverse (Fade In).
	 * @param PlaySpeed				Speed multiplier for the transition.
	 * @param LatentInfo			Internal latent action info.
	 */
	UFUNCTION(BlueprintCallable, Category = "Transition", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ExpandEnumAsExecs = "Mode"))
	static void PlayTransitionAndWait(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float PlaySpeed, struct FLatentActionInfo LatentInfo);
};
