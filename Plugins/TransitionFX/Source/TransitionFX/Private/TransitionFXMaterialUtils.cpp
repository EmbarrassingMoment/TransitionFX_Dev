// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionFXMaterialUtils.h"
#include "TransitionPreset.h"
#include "TransitionFX.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace TransitionFXMaterialUtils
{
	void ApplyParameters(UMaterialInstanceDynamic* MID, const FTransitionParameters& Params)
	{
		if (!MID)
		{
			return;
		}

		const UMaterialInterface* ParentMaterial = MID->Parent;

		for (const auto& Pair : Params.ScalarParams)
		{
			// GetScalarParameterValue returns false when the parameter does not exist on
			// the material, so a missing parameter would otherwise be silently ignored.
			float ExistingValue;
			const FMaterialParameterInfo Info(Pair.Key);
			if (!MID->GetScalarParameterValue(Info, ExistingValue))
			{
				UE_LOG(LogTransitionFX, Warning, TEXT("TransitionFX: Material '%s' has no scalar parameter '%s'. Override ignored."), *GetNameSafe(ParentMaterial), *Pair.Key.ToString());
				continue;
			}
			MID->SetScalarParameterValue(Pair.Key, Pair.Value);
		}

		for (const auto& Pair : Params.VectorParams)
		{
			FLinearColor ExistingValue;
			const FMaterialParameterInfo Info(Pair.Key);
			if (!MID->GetVectorParameterValue(Info, ExistingValue))
			{
				UE_LOG(LogTransitionFX, Warning, TEXT("TransitionFX: Material '%s' has no vector parameter '%s'. Override ignored."), *GetNameSafe(ParentMaterial), *Pair.Key.ToString());
				continue;
			}
			MID->SetVectorParameterValue(Pair.Key, Pair.Value);
		}

		for (const auto& Pair : Params.TextureParams)
		{
			if (!Pair.Value)
			{
				continue;
			}

			UTexture* ExistingTexture = nullptr;
			const FMaterialParameterInfo Info(Pair.Key);
			if (!MID->GetTextureParameterValue(Info, ExistingTexture))
			{
				UE_LOG(LogTransitionFX, Warning, TEXT("TransitionFX: Material '%s' has no texture parameter '%s'. Override ignored."), *GetNameSafe(ParentMaterial), *Pair.Key.ToString());
				continue;
			}
			MID->SetTextureParameterValue(Pair.Key, Pair.Value);
		}
	}
}
