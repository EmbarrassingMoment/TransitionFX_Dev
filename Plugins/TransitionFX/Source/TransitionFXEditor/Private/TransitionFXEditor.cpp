// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionFXEditor.h"
#include "AssetTypeActions_TransitionPreset.h"
#include "AssetTypeActions_TransitionSequence.h"
#include "STransitionPreviewPanel.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Settings/ContentBrowserSettings.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

#define LOCTEXT_NAMESPACE "TransitionFXEditor"

const FName FTransitionFXEditorModule::PreviewTabId(TEXT("TransitionFXPreview"));

void FTransitionFXEditorModule::StartupModule()
{
	// Register AssetTypeActions for TransitionPreset and TransitionSequence
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TransitionPresetActions = MakeShared<FAssetTypeActions_TransitionPreset>();
	AssetTools.RegisterAssetTypeActions(TransitionPresetActions.ToSharedRef());
	TransitionSequenceActions = MakeShared<FAssetTypeActions_TransitionSequence>();
	AssetTools.RegisterAssetTypeActions(TransitionSequenceActions.ToSharedRef());

	// Ensure plugin content is indexed by the AssetRegistry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.ScanPathsSynchronous({TEXT("/TransitionFX")}, /*bForceRescan=*/ true);

	// Enable plugin content visibility in Content Browser and asset pickers
	UContentBrowserSettings* CBSettings = GetMutableDefault<UContentBrowserSettings>();
	if (CBSettings && !CBSettings->GetDisplayPluginFolders())
	{
		CBSettings->SetDisplayPluginFolders(true);
		CBSettings->PostEditChange();
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		PreviewTabId,
		FOnSpawnTab::CreateStatic(&FTransitionFXEditorModule::SpawnPreviewTab))
		.SetDisplayName(LOCTEXT("PreviewTabTitle", "TransitionFX Preview"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTransitionFXEditorModule::RegisterMenus));
}

void FTransitionFXEditorModule::ShutdownModule()
{
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
