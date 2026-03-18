// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "PostProcessTransitionEffect.h"
#include "TransitionPreset.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/PostProcessVolume.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"
#include "TransitionFXConfig.h"
#include "TransitionFX.h"

/**
 * Creates or reuses a dynamic material instance and post-process volume.
 * Validates the preset's material and checks for the required "Progress" parameter.
 */
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
	static const FMaterialParameterInfo ProgressInfo(TransitionFXConfig::ProgressParamName);
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

/** Sets the Progress scalar parameter on the dynamic material and invokes the virtual extension point. */
void UPostProcessTransitionEffect::UpdateProgress(float Progress)
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(TransitionFXConfig::ProgressParamName, Progress);
		UpdateMaterialParameters(DynamicMaterial, Progress);
	}
}

/** Disables the post-process volume to hide the effect. The volume and material are kept for reuse. */
void UPostProcessTransitionEffect::Cleanup()
{
	if (SpawnedVolume)
	{
		SpawnedVolume->Destroy();
		SpawnedVolume = nullptr;
	}

	DynamicMaterial = nullptr;
}

/** Sets the Invert material parameter. The material uses an If node with a 0.5 threshold. */
void UPostProcessTransitionEffect::SetInvert(bool bInvert)
{
	if (DynamicMaterial)
	{
		// Pass 1.0 for True, 0.0 for False.
		// The material will use an "If" node with a threshold of 0.5 to switch logic.
		DynamicMaterial->SetScalarParameterValue(TransitionFXConfig::InvertParamName, bInvert ? 1.0f : 0.0f);
	}
}

/** Applies runtime parameter overrides (scalar, vector, texture) to the dynamic material instance. */
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

/** Virtual extension point for subclasses to apply additional material parameters each frame. */
void UPostProcessTransitionEffect::UpdateMaterialParameters(UMaterialInstanceDynamic* MID, float Progress)
{
	// Base implementation does nothing. Subclasses can override.
}
