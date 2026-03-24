// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TransitionManagerSubsystem.h"
#include "Engine/LatentActionManager.h"
#include "TransitionFXTypes.h"
#include "TransitionBlueprintLibrary.generated.h"

class UCurveFloat;

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
	 * @param bInvert Whether to invert the transition mask.
	 * @param OverrideParams Optional parameters to override material properties.
	 * @param LatentInfo The latent action info.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ExpandEnumAsExecs = "Mode", PlaySpeed = "1.0"))
	static void PlayTransitionAndWait(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float PlaySpeed, bool bInvert, FTransitionParameters OverrideParams, struct FLatentActionInfo LatentInfo);

	/**
	 * Picks a random preset from the list and plays it, waiting for completion.
	 *
	 * @param WorldContextObject The world context object.
	 * @param Presets List of presets to choose from.
	 * @param Mode The transition mode (Forward or Reverse).
	 * @param PlaySpeed The speed multiplier for the transition.
	 * @param bInvert Whether to invert the transition mask.
	 * @param OverrideParams Optional parameters to override material properties.
	 * @param LatentInfo The latent action info.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ExpandEnumAsExecs = "Mode", PlaySpeed = "1.0"))
	static void PlayRandomTransitionAndWait(const UObject* WorldContextObject, const TArray<UTransitionPreset*>& Presets, ETransitionMode Mode, float PlaySpeed, bool bInvert, FTransitionParameters OverrideParams, struct FLatentActionInfo LatentInfo);

	/**
	 * Plays a transition with a specific duration and waits for it to complete.
	 *
	 * @param WorldContextObject The world context object.
	 * @param Preset The transition preset to use.
	 * @param Mode The transition mode (Forward or Reverse).
	 * @param Duration The duration of the transition in seconds.
	 * @param bInvert Whether to invert the transition mask.
	 * @param OverrideParams Optional parameters to override material properties.
	 * @param LatentInfo The latent action info.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", ExpandEnumAsExecs = "Mode"))
	static void PlayTransitionAndWaitWithDuration(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float Duration, bool bInvert, FTransitionParameters OverrideParams, struct FLatentActionInfo LatentInfo);

	/**
	 * Returns true if any transition is currently playing.
	 *
	 * @param WorldContextObject The world context object.
	 */
	UFUNCTION(BlueprintPure, Category = "TransitionFX", meta = (WorldContext = "WorldContextObject"))
	static bool IsAnyTransitionPlaying(const UObject* WorldContextObject);

	/**
	 * Quickly fades the screen to black using a transient preset.
	 *
	 * @param WorldContextObject The world context object.
	 * @param Duration The duration of the fade.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (WorldContext = "WorldContextObject"))
	static void QuickFadeToBlack(const UObject* WorldContextObject, float Duration = 1.0f);

	/**
	 * Quickly fades the screen from black using a transient preset.
	 *
	 * @param WorldContextObject The world context object.
	 * @param Duration The duration of the fade.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (WorldContext = "WorldContextObject"))
	static void QuickFadeFromBlack(const UObject* WorldContextObject, float Duration = 1.0f);

	/**
	 * Handles the sequence of "Fade Out -> Open Level -> Fade In".
	 * Persists state across level transitions.
	 *
	 * @param WorldContextObject The world context object.
	 * @param LevelName The name of the level to open.
	 * @param Preset The transition preset to use.
	 * @param Duration The duration of the transition (applies to both fade out and fade in).
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (WorldContext = "WorldContextObject"))
	static void OpenLevelWithTransition(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration = 1.0f);

	/**
	 * Plays a transition, waits for it to complete (Fade Out), then opens the specified level.
	 * The new level will automatically play the reverse transition (Fade In).
	 *
	 * @param WorldContextObject The world context object.
	 * @param LevelName The name of the level to open.
	 * @param Preset The transition preset to use.
	 * @param Duration The duration of the transition (applies to both fade out and fade in).
	 * @param LatentInfo The latent action info.
	 */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX", meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject", Duration = "1.0"))
	static void OpenLevelWithTransitionAndWait(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration, struct FLatentActionInfo LatentInfo);

	/**
	 * Applies an easing function to the given alpha value.
	 *
	 * @param Alpha The input alpha value (0.0 to 1.0).
	 * @param EasingType The easing type to apply.
	 * @param CustomCurve Optional custom curve to use if EasingType is Custom.
	 * @return The eased alpha value.
	 */
	UFUNCTION(BlueprintPure, Category = "TransitionFX|Math")
	static float ApplyEasing(float Alpha, ETransitionEasing EasingType, const UCurveFloat* CustomCurve = nullptr);

	/** Retrieves the TransitionManagerSubsystem from the world context. Returns nullptr on failure. */
	static UTransitionManagerSubsystem* GetTransitionManager(const UObject* WorldContextObject);
};
