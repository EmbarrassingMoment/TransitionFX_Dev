#include "WidgetLayerSampleWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "TransitionManagerSubsystem.h"
#include "TransitionPreset.h"

namespace
{
	const TCHAR* IdleStatus = TEXT("Idle. Play Widget Layer covers this panel too; Play PostProcess leaves it visible.");

	FString PresetDisplayName(const TSoftObjectPtr<UTransitionPreset>& Preset)
	{
		return Preset.IsNull() ? FString(TEXT("(none)")) : Preset.GetAssetName();
	}
}

void UWidgetLayerSampleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PrevButton)
	{
		PrevButton->OnClicked.AddUniqueDynamic(this, &UWidgetLayerSampleWidget::HandlePrevClicked);
	}
	if (NextButton)
	{
		NextButton->OnClicked.AddUniqueDynamic(this, &UWidgetLayerSampleWidget::HandleNextClicked);
	}
	if (PlayWidgetButton)
	{
		PlayWidgetButton->OnClicked.AddUniqueDynamic(this, &UWidgetLayerSampleWidget::HandlePlayWidgetClicked);
	}
	if (PlayPostProcessButton)
	{
		PlayPostProcessButton->OnClicked.AddUniqueDynamic(this, &UWidgetLayerSampleWidget::HandlePlayPostProcessClicked);
	}

	if (UTransitionManagerSubsystem* Manager = GetTransitionManager())
	{
		Manager->OnTransitionHoldStarted.AddUniqueDynamic(this, &UWidgetLayerSampleWidget::HandleTransitionHoldStarted);
		Manager->OnTransitionCompleted.AddUniqueDynamic(this, &UWidgetLayerSampleWidget::HandleTransitionCompleted);

		// Resolve the soft references up front and warm up the materials so the first
		// click plays without a shader-compile hitch. Synchronous loading is fine for a sample.
		TArray<UTransitionPreset*> Loaded;
		for (const TSoftObjectPtr<UTransitionPreset>& Soft : WidgetPresets)
		{
			if (UTransitionPreset* Preset = Soft.LoadSynchronous())
			{
				Loaded.Add(Preset);
			}
		}
		for (const TSoftObjectPtr<UTransitionPreset>& Soft : PostProcessPresets)
		{
			if (UTransitionPreset* Preset = Soft.LoadSynchronous())
			{
				Loaded.Add(Preset);
			}
		}
		Manager->PreloadTransitionPresets(Loaded);
	}

	CurrentIndex = 0;
	bIsPlaying = false;
	RefreshPresetLabel();
	SetStatus(WidgetPresets.Num() > 0
		? FString(IdleStatus)
		: FString(TEXT("No presets assigned. Fill WidgetPresets on the Widget Blueprint.")));
}

void UWidgetLayerSampleWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoldTimerHandle);
	}
	if (UTransitionManagerSubsystem* Manager = GetTransitionManager())
	{
		Manager->OnTransitionHoldStarted.RemoveDynamic(this, &UWidgetLayerSampleWidget::HandleTransitionHoldStarted);
		Manager->OnTransitionCompleted.RemoveDynamic(this, &UWidgetLayerSampleWidget::HandleTransitionCompleted);
	}
	Super::NativeDestruct();
}

void UWidgetLayerSampleWidget::HandlePrevClicked()
{
	if (WidgetPresets.Num() == 0)
	{
		return;
	}
	CurrentIndex = (CurrentIndex - 1 + WidgetPresets.Num()) % WidgetPresets.Num();
	RefreshPresetLabel();
}

void UWidgetLayerSampleWidget::HandleNextClicked()
{
	if (WidgetPresets.Num() == 0)
	{
		return;
	}
	CurrentIndex = (CurrentIndex + 1) % WidgetPresets.Num();
	RefreshPresetLabel();
}

void UWidgetLayerSampleWidget::HandlePlayWidgetClicked()
{
	Play(WidgetPresets, TEXT("widget layer"));
}

