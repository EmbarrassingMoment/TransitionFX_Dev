// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionEffectPool.h"
#include "TransitionFX.h"

UObject* FTransitionEffectPoolManager::Acquire(UClass* EffectClass, UObject* Outer)
{
	if (!EffectClass)
	{
		return nullptr;
	}

	// Reuse a pooled instance to avoid allocation and reduce GC pressure
	FTransitionEffectPool& Pool = Pools.FindOrAdd(EffectClass);
	if (Pool.Effects.Num() > 0)
	{
		return Pool.Effects.Pop();
	}

	return NewObject<UObject>(Outer, EffectClass);
}

void FTransitionEffectPoolManager::Release(UObject* EffectObj)
{
	if (!EffectObj)
	{
		return;
	}

	FTransitionEffectPool& Pool = Pools.FindOrAdd(EffectObj->GetClass());

	// Cap the pool size to prevent memory bloat
	if (Pool.Effects.Num() < MaxPoolSize)
	{
		Pool.Effects.Add(EffectObj);
	}
	else
	{
		// Do nothing. Let the Garbage Collector handle the unreferenced object.
		UE_LOG(LogTransitionFX, Verbose, TEXT("Pool for %s is full. Discarding effect instance for GC."), *EffectObj->GetClass()->GetName());
	}
}

void FTransitionEffectPoolManager::Empty()
{
	Pools.Empty();
}
