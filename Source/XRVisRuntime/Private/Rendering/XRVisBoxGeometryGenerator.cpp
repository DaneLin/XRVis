#include "Rendering/XRVisBoxGeometryGenerator.h"

#include "PropertyEditorModule.h"
#include "Rendering/XRVisRenderUtils.h"
#include "RenderGraphBuilder.h"

DECLARE_GPU_DRAWCALL_STAT(BoxGeneration); // Unreal Insights

IMPLEMENT_GLOBAL_SHADER(FXRVisBoxGenCS, "/XRVis/XRVisGPUBoxShader.usf", "MainCS", SF_Compute);


FXRVisBoxGeometryGenerator::FXRVisBoxGeometryGenerator()
{
	
}

FXRVisBoxGeometryGenerator::~FXRVisBoxGeometryGenerator()
{
}


void FXRVisBoxGeometryGenerator::SetParameters(const FXRVisBoxGeometryParams& InParams)
{
	Params = InParams;
	MarkForUpdate();
}

void FXRVisBoxGeometryGenerator::GenerateGeometry_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
	if(!NeedsUpdate())
	{
		return;
	}

	RDG_GPU_STAT_SCOPE(GraphBuilder, BoxGeneration); // Unreal Insights
	RDG_EVENT_SCOPE(GraphBuilder,  "Box Generation"); // RenderDoc
	
	// 计算实际需要的缓冲区大小
	const uint32 NumBoxes = Params.RowCount * Params.ColumnCount;
	constexpr uint32 VerticesPerBox = 24;
	constexpr uint32 IndexPerBox = 36;
	const uint32 NumVertices = NumBoxes * VerticesPerBox;
	const uint32 NumIndices = NumBoxes * IndexPerBox;
	// 创建高度缓冲区
	FRDGBufferRef HeightBufferRDG = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateStructuredDesc(sizeof(float), NumBoxes),
		TEXT("BoxHeightBuffer"));
	GraphBuilder.QueueBufferUpload(HeightBufferRDG, Params.HeightValues.GetData(), Params.HeightValues.Num() * sizeof(float));

	// FRDGBufferRef ColorBufferRDG = GraphBuilder.CreateBuffer(
	// 	FRDGBufferDesc::CreateStructuredDesc(sizeof(FColor), 11),
	// 	TEXT("BoxColorBuffer"));
	// GraphBuilder.QueueBufferUpload(ColorBufferRDG, Params.ColorValues.GetData(), Params.ColorValues.Num() * sizeof(FColor));
    
	// 创建输出缓冲区
	FRDGBufferRef VertexBufferRDG = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f) , NumVertices),
		TEXT("BoxVertexBuffer"));

	// FRDGBufferRef VertexColorBufferRDG = GraphBuilder.CreateBuffer(
	// 	FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f) , NumVertices),
	// 	TEXT("BoxVertexColorBuffer"));
    
	FRDGBufferRef IndexBufferRDG = GraphBuilder.CreateBuffer(
		FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), NumIndices),
		TEXT("BoxIndexBuffer"));
    
    // 创建着色器参数
    FXRVisBoxGenCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FXRVisBoxGenCS::FParameters>();
    PassParameters->rowsNum = Params.RowCount;
    PassParameters->colsNum = Params.ColumnCount;
    PassParameters->widthx = Params.Width;
    PassParameters->widthy = Params.Height;
    PassParameters->spaceX = Params.SpaceX;
    PassParameters->spaceY = Params.SpaceY;
    PassParameters->heightBuffer = GraphBuilder.CreateSRV(HeightBufferRDG);
    // PassParameters->colorBuffer = GraphBuilder.CreateUAV(ColorBufferRDG);
    PassParameters->vertexBuffer = GraphBuilder.CreateUAV(VertexBufferRDG);
    // PassParameters->vertexColorBuffer = GraphBuilder.CreateUAV(VertexColorBufferRDG);
    PassParameters->indexBuffer = GraphBuilder.CreateUAV(IndexBufferRDG);
    
    
    // 计算线程组数量
    FIntVector GroupCount(
        FMath::DivideAndRoundUp((int32)NumBoxes,XRVisBoxParams::ThreadsX),
        1,
        1
    );

	const FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    
    // 直接调用基类提供的AddComputePass方法
    AddComputePass<FXRVisBoxGenCS>(
        GraphBuilder,
        GlobalShaderMap,
        RDG_EVENT_NAME("Generate Box Geometry"),
        PassParameters,
        GroupCount
    );
	
	// 转换为外部缓冲区
	Results.VertexPooledBuffer = GraphBuilder.ConvertToExternalBuffer(VertexBufferRDG);
	Results.IndexPooledBuffer = GraphBuilder.ConvertToExternalBuffer(IndexBufferRDG);

	Results.CachedVertexBufferRHI = Results.VertexPooledBuffer->GetRHI();
	Results.CachedIndexBufferRHI = Results.IndexPooledBuffer->GetRHI();

	if(!bKeepUpdate)
	{
		MarkDataUsed();
	}
}
