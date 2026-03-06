// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTransitionFX, Log, All);

class FTransitionFXModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
