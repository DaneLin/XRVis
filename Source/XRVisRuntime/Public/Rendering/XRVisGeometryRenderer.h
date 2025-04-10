#pragma once

#include "CoreMinimal.h"
#include "XRVisGeometryGenerator.h"

struct FPostProcessingInputs;

class XRVISRUNTIME_API FXRVisGeometryRenderer : public TSharedFromThis<FXRVisGeometryRenderer>
{
public:
	virtual ~FXRVisGeometryRenderer() {}

	virtual void Render(FRDGBuilder& GraphBuilder, const FSceneView& InView,const FPostProcessingInputs& Inputs,FXRVisGeometryGenerator& GeometryGenerators) = 0;

	void SetModelMatrix(const FMatrix44f& InModelMatrix) {ModelMatrix = InModelMatrix;}

protected:
	FMatrix44f ModelMatrix;
};
