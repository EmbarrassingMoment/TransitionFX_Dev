// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "STransitionPreviewPanel.h"
#include "TransitionPreviewViewport.h"
#include "TransitionPreviewGifCapture.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "TransitionBlueprintLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/MaterialInstanceConstant.h"
#include "DesktopPlatformModule.h"

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
	CaptureFrameRate = 30;
	GifPlaySpeed = 0.5f;
	ViewportWidth = 480.0f;
	ViewportHeight = 270.0f;
#if TRANSITIONFX_DEV_TOOLS
	bIsBatchCapturing = false;
	BatchCaptureIndex = 0;
	bIsBatchCapturingEasing = false;
	BatchEasingIndex = 0;
	SavedEffectIndex = 0;
	SavedEasing = ETransitionEasing::Linear;
#endif

	// Create the GIF capture engine before the widget tree binds lambdas that query it
	GifCapture = MakeUnique<FTransitionPreviewGifCapture>();
#if TRANSITIONFX_DEV_TOOLS
	GifCapture->OnWriteFinished.BindSP(this, &STransitionPreviewPanel::OnGifWriteFinished);
#endif

	// Resolution options
	ResolutionOptions.Add(MakeShared<FString>(TEXT("480 x 270")));
	ResolutionOptions.Add(MakeShared<FString>(TEXT("640 x 360")));
	ResolutionOptions.Add(MakeShared<FString>(TEXT("800 x 450")));

	// Easing options
	SelectedEasing = ETransitionEasing::Linear;
	PopulateEasingOptions();

	// Discover effects
	DiscoverEffects();

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
			.Padding(0, 0, 8, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 4, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("EasingLabel", "Easing"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&EasingNames)
					.OnSelectionChanged(this, &STransitionPreviewPanel::OnEasingSelected)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
					{
						return SNew(STextBlock).Text(FText::FromString(*Item));
					})
					.Content()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const UEnum* EnumPtr = StaticEnum<ETransitionEasing>();
							return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(SelectedEasing));
						})
					]
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

		// Viewport (SEditorViewport subclass - manages lifecycle internally)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(4.0f)
		[
			SNew(SBox)
			.WidthOverride_Lambda([this]() { return ViewportWidth; })
			.HeightOverride_Lambda([this]() { return ViewportHeight; })
			[
				SAssignNew(PreviewViewport, STransitionPreviewViewport)
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

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// GIF capture
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(this, &STransitionPreviewPanel::GetCaptureButtonText)
				.IsEnabled(this, &STransitionPreviewPanel::IsCaptureButtonEnabled)
				.OnClicked_Lambda([this]() { StartGifCapture(); return FReply::Handled(); })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8, 0, 4, 0)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("GifSpeedLabel", "GIF Speed"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 8, 0)
			[
				SNew(SSpinBox<float>)
				.MinValue(0.1f)
				.MaxValue(2.0f)
				.Delta(0.1f)
				.Value_Lambda([this]() { return GifPlaySpeed; })
				.OnValueChanged_Lambda([this](float NewValue) { GifPlaySpeed = NewValue; })
				.OnValueCommitted_Lambda([this](float NewValue, ETextCommit::Type) { GifPlaySpeed = NewValue; })
				.MinDesiredWidth(60.0f)
			]

#if TRANSITIONFX_DEV_TOOLS
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8, 0, 0, 0)
			[
				SNew(SButton)
				.Text(this, &STransitionPreviewPanel::GetBatchCaptureButtonText)
				.IsEnabled(this, &STransitionPreviewPanel::IsBatchCaptureButtonEnabled)
				.OnClicked_Lambda([this]() { StartBatchCapture(); return FReply::Handled(); })
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8, 0, 0, 0)
			[
				SNew(SButton)
				.Text(this, &STransitionPreviewPanel::GetBatchCaptureEasingButtonText)
				.IsEnabled(this, &STransitionPreviewPanel::IsBatchCaptureEasingButtonEnabled)
				.OnClicked_Lambda([this]() { StartBatchCaptureEasing(); return FReply::Handled(); })
			]
