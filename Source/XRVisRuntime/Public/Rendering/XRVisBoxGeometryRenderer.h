#pragma once

#include "XRVisGeometryRenderer.h"

class XRVISRUNTIME_API FXRVisBoxGeometryRenderer : public FXRVisGeometryRenderer
{
public:
	virtual void Render(FRDGBuilder& GraphBuilder, const FSceneView& InView,const FPostProcessingInputs& Inputs,FXRVisGeometryGenerator& GeometryGenerators) override;

public:
	struct VertexAttr
	{
		FVector3f Position;
		// FColor Color;
	};
	class FVertexAttrDeclaration : public FRenderResource
	{
	public:
		FVertexDeclarationRHIRef VertexDeclarationRHI;

		virtual void InitRHI(FRHICommandListBase& RHICmdList) override
		{
			FVertexDeclarationElementList elemeList;
			uint16 stride = sizeof(VertexAttr);
			elemeList.Emplace(0, STRUCT_OFFSET(VertexAttr, Position), VET_Float3, 0, stride);
			// elemeList.Emplace(0, STRUCT_OFFSET(VertexAttr, Color), VET_Color, 1, stride);

			VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(elemeList);
		}

		void virtual ReleaseRHI() override { VertexDeclarationRHI.SafeRelease(); }
	};
};
