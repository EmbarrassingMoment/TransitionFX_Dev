// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "AssetTypeActions_TransitionSequence.h"
#include "TransitionSequence.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_TransitionSequence"

FText FAssetTypeActions_TransitionSequence::GetName() const
{
	return LOCTEXT("AssetName", "Transition Sequence");
}

UClass* FAssetTypeActions_TransitionSequence::GetSupportedClass() const
{
	return UTransitionSequence::StaticClass();
}

FColor FAssetTypeActions_TransitionSequence::GetTypeColor() const
{
	return FColor(220, 120, 0);
}

uint32 FAssetTypeActions_TransitionSequence::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

#undef LOCTEXT_NAMESPACE
