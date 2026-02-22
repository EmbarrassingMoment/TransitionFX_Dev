// Copyright Kurorekishi. All Rights Reserved.

#include "PostProcessTransitionEffect.h"
#include "TransitionPreset.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/PostProcessVolume.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"
#include "Engine/Texture.h"
#include "TransitionFX.h"

void UPostProcessTransitionEffect::Initialize(UWorld* World, UTransitionPreset* Preset)
{
	if (!World || !Preset)
	{
		return;
	}

	if (!Preset->TransitionMaterial)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("PostProcessTransitionEffect: TransitionMaterial is null in Preset %s"), *Preset->GetName());
		return;
	}

	// Create or Reuse Dynamic Material
	if (DynamicMaterial && DynamicMaterial->Parent == Preset->TransitionMaterial)
	{
		DynamicMaterial->ClearParameterValues();
	}
	else
	{
		DynamicMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(World, Preset->TransitionMaterial);
	}

	if (!DynamicMaterial)
	{
		UE_LOG(LogTransitionFX, Error, TEXT("PostProcessTransitionEffect: Failed to create Dynamic Material Instance"));
		return;
	}

	// Check for "Progress" Parameter
	float TempVal = 0.0f;
	static const FName ProgressParamName(TEXT("Progress"));
	FMaterialParameterInfo ProgressInfo(ProgressParamName);
	if (!DynamicMaterial->GetScalarParameterValue(ProgressInfo, TempVal))
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("TransitionFX: Material '%s' is missing 'Progress' parameter. Transition will not animate."), *Preset->TransitionMaterial->GetName());
	}

	// Invalidate volume if it belongs to a different (stale) world
	if (SpawnedVolume && SpawnedVolume->GetWorld() != World)
	{
		SpawnedVolume = nullptr;
	}

	// Reuse or Spawn Post Process Volume
	if (!SpawnedVolume)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags = RF_Transient; // Don't save this actor

		SpawnedVolume = World->SpawnActor<APostProcessVolume>(SpawnParams);
		if (SpawnedVolume)
		{
			SpawnedVolume->bUnbound = true; // Infinite extent
		}
	}

	if (SpawnedVolume)
	{
		SpawnedVolume->bEnabled = true;
		SpawnedVolume->Priority = Preset->Priority;

		// Update Weighted Blendables
		bool bNeedsUpdate = true;
		if (SpawnedVolume->Settings.WeightedBlendables.Array.Num() == 1)
		{
			const FWeightedBlendable& ExistingBlendable = SpawnedVolume->Settings.WeightedBlendables.Array[0];
			if (ExistingBlendable.Object == DynamicMaterial && FMath::IsNearlyEqual(ExistingBlendable.Weight, 1.0f))
			{
				bNeedsUpdate = false;
			}
		}

		if (bNeedsUpdate)
		{
			SpawnedVolume->Settings.WeightedBlendables.Array.Reset();
			SpawnedVolume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, DynamicMaterial));
		}
	}
}

void UPostProcessTransitionEffect::UpdateProgress(float Progress)
{
	if (DynamicMaterial)
	{
		static const FName ProgressParamName(TEXT("Progress"));
		DynamicMaterial->SetScalarParameterValue(ProgressParamName, Progress);
		UpdateMaterialParameters(DynamicMaterial, Progress);
	}
}

void UPostProcessTransitionEffect::Cleanup()
{
	if (SpawnedVolume)
	{
		// Disable volume instead of destroying to allow reuse
		SpawnedVolume->bEnabled = false;
	}

	// Keep DynamicMaterial for potential reuse (or just let it be overwritten in Initialize)
}

void UPostProcessTransitionEffect::SetInvert(bool bInvert)
{
	if (DynamicMaterial)
	{
		// Pass 1.0 for True, 0.0 for False.
		// The material will use an "If" node with a threshold of 0.5 to switch logic.
		static const FName InvertParamName(TEXT("Invert"));
		DynamicMaterial->SetScalarParameterValue(InvertParamName, bInvert ? 1.0f : 0.0f);
	}
}

void UPostProcessTransitionEffect::SetParameters(const FTransitionParameters& Params)
{
	if (!DynamicMaterial)
	{
		return;
	}

	for (const auto& Pair : Params.ScalarParams)
	{
		DynamicMaterial->SetScalarParameterValue(Pair.Key, Pair.Value);
	}

	for (const auto& Pair : Params.VectorParams)
	{
		DynamicMaterial->SetVectorParameterValue(Pair.Key, Pair.Value);
	}

	for (const auto& Pair : Params.TextureParams)
	{
		if (Pair.Value)
		{
			DynamicMaterial->SetTextureParameterValue(Pair.Key, Pair.Value);
		}
	}
}

void UPostProcessTransitionEffect::UpdateMaterialParameters(UMaterialInstanceDynamic* MID, float Progress)
{
	// Base implementation does nothing. Subclasses can override.
}
