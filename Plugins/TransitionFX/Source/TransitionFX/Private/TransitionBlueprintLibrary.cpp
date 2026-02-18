// Copyright Kurorekishi. All Rights Reserved.

#include "TransitionBlueprintLibrary.h"
#include "TransitionFXConfig.h"
#include "TransitionManagerSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/LatentActionManager.h"
#include "Kismet/GameplayStatics.h"
#include "PostProcessTransitionEffect.h"
#include "TransitionPreset.h"
#include "UObject/Package.h"
#include "Curves/CurveFloat.h"
#include "TransitionFX.h"

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
			bFinished = Manager->IsCurrentTransitionFinished();
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

void UTransitionBlueprintLibrary::PlayTransitionAndWait(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float PlaySpeed, bool bInvert, FTransitionParameters OverrideParams, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FTransitionLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			UTransitionManagerSubsystem* Manager = World->GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>();
			if (Manager)
			{
				Manager->StartTransition(Preset, Mode, PlaySpeed, bInvert, false, OverrideParams);
				LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransitionLatentAction(LatentInfo, Manager));
			}
		}
	}
}

void UTransitionBlueprintLibrary::PlayRandomTransitionAndWait(const UObject* WorldContextObject, const TArray<UTransitionPreset*>& Presets, ETransitionMode Mode, float PlaySpeed, bool bInvert, FTransitionParameters OverrideParams, struct FLatentActionInfo LatentInfo)
{
	if (Presets.IsEmpty())
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("PlayRandomTransitionAndWait: Presets array is empty."));

		// Finish immediately to prevent hanging
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
			if (LatentActionManager.FindExistingAction<FTransitionLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
			{
				// Passing nullptr for Manager will cause FTransitionLatentAction::UpdateOperation to return true immediately
				LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransitionLatentAction(LatentInfo, nullptr));
			}
		}
		return;
	}

	int32 Index = FMath::RandRange(0, Presets.Num() - 1);
	UTransitionPreset* SelectedPreset = Presets[Index];

	PlayTransitionAndWait(WorldContextObject, SelectedPreset, Mode, PlaySpeed, bInvert, OverrideParams, LatentInfo);
}

float UTransitionBlueprintLibrary::ApplyEasing(float Alpha, ETransitionEasing EasingType, const UCurveFloat* CustomCurve)
{
	// Clamp input
	float x = FMath::Clamp(Alpha, 0.0f, 1.0f);

	switch (EasingType)
	{
	case ETransitionEasing::Linear:
		return x;

	case ETransitionEasing::EaseInSine:
		return 1.0f - FMath::Cos((x * PI) / 2.0f);

	case ETransitionEasing::EaseOutSine:
		return FMath::Sin((x * PI) / 2.0f);

	case ETransitionEasing::EaseInOutSine:
		return -(FMath::Cos(PI * x) - 1.0f) / 2.0f;

	case ETransitionEasing::EaseInCubic:
		return x * x * x;

	case ETransitionEasing::EaseOutCubic:
		return 1.0f - FMath::Pow(1.0f - x, 3.0f);

	case ETransitionEasing::EaseInOutCubic:
		return x < 0.5f ? 4.0f * x * x * x : 1.0f - FMath::Pow(-2.0f * x + 2.0f, 3.0f) / 2.0f;

	case ETransitionEasing::EaseInExpo:
		return x == 0.0f ? 0.0f : FMath::Pow(2.0f, 10.0f * (x - 1.0f));

	case ETransitionEasing::EaseOutExpo:
		return x == 1.0f ? 1.0f : 1.0f - FMath::Pow(2.0f, -10.0f * x);

	case ETransitionEasing::EaseInOutExpo:
		if (x == 0.0f) return 0.0f;
		if (x == 1.0f) return 1.0f;
		if ((x *= 2.0f) < 1.0f) return 0.5f * FMath::Pow(2.0f, 10.0f * (x - 1.0f));
		return 0.5f * (2.0f - FMath::Pow(2.0f, -10.0f * (x - 1.0f)));

	case ETransitionEasing::EaseOutElastic:
	{
		static constexpr float ELASTIC_C4 = (2.0f * PI) / 3.0f;
		return x == 0.0f ? 0.0f : x == 1.0f ? 1.0f : FMath::Pow(2.0f, -10.0f * x) * FMath::Sin((x * 10.0f - 0.75f) * ELASTIC_C4) + 1.0f;
	}

	case ETransitionEasing::EaseOutBounce:
	{
		static constexpr float BOUNCE_N1 = 7.5625f;
		static constexpr float BOUNCE_D1 = 2.75f;

		if (x < 1.0f / BOUNCE_D1)
		{
			return BOUNCE_N1 * x * x;
		}
		else if (x < 2.0f / BOUNCE_D1)
		{
			x -= 1.5f / BOUNCE_D1;
			return BOUNCE_N1 * x * x + 0.75f;
		}
		else if (x < 2.5f / BOUNCE_D1)
		{
			x -= 2.25f / BOUNCE_D1;
			return BOUNCE_N1 * x * x + 0.9375f;
		}
		else
		{
			x -= 2.625f / BOUNCE_D1;
			return BOUNCE_N1 * x * x + 0.984375f;
		}
	}

	case ETransitionEasing::Custom:
		if (CustomCurve)
		{
			return CustomCurve->GetFloatValue(x);
		}
		return x;

	default:
		return x;
	}
}

