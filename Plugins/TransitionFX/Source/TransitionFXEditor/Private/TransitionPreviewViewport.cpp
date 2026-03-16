// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionPreviewViewport.h"
#include "PreviewScene.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TransitionFXConfig.h"

FTransitionPreviewViewportClient::FTransitionPreviewViewportClient(FAssetEditorModeManager* InModeManager, FPreviewScene* InPreviewScene)
	: FEditorViewportClient(InModeManager, InPreviewScene)
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
