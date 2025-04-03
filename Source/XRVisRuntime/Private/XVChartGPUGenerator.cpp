// Fill out your copyright notice in the Description page of Project Settings.


#include "XVChartGPUGenerator.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIGPUReadback.h"
#include "ShaderParameterStruct.h"

#define QUERY_TIME 1

constexpr uint32 NUM_THREADS_GPUBoxShader_X = 32;
constexpr uint32 NUM_THREADS_GPUBoxShader_Y = 1;
constexpr uint32 NUM_THREADS_GPUBoxShader_Z = 1;

class FXVGPUBoxShaderDeclaration : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FXVGPUBoxShaderDeclaration);
	SHADER_USE_PARAMETER_STRUCT(FXVGPUBoxShaderDeclaration, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, rowsNum)
		SHADER_PARAMETER(uint32, colsNum)
		SHADER_PARAMETER(float, widthx)
		SHADER_PARAMETER(float, widthy)
		SHADER_PARAMETER(float, spaceX)
		SHADER_PARAMETER(float, spaceY)
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float>, Height)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, OutputVertices)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<int32>, OutputIndices)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, OutputNormals)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, OutputUVs)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float4>, OutputVertexColors)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_GPUBoxShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_GPUBoxShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_GPUBoxShader_Z);
	}
};

IMPLEMENT_GLOBAL_SHADER(FXVGPUBoxShaderDeclaration, "/XRVis/XRVisGPUBoxShader.usf", "MainCS", SF_Compute);

