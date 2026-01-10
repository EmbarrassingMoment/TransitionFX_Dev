#include "PostProcessTransitionEffect.h"
#include "TransitionPreset.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/PostProcessVolume.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"

void UPostProcessTransitionEffect::Initialize(UWorld* World, UTransitionPreset* Preset)
{
	if (!World || !Preset)
	{
		return;
	}

	if (!Preset->TransitionMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("PostProcessTransitionEffect: TransitionMaterial is null in Preset %s"), *Preset->GetName());
		return;
	}

	// Create Dynamic Material
	DynamicMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(World, Preset->TransitionMaterial);
	if (!DynamicMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("PostProcessTransitionEffect: Failed to create Dynamic Material Instance"));
		return;
	}

	// Spawn Post Process Volume
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags = RF_Transient; // Don't save this actor

	SpawnedVolume = World->SpawnActor<APostProcessVolume>(SpawnParams);

	if (SpawnedVolume)
	{
		SpawnedVolume->bUnbound = true; // Infinite extent
		SpawnedVolume->Priority = Preset->Priority;

		// Add Dynamic Material to Weighted Blendables
		SpawnedVolume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, DynamicMaterial));
	}
}

void UPostProcessTransitionEffect::UpdateProgress(float Progress)
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(FName("Progress"), Progress);
		UpdateMaterialParameters(DynamicMaterial, Progress);
	}
}

void UPostProcessTransitionEffect::Cleanup()
{
	if (SpawnedVolume)
	{
		SpawnedVolume->Destroy();
		SpawnedVolume = nullptr;
	}

	DynamicMaterial = nullptr;
}

void UPostProcessTransitionEffect::UpdateMaterialParameters(UMaterialInstanceDynamic* MID, float Progress)
{
	// Base implementation does nothing. Subclasses can override.
}