#endif

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
#if TRANSITIONFX_DEV_TOOLS
					if (GifCapture->IsCapturing() && bIsBatchCapturingEasing)
					{
						return FText::Format(
							LOCTEXT("EasingBatchCaptureProgress", "Easing {0}/{1} — Frame {2}/{3}"),
							FText::AsNumber(BatchEasingIndex + 1),
							FText::AsNumber(BatchEasingList.Num()),
							FText::AsNumber(GifCapture->GetFrameIndex()),
							FText::AsNumber(GifCapture->GetTotalFrames()));
					}
					if (GifCapture->IsCapturing() && bIsBatchCapturing)
					{
						return FText::Format(
							LOCTEXT("BatchCaptureProgress", "Batch {0}/{1} — Frame {2}/{3}"),
							FText::AsNumber(BatchCaptureIndex + 1),
							FText::AsNumber(Effects.Num()),
							FText::AsNumber(GifCapture->GetFrameIndex()),
							FText::AsNumber(GifCapture->GetTotalFrames()));
					}
#endif
					if (GifCapture->IsCapturing())
					{
						return FText::Format(
							LOCTEXT("CaptureProgress", "Capturing... {0}/{1}"),
							FText::AsNumber(GifCapture->GetFrameIndex()),
							FText::AsNumber(GifCapture->GetTotalFrames()));
					}
					return FText::GetEmpty();
				})
			]
		]
	];

	// Select first effect if available
	if (Effects.Num() > 0)
	{
		OnEffectSelected(EffectNames[0], ESelectInfo::Direct);
	}

	// Register tick
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateSP(this, &STransitionPreviewPanel::OnTick));
}

STransitionPreviewPanel::STransitionPreviewPanel() = default;

STransitionPreviewPanel::~STransitionPreviewPanel()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
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

	int32 FoundIndex = EffectNames.IndexOfByKey(NewValue);
	if (FoundIndex != INDEX_NONE)
	{
		SelectedIndex = FoundIndex;
	}

	// Load material and set on viewport
	UMaterialInterface* Material = Cast<UMaterialInterface>(Effects[SelectedIndex].MaterialPath.TryLoad());
	if (Material && PreviewViewport.IsValid())
	{
		PreviewViewport->SetPreviewMaterial(Material);
		PreviewViewport->SetInvert(bInvert);
		CurrentProgress = 0.0f;
		bIsPlaying = false;
	}
}

bool STransitionPreviewPanel::OnTick(float DeltaTime)
{
	// GIF capture mode — driven by fixed-step frame capture
	if (GifCapture->IsCapturing())
	{
		GifCapture->Tick();
		// Keep the progress slider tracking the capture
		CurrentProgress = GifCapture->GetCurrentRawProgress();
		return true;
	}

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
			bIsReversing = false;
		}
		else
		{
			bIsPlaying = false;
		}
	}

	if (PreviewViewport.IsValid())
	{
		PreviewViewport->SetProgress(GetEasedProgress(CurrentProgress));
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
	if (PreviewViewport.IsValid())
	{
		PreviewViewport->SetProgress(0.0f);
	}
}

void STransitionPreviewPanel::ToggleLoop()
{
	bLooping = !bLooping;
}

