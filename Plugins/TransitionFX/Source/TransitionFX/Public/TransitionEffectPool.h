// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "TransitionEffectPool.generated.h"

/** Pool for transition effects. */
USTRUCT()
struct FTransitionEffectPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UObject>> Effects;
};

/**
 * Owns the per-class pools of reusable transition effect instances.
 * Embed as a UPROPERTY(Transient) member so pooled objects stay referenced for GC.
 */
USTRUCT()
struct TRANSITIONFX_API FTransitionEffectPoolManager
{
	GENERATED_BODY()

	/** Maximum pooled instances kept per effect class. */
	static constexpr int32 MaxPoolSize = 3;

	/** Returns a pooled instance of EffectClass, or creates a new one with Outer as its outer. */
	UObject* Acquire(UClass* EffectClass, UObject* Outer);

	/** Returns a used effect to its class pool, discarding it for GC when the pool is full. */
	void Release(UObject* EffectObj);

	/** Drops all pooled instances, leaving them to the garbage collector. */
	void Empty();

private:
	/** Pools of available effects, keyed by effect class. */
	UPROPERTY(Transient)
	TMap<UClass*, FTransitionEffectPool> Pools;
};
