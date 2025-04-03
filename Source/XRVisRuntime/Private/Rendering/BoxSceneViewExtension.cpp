#include "Rendering/BoxSceneViewExtension.h"

#include "RenderGraphEvent.h"

DECLARE_GPU_DRAWCALL_STAT(AxisChartDataGeneration);

FBoxSceneViewExtension::FBoxSceneViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
}

void FBoxSceneViewExtension::PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	FSceneViewExtensionBase::PostRenderView_RenderThread(GraphBuilder, InView);

	const FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

	RDG_GPU_STAT_SCOPE(GraphBuilder, AxisChartDataGeneration); // for unreal insights
	RDG_EVENT_SCOPE(GraphBuilder, "Axis Chart Data Gneration"); // for render doc

	
}

