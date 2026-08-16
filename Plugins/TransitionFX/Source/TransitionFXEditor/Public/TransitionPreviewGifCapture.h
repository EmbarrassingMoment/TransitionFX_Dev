// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"

class STransitionPreviewViewport;

/**
 * Drives a single GIF capture of the preview viewport: fixed-step frame
 * grabbing, save-path resolution, GIF89a encoding, and user notifications.
 *
 * Owned by STransitionPreviewPanel, which forwards its ticker to Tick()
 * while IsCapturing() and polls the getters for its progress UI. Callers
 * that already know the destination (batch modes) pass an explicit
 * SavePath; interactive captures leave it unset to get a save dialog
 * after the last frame.
 */
class FTransitionPreviewGifCapture
{
public:
	/** Parameters for one capture run, fixed at Start(). */
	struct FStartParams
	{
		/** Viewport to step and read pixels from. Required. Held weakly after Start(). */
		TSharedPtr<STransitionPreviewViewport> Viewport;

		/** Capture frame rate in frames per second. */
		int32 FrameRate = 30;

		/** Transition duration in seconds (drives the frame count). */
		float Duration = 1.0f;

		/** Playback speed multiplier baked into the GIF frame delay. */
		float GifPlaySpeed = 0.5f;

		/** Maps raw [0,1] progress to eased progress before it reaches the viewport. Required. */
		TFunction<float(float)> EasingEvaluator;

		/** Explicit output path. Unset → a save dialog is shown after capture. */
		TOptional<FString> SavePath;

		/** Default file name (without extension) suggested by the save dialog. */
		FString DialogDefaultFileName = TEXT("Transition");

		/** Whether to show the "GIF saved" notification on success (batch modes suppress it). */
		bool bShowSuccessNotification = true;
	};

	/**
	 * Fired after the encode/write attempt, with the write result. NOT fired
	 * when the capture aborts earlier (pixel read failure, dimension mismatch,
	 * or the user cancelling the save dialog) — this mirrors the behaviour the
	 * panel's batch drivers relied on before the extraction.
	 */
	DECLARE_DELEGATE_OneParam(FOnWriteFinished, bool /*bSucceeded*/);
	FOnWriteFinished OnWriteFinished;

	/** Begins a capture. Returns false (and does nothing) if already capturing or params are invalid. */
	bool Start(const FStartParams& InParams);

	/** Advances the capture by one ticker frame. No-op unless capturing. */
	void Tick();

	/** True while frames are being captured. */
	bool IsCapturing() const { return bIsCapturing; }

	/** Index of the next frame to capture (for progress UI). */
	int32 GetFrameIndex() const { return CaptureFrameIndex; }

	/** Total frames for the current run (for progress UI). */
	int32 GetTotalFrames() const { return TotalCaptureFrames; }

	/** Raw [0,1] progress most recently pushed to the viewport. */
	float GetCurrentRawProgress() const { return CurrentRawProgress; }

private:
	/** Grabs one frame (after stabilize/wait ticks) and schedules the next, or finalizes. */
	void CaptureFrameTick();

	/** Resolves the save path, encodes, writes, notifies, then fires OnWriteFinished. */
	void Finalize();

	/** Stops the capture, discards frames, and shows a failure notification. Does not fire OnWriteFinished. */
	void Abort(const FText& NotificationText);

	/** Parameters of the current run (Viewport cleared after Start — see ViewportWeak). */
	FStartParams Params;

	/** The viewport is held weakly so an idle capture object never extends its lifetime. */
	TWeakPtr<STransitionPreviewViewport> ViewportWeak;

	bool bIsCapturing = false;

	/** True when the viewport still needs a frame to render the progress set last tick. */
	bool bCaptureWaitFrame = false;

	/** Extra ticks to wait after Start() before the first grab, letting the viewport stabilize. */
	int32 CaptureStabilizeFrames = 0;

	int32 CaptureFrameIndex = 0;
	int32 TotalCaptureFrames = 0;
	float CurrentRawProgress = 0.0f;
	TArray<TArray<FColor>> CapturedFrames;
};
