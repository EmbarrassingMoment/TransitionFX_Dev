// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionPresetPreloader.h"
#include "TransitionFX.h"
#include "TransitionFXConfig.h"
#include "TransitionPreset.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Materials/MaterialInstanceDynamic.h"

void FTransitionPresetPreloader::WarmupShaders(UObject* Outer, const TArray<UTransitionPreset*>& Presets)
{
	if (Presets.IsEmpty())
	{
		return;
	}

	UE_LOG(LogTransitionFX, Log, TEXT("Preloading %d Transition Presets..."), Presets.Num());

	TSet<UMaterialInterface*> ProcessedMaterials;

	for (UTransitionPreset* Preset : Presets)
	{
		if (Preset && Preset->TransitionMaterial)
		{
			bool bIsAlreadyInSet = false;
			ProcessedMaterials.Add(Preset->TransitionMaterial, &bIsAlreadyInSet);

			if (bIsAlreadyInSet)
			{
				continue;
			}

			// Create a temporary Dynamic Material Instance (MID)
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Preset->TransitionMaterial, Outer);

			if (MID)
			{
				// Set a scalar parameter to ensure the uniform buffer is initialized
				MID->SetScalarParameterValue(TransitionFXConfig::ProgressParamName, 0.0f);

				// Do not store this MID. We want it to be garbage collected immediately.
				// The sole purpose is to force the engine to compile/cache the PSOs (Pipeline State Objects) for this material.
			}
		}
	}
}

void FTransitionPresetPreloader::AsyncLoadAndWarmup(UObject* LifetimeOwner, const TArray<TSoftObjectPtr<UTransitionPreset>>& SoftPresets, TFunction<void()> OnComplete)
{
	if (SoftPresets.Num() == 0)
	{
		if (OnComplete)
		{
			OnComplete();
		}
		return;
	}

	// Create Path List
	TArray<FSoftObjectPath> ItemsToStream;
	for (const auto& Ref : SoftPresets)
	{
		ItemsToStream.Add(Ref.ToSoftObjectPath());
	}

	// Get StreamableManager (from AssetManager)
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	// Kick Async Load. The weak binding drops the callback if LifetimeOwner dies first.
	Streamable.RequestAsyncLoad(ItemsToStream, FStreamableDelegate::CreateWeakLambda(LifetimeOwner, [LifetimeOwner, SoftPresets, OnComplete]()
	{
		// Post-Load Processing
		TArray<UTransitionPreset*> LoadedPresets;
		for (const auto& Ref : SoftPresets)
		{
			if (UTransitionPreset* Preset = Ref.Get())
			{
				LoadedPresets.Add(Preset);
			}
		}

		// Execute Shader Warmup (Synchronous Preload)
		WarmupShaders(LifetimeOwner, LoadedPresets);

		// Notify Completion
		if (OnComplete)
		{
			OnComplete();
		}
	}));
}
