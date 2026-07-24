// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"

class UTransitionPreset;

/**
 * Stateless helpers for preset preloading and shader (PSO) warmup.
 * UTransitionManagerSubsystem's Blueprint-facing preload API delegates here.
 */
class TRANSITIONFX_API FTransitionPresetPreloader
{
public:
	/**
	 * Synchronously creates temporary dynamic material instances for each unique material
	 * to force PSO (Pipeline State Object) compilation and shader cache warmup.
	 * The temporary instances are owned by Outer and left for the garbage collector.
	 */
	static void WarmupShaders(UObject* Outer, const TArray<UTransitionPreset*>& Presets);

	/**
	 * Asynchronously streams in soft-referenced presets, then performs synchronous shader warmup.
	 * OnComplete runs when all presets are loaded and warmed up; it is dropped (never called)
	 * if LifetimeOwner is destroyed before loading finishes.
	 */
	static void AsyncLoadAndWarmup(UObject* LifetimeOwner, const TArray<TSoftObjectPtr<UTransitionPreset>>& SoftPresets, TFunction<void()> OnComplete);
};
