// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class XRVis : ModuleRules
{
	public XRVis(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
                "ProceduralMeshComponent",
                "Json",
                "Engine",
                "Renderer",
                "RenderCore",
                "RHI",
                "XRVisRuntime",
                "UMG",
                "Slate",
                "SlateCore",
                "DesktopPlatform"
				// ... add other public dependencies that you statically link with here ...
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
				"GPULightmass",
				"XRVisRuntime",
				"UMG",
                "Slate",
                "SlateCore",
                "DesktopPlatform", "DatasmithCore"
                // ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
