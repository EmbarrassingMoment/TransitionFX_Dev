// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "STransitionPreviewPanel.h"
#include "TransitionPreviewViewport.h"
#include "PreviewScene.h"
#include "Widgets/SViewport.h"
#include "Slate/SceneViewport.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/MaterialInstanceConstant.h"

#define LOCTEXT_NAMESPACE "TransitionFXEditor"

void STransitionPreviewPanel::Construct(const FArguments& InArgs)
{
	// Initialize state
	SelectedIndex = 0;
	CurrentProgress = 0.0f;
	PlaySpeed = 1.0f;
	Duration = 1.0f;
	bIsPlaying = false;
	bIsReversing = false;
	bLooping = true;
	bInvert = false;
	bSliderCaptured = false;
	ViewportWidth = 480.0f;
	ViewportHeight = 270.0f;

	// Resolution options
	ResolutionOptions.Add(MakeShared<FString>(TEXT("480 x 270")));
	ResolutionOptions.Add(MakeShared<FString>(TEXT("640 x 360")));
	ResolutionOptions.Add(MakeShared<FString>(TEXT("800 x 450")));

	// Discover effects
	DiscoverEffects();

	// Create preview scene
	PreviewScene = MakeShared<FPreviewScene>(FPreviewScene::ConstructionValues());

	// Create viewport client
	ViewportClient = MakeShared<FTransitionPreviewViewportClient>(PreviewScene.Get());

	// Build UI
	ChildSlot
	[
		SNew(SVerticalBox)

		// Top bar: Effect selector + Invert
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0, 0, 8, 0)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&EffectNames)
				.OnSelectionChanged(this, &STransitionPreviewPanel::OnEffectSelected)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(*Item));
				})
				.Content()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (EffectNames.IsValidIndex(SelectedIndex))
						{
							return FText::FromString(*EffectNames[SelectedIndex]);
						}
						return FText::GetEmpty();
					})
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 4, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("InvertLabel", "Invert"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return bInvert ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged(this, &STransitionPreviewPanel::OnInvertChanged)
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Viewport
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(4.0f)
		[
			SNew(SBox)
			.WidthOverride_Lambda([this]() { return ViewportWidth; })
			.HeightOverride_Lambda([this]() { return ViewportHeight; })
			[
				SAssignNew(ViewportWidget, SViewport)
				.EnableGammaCorrection(false)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// Progress bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ProgressLabel", "Progress"))
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SSlider)
				.Value_Lambda([this]() { return CurrentProgress; })
				.OnValueChanged(this, &STransitionPreviewPanel::OnProgressChanged)
				.OnMouseCaptureBegin(this, &STransitionPreviewPanel::OnProgressCaptureBegin)
				.OnMouseCaptureEnd(this, &STransitionPreviewPanel::OnProgressCaptureEnd)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8, 0, 4, 0)
			[
				SNew(STextBlock)
				.Text(this, &STransitionPreviewPanel::GetProgressText)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("ResetButton", "Reset"))
				.OnClicked_Lambda([this]() { Reset(); return FReply::Handled(); })
			]
		]

		// Playback controls
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("PlayButton", "Play"))
				.OnClicked_Lambda([this]() { PlayForward(); return FReply::Handled(); })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("ReverseButton", "Reverse"))
				.OnClicked_Lambda([this]() { PlayReverse(); return FReply::Handled(); })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 4, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("StopButton", "Stop"))
				.OnClicked_Lambda([this]() { Stop(); return FReply::Handled(); })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 12, 0)
			[
				SNew(SButton)
				.Text(this, &STransitionPreviewPanel::GetLoopButtonText)
				.OnClicked_Lambda([this]() { ToggleLoop(); return FReply::Handled(); })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SpeedLabel", "Speed"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 12, 0)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.1f)
				.MaxValue(5.0f)
				.Delta(0.1f)
				.Value_Lambda([this]() { return PlaySpeed; })
				.OnValueChanged(this, &STransitionPreviewPanel::OnSpeedChanged)
				.OnValueCommitted(this, &STransitionPreviewPanel::OnSpeedCommitted)
				.MinDesiredWidth(60.0f)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ResolutionLabel", "Size"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&ResolutionOptions)
				.OnSelectionChanged(this, &STransitionPreviewPanel::OnResolutionSelected)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(*Item));
				})
				.Content()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						FString Current = FString::Printf(TEXT("%.0f x %.0f"), ViewportWidth, ViewportHeight);
						return FText::FromString(Current);
					})
				]
			]
		]
	];

	// Create scene viewport and attach to widget
	SceneViewport = MakeShared<FSceneViewport>(ViewportClient.Get(), ViewportWidget);
	ViewportWidget->SetViewportInterface(SceneViewport.ToSharedRef());

	// Select first effect if available
	if (Effects.Num() > 0)
	{
		OnEffectSelected(EffectNames[0], ESelectInfo::Direct);
	}

	// Register tick
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateSP(this, &STransitionPreviewPanel::OnTick));
}

STransitionPreviewPanel::~STransitionPreviewPanel()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);

	SceneViewport.Reset();
	ViewportClient.Reset();
	PreviewScene.Reset();
}

