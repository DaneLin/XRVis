#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

/**
 *
 */
class XRVISRUNTIME_API FBoxSceneViewExtension : public FSceneViewExtensionBase
{
public:
	FBoxSceneViewExtension(const FAutoRegister& AutoRegister);
	virtual ~FBoxSceneViewExtension() = default;

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}


	virtual void PostRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;
};

