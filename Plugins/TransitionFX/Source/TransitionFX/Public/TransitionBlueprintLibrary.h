#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TransitionManagerSubsystem.h"
#include "Engine/LatentActionManager.h"
#include "TransitionBlueprintLibrary.generated.h"

/**
 * Blueprint function library for transition effects.
 */
UCLASS()
class TRANSITIONFX_API UTransitionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Plays a transition and waits for it to complete.
	 *
	 * @param WorldContextObject The world context object.
	 * @param Preset The transition preset to use.
	 * @param Mode The transition mode (Forward or Reverse).
	 * @param PlaySpeed The speed multiplier for the transition.
	 * @param LatentInfo The latent action info.
	 */
	UFUNCTION(BlueprintCallable, Category = "Transition", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ExpandEnumAsExecs = "Mode"))
	static void PlayTransitionAndWait(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float PlaySpeed, struct FLatentActionInfo LatentInfo);

	/**
	 * Plays a transition with a specific duration and waits for it to complete.
	 *
	 * @param WorldContextObject The world context object.
	 * @param Preset The transition preset to use.
	 * @param Mode The transition mode (Forward or Reverse).
	 * @param Duration The duration of the transition in seconds.
	 * @param bInvert Whether to invert the transition mask.
	 * @param LatentInfo The latent action info.
	 */
	UFUNCTION(BlueprintCallable, Category = "Transition", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ExpandEnumAsExecs = "Mode"))
	static void PlayTransitionAndWaitWithDuration(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float Duration, bool bInvert, struct FLatentActionInfo LatentInfo);

	/**
	 * Returns true if a transition is currently playing.
	 *
	 * @param WorldContextObject The world context object.
	 * @return True if a transition is active, false otherwise.
	 */
	UFUNCTION(BlueprintPure, Category = "Transition", meta = (WorldContext = "WorldContextObject"))
	static bool IsTransitionPlaying(const UObject* WorldContextObject);
};
