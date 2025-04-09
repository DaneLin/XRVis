#include "Rendering/XRVisSceneViewExtension.h"

#include "RenderGraphEvent.h"
#include "Rendering/XRVisGeometryGenerator.h"

DECLARE_GPU_DRAWCALL_STAT(AxisChartDataGeneration);

// 单例实例
FXRVisSceneViewExtension* FXRVisSceneViewExtension::Instance = nullptr;

FXRVisSceneViewExtension::FXRVisSceneViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
	Instance = this;
}

FXRVisSceneViewExtension* FXRVisSceneViewExtension::Get()
{
	return Instance;
}

void FXRVisSceneViewExtension::PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	FSceneViewExtensionBase::PostRenderView_RenderThread(GraphBuilder, InView);

	RDG_GPU_STAT_SCOPE(GraphBuilder, AxisChartDataGeneration); // for unreal insights
	RDG_EVENT_SCOPE(GraphBuilder, "Chart Data Gneration"); // for render doc

	for (FXRVisGeometryGenerator* Generator : GeometryGenerators)
	{
		if (Generator && Generator->NeedsUpdate())
		{
			Generator->GenerateGeometry_RenderThread(GraphBuilder, InView);
		}
	}
}

void FXRVisSceneViewExtension::RegisterGeometryGenerator(FXRVisGeometryGenerator* InGeometryGenerator)
{
	GeometryGenerators.Add(InGeometryGenerator);
}

void FXRVisSceneViewExtension::UnregisterGeometryGenerator(FXRVisGeometryGenerator* InGeometryGenerator)
{
	if (GeometryGenerators.Contains(InGeometryGenerator))
	{
		GeometryGenerators.Remove(InGeometryGenerator);
	}
}


