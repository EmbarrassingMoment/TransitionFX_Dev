// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;
struct FGraphPanelPinFactory;

/**
 * Editor module for the TransitionFX plugin.
 * Registers the transition preview tab, editor menus and the TransitionFX asset pin widgets,
 * and makes the cook include the assets the runtime loads by path.
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
	TSharedPtr<IAssetTypeActions> TransitionSequenceActions;
	TSharedPtr<FGraphPanelPinFactory> GraphPinFactory;

	/** Re-registers the runtime-loaded assets for the next cook in this process. */
	FDelegateHandle CookFinishedHandle;
};
