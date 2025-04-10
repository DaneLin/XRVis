#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "XRVisGeometryGenerator.h"
#include "XRVisGeometryRenderer.h"


/**
 *
 */
class XRVISRUNTIME_API FXRVisSceneViewExtension : public FSceneViewExtensionBase
{
public:
	FXRVisSceneViewExtension(const FAutoRegister& AutoRegister);
	virtual ~FXRVisSceneViewExtension() = default;

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override;

	// 几何体生成管理
	void RegisterGeometryGenerator(FXRVisGeometryGenerator* InGeometryGenerator);
	void UnregisterGeometryGenerator();

	void RegisterGeometryRenderer(FXRVisGeometryRenderer* InGeometryRenderer);
	void UnregisterGeometryRenderer();

private:
	// 几何体生成管理
	FXRVisGeometryGenerator* GeometryGenerator;

	FXRVisGeometryRenderer* GeometryRenderer;
};

