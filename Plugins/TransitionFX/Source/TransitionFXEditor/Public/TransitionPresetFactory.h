// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "TransitionPresetFactory.generated.h"

/** Factory for creating UTransitionPreset assets via the content browser "New Asset" menu. */
UCLASS()
class UTransitionPresetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UTransitionPresetFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
