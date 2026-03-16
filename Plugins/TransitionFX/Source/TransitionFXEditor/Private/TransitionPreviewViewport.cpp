// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionPreviewViewport.h"
#include "PreviewScene.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TransitionFXConfig.h"

// --- FTransitionPreviewViewportClient ---

FTransitionPreviewViewportClient::FTransitionPreviewViewportClient(FEditorModeTools* InModeTools, FPreviewScene* InPreviewScene)
	: FEditorViewportClient(InModeTools, InPreviewScene)
	, PreviewScene(InPreviewScene)
	, PreviewVolume(nullptr)
	, DynamicMaterial(nullptr)
{
	SetRealtime(true);

	EngineShowFlags.SetGrid(false);
	EngineShowFlags.SetPostProcessing(true);

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
	// SEditorViewport creates ModeTools (FAssetEditorModeManager) in Construct()
	// before calling MakeEditorViewportClient(), so ModeTools.Get() is valid here.
	ViewportClient = MakeShareable(new FTransitionPreviewViewportClient(
		ModeTools.Get(), PreviewScene.Get()));

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