void STransitionPreviewPanel::DiscoverEffects()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> MaterialAssets;
	AssetRegistry.GetAssetsByPath(FName("/TransitionFX/Materials/Instances"), MaterialAssets, false);

	for (const FAssetData& Asset : MaterialAssets)
	{
		if (Asset.AssetClassPath == UMaterialInstanceConstant::StaticClass()->GetClassPathName())
		{
			FEffectEntry Entry;
			FString AssetName = Asset.AssetName.ToString();
			// MI_Transition_Blinds -> Blinds
			AssetName.RemoveFromStart(TEXT("MI_Transition_"));
			Entry.DisplayName = AssetName;
			Entry.MaterialPath = Asset.GetSoftObjectPath();
			Effects.Add(Entry);
		}
	}

	// Sort alphabetically
	Effects.Sort([](const FEffectEntry& A, const FEffectEntry& B)
	{
		return A.DisplayName < B.DisplayName;
	});

	// Build name list for combo box
	for (const FEffectEntry& Entry : Effects)
	{
		EffectNames.Add(MakeShared<FString>(Entry.DisplayName));
	}
}

void STransitionPreviewPanel::OnEffectSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if (!NewValue.IsValid())
	{
		return;
	}

	for (int32 i = 0; i < EffectNames.Num(); ++i)
	{
		if (*EffectNames[i] == *NewValue)
		{
			SelectedIndex = i;
			break;
		}
	}

	// Load material and set on viewport
	UMaterialInterface* Material = Cast<UMaterialInterface>(Effects[SelectedIndex].MaterialPath.TryLoad());
	if (Material && ViewportClient.IsValid())
	{
		ViewportClient->SetPreviewMaterial(Material);
		ViewportClient->SetInvert(bInvert);
		CurrentProgress = 0.0f;
		bIsPlaying = false;
	}
}

bool STransitionPreviewPanel::OnTick(float DeltaTime)
{
	if (!bIsPlaying || bSliderCaptured)
	{
		return true;
	}

	float Delta = DeltaTime / Duration * PlaySpeed;
	if (bIsReversing)
	{
		Delta = -Delta;
	}

	CurrentProgress += Delta;

	if (CurrentProgress >= 1.0f)
	{
		CurrentProgress = 1.0f;
		if (bLooping)
		{
			// Ping-pong
			bIsReversing = true;
		}
		else
		{
			bIsPlaying = false;
		}
	}
	else if (CurrentProgress <= 0.0f)
	{
		CurrentProgress = 0.0f;
		if (bLooping)
		{
			// Ping-pong
			bIsReversing = false;
		}
		else
		{
			bIsPlaying = false;
		}
	}

	if (ViewportClient.IsValid())
	{
		ViewportClient->SetProgress(CurrentProgress);
	}

	return true;
}

void STransitionPreviewPanel::PlayForward()
{
	bIsPlaying = true;
	bIsReversing = false;
	if (CurrentProgress >= 1.0f)
	{
		CurrentProgress = 0.0f;
	}
}

void STransitionPreviewPanel::PlayReverse()
{
	bIsPlaying = true;
	bIsReversing = true;
	if (CurrentProgress <= 0.0f)
	{
		CurrentProgress = 1.0f;
	}
}

void STransitionPreviewPanel::Stop()
{
	bIsPlaying = false;
}

void STransitionPreviewPanel::Reset()
{
	CurrentProgress = 0.0f;
	bIsPlaying = false;
	bIsReversing = false;
	if (ViewportClient.IsValid())
	{
		ViewportClient->SetProgress(0.0f);
	}
}

void STransitionPreviewPanel::ToggleLoop()
{
	bLooping = !bLooping;
}

void STransitionPreviewPanel::OnProgressChanged(float NewValue)
{
	CurrentProgress = NewValue;
	if (ViewportClient.IsValid())
	{
		ViewportClient->SetProgress(CurrentProgress);
	}
}

void STransitionPreviewPanel::OnProgressCaptureBegin()
{
	bSliderCaptured = true;
}

void STransitionPreviewPanel::OnProgressCaptureEnd()
{
	bSliderCaptured = false;
}

void STransitionPreviewPanel::OnInvertChanged(ECheckBoxState NewState)
{
	bInvert = (NewState == ECheckBoxState::Checked);
	if (ViewportClient.IsValid())
	{
		ViewportClient->SetInvert(bInvert);
	}
}

void STransitionPreviewPanel::OnSpeedChanged(float NewValue)
{
	PlaySpeed = NewValue;
}

void STransitionPreviewPanel::OnSpeedCommitted(float NewValue, ETextCommit::Type CommitType)
{
	PlaySpeed = NewValue;
}

void STransitionPreviewPanel::OnResolutionSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if (!NewValue.IsValid())
	{
		return;
	}

	if (*NewValue == TEXT("480 x 270"))
	{
		ViewportWidth = 480.0f;
		ViewportHeight = 270.0f;
	}
	else if (*NewValue == TEXT("640 x 360"))
	{
		ViewportWidth = 640.0f;
		ViewportHeight = 360.0f;
	}
	else if (*NewValue == TEXT("800 x 450"))
	{
		ViewportWidth = 800.0f;
		ViewportHeight = 450.0f;
	}
}

FText STransitionPreviewPanel::GetProgressText() const
{
	return FText::AsNumber(FMath::RoundToInt(CurrentProgress * 100));
}

FText STransitionPreviewPanel::GetSpeedText() const
{
	return FText::Format(LOCTEXT("SpeedFormat", "{0}x"), FText::AsNumber(PlaySpeed));
}

FText STransitionPreviewPanel::GetLoopButtonText() const
{
	return bLooping ? LOCTEXT("LoopOn", "Loop: ON") : LOCTEXT("LoopOff", "Loop: OFF");
}

#undef LOCTEXT_NAMESPACE