void UXVChartGPUGenerator::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FXVGPUBoxDispatchParams Params,
	TFunction<void(FGPUBoxOutputPtr)> AsyncCallback)
{
	FRDGBuilder GraphBuilder(RHICmdList);
    FXVGPUBoxShaderDeclaration::FParameters* Parameters = GraphBuilder.AllocParameters<FXVGPUBoxShaderDeclaration::FParameters>();

    // Set basic parameters
    Parameters->rowsNum = Params.RowsNum;
    Parameters->colsNum = Params.ColsNum;
    Parameters->widthx = Params.WidthX;
    Parameters->widthy = Params.WidthY;
    Parameters->spaceX = Params.SpaceX;
    Parameters->spaceY = Params.SpaceY;

    // Calculate buffer sizes based on shader implementation
    uint32 NumBoxes = Params.RowsNum * Params.ColsNum;
    uint32 VerticesPerBox = 24;  // 6 faces * 4 vertices per face
    uint32 IndicesPerBox = 36;   // 6 faces * 6 indices per face (2 triangles)
    uint32 NumVertices = NumBoxes * VerticesPerBox;
    uint32 NumIndices = NumBoxes * IndicesPerBox;

    // Create and upload height buffer
    FRDGBufferRef HeightBuffer = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(float), NumBoxes),
        TEXT("HeightBuffer"));
    GraphBuilder.QueueBufferUpload(HeightBuffer, Params.Heights.GetData(), NumBoxes * sizeof(float));
    Parameters->Height = GraphBuilder.CreateSRV(HeightBuffer);

    // Create output buffers matching shader structure
    FRDGBufferRef OutputVerticesBuffer = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), NumVertices),
        TEXT("OutputVertices"));
    FRDGBufferRef OutputIndicesBuffer = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(int32), NumIndices),
        TEXT("OutputIndices"));
    FRDGBufferRef OutputNormalsBuffer = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), NumVertices),
        TEXT("OutputNormals"));
    FRDGBufferRef OutputUVsBuffer = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector2f), NumVertices),
        TEXT("OutputUVs"));
    FRDGBufferRef OutputVertexColorsBuffer = GraphBuilder.CreateBuffer(
        FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector4f), NumVertices),
        TEXT("OutputVertexColors"));

    // Create UAVs for output buffers
    Parameters->OutputVertices = GraphBuilder.CreateUAV(OutputVerticesBuffer);
    Parameters->OutputIndices = GraphBuilder.CreateUAV(OutputIndicesBuffer);
    Parameters->OutputNormals = GraphBuilder.CreateUAV(OutputNormalsBuffer);
    Parameters->OutputUVs = GraphBuilder.CreateUAV(OutputUVsBuffer);
    Parameters->OutputVertexColors = GraphBuilder.CreateUAV(OutputVertexColorsBuffer);

    UE_LOG(LogTemp,Warning, TEXT("GRHIMaxDispatchThreadGroupsPerDimension: %d %d %d"),GRHIMaxDispatchThreadGroupsPerDimension.X, GRHIMaxDispatchThreadGroupsPerDimension.Y,GRHIMaxDispatchThreadGroupsPerDimension.Z);

    TShaderMapRef<FXVGPUBoxShaderDeclaration> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    GraphBuilder.AddPass(
        RDG_EVENT_NAME("GenerateBoxesOnGPU"),
        Parameters,
        ERDGPassFlags::Compute,
        [ComputeShader, Parameters, &NumBoxes](FRHIComputeCommandList& RHICmdList)
        {
            FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *Parameters, FIntVector(FMath::DivideAndRoundUp(NumBoxes, NUM_THREADS_GPUBoxShader_X), 1, 1));
        });

	// TODO: remove these readbacks
    // Setup readbacks for output data
    FRHIGPUBufferReadback* VerticesReadback = new FRHIGPUBufferReadback(TEXT("VerticesReadback"));
    FRHIGPUBufferReadback* IndicesReadback = new FRHIGPUBufferReadback(TEXT("IndicesReadback"));
    FRHIGPUBufferReadback* NormalsReadback = new FRHIGPUBufferReadback(TEXT("NormalsReadback"));
    FRHIGPUBufferReadback* UVsReadback = new FRHIGPUBufferReadback(TEXT("UVsReadback"));
    FRHIGPUBufferReadback* VertexColorsReadback = new FRHIGPUBufferReadback(TEXT("VertexColorsReadback"));

    // Queue readback passes
    AddEnqueueCopyPass(GraphBuilder, VerticesReadback, OutputVerticesBuffer, 0u);
    AddEnqueueCopyPass(GraphBuilder, IndicesReadback, OutputIndicesBuffer, 0u);
    AddEnqueueCopyPass(GraphBuilder, NormalsReadback, OutputNormalsBuffer, 0u);
    AddEnqueueCopyPass(GraphBuilder, UVsReadback, OutputUVsBuffer, 0u);
    AddEnqueueCopyPass(GraphBuilder, VertexColorsReadback, OutputVertexColorsBuffer, 0u);

    // Setup completion callback with readback processing
    auto RunnerFunc = [=](auto&& RunnerFunc) -> void
        {
            if (VerticesReadback->IsReady() &&
                IndicesReadback->IsReady() &&
                NormalsReadback->IsReady() &&
                UVsReadback->IsReady() &&
                VertexColorsReadback->IsReady() )
            {
				FGPUBoxOutputPtr OutVal = MakeShared<FGPUBoxOutputParams>();
				OutVal->Vertices.SetNumUninitialized(NumVertices);
				OutVal->Indices.SetNumUninitialized(NumIndices);
				OutVal->Normals.SetNumUninitialized(NumVertices);
				OutVal->UVs.SetNumUninitialized(NumVertices);
				OutVal->VertexColors.SetNumUninitialized(NumVertices);
				OutVal->Tangents.SetNumUninitialized(NumVertices);

                // 获取数据指针
                const FVector3f* Vertices = (const FVector3f*)VerticesReadback->Lock(NumVertices * sizeof(FVector3f));
                const int32* Indices = (const int32*)IndicesReadback->Lock(NumIndices * sizeof(int32));
                const FVector3f* Normals = (const FVector3f*)NormalsReadback->Lock(NumVertices * sizeof(FVector3f));
                const FVector2f* UVs = (const FVector2f*)UVsReadback->Lock(NumVertices * sizeof(FVector2f));
                const FVector4f* Colors = (const FVector4f*)VertexColorsReadback->Lock(NumVertices * sizeof(FVector4f));

				// Copy readback data to output struct                    
				FMemory::Memcpy(OutVal->Vertices.GetData(), Vertices, NumVertices * sizeof(FVector3f));
				FMemory::Memcpy(OutVal->Indices.GetData(), Indices, NumIndices * sizeof(int32));
				FMemory::Memcpy(OutVal->Normals.GetData(), Normals, NumVertices * sizeof(FVector3f));
				FMemory::Memcpy(OutVal->UVs.GetData(), UVs, NumVertices * sizeof(FVector2f));
				FMemory::Memcpy(OutVal->VertexColors.GetData(), Colors, NumVertices * sizeof(FVector4f));
                       
                AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal]()
                    {
						if (AsyncCallback)
						{
                            AsyncCallback(OutVal);
						}
                        
                    });

                // Cleanup readbacks
                delete VerticesReadback;
                delete IndicesReadback;
                delete NormalsReadback;
                delete UVsReadback;
                delete VertexColorsReadback;
            }
            else
            {
                // Check again on next frame if not ready
                AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
                    RunnerFunc(RunnerFunc);
                    });
            }
        };

    // Start async readback process
    AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc]() {
        RunnerFunc(RunnerFunc);
        });

    // Execute render graph
    GraphBuilder.Execute();
}

void UXVChartGPUGenerator::Activate()
{
	Dispatch(BoxDispatchParams, [this](FGPUBoxOutputPtr)
			{
				UE_LOG(LogTemp, Warning, TEXT("GPUBoxGenerator Completed"));
			});
}

UXVChartGPUGenerator* UXVChartGPUGenerator::ExecuteBoxGenerator(UObject* WorldContextObject, const FXVGPUBoxDispatchParams& DispatchParams)
{
	UXVChartGPUGenerator* Action = NewObject<UXVChartGPUGenerator>();
	Action->BoxDispatchParams = DispatchParams;
	Action->RegisterWithGameInstance(WorldContextObject);

	return Action;
}
