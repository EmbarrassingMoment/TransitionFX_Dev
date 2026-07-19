// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TransitionFXSettings.generated.h"

/**
 * Project-wide settings for the TransitionFX plugin.
 * Appears under Project Settings > Plugins > TransitionFX and is saved to DefaultGame.ini.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "TransitionFX"))
class TRANSITIONFX_API UTransitionFXSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UDeveloperSettings Interface

	/** Places the settings page under the "Plugins" category in Project Settings. */
	virtual FName GetCategoryName() const override;

	/**
	 * Maximum number of pooled effect instances kept per effect class.
	 * Higher values reduce allocations when frequently switching between different
	 * effects; lower values reduce memory held by idle instances.
	 * Set to 0 to disable pooling entirely (used instances are released for GC).
	 */
	UPROPERTY(EditAnywhere, Config, Category = "TransitionFX", meta = (ClampMin = "0", UIMin = "0", UIMax = "10"))
	int32 MaxPoolSizePerEffectClass = 3;
};
