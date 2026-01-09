
using UnrealBuildTool;

public class TransitionFX : ModuleRules
{
    public TransitionFX(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

      
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "UMG",           
                "Slate",         
                "SlateCore",     
                "RenderCore",    
                "DeveloperSettings"
            }
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects"
            }
        );
    }
}