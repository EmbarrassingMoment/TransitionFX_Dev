// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionFXEditor.h"
#include "AssetTypeActions_TransitionPreset.h"
#include "AssetTypeActions_TransitionSequence.h"
#include "STransitionPreviewPanel.h"
#include "TransitionFXGraphPinFactory.h"
#include "AssetToolsModule.h"
#include "EdGraphUtilities.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/RedirectCollector.h"
#include "UObject/ICookInfo.h"
#include "UObject/SoftObjectPath.h"
#include "TransitionFXConfig.h"

#define LOCTEXT_NAMESPACE "TransitionFXEditor"

const FName FTransitionFXEditorModule::PreviewTabId(TEXT("TransitionFXPreview"));

namespace
{
	/**
	 * Adds the assets the runtime loads by path (the default fade preset used by QuickFade and by level
	 * transitions without a preset, and QuickFade's fallback material) to the cook as startup soft references,
	 * so a packaged game contains them even when no cooked asset references them.
	 *
	 * The ModifyCook delegate is deliberately not used: the cooker counts packages added there as explicit
	 * requests, which turns off "cook all content" for projects that list no maps to cook. Startup soft
	 * references are added after that decision. The cooker consumes them, so they are registered again after
	 * every cook in this process.
	 */
	void RegisterRuntimeLoadedAssetsForCook()
	{
		GRedirectCollector.OnSoftObjectPathLoaded(FSoftObjectPath(TransitionFXConfig::DefaultFadePresetPath), nullptr);
		GRedirectCollector.OnSoftObjectPathLoaded(FSoftObjectPath(TransitionFXConfig::FallbackFadeMaterialPath), nullptr);
	}
}

void FTransitionFXEditorModule::StartupModule()
{
	// Cook the default fade assets even when the project references none of them
	RegisterRuntimeLoadedAssetsForCook();
	CookFinishedHandle = UE::Cook::FDelegates::CookFinished.AddLambda([](UE::Cook::ICookInfo&)
	{
		RegisterRuntimeLoadedAssetsForCook();
	});

	// Register AssetTypeActions for TransitionPreset and TransitionSequence
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TransitionPresetActions = MakeShared<FAssetTypeActions_TransitionPreset>();
	AssetTools.RegisterAssetTypeActions(TransitionPresetActions.ToSharedRef());
	TransitionSequenceActions = MakeShared<FAssetTypeActions_TransitionSequence>();
	AssetTools.RegisterAssetTypeActions(TransitionSequenceActions.ToSharedRef());

	// List the bundled presets in TransitionPreset / TransitionSequence pin pickers regardless of Show Plugin Content
	GraphPinFactory = MakeShared<FTransitionFXGraphPinFactory>();
	FEdGraphUtilities::RegisterVisualPinFactory(GraphPinFactory);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		PreviewTabId,
		FOnSpawnTab::CreateStatic(&FTransitionFXEditorModule::SpawnPreviewTab))
		.SetDisplayName(LOCTEXT("PreviewTabTitle", "TransitionFX Preview"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTransitionFXEditorModule::RegisterMenus));
}

void FTransitionFXEditorModule::ShutdownModule()
{
	UE::Cook::FDelegates::CookFinished.Remove(CookFinishedHandle);

	// Unregister AssetTypeActions
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (TransitionPresetActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(TransitionPresetActions.ToSharedRef());
		}
		if (TransitionSequenceActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(TransitionSequenceActions.ToSharedRef());
		}
	}
	TransitionPresetActions.Reset();
	TransitionSequenceActions.Reset();

	if (GraphPinFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(GraphPinFactory);
		GraphPinFactory.Reset();
	}

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PreviewTabId);
}

TSharedRef<SDockTab> FTransitionFXEditorModule::SpawnPreviewTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STransitionPreviewPanel)
		];
}

void FTransitionFXEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = Menu->FindOrAddSection("TransitionFX");
	Section.AddMenuEntry(
		"OpenTransitionFXPreview",
		LOCTEXT("OpenPreview", "TransitionFX Preview"),
		LOCTEXT("OpenPreviewTooltip", "Open the transition effect preview panel for GIF capture"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(PreviewTabId);
		}))
	);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTransitionFXEditorModule, TransitionFXEditor)
