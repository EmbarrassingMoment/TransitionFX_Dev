using UnrealBuildTool;

public class DevMaterialTools : ModuleRules
{
	public DevMaterialTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		// UMaterialEditingLibrary lives in MaterialEditor, hence this module is Editor-only.
		// UMG / UMGEditor: UWidgetBlueprint + UWidgetTree helpers for Python-driven widget layout building.
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MaterialEditor", "UMG", "UMGEditor" });
	}
}