bool UTransitionBlueprintLibrary::IsAnyTransitionPlaying(const UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTransitionManagerSubsystem* Manager = GameInstance->GetSubsystem<UTransitionManagerSubsystem>())
			{
				return Manager->IsTransitionPlaying();
			}
		}
	}
	return false;
}

static void QuickFadeInternal(const UObject* WorldContextObject, float Duration, ETransitionMode Mode)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTransitionManagerSubsystem* Manager = GameInstance->GetSubsystem<UTransitionManagerSubsystem>())
			{
				UTransitionPreset* TempPreset = NewObject<UTransitionPreset>(GetTransientPackage());

				// Try to load default Fade data to get the material
				UTransitionPreset* FadeData = Manager->GetDefaultFadePreset();

				if (FadeData)
				{
					TempPreset->EffectClass = FadeData->EffectClass;
					TempPreset->TransitionMaterial = FadeData->TransitionMaterial;
				}
				else
				{
					// Fallback to manual setup if DataAsset is missing
					TempPreset->EffectClass = UPostProcessTransitionEffect::StaticClass();

					// Try to load the Master material mentioned in docs
					TempPreset->TransitionMaterial = Manager->GetDefaultMasterMaterial();

					if (!TempPreset->TransitionMaterial)
					{
						// Last resort: Log warning
						UE_LOG(LogTransitionFX, Warning, TEXT("QuickFade: Could not find DA_FadeToBlack or M_Transition_Master. Transition may not be visible."));
					}
				}

				TempPreset->DefaultDuration = Duration;

				FTransitionParameters Params;
				static const FName ColorParamName(TEXT("Color"));
				Params.VectorParams.Add(ColorParamName, FLinearColor::Black);

				Manager->StartTransition(TempPreset, Mode, 1.0f, false, false, Params);
			}
		}
	}
}

void UTransitionBlueprintLibrary::QuickFadeToBlack(const UObject* WorldContextObject, float Duration)
{
	QuickFadeInternal(WorldContextObject, Duration, ETransitionMode::Forward);
}

void UTransitionBlueprintLibrary::QuickFadeFromBlack(const UObject* WorldContextObject, float Duration)
{
	QuickFadeInternal(WorldContextObject, Duration, ETransitionMode::Reverse);
}

void UTransitionBlueprintLibrary::PlayTransitionAndWaitWithDuration(const UObject* WorldContextObject, UTransitionPreset* Preset, ETransitionMode Mode, float Duration, bool bInvert, FTransitionParameters OverrideParams, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FTransitionLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			UTransitionManagerSubsystem* Manager = World->GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>();
			if (Manager)
			{
				if (Preset)
				{
					float PlaySpeed = 1.0f;
					if (Duration <= TransitionFXConfig::MinDurationThreshold)
					{
						PlaySpeed = TransitionFXConfig::FallbackPlaySpeed;
					}
					else
					{
						PlaySpeed = Preset->DefaultDuration / Duration;
					}

					Manager->StartTransition(Preset, Mode, PlaySpeed, bInvert, false, OverrideParams);
					LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransitionLatentAction(LatentInfo, Manager));
				}
				else
				{
					UE_LOG(LogTransitionFX, Warning, TEXT("PlayTransitionAndWaitWithDuration called with null preset."));
				}
			}
		}
	}
}
