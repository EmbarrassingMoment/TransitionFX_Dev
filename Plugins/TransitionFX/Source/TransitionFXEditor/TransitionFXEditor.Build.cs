// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

using UnrealBuildTool;

public class TransitionFXEditor : ModuleRules
{
    public TransitionFXEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Set to 1 to enable batch capture buttons (documentation asset generation tooling).
        PublicDefinitions.Add("TRANSITIONFX_DEV_TOOLS=0");

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
