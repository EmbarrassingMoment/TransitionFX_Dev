// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionPreset.h"

#if WITH_EDITOR
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Misc/DataValidation.h"
#include "PostProcessTransitionEffect.h"
#include "TransitionFXConfig.h"
#include "WidgetTransitionEffect.h"

#define LOCTEXT_NAMESPACE "TransitionPreset"

EDataValidationResult UTransitionPreset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	bool bHasErrors = false;

	if (EffectClass == nullptr)
	{
		Context.AddError(LOCTEXT("NoEffectClass", "Effect Class is not set. Set it to PostProcessTransitionEffect or WidgetTransitionEffect; StartTransition rejects presets without one."));
		bHasErrors = true;
	}

	if (TransitionMaterial == nullptr)
	{
		Context.AddError(LOCTEXT("NoMaterial", "Transition Material is not set. StartTransition rejects presets without one."));
		bHasErrors = true;
	}
	else
	{
		const FText MaterialName = FText::FromString(TransitionMaterial->GetName());
		float ScalarValue = 0.0f;
		FLinearColor VectorValue;

		if (!TransitionMaterial->GetScalarParameterValue(FMaterialParameterInfo(TransitionFXConfig::ProgressParamName), ScalarValue))
		{
			Context.AddError(FText::Format(LOCTEXT("NoProgressParam", "Transition Material '{0}' has no scalar parameter '{1}', so the transition will not animate."),
				MaterialName, FText::FromName(TransitionFXConfig::ProgressParamName)));
			bHasErrors = true;
		}

		if (!TransitionMaterial->GetScalarParameterValue(FMaterialParameterInfo(TransitionFXConfig::InvertParamName), ScalarValue))
		{
			Context.AddWarning(FText::Format(LOCTEXT("NoInvertParam", "Transition Material '{0}' has no scalar parameter '{1}', so inverted playback (bInvert, InvertTransition) has no effect."),
				MaterialName, FText::FromName(TransitionFXConfig::InvertParamName)));
		}

		if (bOverrideTransitionColor && !TransitionMaterial->GetVectorParameterValue(FMaterialParameterInfo(TransitionFXConfig::ColorParamName), VectorValue))
		{
			Context.AddWarning(FText::Format(LOCTEXT("NoColorParam", "Override Transition Color is enabled, but Transition Material '{0}' has no vector parameter '{1}', so Transition Color has no effect."),
				MaterialName, FText::FromName(TransitionFXConfig::ColorParamName)));
		}

		// Only the built-in effect classes have a known material domain; custom effects are not checked
		const UMaterial* BaseMaterial = TransitionMaterial->GetMaterial();
		const EMaterialDomain Domain = BaseMaterial ? BaseMaterial->MaterialDomain.GetValue() : MD_Surface;
		if (EffectClass && EffectClass->IsChildOf<UWidgetTransitionEffect>() && Domain != MD_UI)
		{
			Context.AddError(FText::Format(LOCTEXT("WidgetNeedsUIDomain", "Effect Class {0} draws the material on a UI widget, but Transition Material '{1}' is not a User Interface domain material. Use an MI_Widget_* material."),
				EffectClass->GetDisplayNameText(), MaterialName));
			bHasErrors = true;
		}
		else if (EffectClass && EffectClass->IsChildOf<UPostProcessTransitionEffect>() && Domain != MD_PostProcess)
		{
			Context.AddError(FText::Format(LOCTEXT("PostProcessNeedsPPDomain", "Effect Class {0} draws the material as a post process blendable, but Transition Material '{1}' is not a Post Process domain material. Use an MI_Transition_* material, or WidgetTransitionEffect for an MI_Widget_* material."),
				EffectClass->GetDisplayNameText(), MaterialName));
			bHasErrors = true;
		}
	}

	if (EasingType == ETransitionEasing::Custom && ProgressCurve == nullptr)
	{
		Context.AddWarning(LOCTEXT("NoProgressCurve", "Easing Type is Custom but Progress Curve is not set, so progress stays linear."));
	}

	if (bHasErrors)
	{
		Result = EDataValidationResult::Invalid;
	}
	else if (Result == EDataValidationResult::NotValidated)
	{
		Result = EDataValidationResult::Valid;
	}
	return Result;
}

#undef LOCTEXT_NAMESPACE
#endif // WITH_EDITOR
