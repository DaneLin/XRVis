#include "Rendering/XRVisSceneViewExtension.h"
#include "RenderGraphEvent.h"

DECLARE_GPU_DRAWCALL_STAT(AxisChartDataGeneration);


FXRVisSceneViewExtension::FXRVisSceneViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}


void FXRVisSceneViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	FSceneViewExtensionBase::PreRenderView_RenderThread(GraphBuilder, InView);

	RDG_GPU_STAT_SCOPE(GraphBuilder, AxisChartDataGeneration); // for unreal insights
	RDG_EVENT_SCOPE(GraphBuilder, "Chart Data Gneration"); // for render doc
	
	if(GeometryGenerator->NeedsUpdate())
	{
		GeometryGenerator->GenerateGeometry_RenderThread(GraphBuilder, InView);
	}
}


void FXRVisSceneViewExtension::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
	const FPostProcessingInputs& Inputs)
{
	FSceneViewExtensionBase::PrePostProcessPass_RenderThread(GraphBuilder, View, Inputs);
	
	GeometryRenderer->Render(GraphBuilder, View, Inputs,*GeometryGenerator);

}

void FXRVisSceneViewExtension::RegisterGeometryGenerator(FXRVisGeometryGenerator* InGeometryGenerator)
{
	GeometryGenerator = InGeometryGenerator;
}

void FXRVisSceneViewExtension::UnregisterGeometryGenerator()
{
	GeometryGenerator = nullptr;
}

void FXRVisSceneViewExtension::RegisterGeometryRenderer(FXRVisGeometryRenderer* InGeometryRenderer)
{
	GeometryRenderer = InGeometryRenderer;
}

void FXRVisSceneViewExtension::UnregisterGeometryRenderer()
{
	GeometryRenderer = nullptr;
}


