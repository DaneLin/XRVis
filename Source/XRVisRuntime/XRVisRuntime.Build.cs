using UnrealBuildTool;

public class XRVisRuntime : ModuleRules
{
    public XRVisRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "ProceduralMeshComponent",
                "Json"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Renderer",
                "RenderCore",
                "RHI",
                "Projects", 
                "ProceduralMeshComponent",
            }
        );
    }
}