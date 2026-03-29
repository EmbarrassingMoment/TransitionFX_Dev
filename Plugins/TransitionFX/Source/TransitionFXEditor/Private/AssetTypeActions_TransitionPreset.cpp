// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "AssetTypeActions_TransitionPreset.h"
#include "TransitionPreset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_TransitionPreset"

FText FAssetTypeActions_TransitionPreset::GetName() const
{
	return LOCTEXT("AssetName", "Transition Preset");
}

UClass* FAssetTypeActions_TransitionPreset::GetSupportedClass() const
{
	return UTransitionPreset::StaticClass();
}

FColor FAssetTypeActions_TransitionPreset::GetTypeColor() const
{
	return FColor(0, 180, 220);
}

uint32 FAssetTypeActions_TransitionPreset::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

#undef LOCTEXT_NAMESPACE
