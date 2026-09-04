// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "WidgetTransitionEffect.h"
#include "TransitionPreset.h"
#include "TransitionFXConfig.h"
#include "TransitionFXMaterialUtils.h"
#include "TransitionFX.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/Images/SImage.h"

/**
 * Creates or reuses the dynamic material instance, then (re)registers a full-screen SImage
 * on the game viewport overlay. Re-running Initialize is safe: the widget is removed and
 * re-added so it survives level travel (which clears all viewport widgets) and Z-order changes.
 */
void UWidgetTransitionEffect::Initialize(UWorld* World, UTransitionPreset* Preset)
{
	if (!World || !Preset)
	{
		return;
	}

	if (!Preset->TransitionMaterial)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("WidgetTransitionEffect: TransitionMaterial is null in Preset %s"), *Preset->GetName());
		return;
	}

	// Create or reuse the dynamic material. The effect object is the Outer (not the World)
	// because Brush keeps a reference to it; a World-outered MID would pin the old world
	// in memory across level travel.
	if (DynamicMaterial && DynamicMaterial->Parent == Preset->TransitionMaterial)
	{
		DynamicMaterial->ClearParameterValues();
	}
	else
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(Preset->TransitionMaterial, this);
	}

	if (!DynamicMaterial)
	{
		UE_LOG(LogTransitionFX, Error, TEXT("WidgetTransitionEffect: Failed to create Dynamic Material Instance"));
		return;
	}

	// Check for "Progress" parameter
	float TempVal = 0.0f;
	static const FMaterialParameterInfo ProgressInfo(TransitionFXConfig::ProgressParamName);
	if (!DynamicMaterial->GetScalarParameterValue(ProgressInfo, TempVal))
	{
		UE_LOG(LogTransitionFX, Error, TEXT("TransitionFX: Material '%s' is missing 'Progress' parameter. Transition will not animate."), *Preset->TransitionMaterial->GetName());
		DynamicMaterial = nullptr;
		return;
	}

	UGameViewportClient* Viewport = World->GetGameViewport();
	if (!Viewport)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("WidgetTransitionEffect: No game viewport available for world '%s'. Transition will not be visible."), *World->GetName());
		return;
	}

	// Build the overlay widget on first use. HitTestInvisible so it never swallows input;
	// input blocking is handled by the subsystem via CinematicMode.
	if (!OverlayWidget.IsValid())
	{
		OverlayWidget = SNew(SImage)
			.Visibility(EVisibility::HitTestInvisible);
	}

	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.SetResourceObject(DynamicMaterial);
	OverlayWidget->SetImage(&Brush);

	// Always remove before adding: makes Initialize idempotent and applies Z-order changes.
	RemoveOverlayFromViewport();
	Viewport->AddViewportWidgetContent(OverlayWidget.ToSharedRef(), Preset->WidgetZOrder);
	RegisteredViewport = Viewport;
}

/** Sets the Progress scalar parameter on the dynamic material. */
void UWidgetTransitionEffect::UpdateProgress(float Progress)
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(TransitionFXConfig::ProgressParamName, Progress);
	}
}

/** Removes the overlay from the viewport and releases the material. The widget is kept for reuse. */
void UWidgetTransitionEffect::Cleanup()
{
	RemoveOverlayFromViewport();

	Brush.SetResourceObject(nullptr);
	DynamicMaterial = nullptr;
}

/** Sets the Invert material parameter. The material uses |Invert - Mask| to flip the alpha. */
void UWidgetTransitionEffect::SetInvert(bool bInvert)
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(TransitionFXConfig::InvertParamName, bInvert ? 1.0f : 0.0f);
	}
}

/** Applies runtime parameter overrides (scalar, vector, texture) to the dynamic material instance. */
void UWidgetTransitionEffect::SetParameters(const FTransitionParameters& Params)
{
	TransitionFXMaterialUtils::ApplyParameters(DynamicMaterial, Params);
}

/** Guards against the effect being garbage collected while still on screen (e.g. pooling disabled). */
void UWidgetTransitionEffect::BeginDestroy()
{
	RemoveOverlayFromViewport();
	OverlayWidget.Reset();

	Super::BeginDestroy();
}

/** Removes the overlay widget from whichever viewport it was last registered on. */
void UWidgetTransitionEffect::RemoveOverlayFromViewport()
{
	if (UGameViewportClient* Viewport = RegisteredViewport.Get())
	{
		if (OverlayWidget.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(OverlayWidget.ToSharedRef());
		}
	}
	RegisteredViewport.Reset();
}
