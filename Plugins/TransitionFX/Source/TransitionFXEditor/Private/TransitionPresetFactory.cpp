// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionPresetFactory.h"
#include "TransitionPreset.h"

UTransitionPresetFactory::UTransitionPresetFactory()
{
	SupportedClass = UTransitionPreset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UTransitionPresetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UTransitionPreset>(InParent, InClass, InName, Flags);
}