void UWidgetLayerSampleWidget::HandlePlayPostProcessClicked()
{
	Play(PostProcessPresets, TEXT("PostProcess"));
}

void UWidgetLayerSampleWidget::Play(const TArray<TSoftObjectPtr<UTransitionPreset>>& Presets, const FString& LayerLabel)
{
	if (bIsPlaying)
	{
		SetStatus(TEXT("Still playing. Wait for the transition to finish."));
		return;
	}

	UTransitionManagerSubsystem* Manager = GetTransitionManager();
	if (!Manager)
	{
		SetStatus(TEXT("TransitionManagerSubsystem not found."));
		return;
	}

	UTransitionPreset* Preset = Presets.IsValidIndex(CurrentIndex) ? Presets[CurrentIndex].LoadSynchronous() : nullptr;
	if (!Preset)
	{
		SetStatus(FString::Printf(TEXT("No %s preset assigned at index %d."), *LayerLabel, CurrentIndex));
		return;
	}

	bIsPlaying = true;
	Manager->StartTransition(Preset, ETransitionMode::Forward, 1.0f, /*bInvert=*/false, /*bHoldAtMax=*/true);
	SetStatus(FString::Printf(TEXT("Playing %s (%s): FadeOut (Forward) -> Hold %.1fs -> FadeIn (Forward, Invert)"),
		*Preset->GetName(), *LayerLabel, HoldDuration));
}

void UWidgetLayerSampleWidget::HandleTransitionHoldStarted()
{
	if (!bIsPlaying)
	{
		return;
	}

	// Fade in by flipping the mask and replaying forward (0 -> 1) instead of rewinding
	// the progress (Reverse). This is the same FadeOut -> FadeIn pattern the runtime uses
	// for OpenLevelWithTransition, so the sample exercises the Invert path of the widget
	// materials as well. bAutoComplete=true lets the fade-in finish and auto-stop.
	TWeakObjectPtr<UWidgetLayerSampleWidget> WeakThis(this);
	auto FadeIn = [WeakThis]()
	{
		if (UWidgetLayerSampleWidget* Self = WeakThis.Get())
		{
			if (UTransitionManagerSubsystem* Manager = Self->GetTransitionManager())
			{
				Manager->InvertTransition(/*bAutoComplete=*/true);
			}
		}
	};

	UWorld* World = GetWorld();
	if (HoldDuration <= 0.0f || !World)
	{
		FadeIn();
		return;
	}
	World->GetTimerManager().SetTimer(HoldTimerHandle, FTimerDelegate::CreateLambda(FadeIn), HoldDuration, false);
}

void UWidgetLayerSampleWidget::HandleTransitionCompleted()
{
	if (!bIsPlaying)
	{
		return;
	}
	bIsPlaying = false;
	SetStatus(IdleStatus);
}

void UWidgetLayerSampleWidget::RefreshPresetLabel()
{
	if (!PresetNameText)
	{
		return;
	}
	if (!WidgetPresets.IsValidIndex(CurrentIndex))
	{
		PresetNameText->SetText(FText::FromString(TEXT("No presets assigned")));
		return;
	}

	const FString PostProcessName = PostProcessPresets.IsValidIndex(CurrentIndex)
		? PresetDisplayName(PostProcessPresets[CurrentIndex])
		: FString(TEXT("(none)"));
	PresetNameText->SetText(FText::FromString(FString::Printf(TEXT("[%d/%d] %s   |   PostProcess: %s"),
		CurrentIndex + 1, WidgetPresets.Num(), *PresetDisplayName(WidgetPresets[CurrentIndex]), *PostProcessName)));
}

void UWidgetLayerSampleWidget::SetStatus(const FString& Status)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Status));
	}
}

UTransitionManagerSubsystem* UWidgetLayerSampleWidget::GetTransitionManager() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UTransitionManagerSubsystem>() : nullptr;
}
