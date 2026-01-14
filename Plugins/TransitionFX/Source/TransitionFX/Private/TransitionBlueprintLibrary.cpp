#include "TransitionBlueprintLibrary.h"
#include "TransitionManagerSubsystem.h"
#include "Engine/Engine.h"
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

	FTransitionLatentAction(const FLatentActionInfo& InLatentInfo, UTransitionManagerSubsystem* InManager)
		: ExecutionFunction(InLatentInfo)
		, OutputLink(InLatentInfo.Linkage)
		, CallbackTarget(InLatentInfo.CallbackTarget)
		, Manager(InManager)
	{
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		bool bFinished = true;

		if (Manager.IsValid())
		{
			// If we're done (finished hold phase or fully completed), we trigger
			if (Manager->IsCurrentTransitionFinished())
			{
				bFinished = true;
			}
			else
			{
				// Still playing
				bFinished = false;
			}
		}

		Response.FinishAndTriggerIf(bFinished, ExecutionFunction.ExecutionFunction, OutputLink, CallbackTarget);
	}

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return TEXT("Waiting for transition...");
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
			UTransitionManagerSubsystem* Manager = World->GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>();
			if (Manager)
			{
				Manager->StartTransition(Preset, Mode, PlaySpeed);
				LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransitionLatentAction(LatentInfo, Manager));
			}
		}
	}
}
