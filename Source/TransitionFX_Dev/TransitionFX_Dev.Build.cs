// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TransitionFX_Dev : ModuleRules
{
	public TransitionFX_Dev(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		// Widget-layer sample (Content/SampleLevel/L_WidgetLayerSample + Content/Widget/WBP_WidgetLayerSample):
		// UMG for the sample panel, TransitionFX for the subsystem / preset API.
		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore", "TransitionFX" });
	}
}
