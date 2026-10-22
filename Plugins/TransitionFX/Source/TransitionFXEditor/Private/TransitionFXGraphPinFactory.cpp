// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionFXGraphPinFactory.h"
#include "TransitionPreset.h"
#include "TransitionSequence.h"
#include "ContentBrowserModule.h"
#include "EdGraphSchema_K2.h"
#include "IContentBrowserSingleton.h"
#include "KismetPins/SGraphPinObject.h"
#include "Misc/PackageName.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	/** Object pin whose asset picker always shows plugin content. */
	class SGraphPinTransitionFXAsset : public SGraphPinObject
	{
	public:
		SLATE_BEGIN_ARGS(SGraphPinTransitionFXAsset) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
		{
			SGraphPinObject::Construct(SGraphPinObject::FArguments(), InGraphPinObj);
		}

	protected:
		/**
		 * Same picker as SGraphPinObject::GenerateAssetPicker (unchanged from UE 5.5 to 5.8) with
		 * bForceShowPluginContent set. The base picker follows the editor-wide Show Plugin Content
		 * setting, which hides everything under /TransitionFX while it is off.
		 */
		virtual TSharedRef<SWidget> GenerateAssetPicker() override
		{
			UClass* AllowedClass = Cast<UClass>(GraphPinObj->PinType.PinSubCategoryObject.Get());
			if (AllowedClass == nullptr)
			{
				AllowedClass = UObject::StaticClass();
			}

			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

			FAssetPickerConfig AssetPickerConfig;
			AssetPickerConfig.Filter.ClassPaths.Add(AllowedClass->GetClassPathName());
			AssetPickerConfig.bAllowNullSelection = true;
			AssetPickerConfig.Filter.bRecursiveClasses = true;
			AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SGraphPinTransitionFXAsset::OnAssetSelectedFromPicker);
			AssetPickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateSP(this, &SGraphPinTransitionFXAsset::OnAssetEnterPressedInPicker);
			AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
			AssetPickerConfig.bAllowDragging = false;
			AssetPickerConfig.InitialAssetSelection = CachedAssetData.GetAsset();
			AssetPickerConfig.bForceShowPluginContent = true;

			UObject* OwningAsset = GraphPinObj->GetOwningNode();
			while (OwningAsset && !OwningAsset->IsAsset())
			{
				OwningAsset = OwningAsset->GetOuter();
			}
			if (OwningAsset)
			{
				AssetPickerConfig.AdditionalReferencingAssets.Add(FAssetData(OwningAsset));
			}

			// Honor the pin's AllowedClasses / DisallowedClasses metadata like the base picker
			const FString AllowedClassesFilterString = GraphPinObj->GetOwningNode()->GetPinMetaData(GraphPinObj->PinName, FName(TEXT("AllowedClasses")));
			if (!AllowedClassesFilterString.IsEmpty())
			{
				AssetPickerConfig.Filter.ClassPaths.Empty();

				TArray<FString> AllowedClassesFilterNames;
				AllowedClassesFilterString.ParseIntoArrayWS(AllowedClassesFilterNames, TEXT(","), true);
				for (const FString& AllowedClassesFilterName : AllowedClassesFilterNames)
				{
					if (!FPackageName::IsShortPackageName(AllowedClassesFilterName))
					{
						AssetPickerConfig.Filter.ClassPaths.Add(FTopLevelAssetPath(AllowedClassesFilterName));
					}
				}
			}

			const FString DisallowedClassesFilterString = GraphPinObj->GetOwningNode()->GetPinMetaData(GraphPinObj->PinName, FName(TEXT("DisallowedClasses")));
			if (!DisallowedClassesFilterString.IsEmpty())
			{
				TArray<FString> DisallowedClassesFilterNames;
				DisallowedClassesFilterString.ParseIntoArrayWS(DisallowedClassesFilterNames, TEXT(","), true);
				for (const FString& DisallowedClassesFilterName : DisallowedClassesFilterNames)
				{
					if (!FPackageName::IsShortPackageName(DisallowedClassesFilterName))
					{
						AssetPickerConfig.Filter.RecursiveClassPathsExclusionSet.Add(FTopLevelAssetPath(DisallowedClassesFilterName));
					}
				}
			}

			return
				SNew(SBox)
				.HeightOverride(300.0f)
				.WidthOverride(300.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("Menu.Background"))
					[
						ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
					]
				];
		}
	};
}

TSharedPtr<SGraphPin> FTransitionFXGraphPinFactory::CreatePin(UEdGraphPin* Pin) const
{
	if (Pin == nullptr || Pin->PinType.IsContainer())
	{
		return nullptr;
	}

	const FName& Category = Pin->PinType.PinCategory;
	if (Category != UEdGraphSchema_K2::PC_Object && Category != UEdGraphSchema_K2::PC_SoftObject)
	{
		return nullptr;
	}

	const UClass* PinClass = Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get());
	if (PinClass && (PinClass->IsChildOf(UTransitionPreset::StaticClass()) || PinClass->IsChildOf(UTransitionSequence::StaticClass())))
	{
		return SNew(SGraphPinTransitionFXAsset, Pin);
	}

	return nullptr;
}
