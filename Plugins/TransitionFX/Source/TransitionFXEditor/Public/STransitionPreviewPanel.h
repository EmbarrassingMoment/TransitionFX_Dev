// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "TransitionFXTypes.h"

class STransitionPreviewViewport;
class FTransitionPreviewGifCapture;

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

	// Defined in the .cpp (not defaulted inline) so the TUniquePtr members'
	// deleters instantiate where FTransitionPreviewGifCapture is complete.
	STransitionPreviewPanel();
	virtual ~STransitionPreviewPanel() override;

private:
	// Effect discovery
	void DiscoverEffects();
	void OnEffectSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

	// Easing
	void PopulateEasingOptions();
	void OnEasingSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	float GetEasedProgress(float RawProgress) const;

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

	// GIF capture (frame grabbing / encoding / saving lives in FTransitionPreviewGifCapture)
	void StartGifCapture();
	FText GetCaptureButtonText() const;
	bool IsCaptureButtonEnabled() const;

#if TRANSITIONFX_DEV_TOOLS
	/** Bound to the capture engine's OnWriteFinished; advances whichever batch is running. */
	void OnGifWriteFinished(bool bSucceeded);

	// Batch GIF capture (captures all effects with filenames from MISSING_IMAGES.md)
	static FString GetGifFilenameForEffect(const FString& DisplayName);
	void StartBatchCapture();
	void AdvanceBatchCapture();
	FText GetBatchCaptureButtonText() const;
	bool IsBatchCaptureButtonEnabled() const;

	// Batch easing GIF capture (captures Iris effect with all easing types)
	static FString GetGifFilenameForEasing(ETransitionEasing Easing);
	void StartBatchCaptureEasing();
	void AdvanceBatchCaptureEasing();
	FText GetBatchCaptureEasingButtonText() const;
	bool IsBatchCaptureEasingButtonEnabled() const;
#endif

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

	// Easing
	TArray<TSharedPtr<FString>> EasingNames;
	ETransitionEasing SelectedEasing;

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

	// GIF capture engine (owns all per-run capture state)
	TUniquePtr<FTransitionPreviewGifCapture> GifCapture;
	int32 CaptureFrameRate;
	float GifPlaySpeed;

#if TRANSITIONFX_DEV_TOOLS
	// Batch capture state
	bool bIsBatchCapturing;
	int32 BatchCaptureIndex;
	FString BatchOutputDir;

	// Easing batch capture state
	bool bIsBatchCapturingEasing;
	int32 BatchEasingIndex;
	TArray<ETransitionEasing> BatchEasingList;
	int32 SavedEffectIndex;
	ETransitionEasing SavedEasing;
#endif

	// Tick delegate
	FTSTicker::FDelegateHandle TickDelegateHandle;
};
