#include "Rendering/XRVisBoxGeometryRenderer.h"

#include "FXRenderingUtils.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"
#include "Runtime/Renderer/Private/PostProcess/PostProcessing.h"

DECLARE_GPU_DRAWCALL_STAT(BoxDraw); // Unreal Insights

TGlobalResource<FXRVisBoxGeometryRenderer::FVertexAttrDeclaration> GXRVisBoxGeometryRendererVertexAttrDeclaration;

class XRVISRUNTIME_API FXRVisBoxShader : public FGlobalShader
{
public:
	SHADER_USE_PARAMETER_STRUCT(FXRVisBoxShader, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, XRVISRUNTIME_API)
		SHADER_PARAMETER(FMatrix44f, ViewProj)
		SHADER_PARAMETER(FMatrix44f, ModelMatrix)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters &Parameters) {
		return true;
	}
};

class XRVISRUNTIME_API FXRVisBoxShaderVS : public FXRVisBoxShader {
public:
	DECLARE_GLOBAL_SHADER(FXRVisBoxShaderVS)

	FXRVisBoxShaderVS() {}
	FXRVisBoxShaderVS(const ShaderMetaType::CompiledShaderInitializerType &Initializer)
		: FXRVisBoxShader(Initializer) {}
};

class XRVISRUNTIME_API FXRVisBoxShaderPS : public FXRVisBoxShader {
public:
	DECLARE_GLOBAL_SHADER(FXRVisBoxShaderPS)

	FXRVisBoxShaderPS() {}
	FXRVisBoxShaderPS(const ShaderMetaType::CompiledShaderInitializerType &Initializer)
		: FXRVisBoxShader(Initializer) {}
};

IMPLEMENT_GLOBAL_SHADER(FXRVisBoxShaderVS, "/XRVis/XRVisBoxDrawShader.usf", "VS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FXRVisBoxShaderPS, "/XRVis/XRVisBoxDrawShader.usf", "PS", SF_Pixel);

void FXRVisBoxGeometryRenderer::Render(FRDGBuilder& GraphBuilder,const  FSceneView& InView,const FPostProcessingInputs& Inputs,FXRVisGeometryGenerator& GeometryGenerators)
{
	RDG_GPU_STAT_SCOPE(GraphBuilder, BoxDraw); // Unreal Insights
	RDG_EVENT_SCOPE(GraphBuilder,  "Box Draw"); // RenderDoc
	
	const FIntRect PrimaryViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(InView);
	FScreenPassTexture SceneColor((*Inputs.SceneTextures)->SceneColorTexture, PrimaryViewRect);
	FMatrix ViewProj = InView.ViewMatrices.GetViewProjectionMatrix();

	FXRVisBoxShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FXRVisBoxShader::FParameters>();
	FScreenPassRenderTarget SceneColorRenderTarget(SceneColor, ERenderTargetLoadAction::ELoad);
	PassParameters->RenderTargets[0] = SceneColorRenderTarget.GetRenderTargetBinding();
	PassParameters->ViewProj = FMatrix44f(ViewProj);
	PassParameters->ModelMatrix = FMatrix44f(ModelMatrix);
	
	GraphBuilder.AddPass(RDG_EVENT_NAME("Draw Generated Box"),
		PassParameters,
		ERDGPassFlags::Raster,
		[&GeometryGenerators, PrimaryViewRect,PassParameters](FRHICommandList &RHICmdList)
		{
			RHICmdList.SetViewport(0.f, 0.f, 0.f, PrimaryViewRect.Max.X, PrimaryViewRect.Max.Y, 1.f);

			TShaderMapRef<FXRVisBoxShaderVS> ShaderVS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			TShaderMapRef<FXRVisBoxShaderPS> ShaderPS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			

			FGraphicsPipelineStateInitializer GraphicsPSOInit;
			RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_CCW>::GetRHI();
			GraphicsPSOInit.DepthStencilState =TStaticDepthStencilState<false, CF_Greater>::GetRHI();
			GraphicsPSOInit.BlendState =TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha>::GetRHI();
			GraphicsPSOInit.PrimitiveType = PT_TriangleList;
			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =GXRVisBoxGeometryRendererVertexAttrDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = ShaderVS.GetVertexShader();
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = ShaderPS.GetPixelShader();
			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

			SetShaderParameters(RHICmdList, ShaderVS, ShaderVS.GetVertexShader(), *PassParameters);
			SetShaderParameters(RHICmdList, ShaderPS, ShaderPS.GetPixelShader(), *PassParameters);
			
			RHICmdList.SetStreamSource(0, GeometryGenerators.GetGeometryResult().CachedVertexBufferRHI, 0);
			RHICmdList.DrawIndexedPrimitive(GeometryGenerators.GetGeometryResult().CachedIndexBufferRHI, 0, 0, GeometryGenerators.GetVertexNum(), 0, GeometryGenerators.GetIndexNum() / 3, 1);
		});
}
