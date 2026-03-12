// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTransitionFX, Log, All);

/**
 * Module class for the TransitionFX plugin.
 * Handles module startup and shutdown lifecycle.
 */
class FTransitionFXModule : public IModuleInterface
{
public:

	/** Called when the module is loaded into memory. */
	virtual void StartupModule() override;

	/** Called when the module is unloaded from memory. */
	virtual void ShutdownModule() override;
};
