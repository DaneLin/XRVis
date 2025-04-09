#pragma once

#include "CoreMinimal.h"
#include "RenderGraphUtils.h"
#include "RenderGraphResources.h"
#include "RHI.h"
#include "RHIResources.h"

/**
 * 将RDG缓冲区转换为外部持久缓冲区和SRV
 */
void ConvertToExternalRDGBuffer(
    FRDGBuilder& GraphBuilder,
    FRDGBufferRef SourceBuffer,
    TRefCountPtr<FRDGPooledBuffer>& OutPooledBuffer,
    TRefCountPtr<FRHIShaderResourceView>& OutSRV)
{
    checkf(IsInRenderingThread() || IsInRHIThread(), TEXT("Cannot create from outside the rendering thread"));
    

        
    OutPooledBuffer = GraphBuilder.ConvertToExternalBuffer(SourceBuffer);
    
    
    // // 设置缓冲区最终访问状态为可读取（SRV）
    // GraphBuilder.SetBufferAccessFinal(SourceBuffer, ERHIAccess::SRVGraphics);
    //
    // // 添加缓冲区提取
    // GraphBuilder.QueueBufferExtraction(SourceBuffer, &OutPooledBuffer);
    //
    // // 直接使用ENQUEUE_RENDER_COMMAND来在RDG执行后创建SRV
    // ENQUEUE_RENDER_COMMAND(CreateSRVFromExtractedBuffer)(
    //     [SourceBuffer, &OutPooledBuffer, &OutSRV](FRHICommandListImmediate& RHICmdList)
    //     {
    //         if (OutPooledBuffer.IsValid())
    //         {
    //             FRHIBuffer* RHIBuffer = OutPooledBuffer->GetRHI();
    //             if (RHIBuffer)
    //             {
    //                 OutSRV = RHICreateShaderResourceView(RHIBuffer);
    //             }
    //         }
    //     });
} 