void STransitionPreviewPanel::OnProgressChanged(float NewValue)
{
	CurrentProgress = NewValue;
	if (PreviewViewport.IsValid())
	{
		PreviewViewport->SetProgress(GetEasedProgress(CurrentProgress));
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
	if (PreviewViewport.IsValid())
	{
		PreviewViewport->SetInvert(bInvert);
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

void STransitionPreviewPanel::PopulateEasingOptions()
{
	const UEnum* EnumPtr = StaticEnum<ETransitionEasing>();
	if (!EnumPtr)
	{
		return;
	}

	for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i) // -1 to skip _MAX
	{
		ETransitionEasing Value = static_cast<ETransitionEasing>(EnumPtr->GetValueByIndex(i));

		// Skip Custom — requires a UCurveFloat asset
		if (Value == ETransitionEasing::Custom)
		{
			continue;
		}

		FText DisplayName = EnumPtr->GetDisplayNameTextByIndex(i);
		EasingNames.Add(MakeShared<FString>(DisplayName.ToString()));
	}
}

void STransitionPreviewPanel::OnEasingSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if (!NewValue.IsValid())
	{
		return;
	}

	const UEnum* EnumPtr = StaticEnum<ETransitionEasing>();
	if (!EnumPtr)
	{
		return;
	}

	for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
	{
		FText DisplayName = EnumPtr->GetDisplayNameTextByIndex(i);
		if (DisplayName.ToString() == *NewValue)
		{
			SelectedEasing = static_cast<ETransitionEasing>(EnumPtr->GetValueByIndex(i));
			break;
		}
	}

	// Re-apply easing to current progress
	if (PreviewViewport.IsValid())
	{
		PreviewViewport->SetProgress(GetEasedProgress(CurrentProgress));
	}
}

float STransitionPreviewPanel::GetEasedProgress(float RawProgress) const
{
	return UTransitionBlueprintLibrary::ApplyEasing(RawProgress, SelectedEasing);
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

// ─────────────────────────────────────────────
// GIF Capture
// ─────────────────────────────────────────────

void STransitionPreviewPanel::StartGifCapture()
{
	if (GifCapture->IsCapturing() || !PreviewViewport.IsValid() || Effects.Num() == 0)
	{
		return;
	}

	// Stop normal playback and reset
	bIsPlaying = false;
	bIsReversing = false;
	CurrentProgress = 0.0f;

	FTransitionPreviewGifCapture::FStartParams CaptureParams;
	CaptureParams.Viewport = PreviewViewport;
	CaptureParams.FrameRate = CaptureFrameRate;
	CaptureParams.Duration = Duration;
	CaptureParams.GifPlaySpeed = GifPlaySpeed;
	CaptureParams.EasingEvaluator = [this](float RawProgress) { return GetEasedProgress(RawProgress); };

	if (EffectNames.IsValidIndex(SelectedIndex))
	{
		CaptureParams.DialogDefaultFileName = *EffectNames[SelectedIndex];
	}

#if TRANSITIONFX_DEV_TOOLS
	// Batch modes save to a mapped filename without prompting or notifying per file.
	if (bIsBatchCapturingEasing)
	{
		CaptureParams.SavePath = BatchOutputDir / GetGifFilenameForEasing(SelectedEasing);
		CaptureParams.bShowSuccessNotification = false;
	}
	else if (bIsBatchCapturing)
	{
		CaptureParams.SavePath = BatchOutputDir / GetGifFilenameForEffect(Effects[SelectedIndex].DisplayName);
		CaptureParams.bShowSuccessNotification = false;
	}
#endif

	GifCapture->Start(CaptureParams);
}

#if TRANSITIONFX_DEV_TOOLS
void STransitionPreviewPanel::OnGifWriteFinished(bool bSucceeded)
{
	// Advance batch if in batch mode (also on write failure, matching pre-extraction behaviour)
	if (bIsBatchCapturingEasing)
	{
		AdvanceBatchCaptureEasing();
	}
	else if (bIsBatchCapturing)
	{
		AdvanceBatchCapture();
	}
}
#endif

FText STransitionPreviewPanel::GetCaptureButtonText() const
{
	return GifCapture->IsCapturing()
		? LOCTEXT("CaptureButtonCapturing", "Capturing...")
		: LOCTEXT("CaptureButtonIdle", "Capture GIF");
}

bool STransitionPreviewPanel::IsCaptureButtonEnabled() const
{
	return !GifCapture->IsCapturing()
#if TRANSITIONFX_DEV_TOOLS
		&& !bIsBatchCapturing && !bIsBatchCapturingEasing
#endif
		&& Effects.Num() > 0;
}

#if TRANSITIONFX_DEV_TOOLS

// ─────────────────────────────────────────────
// Batch GIF Capture
// ─────────────────────────────────────────────

FString STransitionPreviewPanel::GetGifFilenameForEffect(const FString& DisplayName)
{
	// Mapping from dropdown DisplayName to MISSING_IMAGES.md filenames
	static const TMap<FString, FString> NameToFile = {
		{ TEXT("Fade"),          TEXT("effect_fade.gif") },
		{ TEXT("Iris"),          TEXT("effect_iris.gif") },
		{ TEXT("Heart"),         TEXT("effect_heart_iris.gif") },
		{ TEXT("FlowerIris"),    TEXT("effect_flower_iris.gif") },
		{ TEXT("Diamond"),       TEXT("effect_diamond.gif") },
		{ TEXT("Box"),           TEXT("effect_box.gif") },
		{ TEXT("LinearWipe"),    TEXT("effect_linear_wipe.gif") },
		{ TEXT("Split"),         TEXT("effect_split.gif") },
		{ TEXT("WavyCurtain"),   TEXT("effect_wavy_curtain.gif") },
		{ TEXT("RadialWipe"),    TEXT("effect_radial_wipe.gif") },
		{ TEXT("Tiles"),         TEXT("effect_tiles.gif") },
		{ TEXT("PolkaDots"),     TEXT("effect_polka_dots.gif") },
		{ TEXT("Blinds"),        TEXT("effect_blinds.gif") },
		{ TEXT("Spiral"),        TEXT("effect_spiral.gif") },
		{ TEXT("RandomTiles"),   TEXT("effect_random_tiles.gif") },
		{ TEXT("Wind"),          TEXT("effect_wind.gif") },
		{ TEXT("CrossWipe"),     TEXT("effect_cross_wipe.gif") },
		{ TEXT("ZoomWipe"),      TEXT("effect_zoom_wipe.gif") },
		{ TEXT("TextureMask"),   TEXT("effect_texture_mask.gif") },
		{ TEXT("TVSwitchOff"),   TEXT("effect_tv_switch_off.gif") },
		{ TEXT("Hexagon"),       TEXT("effect_hexagon.gif") },
		{ TEXT("Checkerboard"),  TEXT("effect_checkerboard.gif") },
		{ TEXT("Triangle"),      TEXT("effect_triangle.gif") },
		{ TEXT("Pixelate"),      TEXT("effect_pixelate.gif") },
	};

	const FString* Found = NameToFile.Find(DisplayName);
	if (Found)
	{
		return *Found;
	}

	// Fallback: convert PascalCase to snake_case
	FString Result;
	Result.Reserve(7 + DisplayName.Len() * 2 + 4); // "effect_" + worst-case snake_case + ".gif"
	Result += TEXT("effect_");
	for (int32 i = 0; i < DisplayName.Len(); ++i)
	{
		TCHAR Ch = DisplayName[i];
		if (FChar::IsUpper(Ch) && i > 0)
		{
			Result += TEXT("_");
		}
		Result += FChar::ToLower(Ch);
	}
	Result += TEXT(".gif");
	return Result;
}

void STransitionPreviewPanel::StartBatchCapture()
{
	if (GifCapture->IsCapturing() || bIsBatchCapturing || bIsBatchCapturingEasing || Effects.Num() == 0)
	{
		return;
	}

	// Ask user for output directory
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	// Default to docs/images/ under project root
	FString DefaultDir = FPaths::ProjectDir() / TEXT("docs") / TEXT("images");
	FString ChosenDir;
	bool bChosen = DesktopPlatform->OpenDirectoryDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Select Batch GIF Output Folder"),
		DefaultDir,
		ChosenDir);

	if (!bChosen)
	{
		return;
	}

	// Ensure directory exists
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*ChosenDir);

	BatchOutputDir = ChosenDir;
	BatchCaptureIndex = 0;
	bIsBatchCapturing = true;

	FNotificationInfo Info(FText::Format(
		LOCTEXT("BatchCaptureStartNotification", "Batch capture started: {0} effects"),
		FText::AsNumber(Effects.Num())));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// Select the first effect and start capturing
	SelectedIndex = BatchCaptureIndex;
	OnEffectSelected(EffectNames[SelectedIndex], ESelectInfo::Direct);
	StartGifCapture();
}

