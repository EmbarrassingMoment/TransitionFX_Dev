// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Styling/SlateBrush.h"
#include "ITransitionEffect.h"
#include "WidgetTransitionEffect.generated.h"

class UTransitionPreset;
class UMaterialInstanceDynamic;
class UGameViewportClient;
class SImage;

/**
 * Transition effect that renders a UI-domain material through a full-screen Slate widget
 * placed on top of the game viewport. Unlike UPostProcessTransitionEffect, this covers
 * UMG/Slate widgets added to the viewport with a lower Z-order.
 *
 * Requires a UI-domain material (see M_Widget_* masters) that exposes the same
 * Progress / Invert / FadeColor parameter triad as the post-process materials.
 */
UCLASS(Blueprintable, BlueprintType)
class TRANSITIONFX_API UWidgetTransitionEffect : public UObject, public ITransitionEffect
{
	GENERATED_BODY()

public:
	// ITransitionEffect Interface

	/** Creates or reuses the dynamic material and registers the overlay widget on the game viewport. */
	virtual void Initialize(UWorld* World, UTransitionPreset* Preset) override;

	/** Updates the material's Progress parameter. */
	virtual void UpdateProgress(float Progress) override;

	/** Removes the overlay widget from the viewport and clears the dynamic material reference. */
	virtual void Cleanup() override;

	/** Sets the material's Invert parameter (1.0 for inverted, 0.0 for normal). */
	virtual void SetInvert(bool bInvert) override;

	/** Applies scalar, vector, and texture parameter overrides to the dynamic material. */
	virtual void SetParameters(const FTransitionParameters& Params) override;

	// UObject Interface

	/** Ensures the overlay widget is removed if the effect is garbage collected while active. */
	virtual void BeginDestroy() override;

protected:
	/** Removes the overlay widget from the viewport it was registered on, if any. */
	void RemoveOverlayFromViewport();

	/** The dynamic material instance created at runtime. Outered to this effect so it survives level travel. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "TransitionFX")
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	/** Brush that carries the dynamic material to Slate. Declared as a UPROPERTY so the material is kept alive. */
	UPROPERTY(Transient)
	FSlateBrush Brush;

	/** Full-screen image widget added to the game viewport overlay. Kept across pool cycles for reuse. */
	TSharedPtr<SImage> OverlayWidget;

	/** The viewport the overlay widget is currently registered on. */
	TWeakObjectPtr<UGameViewportClient> RegisteredViewport;
};
