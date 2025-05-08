// Source/XRVisRuntime/Public/Rendering/XRVisBoxGeometryGenerator.h
#pragma once

#include "CoreMinimal.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "XRVisGeometryGenerator.h"
#include "RenderResource.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "XRVisGeometryTypes.h"

namespace XRVisBoxParams
{
    static constexpr int32 ThreadsX = 256;
    static constexpr int32 ThreadsY = 1;
    static constexpr int32 ThreadsZ = 1;
}

/**
 * 计算着色器类声明
 */
class FXRVisBoxGenCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FXRVisBoxGenCS);
    SHADER_USE_PARAMETER_STRUCT(FXRVisBoxGenCS, FGlobalShader);

    // 着色器参数结构体
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(uint32, rowsNum)
        SHADER_PARAMETER(uint32, colsNum)
        SHADER_PARAMETER(float, widthx)
        SHADER_PARAMETER(float, widthy)
        SHADER_PARAMETER(float, spaceX)
        SHADER_PARAMETER(float, spaceY)
        SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<float>, heightBuffer)
        // SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, colorBuffer)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, vertexBuffer)
        // SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, vertexColorBuffer)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, indexBuffer)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

    static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
        FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
        OutEnvironment.SetDefine(TEXT("THREADS_X"), XRVisBoxParams::ThreadsX);
        OutEnvironment.SetDefine(TEXT("THREADS_Y"), XRVisBoxParams::ThreadsY);
        OutEnvironment.SetDefine(TEXT("THREADS_Z"), XRVisBoxParams::ThreadsZ);
    }
};

/**
 * 立方体几何体GPU生成器
 */
class XRVISRUNTIME_API FXRVisBoxGeometryGenerator : public FXRVisGeometryGenerator
{
public:
    FXRVisBoxGeometryGenerator();
    virtual ~FXRVisBoxGeometryGenerator() override;

    // 设置几何体参数
    void SetParameters(const FXRVisBoxGeometryParams& InParams);

    // 从FXRVisGeometryGenerator继承的方法
    virtual void GenerateGeometry_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) override;

    virtual bool IsResultValid()  override {return Results.IsValid();}

    virtual FRHIShaderResourceView* GetVertexBufferSRV() const override { return Results.VertexBufferSRV; }
    virtual FRHIShaderResourceView* GetIndexBufferSRV() const override { return Results.IndexBufferSRV; }

    virtual uint32 GetVertexNum() const override { return Params.ColumnCount * Params.RowCount * 24;}
    virtual uint32 GetIndexNum() const override { return Params.ColumnCount * Params.RowCount * 36; }

    virtual FXRVisGeometryResults GetGeometryResult() const override {return Results;}
    
    virtual uint32 GetVertexStride() const override
    {
        return sizeof(FVector4f);
    }

private:
    // 当前参数
    FXRVisBoxGeometryParams Params;

    // 生成的几何体数据
    FXRVisGeometryResults Results;
    
};

// 在CPP文件中实现以上类