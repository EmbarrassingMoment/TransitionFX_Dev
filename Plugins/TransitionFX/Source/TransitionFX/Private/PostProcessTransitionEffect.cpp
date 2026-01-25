#include "PostProcessTransitionEffect.h"
#include "TransitionPreset.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/PostProcessVolume.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"

static TArray<TWeakObjectPtr<APostProcessVolume>> GlobalVolumePool;

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

	// Retrieve from pool or Spawn
	APostProcessVolume* VolumeToUse = nullptr;

	// Clean up invalid entries and look for a match
	for (int32 i = GlobalVolumePool.Num() - 1; i >= 0; --i)
	{
		if (!GlobalVolumePool[i].IsValid())
		{
			GlobalVolumePool.RemoveAtSwap(i);
			continue;
		}

		APostProcessVolume* Vol = GlobalVolumePool[i].Get();
		if (Vol->GetWorld() == World)
		{
			VolumeToUse = Vol;
			GlobalVolumePool.RemoveAtSwap(i);
			break;
		}
	}

	if (!VolumeToUse)
	{
		// Spawn Post Process Volume
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags = RF_Transient; // Don't save this actor

		VolumeToUse = World->SpawnActor<APostProcessVolume>(SpawnParams);
	}

	SpawnedVolume = VolumeToUse;

	if (SpawnedVolume)
	{
		SpawnedVolume->bUnbound = true; // Infinite extent
		SpawnedVolume->Priority = Preset->Priority;
		SpawnedVolume->SetActorHiddenInGame(false);
		SpawnedVolume->bEnabled = true;

		// Add Dynamic Material to Weighted Blendables
		SpawnedVolume->Settings.WeightedBlendables.Array.Empty();
		SpawnedVolume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, DynamicMaterial));
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
		SpawnedVolume->SetActorHiddenInGame(true);
		SpawnedVolume->bEnabled = false;
		SpawnedVolume->Settings.WeightedBlendables.Array.Empty();
		GlobalVolumePool.Add(SpawnedVolume);
		SpawnedVolume = nullptr;
	}

	DynamicMaterial = nullptr;
}

void UPostProcessTransitionEffect::SetInvert(bool bInvert)
{
	if (DynamicMaterial)
	{
		// Pass 1.0 for True, 0.0 for False.
		// The material will use an "If" node with a threshold of 0.5 to switch logic.
		DynamicMaterial->SetScalarParameterValue(FName("Invert"), bInvert ? 1.0f : 0.0f);
	}
}

void UPostProcessTransitionEffect::UpdateMaterialParameters(UMaterialInstanceDynamic* MID, float Progress)
{
	// Base implementation does nothing. Subclasses can override.
}
