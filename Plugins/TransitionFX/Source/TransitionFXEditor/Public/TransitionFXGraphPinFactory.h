// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

/**
 * Creates the Blueprint pin widget for UTransitionPreset / UTransitionSequence pins.
 * Its asset picker always lists plugin content, so the bundled DA_* presets appear even while
 * Content Browser > Settings > Show Plugin Content is off (the engine default before UE 5.8).
 */
class FTransitionFXGraphPinFactory : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* Pin) const override;
};
