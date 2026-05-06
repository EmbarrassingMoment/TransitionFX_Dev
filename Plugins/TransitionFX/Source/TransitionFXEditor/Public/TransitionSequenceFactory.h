// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "TransitionSequenceFactory.generated.h"

/** Factory for creating UTransitionSequence assets via the content browser "New Asset" menu. */
UCLASS()
class UTransitionSequenceFactory : public UFactory
{
	GENERATED_BODY()

public:
	UTransitionSequenceFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
