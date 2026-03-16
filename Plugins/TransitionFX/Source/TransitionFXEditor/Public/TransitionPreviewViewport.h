// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
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
	FTransitionPreviewViewportClient(FPreviewScene* InPreviewScene);
	virtual ~FTransitionPreviewViewportClient() override;

	/** Set the material to preview. Creates a new DynamicMaterialInstance. */
	void SetPreviewMaterial(UMaterialInterface* Material);

	/** Update the Progress parameter on the dynamic material. */
	void SetProgress(float Progress);

	/** Set the Invert parameter on the dynamic material. */
	void SetInvert(bool bInvert);

	/** Get the current DynamicMaterialInstance. */
	UMaterialInstanceDynamic* GetDynamicMaterial() const { return DynamicMaterial; }

private:
	void SetupPostProcessVolume();
	void UpdateBlendables();

	FPreviewScene* PreviewScene;
	APostProcessVolume* PreviewVolume;
	UMaterialInstanceDynamic* DynamicMaterial;
};
