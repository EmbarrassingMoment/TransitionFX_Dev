// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "TransitionFXTypes.h"

#if TRANSITIONFX_DEV_TOOLS

/**
 * Dev-tools driver for the preview panel's batch GIF captures
 * (documentation asset generation):
 *
 *  - Effect batch: captures every discovered effect with filenames from
 *    MISSING_IMAGES.md into a user-chosen folder.
 *  - Easing batch: captures the Iris effect once per easing curve, then
 *    restores the user's previous effect/easing selection.
 *
 * The driver never touches Slate or the capture engine directly: it steers
 * the panel through the FPanelHooks callbacks and is advanced by the panel
 * forwarding the capture engine's OnWriteFinished (also on write failure,
 * so one bad file does not stall the batch).
 */
class FTransitionPreviewBatchCaptureDriver
{
public:
	/** Callbacks into STransitionPreviewPanel, bound once at construction. All required. */
	struct FPanelHooks
	{
		/** True while the capture engine is grabbing frames. */
		TFunction<bool()> IsCaptureActive;

		/** Number of discovered effects. */
		TFunction<int32()> GetEffectCount;

		/** Display name of the effect at the given index. */
		TFunction<FString(int32)> GetEffectDisplayName;

		/** Currently selected effect index. */
		TFunction<int32()> GetSelectedEffectIndex;

		/** Selects the effect at the given index (updates combo box and viewport material). */
		TFunction<void(int32)> SelectEffect;

		/** Currently selected easing. */
		TFunction<ETransitionEasing()> GetSelectedEasing;

		/** Selects the given easing curve. */
		TFunction<void(ETransitionEasing)> SelectEasing;

		/** Starts a capture that saves straight to SavePath (no dialog, no per-file success toast). */
		TFunction<void(const FString& /*SavePath*/)> StartCaptureToFile;
	};

	explicit FTransitionPreviewBatchCaptureDriver(FPanelHooks InHooks);

	/** Prompts for an output folder and captures every effect in sequence. */
	void StartEffectBatch();

	/** Prompts for an output folder and captures the Iris effect once per easing curve. */
	void StartEasingBatch();

	/** Called by the panel after each capture's write attempt; advances the active batch. */
	void HandleWriteFinished();

	bool IsEffectBatchActive() const { return bIsBatchCapturing; }
	bool IsEasingBatchActive() const { return bIsBatchCapturingEasing; }
	bool IsAnyBatchActive() const { return bIsBatchCapturing || bIsBatchCapturingEasing; }

	/** Index of the effect currently being captured (for progress UI). */
	int32 GetEffectBatchIndex() const { return BatchCaptureIndex; }

	/** Index of the easing currently being captured (for progress UI). */
	int32 GetEasingBatchIndex() const { return BatchEasingIndex; }

	/** Number of easing curves in the current batch (for progress UI). */
	int32 GetEasingBatchCount() const { return BatchEasingList.Num(); }

	FText GetEffectBatchButtonText() const;
	bool IsEffectBatchButtonEnabled() const;
	FText GetEasingBatchButtonText() const;
	bool IsEasingBatchButtonEnabled() const;

private:
	/** Maps a dropdown DisplayName to its MISSING_IMAGES.md filename (snake_case fallback). */
	static FString GetGifFilenameForEffect(const FString& DisplayName);

	/** Maps an easing curve to its MISSING_IMAGES.md filename (snake_case fallback). */
	static FString GetGifFilenameForEasing(ETransitionEasing Easing);

	/** Shows a directory picker defaulting to docs/images/ and creates the chosen folder. Returns false if cancelled. */
	bool PromptForOutputDir(const FString& DialogTitle, FString& OutDir) const;

	/** Selects the effect at BatchCaptureIndex and starts its capture. */
	void CaptureCurrentBatchEffect();

	void AdvanceEffectBatch();
	void AdvanceEasingBatch();

	FPanelHooks Hooks;

	// Effect batch state
	bool bIsBatchCapturing = false;
	int32 BatchCaptureIndex = 0;
	FString BatchOutputDir;

	// Easing batch state
	bool bIsBatchCapturingEasing = false;
	int32 BatchEasingIndex = 0;
	TArray<ETransitionEasing> BatchEasingList;
	int32 SavedEffectIndex = 0;
	ETransitionEasing SavedEasing = ETransitionEasing::Linear;
};

#endif // TRANSITIONFX_DEV_TOOLS
