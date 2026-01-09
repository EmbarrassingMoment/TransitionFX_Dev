#include "PostProcessTransitionEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/PostProcessVolume.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"

void UPostProcessTransitionEffect::Initialize(UWorld* World)
{
	if (!World)
	{
		return;
	}

	if (!TransitionMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("PostProcessTransitionEffect: TransitionMaterial is null in %s"), *GetName());
		return;
	}

	// Create Dynamic Material
	DynamicMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(World, TransitionMaterial);
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
		SpawnedVolume->Priority = 1000.0f; // Default high priority

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
