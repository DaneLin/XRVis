#include "XRVisRuntime.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FXRVisRuntimeModule"

void FXRVisRuntimeModule::StartupModule()
{
	// 将实际shader路径映射到引擎中
	FString PluginShaderDirectroy = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("XRVis"))->GetBaseDir(), TEXT("Shaders/Private"));
	AddShaderSourceDirectoryMapping("/XRVis", PluginShaderDirectroy);
}

void FXRVisRuntimeModule::ShutdownModule()
{
    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FXRVisRuntimeModule, XRVisRuntime)