void STransitionPreviewPanel::AdvanceBatchCapture()
{
	BatchCaptureIndex++;

	if (BatchCaptureIndex >= Effects.Num())
	{
		// Batch complete
		bIsBatchCapturing = false;

		FNotificationInfo Info(FText::Format(
			LOCTEXT("BatchCaptureCompleteNotification", "Batch capture complete! {0} GIFs saved to {1}"),
			FText::AsNumber(Effects.Num()),
			FText::FromString(BatchOutputDir)));
		Info.ExpireDuration = 8.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// Select next effect and start capture
	SelectedIndex = BatchCaptureIndex;
	OnEffectSelected(EffectNames[SelectedIndex], ESelectInfo::Direct);
	StartGifCapture();
}

FText STransitionPreviewPanel::GetBatchCaptureButtonText() const
{
	if (bIsBatchCapturing)
	{
		return FText::Format(
			LOCTEXT("BatchCapturing", "Batch... ({0}/{1})"),
			FText::AsNumber(BatchCaptureIndex + 1),
			FText::AsNumber(Effects.Num()));
	}
	return LOCTEXT("BatchCaptureButton", "Batch Capture All");
}

bool STransitionPreviewPanel::IsBatchCaptureButtonEnabled() const
{
	return !GifCapture->IsCapturing() && !bIsBatchCapturing && !bIsBatchCapturingEasing && Effects.Num() > 0;
}

// ─────────────────────────────────────────────
// Batch Easing GIF Capture
// ─────────────────────────────────────────────

FString STransitionPreviewPanel::GetGifFilenameForEasing(ETransitionEasing Easing)
{
	static const TMap<ETransitionEasing, FString> EasingToFile = {
		{ ETransitionEasing::Linear,          TEXT("easing_linear.gif") },
		{ ETransitionEasing::EaseInSine,      TEXT("easing_ease_in_sine.gif") },
		{ ETransitionEasing::EaseOutSine,     TEXT("easing_ease_out_sine.gif") },
		{ ETransitionEasing::EaseInOutSine,   TEXT("easing_ease_in_out_sine.gif") },
		{ ETransitionEasing::EaseInCubic,     TEXT("easing_ease_in_cubic.gif") },
		{ ETransitionEasing::EaseOutCubic,    TEXT("easing_ease_out_cubic.gif") },
		{ ETransitionEasing::EaseInOutCubic,  TEXT("easing_ease_in_out_cubic.gif") },
		{ ETransitionEasing::EaseInExpo,      TEXT("easing_ease_in_expo.gif") },
		{ ETransitionEasing::EaseOutExpo,     TEXT("easing_ease_out_expo.gif") },
		{ ETransitionEasing::EaseInOutExpo,   TEXT("easing_ease_in_out_expo.gif") },
		{ ETransitionEasing::EaseOutElastic,  TEXT("easing_ease_out_elastic.gif") },
		{ ETransitionEasing::EaseOutBounce,   TEXT("easing_ease_out_bounce.gif") },
	};

	const FString* Found = EasingToFile.Find(Easing);
	if (Found)
	{
		return *Found;
	}

	// Fallback: convert enum display name to snake_case
	const UEnum* EnumPtr = StaticEnum<ETransitionEasing>();
	FString DisplayName = EnumPtr ? EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(Easing)).ToString() : TEXT("unknown");
	FString Result;
	Result.Reserve(7 + DisplayName.Len() * 2 + 4); // "easing_" + worst-case snake_case + ".gif"
	Result += TEXT("easing_");
	for (int32 i = 0; i < DisplayName.Len(); ++i)
	{
		TCHAR Ch = DisplayName[i];
		if (FChar::IsUpper(Ch) && i > 0)
		{
			Result += TEXT("_");
		}
		Result += FChar::ToLower(Ch);
	}
	Result += TEXT(".gif");
	return Result;
}

