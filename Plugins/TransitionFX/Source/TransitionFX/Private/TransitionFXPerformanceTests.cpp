#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "TransitionManagerSubsystem.h"
#include "TransitionPreset.h"
#include "PostProcessTransitionEffect.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransitionFXPerformanceTest, "TransitionFX.Performance.Pool", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTransitionFXPerformanceTest::RunTest(const FString& Parameters)
{
	// 1. Setup minimal environment
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!World)
	{
		AddError(TEXT("Failed to create UWorld"));
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	UGameInstance* GameInstance = NewObject<UGameInstance>(World);
	// We associate the GameInstance with the world
	World->SetGameInstance(GameInstance);

	// Create the Subsystem. We parent it to the GameInstance as intended.
	UTransitionManagerSubsystem* Subsystem = NewObject<UTransitionManagerSubsystem>(GameInstance);

	// Create a Preset
	UTransitionPreset* Preset = NewObject<UTransitionPreset>();
	Preset->EffectClass = UPostProcessTransitionEffect::StaticClass();
	Preset->DefaultDuration = 0.0f; // Instant for performance test (though StartTransition doesn't tick immediately)

	// 2. Measure
	const int32 Iterations = 1000;
	double StartTime = FPlatformTime::Seconds();

	for (int32 i = 0; i < Iterations; ++i)
	{
		Subsystem->StartTransition(Preset, ETransitionMode::Forward, 1.0f, false);
		// StopTransition is needed to allow the next StartTransition to proceed normally
		// (StartTransition calls StopTransition internally if active, but explicit stop is cleaner for the loop)
		Subsystem->StopTransition();
	}

	double EndTime = FPlatformTime::Seconds();
	double Duration = EndTime - StartTime;

	UE_LOG(LogTemp, Log, TEXT("TransitionFX Performance: %d iterations took %f seconds (Avg: %f ms)."), Iterations, Duration, (Duration / Iterations) * 1000.0);

	// 3. Cleanup
	// Destroying world should clean up GameInstance and Subsystem
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
