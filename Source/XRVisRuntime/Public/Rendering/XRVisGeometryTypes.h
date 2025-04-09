// Source/XRVisRuntime/Public/Rendering/XRVisGeometryTypes.h
#pragma once

#include "CoreMinimal.h"

/**
 * 几何体生成结果 - 所有几何体生成器共用
 */
struct XRVISRUNTIME_API FXRVisGeometryResults
{
     // 持久资源 - 使用TRefCountPtr进行自动管理
    TRefCountPtr<FRDGPooledBuffer> VertexPooledBuffer;
    TRefCountPtr<FRDGPooledBuffer> IndexPooledBuffer;
    TRefCountPtr<FRDGPooledBuffer> DrawIndirectArgsPooledBuffer;
    
    // 资源视图
    TRefCountPtr<FRHIShaderResourceView> VertexBufferSRV;
    TRefCountPtr<FRHIShaderResourceView> IndexBufferSRV;
    TRefCountPtr<FRHIShaderResourceView> DrawIndirectArgsBufferSRV;

	bool IsValid() const
	{
		return VertexPooledBuffer.IsValid() && IndexPooledBuffer.IsValid() && DrawIndirectArgsPooledBuffer.IsValid();
	}
};

/**
 * 盒子几何体生成参数
 */
struct XRVISRUNTIME_API FXRVisBoxGeometryParams
{
    int32 RowCount = 1000;
    int32 ColumnCount = 1000;
    float Width = 10.0f;
    float Height = 10.0f;
    float SpaceX = 5.0f;
    float SpaceY = 5.0f;
    TArray<float> HeightValues; // 每个Box的高度值
};

/**
 * 简单网格顶点
 */
struct XRVISRUNTIME_API FXRVisSimpleMeshVertex
{
    FVector3f Position;
    FVector2f TextureCoordinate; // 只保留一套UV
    FPackedNormal TangentX;
    FPackedNormal TangentZ;
    FColor Color;
    
    FXRVisSimpleMeshVertex()
        : Position(FVector3f::ZeroVector)
        , TextureCoordinate(FVector2f::ZeroVector)
        , TangentX(FVector3f(1,0,0))
        , TangentZ(FVector3f(0,0,1))
        , Color(FColor::White)
    {
        // 默认设置basis determinant为+1.0
        TangentZ.Vector.W = 127;
    }
};