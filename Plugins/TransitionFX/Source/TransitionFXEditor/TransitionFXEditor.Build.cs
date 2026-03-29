// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

using UnrealBuildTool;

public class TransitionFXEditor : ModuleRules
{
    public TransitionFXEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "TransitionFX",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "InputCore",
                "RenderCore",
                "RHI",
                "ToolMenus",
                "EditorFramework",
                "Projects",
                "DesktopPlatform",
                "AssetTools",
                "AssetRegistry",
                "ContentBrowser"
            }
        );
    }
}
