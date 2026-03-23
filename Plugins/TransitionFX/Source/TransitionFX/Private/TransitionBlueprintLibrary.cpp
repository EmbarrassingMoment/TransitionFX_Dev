// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionBlueprintLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/LatentActionManager.h"
#include "LatentActions.h"
#include "TransitionFXConfig.h"
#include "TransitionManagerSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PostProcessTransitionEffect.h"
#include "TransitionPreset.h"
#include "Curves/CurveFloat.h"
#include "TransitionFX.h"

/**
 * Latent action that polls the transition manager and completes
 * when the current transition finishes or the manager becomes invalid.
 */
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

/**
 * Latent action that starts a fade-out transition, waits for it to finish,
 * then opens the specified level. Prepares auto-reverse for the new level.
 */
class FOpenLevelTransitionLatentAction : public FPendingLatentAction
{
public:
	FLatentActionInfo ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	TWeakObjectPtr<UTransitionManagerSubsystem> Manager;
	FName LevelName;
	TWeakObjectPtr<const UObject> WorldContextObject;

	FOpenLevelTransitionLatentAction(const FLatentActionInfo& InLatentInfo, UTransitionManagerSubsystem* InManager, FName InLevelName, UTransitionPreset* Preset, float Duration, const UObject* InWorldContextObject)
		: ExecutionFunction(InLatentInfo)
		, OutputLink(InLatentInfo.Linkage)
		, CallbackTarget(InLatentInfo.CallbackTarget)
		, Manager(InManager)
		, LevelName(InLevelName)
		, WorldContextObject(InWorldContextObject)
	{
		if (Manager.IsValid())
		{
			// Prepare Auto Reverse for the NEXT level
			Manager->PrepareAutoReverseTransition(Preset, Duration);

			// Calculate PlaySpeed
			float PlaySpeed = 1.0f;
			if (Duration <= TransitionFXConfig::MinDurationThreshold)
			{
				PlaySpeed = TransitionFXConfig::FallbackPlaySpeed;
			}
			else
			{
				PlaySpeed = Preset->DefaultDuration / Duration;
			}

			// Start Fade Out (Forward, Invert=False)
			Manager->StartTransition(Preset, ETransitionMode::Forward, PlaySpeed, false);
		}
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		bool bFinished = false;

		if (Manager.IsValid())
		{
			// If transition finished (reached 1.0), we trigger OpenLevel
			if (Manager->IsCurrentTransitionFinished())
			{
				if (const UObject* WorldContext = WorldContextObject.Get())
				{
					UGameplayStatics::OpenLevel(WorldContext, LevelName);
				}
				bFinished = true;
			}
			// Safety: If transition was stopped manually, we should abort to avoid hanging
			else if (!Manager->IsTransitionPlaying())
			{
				UE_LOG(LogTransitionFX, Warning, TEXT("OpenLevelWithTransitionAndWait: Transition stopped manually. Aborting level load."));
				bFinished = true;
			}
		}
		else
		{
			bFinished = true;
		}

		Response.FinishAndTriggerIf(bFinished, ExecutionFunction.ExecutionFunction, OutputLink, CallbackTarget);
	}

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Opening Level %s after transition..."), *LevelName.ToString());
	}
#endif
};

/** Starts a transition and registers a latent action that completes when the transition finishes. */
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

/** Selects a random preset from the array and delegates to PlayTransitionAndWait. */
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

	if (!SelectedPreset)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("PlayRandomTransitionAndWait: Selected preset at index %d is null."), Index);

		// Finish immediately to prevent hanging
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
			if (LatentActionManager.FindExistingAction<FTransitionLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
			{
				LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransitionLatentAction(LatentInfo, nullptr));
			}
		}
		return;
	}

	PlayTransitionAndWait(WorldContextObject, SelectedPreset, Mode, PlaySpeed, bInvert, OverrideParams, LatentInfo);
}

/**
 * Maps a linear alpha value through the specified easing function.
 * Supports sine, cubic, exponential, elastic, bounce, and custom curve easing.
 */
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
	{
		if (x == 0.0f) return 0.0f;
		if (x == 1.0f) return 1.0f;
		const float x2 = x * 2.0f;
		if (x2 < 1.0f) return 0.5f * FMath::Pow(2.0f, 10.0f * (x2 - 1.0f));
		return 0.5f * (2.0f - FMath::Pow(2.0f, -10.0f * (x2 - 1.0f)));
	}

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

/** Queries the transition manager subsystem to check if any transition is active. */
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

/**
 * Internal helper that creates a transient preset from the default fade data
 * and starts a transition in the specified mode with a black color overlay.
 */
static void QuickFadeInternal(const UObject* WorldContextObject, float Duration, ETransitionMode Mode)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTransitionManagerSubsystem* Manager = GameInstance->GetSubsystem<UTransitionManagerSubsystem>())
			{
				UTransitionPreset* TempPreset = NewObject<UTransitionPreset>(Manager);

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

					// Try to load the Fade material as fallback
					TempPreset->TransitionMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/TransitionFX/Materials/M_Transition_Fade.M_Transition_Fade"));

					if (!TempPreset->TransitionMaterial)
					{
						// Last resort: Log warning
						UE_LOG(LogTransitionFX, Warning, TEXT("QuickFade: Could not find DA_FadeToBlack or M_Transition_Fade. Transition may not be visible."));
					}
				}

				TempPreset->DefaultDuration = Duration;

				FTransitionParameters Params;
				Params.VectorParams.Add(TransitionFXConfig::ColorParamName, FLinearColor::Black);

				Manager->StartTransition(TempPreset, Mode, 1.0f, false, false, Params);
			}
		}
	}
}

/** Convenience function to fade the screen to black (forward direction). */
void UTransitionBlueprintLibrary::QuickFadeToBlack(const UObject* WorldContextObject, float Duration)
{
	QuickFadeInternal(WorldContextObject, Duration, ETransitionMode::Forward);
}

/** Convenience function to fade the screen from black (reverse direction). */
void UTransitionBlueprintLibrary::QuickFadeFromBlack(const UObject* WorldContextObject, float Duration)
{
	QuickFadeInternal(WorldContextObject, Duration, ETransitionMode::Reverse);
}

/** Delegates to the manager subsystem to perform a "Fade Out -> Open Level -> Fade In" sequence. */
void UTransitionBlueprintLibrary::OpenLevelWithTransition(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTransitionManagerSubsystem* Manager = GameInstance->GetSubsystem<UTransitionManagerSubsystem>())
			{
				Manager->OpenLevelWithTransition(WorldContextObject, LevelName, Preset, Duration);
			}
		}
	}
}

/** Starts a fade-out, opens the level when done, and completes the latent action. Falls back to default preset if null. */
void UTransitionBlueprintLibrary::OpenLevelWithTransitionAndWait(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FOpenLevelTransitionLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			UTransitionManagerSubsystem* Manager = World->GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>();
			if (Manager)
			{
				if (!Preset)
				{
					UE_LOG(LogTransitionFX, Warning, TEXT("OpenLevelWithTransitionAndWait: Null Preset provided. Falling back to default fade."));
					Preset = Manager->GetDefaultFadePreset();
				}

				LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FOpenLevelTransitionLatentAction(LatentInfo, Manager, LevelName, Preset, Duration, WorldContextObject));
			}
		}
	}
}

/** Converts duration to play speed, starts the transition, and registers a latent action for completion. */
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
					// Finish immediately to prevent Blueprint from hanging
					LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransitionLatentAction(LatentInfo, nullptr));
				}
			}
		}
	}
}
