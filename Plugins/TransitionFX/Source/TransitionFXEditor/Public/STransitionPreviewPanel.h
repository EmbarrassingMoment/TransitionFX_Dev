// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class STransitionPreviewViewport;

/** Entry in the effect dropdown list. */
struct FEffectEntry
{
	FString DisplayName;
	FSoftObjectPath MaterialPath;
};

/**
 * Main Slate panel for previewing transition effects.
 * Provides playback controls and a viewport for GIF capture.
 */
class STransitionPreviewPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STransitionPreviewPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STransitionPreviewPanel() override;

private:
	// Effect discovery
	void DiscoverEffects();
	void OnEffectSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

	// Playback
	bool OnTick(float DeltaTime);
	void PlayForward();
	void PlayReverse();
	void Stop();
	void Reset();
	void ToggleLoop();

	// Progress slider
	void OnProgressChanged(float NewValue);
	void OnProgressCaptureBegin();
	void OnProgressCaptureEnd();

	// Invert
	void OnInvertChanged(ECheckBoxState NewState);

	// Speed
	void OnSpeedChanged(float NewValue);
	void OnSpeedCommitted(float NewValue, ETextCommit::Type CommitType);

	// Resolution
	void OnResolutionSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

	// GIF capture
	void StartGifCapture();
	void OnCaptureFrameTick();
	void FinalizeGifCapture();
	FText GetCaptureButtonText() const;
	bool IsCaptureButtonEnabled() const;

	// Batch GIF capture (captures all effects with filenames from MISSING_IMAGES.md)
	static FString GetGifFilenameForEffect(const FString& DisplayName);
	void StartBatchCapture();
	void AdvanceBatchCapture();
	FText GetBatchCaptureButtonText() const;
	bool IsBatchCaptureButtonEnabled() const;

	// UI helpers
	FText GetProgressText() const;
	FText GetSpeedText() const;
	FText GetLoopButtonText() const;

	// Preview viewport (manages scene, client, and mode tools internally)
	TSharedPtr<STransitionPreviewViewport> PreviewViewport;

	// Effect list
	TArray<FEffectEntry> Effects;
	TArray<TSharedPtr<FString>> EffectNames;
	int32 SelectedIndex;

	// Playback state
	float CurrentProgress;
	float PlaySpeed;
	float Duration;
	bool bIsPlaying;
	bool bIsReversing;
	bool bLooping;
	bool bInvert;
	bool bSliderCaptured;

	// Resolution options
	TArray<TSharedPtr<FString>> ResolutionOptions;
	float ViewportWidth;
	float ViewportHeight;

	// GIF capture state
	bool bIsCapturing;
	bool bCaptureWaitFrame;
	int32 CaptureFrameIndex;
	int32 TotalCaptureFrames;
	int32 CaptureFrameRate;
	int32 CaptureStabilizeFrames;
	TArray<TArray<FColor>> CapturedFrames;
	float GifPlaySpeed;

	// Batch capture state
	bool bIsBatchCapturing;
	int32 BatchCaptureIndex;
	FString BatchOutputDir;

	// Tick delegate
	FTSTicker::FDelegateHandle TickDelegateHandle;
};