void STransitionPreviewPanel::StartBatchCaptureEasing()
{
	if (GifCapture->IsCapturing() || bIsBatchCapturing || bIsBatchCapturingEasing || Effects.Num() == 0)
	{
		return;
	}

	// Ask user for output directory
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	FString DefaultDir = FPaths::ProjectDir() / TEXT("docs") / TEXT("images");
	FString ChosenDir;
	bool bChosen = DesktopPlatform->OpenDirectoryDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Select Easing GIF Output Folder"),
		DefaultDir,
		ChosenDir);

	if (!bChosen)
	{
		return;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*ChosenDir);

	// Build list of easing types (skip Custom)
	BatchEasingList.Reset();
	const UEnum* EnumPtr = StaticEnum<ETransitionEasing>();
	if (EnumPtr)
	{
		for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
		{
			ETransitionEasing Value = static_cast<ETransitionEasing>(EnumPtr->GetValueByIndex(i));
			if (Value == ETransitionEasing::Custom)
			{
				continue;
			}
			BatchEasingList.Add(Value);
		}
	}

	if (BatchEasingList.Num() == 0)
	{
		return;
	}

	// Find the Iris effect
	int32 IrisIndex = INDEX_NONE;
	for (int32 i = 0; i < Effects.Num(); ++i)
	{
		if (Effects[i].DisplayName == TEXT("Iris"))
		{
			IrisIndex = i;
			break;
		}
	}

	if (IrisIndex == INDEX_NONE)
	{
		FNotificationInfo Info(LOCTEXT("EasingBatchNoIris", "Easing batch capture failed: Iris effect not found."));
		Info.ExpireDuration = 4.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// Save current user selection
	SavedEffectIndex = SelectedIndex;
	SavedEasing = SelectedEasing;

	// Select Iris effect
	SelectedIndex = IrisIndex;
	OnEffectSelected(EffectNames[IrisIndex], ESelectInfo::Direct);

	// Start easing batch
	BatchOutputDir = ChosenDir;
	BatchEasingIndex = 0;
	bIsBatchCapturingEasing = true;
	SelectedEasing = BatchEasingList[0];

	FNotificationInfo Info(FText::Format(
		LOCTEXT("EasingBatchStartNotification", "Easing batch capture started: {0} easing curves"),
		FText::AsNumber(BatchEasingList.Num())));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	StartGifCapture();
}

