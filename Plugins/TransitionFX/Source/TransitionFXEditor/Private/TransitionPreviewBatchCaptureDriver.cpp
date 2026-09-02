// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionPreviewBatchCaptureDriver.h"

#if TRANSITIONFX_DEV_TOOLS

#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Framework/Application/SlateApplication.h"
#include "DesktopPlatformModule.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "TransitionFXEditor"

FTransitionPreviewBatchCaptureDriver::FTransitionPreviewBatchCaptureDriver(FPanelHooks InHooks)
	: Hooks(MoveTemp(InHooks))
{
}

// ─────────────────────────────────────────────
// Filename mapping
// ─────────────────────────────────────────────

FString FTransitionPreviewBatchCaptureDriver::GetGifFilenameForEffect(const FString& DisplayName)
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

FString FTransitionPreviewBatchCaptureDriver::GetGifFilenameForEasing(ETransitionEasing Easing)
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

// ─────────────────────────────────────────────
// Shared helpers
// ─────────────────────────────────────────────

bool FTransitionPreviewBatchCaptureDriver::PromptForOutputDir(const FString& DialogTitle, FString& OutDir) const
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return false;
	}

	// Default to docs/images/ under project root
	FString DefaultDir = FPaths::ProjectDir() / TEXT("docs") / TEXT("images");
	FString ChosenDir;
	bool bChosen = DesktopPlatform->OpenDirectoryDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		DialogTitle,
		DefaultDir,
		ChosenDir);

	if (!bChosen)
	{
		return false;
	}

	// Ensure directory exists
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*ChosenDir);

	OutDir = ChosenDir;
	return true;
}

void FTransitionPreviewBatchCaptureDriver::HandleWriteFinished()
{
	// Advance the active batch (also on write failure, so one bad file doesn't stall the run)
	if (bIsBatchCapturingEasing)
	{
		AdvanceEasingBatch();
	}
	else if (bIsBatchCapturing)
	{
		AdvanceEffectBatch();
	}
}

// ─────────────────────────────────────────────
// Effect batch
// ─────────────────────────────────────────────

void FTransitionPreviewBatchCaptureDriver::StartEffectBatch()
{
	if (Hooks.IsCaptureActive() || IsAnyBatchActive() || Hooks.GetEffectCount() == 0)
	{
		return;
	}

	if (!PromptForOutputDir(TEXT("Select Batch GIF Output Folder"), BatchOutputDir))
	{
		return;
	}

	BatchCaptureIndex = 0;
	bIsBatchCapturing = true;

	FNotificationInfo Info(FText::Format(
		LOCTEXT("BatchCaptureStartNotification", "Batch capture started: {0} effects"),
		FText::AsNumber(Hooks.GetEffectCount())));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	CaptureCurrentBatchEffect();
}

void FTransitionPreviewBatchCaptureDriver::CaptureCurrentBatchEffect()
{
	Hooks.SelectEffect(BatchCaptureIndex);
	Hooks.StartCaptureToFile(BatchOutputDir / GetGifFilenameForEffect(Hooks.GetEffectDisplayName(BatchCaptureIndex)));
}

void FTransitionPreviewBatchCaptureDriver::AdvanceEffectBatch()
{
	BatchCaptureIndex++;

	if (BatchCaptureIndex >= Hooks.GetEffectCount())
	{
		// Batch complete
		bIsBatchCapturing = false;

		FNotificationInfo Info(FText::Format(
			LOCTEXT("BatchCaptureCompleteNotification", "Batch capture complete! {0} GIFs saved to {1}"),
			FText::AsNumber(Hooks.GetEffectCount()),
			FText::FromString(BatchOutputDir)));
		Info.ExpireDuration = 8.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	CaptureCurrentBatchEffect();
}

FText FTransitionPreviewBatchCaptureDriver::GetEffectBatchButtonText() const
{
	if (bIsBatchCapturing)
	{
		return FText::Format(
			LOCTEXT("BatchCapturing", "Batch... ({0}/{1})"),
			FText::AsNumber(BatchCaptureIndex + 1),
			FText::AsNumber(Hooks.GetEffectCount()));
	}
	return LOCTEXT("BatchCaptureButton", "Batch Capture All");
}

bool FTransitionPreviewBatchCaptureDriver::IsEffectBatchButtonEnabled() const
{
	return !Hooks.IsCaptureActive() && !IsAnyBatchActive() && Hooks.GetEffectCount() > 0;
}

// ─────────────────────────────────────────────
// Easing batch
// ─────────────────────────────────────────────

void FTransitionPreviewBatchCaptureDriver::StartEasingBatch()
{
	if (Hooks.IsCaptureActive() || IsAnyBatchActive() || Hooks.GetEffectCount() == 0)
	{
		return;
	}

	FString ChosenDir;
	if (!PromptForOutputDir(TEXT("Select Easing GIF Output Folder"), ChosenDir))
	{
		return;
	}

	// Build list of easing types (skip Custom — requires a UCurveFloat asset)
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
	for (int32 i = 0; i < Hooks.GetEffectCount(); ++i)
	{
		if (Hooks.GetEffectDisplayName(i) == TEXT("Iris"))
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
	SavedEffectIndex = Hooks.GetSelectedEffectIndex();
	SavedEasing = Hooks.GetSelectedEasing();

	// Select Iris effect
	Hooks.SelectEffect(IrisIndex);

	// Start easing batch
	BatchOutputDir = ChosenDir;
	BatchEasingIndex = 0;
	bIsBatchCapturingEasing = true;
	Hooks.SelectEasing(BatchEasingList[0]);

	FNotificationInfo Info(FText::Format(
		LOCTEXT("EasingBatchStartNotification", "Easing batch capture started: {0} easing curves"),
		FText::AsNumber(BatchEasingList.Num())));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	Hooks.StartCaptureToFile(BatchOutputDir / GetGifFilenameForEasing(BatchEasingList[0]));
}

void FTransitionPreviewBatchCaptureDriver::AdvanceEasingBatch()
{
	BatchEasingIndex++;

	if (BatchEasingIndex >= BatchEasingList.Num())
	{
		// Batch complete — restore user's previous selection
		bIsBatchCapturingEasing = false;
		Hooks.SelectEasing(SavedEasing);
		Hooks.SelectEffect(SavedEffectIndex);

		FNotificationInfo Info(FText::Format(
			LOCTEXT("EasingBatchCompleteNotification", "Easing batch complete! {0} GIFs saved to {1}"),
			FText::AsNumber(BatchEasingList.Num()),
			FText::FromString(BatchOutputDir)));
		Info.ExpireDuration = 8.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// Set next easing and start capture
	Hooks.SelectEasing(BatchEasingList[BatchEasingIndex]);
	Hooks.StartCaptureToFile(BatchOutputDir / GetGifFilenameForEasing(BatchEasingList[BatchEasingIndex]));
}

FText FTransitionPreviewBatchCaptureDriver::GetEasingBatchButtonText() const
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

bool FTransitionPreviewBatchCaptureDriver::IsEasingBatchButtonEnabled() const
{
	return !Hooks.IsCaptureActive() && !IsAnyBatchActive() && Hooks.GetEffectCount() > 0;
}

#undef LOCTEXT_NAMESPACE

#endif // TRANSITIONFX_DEV_TOOLS
