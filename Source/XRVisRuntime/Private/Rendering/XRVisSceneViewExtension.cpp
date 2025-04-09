#include "Rendering/XRVisSceneViewExtension.h"
#include "RenderGraphEvent.h"

DECLARE_GPU_DRAWCALL_STAT(AxisChartDataGeneration);


FXRVisSceneViewExtension::FXRVisSceneViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}


void FXRVisSceneViewExtension::PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	FSceneViewExtensionBase::PostRenderView_RenderThread(GraphBuilder, InView);

	RDG_GPU_STAT_SCOPE(GraphBuilder, AxisChartDataGeneration); // for unreal insights
	RDG_EVENT_SCOPE(GraphBuilder, "Chart Data Gneration"); // for render doc

	if(GeometryGenerators->NeedsUpdate())
		GeometryGenerators->GenerateGeometry_RenderThread(GraphBuilder, InView);
		
	
}

void FXRVisSceneViewExtension::RegisterGeometryGenerator(FXRVisGeometryGenerator* InGeometryGenerator)
{
	GeometryGenerators = InGeometryGenerator;
}

void FXRVisSceneViewExtension::UnregisterGeometryGenerator()
{
	GeometryGenerators = nullptr;
}


