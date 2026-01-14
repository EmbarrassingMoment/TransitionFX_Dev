#include "TransitionBlueprintLibrary.h"
#include "TransitionManagerSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

class FTransitionLatentAction : public FPendingLatentAction
{
public:
	FLatentActionInfo ExecutionFunction;
	TWeakObjectPtr<UTransitionManagerSubsystem> Manager;
	bool bStarted;

	FTransitionLatentAction(const FLatentActionInfo& InLatentInfo, UTransitionManagerSubsystem* InManager, UTransitionPreset* InPreset, ETransitionMode InMode, float InPlaySpeed)
		: ExecutionFunction(InLatentInfo)
		, Manager(InManager)
		, bStarted(false)
	{
		if (Manager.IsValid())
		{
			Manager->StartTransition(InPreset, InMode, InPlaySpeed);
			bStarted = true;
		}
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		bool bFinished = false;

		if (!Manager.IsValid())
		{
			bFinished = true;
		}
		else
		{
			// If we started the transition, we check if it is finished.
			// IsCurrentTransitionFinished() returns true when bHasCompleted is true.
			// For Reverse mode, it stops automatically, so IsTransitionPlaying() might become false.
			if (Manager->IsCurrentTransitionFinished())
			{
				bFinished = true;
			}
			// Fallback: if transition stopped for some reason (e.g. Reverse completed and stopped, or manually stopped)
			else if (!Manager->IsTransitionPlaying() && bStarted)
			{
				bFinished = true;
			}
		}

		Response.FinishAndTriggerIf(bFinished, ExecutionFunction);
	}

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return TEXT("Waiting for transition to finish...");
	}
#endif
};

void UTransitionBlueprintLibrary::PlayTransitionAndWait(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float PlaySpeed, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FTransitionLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			UGameInstance* GameInstance = World->GetGameInstance();
			UTransitionManagerSubsystem* Manager = GameInstance ? GameInstance->GetSubsystem<UTransitionManagerSubsystem>() : nullptr;

			if (Manager)
			{
				LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransitionLatentAction(LatentInfo, Manager, Preset, Mode, PlaySpeed));
			}
		}
	}
}