void STransitionPreviewPanel::AdvanceBatchCaptureEasing()
{
	BatchEasingIndex++;

	if (BatchEasingIndex >= BatchEasingList.Num())
	{
		// Batch complete — restore user's previous selection
		bIsBatchCapturingEasing = false;
		SelectedEasing = SavedEasing;
		SelectedIndex = SavedEffectIndex;
		if (EffectNames.IsValidIndex(SavedEffectIndex))
		{
			OnEffectSelected(EffectNames[SavedEffectIndex], ESelectInfo::Direct);
		}

		FNotificationInfo Info(FText::Format(
			LOCTEXT("EasingBatchCompleteNotification", "Easing batch complete! {0} GIFs saved to {1}"),
			FText::AsNumber(BatchEasingList.Num()),
			FText::FromString(BatchOutputDir)));
		Info.ExpireDuration = 8.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// Set next easing and start capture
	SelectedEasing = BatchEasingList[BatchEasingIndex];
	StartGifCapture();
}

FText STransitionPreviewPanel::GetBatchCaptureEasingButtonText() const
{
	if (bIsBatchCapturingEasing)
	{
		return FText::Format(
			LOCTEXT("EasingBatchCapturing", "Easing... ({0}/{1})"),
			FText::AsNumber(BatchEasingIndex + 1),
			FText::AsNumber(BatchEasingList.Num()));
	}
	return LOCTEXT("BatchCaptureEasingButton", "Batch Capture Easing");
}

bool STransitionPreviewPanel::IsBatchCaptureEasingButtonEnabled() const
{
	return !GifCapture->IsCapturing() && !bIsBatchCapturing && !bIsBatchCapturingEasing && Effects.Num() > 0;
}

#endif // TRANSITIONFX_DEV_TOOLS

#undef LOCTEXT_NAMESPACE
