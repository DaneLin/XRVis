#pragma once

#include "CoreMinimal.h"
#include "RenderGraphResources.h"
#include "XRVisGeometryTypes.h"
#include "RenderPasses/XRVisComputePassBase.h"

class FSceneView;

/**
 * GPU几何体生成器接口 - 负责数据生成
 */
class XRVISRUNTIME_API FXRVisGeometryGenerator : public FXRVisComputePassBase
{
public:

    virtual ~FXRVisGeometryGenerator() override = default;

    // 对外接口 - 用于SceneViewExtension调用
    virtual void GenerateGeometry_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) = 0;


     // 获取生成的几何体数据接口
    virtual FRHIShaderResourceView* GetVertexBufferSRV() const = 0;
    virtual FRHIShaderResourceView* GetIndexBufferSRV() const = 0;
	virtual FXRVisGeometryResults GetGeometryResult() const = 0;

	virtual uint32 GetVertexNum() const = 0;
	virtual uint32 GetIndexNum() const = 0;

	virtual uint32 GetVertexStride() const = 0;
    virtual bool IsResultValid()  = 0;
    
    // 判断是否需要更新几何体
    virtual bool NeedsUpdate() const { return bNeedsUpdate; }
    
    // 标记已经使用了数据
    virtual void MarkDataUsed() { bNeedsUpdate = false; }

    // 标记数据需要更新
    virtual void MarkForUpdate() { bNeedsUpdate = true; }

	// 标记是否每帧都更新，作为测试代码
	virtual void SetKeepUpdate(bool bNewKeepUpdate) { bKeepUpdate = bNewKeepUpdate; }

protected:

	bool bKeepUpdate = false;

    bool bNeedsUpdate = true;
};

