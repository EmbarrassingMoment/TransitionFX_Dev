// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "STransitionPreviewPanel.h"
#include "TransitionPreviewViewport.h"
#include "GifEncoder.h"
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
	bIsCapturing = false;
	bCaptureWaitFrame = false;
	CaptureFrameIndex = 0;
	TotalCaptureFrames = 0;
	CaptureFrameRate = 30;
	CaptureStabilizeFrames = 0;
	ViewportWidth = 480.0f;
	ViewportHeight = 270.0f;

	// Resolution options
	ResolutionOptions.Add(MakeShared<FString>(TEXT("480 x 270")));
	ResolutionOptions.Add(MakeShared<FString>(TEXT("640 x 360")));
	ResolutionOptions.Add(MakeShared<FString>(TEXT("800 x 450")));

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
			.Padding(8, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					if (bIsCapturing)
					{
						return FText::Format(
							LOCTEXT("CaptureProgress", "Capturing... {0}/{1}"),
							FText::AsNumber(CaptureFrameIndex),
							FText::AsNumber(TotalCaptureFrames));
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
	if (bIsCapturing)
	{
		OnCaptureFrameTick();
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
		PreviewViewport->SetProgress(CurrentProgress);
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
		PreviewViewport->SetProgress(CurrentProgress);
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
	if (bIsCapturing || !PreviewViewport.IsValid() || Effects.Num() == 0)
	{
		return;
	}

	// Calculate total frames: forward only (0 → 1) at CaptureFrameRate fps
	TotalCaptureFrames = FMath::RoundToInt32(CaptureFrameRate * Duration);
	if (TotalCaptureFrames < 2)
	{
		TotalCaptureFrames = 2;
	}

	// Stop normal playback and reset
	bIsPlaying = false;
	bIsReversing = false;
	CurrentProgress = 0.0f;
	PreviewViewport->SetProgress(0.0f);

	// Initialize capture state
	CapturedFrames.Reset();
	CapturedFrames.Reserve(TotalCaptureFrames);
	CaptureFrameIndex = 0;
	CaptureStabilizeFrames = 2; // Wait extra ticks for viewport to stabilize before first capture
	bCaptureWaitFrame = true; // Wait one frame for the viewport to render progress=0
	bIsCapturing = true;
}

void STransitionPreviewPanel::OnCaptureFrameTick()
{
	if (!PreviewViewport.IsValid())
	{
		bIsCapturing = false;
		return;
	}

	// Wait for viewport to stabilize after capture start
	if (CaptureStabilizeFrames > 0)
	{
		CaptureStabilizeFrames--;
		return;
	}

	// Wait one frame after setting progress so the viewport can render
	if (bCaptureWaitFrame)
	{
		bCaptureWaitFrame = false;
		return;
	}

	// Capture the current frame
	TArray<FColor> Pixels;
	if (PreviewViewport->CaptureFrame(Pixels))
	{
		CapturedFrames.Add(MoveTemp(Pixels));
	}
	else
	{
		// Capture failed — abort
		bIsCapturing = false;
		CapturedFrames.Reset();

		FNotificationInfo Info(LOCTEXT("CaptureFailedNotification", "GIF capture failed: could not read viewport pixels."));
		Info.ExpireDuration = 4.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	CaptureFrameIndex++;

	if (CaptureFrameIndex >= TotalCaptureFrames)
	{
		// All frames captured — finalize
		FinalizeGifCapture();
		return;
	}

	// Set progress for the next frame
	float Progress = static_cast<float>(CaptureFrameIndex) / static_cast<float>(TotalCaptureFrames - 1);
	CurrentProgress = Progress;
	PreviewViewport->SetProgress(Progress);
	bCaptureWaitFrame = true; // Wait for render
}

void STransitionPreviewPanel::FinalizeGifCapture()
{
	bIsCapturing = false;

	if (CapturedFrames.Num() == 0)
	{
		return;
	}

	// Get actual render target dimensions (accounts for DPI scaling)
	FIntPoint ActualSize = PreviewViewport->GetViewportSize();
	int32 CaptureWidth = ActualSize.X;
	int32 CaptureHeight = ActualSize.Y;
	int32 FramePixelCount = CapturedFrames[0].Num();

	if (CaptureWidth <= 0 || CaptureHeight <= 0 || CaptureWidth * CaptureHeight != FramePixelCount)
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("CaptureSizeMismatch", "GIF capture failed: dimension mismatch (viewport {0}x{1}, pixels {2})."),
			FText::AsNumber(CaptureWidth),
			FText::AsNumber(CaptureHeight),
			FText::AsNumber(FramePixelCount)));
		Info.ExpireDuration = 4.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		CapturedFrames.Reset();
		return;
	}

	// Show save file dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		CapturedFrames.Reset();
		return;
	}

	// Default filename: EffectName.gif
	FString DefaultName = TEXT("Transition");
	if (EffectNames.IsValidIndex(SelectedIndex))
	{
		DefaultName = *EffectNames[SelectedIndex];
	}

	TArray<FString> OutFiles;
	bool bSaved = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Save GIF"),
		FPaths::ProjectSavedDir(),
		DefaultName + TEXT(".gif"),
		TEXT("GIF Files (*.gif)|*.gif"),
		0,
		OutFiles);

	if (!bSaved || OutFiles.Num() == 0)
	{
		CapturedFrames.Reset();
		return;
	}

	FString SavePath = OutFiles[0];
	if (!SavePath.EndsWith(TEXT(".gif"), ESearchCase::IgnoreCase))
	{
		SavePath += TEXT(".gif");
	}

	// Encode GIF (delay in centiseconds: 100 / fps)
	int32 FrameDelayCentiseconds = FMath::Max(1, FMath::RoundToInt32(100.0f / CaptureFrameRate));
	FGifEncoder Encoder(CaptureWidth, CaptureHeight, FrameDelayCentiseconds);

	for (const TArray<FColor>& Frame : CapturedFrames)
	{
		Encoder.AddFrame(Frame);
	}

	CapturedFrames.Reset();

	if (Encoder.WriteToFile(SavePath))
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("CaptureSuccessNotification", "GIF saved: {0}"),
			FText::FromString(FPaths::GetCleanFilename(SavePath))));
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	else
	{
		FNotificationInfo Info(LOCTEXT("CaptureWriteFailedNotification", "GIF capture failed: could not write file."));
		Info.ExpireDuration = 4.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

FText STransitionPreviewPanel::GetCaptureButtonText() const
{
	return bIsCapturing
		? LOCTEXT("CaptureButtonCapturing", "Capturing...")
		: LOCTEXT("CaptureButtonIdle", "Capture GIF");
}

bool STransitionPreviewPanel::IsCaptureButtonEnabled() const
{
	return !bIsCapturing && Effects.Num() > 0;
}

#undef LOCTEXT_NAMESPACE
