// Copyright Epic Games, Inc. All Rights Reserved.

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
                "RHI",
                "Renderer",
                "RenderCore",
                "ProceduralMeshComponent",
                "Json",
                "Engine"
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

        // 添加引擎渲染器模块的私有包含路径以便找到TranslucentRendering.h
        PrivateIncludePaths.AddRange(
            new string[] {
                EngineDirectory + "/Source/Runtime/Renderer/Private",
            }
        );
    }
}