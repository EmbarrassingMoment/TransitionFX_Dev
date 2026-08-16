// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionPreviewGifCapture.h"
#include "TransitionPreviewViewport.h"
#include "GifEncoder.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Framework/Application/SlateApplication.h"
#include "DesktopPlatformModule.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "TransitionFXEditor"

bool FTransitionPreviewGifCapture::Start(const FStartParams& InParams)
{
	if (bIsCapturing || !InParams.Viewport.IsValid() || !InParams.EasingEvaluator)
	{
		return false;
	}

	Params = InParams;
	ViewportWeak = Params.Viewport;
	Params.Viewport.Reset();

	// Calculate total frames: forward only (0 → 1) at FrameRate fps
	TotalCaptureFrames = FMath::RoundToInt32(Params.FrameRate * Params.Duration);
	if (TotalCaptureFrames < 2)
	{
		TotalCaptureFrames = 2;
	}

	CapturedFrames.Reset();
	CapturedFrames.Reserve(TotalCaptureFrames);
	CaptureFrameIndex = 0;
	CurrentRawProgress = 0.0f;
	CaptureStabilizeFrames = 2; // Wait extra ticks for viewport to stabilize before first capture
	bCaptureWaitFrame = true; // Wait one frame for the viewport to render progress=0
	bIsCapturing = true;

	if (TSharedPtr<STransitionPreviewViewport> Viewport = ViewportWeak.Pin())
	{
		Viewport->SetProgress(Params.EasingEvaluator(0.0f));
	}

	return true;
}

void FTransitionPreviewGifCapture::Tick()
{
	if (bIsCapturing)
	{
		CaptureFrameTick();
	}
}

void FTransitionPreviewGifCapture::CaptureFrameTick()
{
	TSharedPtr<STransitionPreviewViewport> Viewport = ViewportWeak.Pin();
	if (!Viewport.IsValid())
	{
		bIsCapturing = false;
		CapturedFrames.Reset();
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
	if (Viewport->CaptureFrame(Pixels))
	{
		CapturedFrames.Add(MoveTemp(Pixels));
	}
	else
	{
		Abort(LOCTEXT("CaptureFailedNotification", "GIF capture failed: could not read viewport pixels."));
		return;
	}

	CaptureFrameIndex++;

	if (CaptureFrameIndex >= TotalCaptureFrames)
	{
		// All frames captured — finalize
		Finalize();
		return;
	}

	// Set progress for the next frame
	float Progress = static_cast<float>(CaptureFrameIndex) / static_cast<float>(TotalCaptureFrames - 1);
	CurrentRawProgress = Progress;
	Viewport->SetProgress(Params.EasingEvaluator(Progress));
	bCaptureWaitFrame = true; // Wait for render
}

void FTransitionPreviewGifCapture::Finalize()
{
	bIsCapturing = false;

	if (CapturedFrames.Num() == 0)
	{
		return;
	}

	TSharedPtr<STransitionPreviewViewport> Viewport = ViewportWeak.Pin();
	if (!Viewport.IsValid())
	{
		CapturedFrames.Reset();
		return;
	}

	// Get actual render target dimensions (accounts for DPI scaling)
	FIntPoint ActualSize = Viewport->GetViewportSize();
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

	// Determine save path
	FString SavePath;

	if (Params.SavePath.IsSet())
	{
		// Explicit destination (batch modes): save without prompting.
		SavePath = Params.SavePath.GetValue();
	}
	else
	{
		// Interactive mode: show save file dialog
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (!DesktopPlatform)
		{
			CapturedFrames.Reset();
			return;
		}

		TArray<FString> OutFiles;
		bool bSaved = DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Save GIF"),
			FPaths::ProjectSavedDir(),
			Params.DialogDefaultFileName + TEXT(".gif"),
			TEXT("GIF Files (*.gif)|*.gif"),
			0,
			OutFiles);

		if (!bSaved || OutFiles.Num() == 0)
		{
			CapturedFrames.Reset();
			return;
		}

		SavePath = OutFiles[0];
		if (!SavePath.EndsWith(TEXT(".gif"), ESearchCase::IgnoreCase))
		{
			SavePath += TEXT(".gif");
		}
	}

	// Encode GIF (delay in centiseconds: 100 / fps)
	int32 FrameDelayCentiseconds = FMath::Max(1, FMath::RoundToInt32(100.0f / Params.FrameRate / Params.GifPlaySpeed));
	FGifEncoder Encoder(CaptureWidth, CaptureHeight, FrameDelayCentiseconds);

	for (const TArray<FColor>& Frame : CapturedFrames)
	{
		Encoder.AddFrame(Frame);
	}

	CapturedFrames.Reset();

	const bool bWriteSucceeded = Encoder.WriteToFile(SavePath);

	if (bWriteSucceeded)
	{
		if (Params.bShowSuccessNotification)
		{
			FNotificationInfo Info(FText::Format(
				LOCTEXT("CaptureSuccessNotification", "GIF saved: {0}"),
				FText::FromString(FPaths::GetCleanFilename(SavePath))));
			Info.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}
	else
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("CaptureWriteFailedNotification2", "GIF capture failed: could not write {0}"),
			FText::FromString(FPaths::GetCleanFilename(SavePath))));
		Info.ExpireDuration = 4.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	// Last statement on purpose: the handler may immediately Start() the next
	// capture (batch advance), which re-initializes this object's state.
	OnWriteFinished.ExecuteIfBound(bWriteSucceeded);
}

void FTransitionPreviewGifCapture::Abort(const FText& NotificationText)
{
	bIsCapturing = false;
	CapturedFrames.Reset();

	FNotificationInfo Info(NotificationText);
	Info.ExpireDuration = 4.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

#undef LOCTEXT_NAMESPACE
