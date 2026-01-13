#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/LatentActionManager.h"
#include "TransitionManagerSubsystem.h"
#include "TransitionBlueprintLibrary.generated.h"

/**
 * Blueprint function library for TransitionFX.
 */
UCLASS()
class TRANSITIONFX_API UTransitionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Plays a transition and waits until it completes.
	 *
	 * @param WorldContextObject	The world context object.
	 * @param Preset				The transition preset to use.
	 * @param Mode					The transition mode (Forward or Reverse).
	 * @param PlaySpeed				The playback speed multiplier.
	 * @param LatentInfo			The latent action info.
	 */
	UFUNCTION(BlueprintCallable, Category = "Transition", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ExpandEnumAsExecs = "Mode"))
	static void PlayTransitionAndWait(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float PlaySpeed, struct FLatentActionInfo LatentInfo);
};
