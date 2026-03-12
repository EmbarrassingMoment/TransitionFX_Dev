// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionFX.h"

#define LOCTEXT_NAMESPACE "FTransitionFXModule"

DEFINE_LOG_CATEGORY(LogTransitionFX);

/** Called when the TransitionFX module is loaded into memory. */
void FTransitionFXModule::StartupModule()
{
}

/** Called when the TransitionFX module is unloaded from memory. */
void FTransitionFXModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTransitionFXModule, TransitionFX)