#pragma once

#include "CoreMinimal.h"
#include "TransitionFXTypes.generated.h"

/**
 * Procedural easing functions for transitions.
 */
UENUM(BlueprintType)
enum class ETransitionEasing : uint8
{
	Linear UMETA(DisplayName = "Linear"),

	EaseInSine UMETA(DisplayName = "Sine In"),
	EaseOutSine UMETA(DisplayName = "Sine Out"),
	EaseInOutSine UMETA(DisplayName = "Sine In/Out"),

	EaseInCubic UMETA(DisplayName = "Cubic In"),
	EaseOutCubic UMETA(DisplayName = "Cubic Out"),
	EaseInOutCubic UMETA(DisplayName = "Cubic In/Out"),

	EaseInExpo UMETA(DisplayName = "Expo In"),
	EaseOutExpo UMETA(DisplayName = "Expo Out"),
	EaseInOutExpo UMETA(DisplayName = "Expo In/Out"),

	EaseOutElastic UMETA(DisplayName = "Elastic Out"),
	EaseOutBounce UMETA(DisplayName = "Bounce Out"),

	Custom UMETA(DisplayName = "Custom Curve")
};
