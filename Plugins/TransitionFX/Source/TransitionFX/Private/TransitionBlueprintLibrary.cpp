#include "TransitionBlueprintLibrary.h"
#include "TransitionManagerSubsystem.h"
#include "Engine/World.h"
#include "Engine/LatentActionManager.h"
#include "Kismet/GameplayStatics.h"

class FTransitionLatentAction : public FPendingLatentAction
{
public:
	FLatentActionInfo ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	TWeakObjectPtr<UTransitionManagerSubsystem> Manager;
	ETransitionMode Mode;

	FTransitionLatentAction(const FLatentActionInfo& InLatentInfo, UTransitionManagerSubsystem* InManager, UTransitionPreset* InPreset, ETransitionMode InMode, float InPlaySpeed)
		: ExecutionFunction(InLatentInfo)
		, OutputLink(InLatentInfo.Linkage)
		, CallbackTarget(InLatentInfo.CallbackTarget)
		, Manager(InManager)
		, Mode(InMode)
	{
		if (Manager.IsValid())
		{
			Manager->StartTransition(InPreset, InMode, InPlaySpeed);
		}
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		bool bFinished = true;

		if (Manager.IsValid())
		{
			// Check if completed normally
			bool bIsFinished = Manager->IsCurrentTransitionFinished();

			// Also check if no longer playing (only relevant if we were expecting it to play).
			// For Reverse mode, it stops when finished, so IsTransitionPlaying() becomes false.
			// For Forward mode, it stays playing at 1.0 progress.

			if (!bIsFinished)
			{
				// If not marked finished, check if it was stopped prematurely or is still running
				if (Manager->IsTransitionPlaying())
				{
					bFinished = false; // Still playing and not finished
				}
				else
				{
					// Not playing and not marked finished?
					// Could happen if StopTransition() was called manually before completion.
					// In that case, we should probably finish the latent action.
					bFinished = true;
				}
			}
			else
			{
				bFinished = true;
			}
		}

		Response.FinishAndTriggerIf(bFinished, ExecutionFunction.ExecutionFunction, OutputLink, CallbackTarget);
	}

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		return TEXT("Waiting for transition to finish...");
	}
#endif
};

void UTransitionBlueprintLibrary::PlayTransitionAndWait(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float PlaySpeed, struct FLatentActionInfo LatentInfo)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UTransitionManagerSubsystem* Manager = GameInstance->GetSubsystem<UTransitionManagerSubsystem>();
	if (!Manager)
	{
		return;
	}

	FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
	if (LatentActionManager.FindExistingAction<FTransitionLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
	{
		LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransitionLatentAction(LatentInfo, Manager, Preset, Mode, PlaySpeed));
	}
}
