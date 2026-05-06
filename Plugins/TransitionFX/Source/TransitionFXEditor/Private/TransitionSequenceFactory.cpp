// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionSequenceFactory.h"
#include "TransitionSequence.h"

UTransitionSequenceFactory::UTransitionSequenceFactory()
{
	SupportedClass = UTransitionSequence::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UTransitionSequenceFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UTransitionSequence>(InParent, InClass, InName, Flags);
}
