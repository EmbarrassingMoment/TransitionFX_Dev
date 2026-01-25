#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "PostProcessTransitionEffect.h"
#include "TransitionPreset.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransitionFXSpawnVolumePerfTest, "TransitionFX.Performance.SpawnVolume", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTransitionFXSpawnVolumePerfTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;

#if WITH_EDITOR
	if (GEditor)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
#endif

	if (!World && GEngine)
	{
		if (const FWorldContext* Context = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport))
		{
			World = Context->World();
		}
	}

	if (!World)
	{
		AddError("No valid World found for test. Please run this test in a level.");
		return false;
	}

	UTransitionPreset* Preset = NewObject<UTransitionPreset>();
	if (!Preset)
	{
		AddError("Failed to create TransitionPreset.");
		return false;
	}

	// Use Default Material as a fallback if specific ones aren't found
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial"));
	Preset->TransitionMaterial = Material;
	Preset->Priority = 1000;

	if (!Preset->TransitionMaterial)
	{
		AddError("Failed to load DefaultMaterial.");
		return false;
	}

	UPostProcessTransitionEffect* Effect = NewObject<UPostProcessTransitionEffect>();
	if (!Effect)
	{
		AddError("Failed to create PostProcessTransitionEffect.");
		return false;
	}

	// Warm up
	Effect->Initialize(World, Preset);
	Effect->Cleanup();

	const int32 Iterations = 500;
	double StartTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < Iterations; ++i)
	{
		Effect->Initialize(World, Preset);
		Effect->Cleanup();
	}

	double EndTime = FPlatformTime::Seconds();
	double TotalTime = EndTime - StartTime;

	UE_LOG(LogTemp, Display, TEXT("TransitionFX SpawnVolume Performance: %f seconds for %d iterations (Average: %f ms)"),
		TotalTime, Iterations, (TotalTime / Iterations) * 1000.0);

	// Cleanup just in case
	Effect->Cleanup();

	return true;
}
