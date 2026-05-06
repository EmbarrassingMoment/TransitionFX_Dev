// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

/** Asset type actions for UTransitionSequence. Registers display name, color, and category in the content browser. */
class FAssetTypeActions_TransitionSequence : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
};
