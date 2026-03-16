// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FPreviewScene;
class FTransitionPreviewViewportClient;
class SViewport;
class FSceneViewport;

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

	// UI helpers
	FText GetProgressText() const;
	FText GetSpeedText() const;
	FText GetLoopButtonText() const;

	// Preview scene and viewport
	TSharedPtr<FPreviewScene> PreviewScene;
	TSharedPtr<FTransitionPreviewViewportClient> ViewportClient;
	TSharedPtr<SViewport> ViewportWidget;
	TSharedPtr<FSceneViewport> SceneViewport;

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

	// Tick delegate
	FTSTicker::FDelegateHandle TickDelegateHandle;
};
