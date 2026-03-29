// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;

/**
 * Editor module for the TransitionFX plugin.
 * Registers the transition preview tab and editor menus.
 */
class FTransitionFXEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	static TSharedRef<class SDockTab> SpawnPreviewTab(const class FSpawnTabArgs& Args);
	void RegisterMenus();

	static const FName PreviewTabId;

	TSharedPtr<IAssetTypeActions> TransitionPresetActions;
};
