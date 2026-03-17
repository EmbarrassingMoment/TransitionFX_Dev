// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"

class FPreviewScene;
class APostProcessVolume;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * Viewport client for previewing transition effects.
 * Renders a white background with a post-process material overlay.
 */
class FTransitionPreviewViewportClient : public FEditorViewportClient
{
public:
	FTransitionPreviewViewportClient(FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget);
	virtual ~FTransitionPreviewViewportClient() override;

	void SetPreviewMaterial(UMaterialInterface* Material);
	void SetProgress(float Progress);
	void SetInvert(bool bInvert);

	virtual FLinearColor GetBackgroundColor() const override { return FLinearColor::White; }

private:
	void SetupPostProcessVolume();
	void UpdateBlendables();

	FPreviewScene* PreviewScene;
	APostProcessVolume* PreviewVolume;
	UMaterialInstanceDynamic* DynamicMaterial;
};

/**
 * SEditorViewport subclass that manages the viewport client lifecycle.
 * This is the standard UE5 pattern that properly initializes
 * FEditorModeTools and the EditorInteractiveToolsFramework.
 */
class STransitionPreviewViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(STransitionPreviewViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STransitionPreviewViewport() override;

	void SetPreviewMaterial(UMaterialInterface* Material);
	void SetProgress(float Progress);
	void SetInvert(bool bInvert);

	/** Read the current viewport pixels (BGRA). Returns true on success. */
	bool CaptureFrame(TArray<FColor>& OutPixels);

	/** Get the actual render target dimensions (accounts for DPI scaling). */
	FIntPoint GetViewportSize() const;

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	TSharedPtr<FPreviewScene> PreviewScene;
	TSharedPtr<FTransitionPreviewViewportClient> ViewportClient;
};
