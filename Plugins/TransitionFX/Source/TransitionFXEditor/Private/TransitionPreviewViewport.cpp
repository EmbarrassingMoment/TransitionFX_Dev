// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionPreviewViewport.h"
#include "PreviewScene.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RenderingThread.h"
#include "TransitionFXConfig.h"

// --- FTransitionPreviewViewportClient ---

FTransitionPreviewViewportClient::FTransitionPreviewViewportClient(FPreviewScene* InPreviewScene, const TWeakPtr<SEditorViewport>& InEditorViewportWidget)
	: FEditorViewportClient(nullptr, InPreviewScene, InEditorViewportWidget)
	, PreviewScene(InPreviewScene)
	, PreviewVolume(nullptr)
	, DynamicMaterial(nullptr)
{
	SetRealtime(true);

	EngineShowFlags.SetGrid(false);
	EngineShowFlags.SetPostProcessing(true);
	EngineShowFlags.SetPostProcessMaterial(true);

	SetupPostProcessVolume();
}

FTransitionPreviewViewportClient::~FTransitionPreviewViewportClient()
{
	if (PreviewVolume)
	{
		PreviewVolume->Destroy();
		PreviewVolume = nullptr;
	}
	DynamicMaterial = nullptr;
}

void FTransitionPreviewViewportClient::SetupPostProcessVolume()
{
	if (!PreviewScene)
	{
		return;
	}

	UWorld* World = PreviewScene->GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags = RF_Transient;

	PreviewVolume = World->SpawnActor<APostProcessVolume>(SpawnParams);
	if (PreviewVolume)
	{
		PreviewVolume->bUnbound = true;
		PreviewVolume->Priority = 1000.0f;
		PreviewVolume->bEnabled = true;
	}
}

void FTransitionPreviewViewportClient::SetPreviewMaterial(UMaterialInterface* Material)
{
	if (!Material || !PreviewVolume)
	{
		return;
	}

	DynamicMaterial = UMaterialInstanceDynamic::Create(Material, GetTransientPackage());
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(TransitionFXConfig::ProgressParamName, 0.0f);
		DynamicMaterial->SetScalarParameterValue(TransitionFXConfig::InvertParamName, 0.0f);
		UpdateBlendables();
	}
}

void FTransitionPreviewViewportClient::SetProgress(float Progress)
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(TransitionFXConfig::ProgressParamName, Progress);
	}
}

void FTransitionPreviewViewportClient::SetInvert(bool bInvert)
{
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(TransitionFXConfig::InvertParamName, bInvert ? 1.0f : 0.0f);
	}
}

void FTransitionPreviewViewportClient::UpdateBlendables()
{
	if (!PreviewVolume || !DynamicMaterial)
	{
		return;
	}

	PreviewVolume->Settings.WeightedBlendables.Array.Reset();
	PreviewVolume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, DynamicMaterial));
}

// --- STransitionPreviewViewport ---

void STransitionPreviewViewport::Construct(const FArguments& InArgs)
{
	PreviewScene = MakeShared<FPreviewScene>(FPreviewScene::ConstructionValues());

	// SEditorViewport::Construct calls MakeEditorViewportClient() internally
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

STransitionPreviewViewport::~STransitionPreviewViewport()
{
	if (ViewportClient.IsValid())
	{
		ViewportClient->Viewport = nullptr;
	}
}

TSharedRef<FEditorViewportClient> STransitionPreviewViewport::MakeEditorViewportClient()
{
	ViewportClient = MakeShareable(new FTransitionPreviewViewportClient(
		PreviewScene.Get(), SharedThis(this)));

	return ViewportClient.ToSharedRef();
}

void STransitionPreviewViewport::SetPreviewMaterial(UMaterialInterface* Material)
{
	if (ViewportClient.IsValid())
	{
		ViewportClient->SetPreviewMaterial(Material);
	}
}

void STransitionPreviewViewport::SetProgress(float Progress)
{
	if (ViewportClient.IsValid())
	{
		ViewportClient->SetProgress(Progress);
	}
}

void STransitionPreviewViewport::SetInvert(bool bInvert)
{
	if (ViewportClient.IsValid())
	{
		ViewportClient->SetInvert(bInvert);
	}
}

bool STransitionPreviewViewport::CaptureFrame(TArray<FColor>& OutPixels)
{
	if (ViewportClient.IsValid() && ViewportClient->Viewport)
	{
		FlushRenderingCommands();
		return ViewportClient->Viewport->ReadPixels(OutPixels);
	}
	return false;
}

FIntPoint STransitionPreviewViewport::GetViewportSize() const
{
	if (ViewportClient.IsValid() && ViewportClient->Viewport)
	{
		return ViewportClient->Viewport->GetSizeXY();
	}
	return FIntPoint::ZeroValue;
}
