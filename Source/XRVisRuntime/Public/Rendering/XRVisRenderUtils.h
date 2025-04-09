#pragma once

#include "CoreMinimal.h"
#include "RenderGraphUtils.h"

/**
 * 将RDG缓冲区转换为外部持久缓冲区和SRV
 */
void ConvertToExternalRDGBuffer(
    FRDGBuilder& GraphBuilder,
    FRDGBufferRef SourceBuffer,
    TRefCountPtr<FRDGPooledBuffer>& OutPooledBuffer,
    TRefCountPtr<FRHIShaderResourceView>& OutSRV)
{
    // 确保缓冲区最终状态适合读取
    GraphBuilder.SetBufferAccessFinal(SourceBuffer, ERHIAccess::SRVGraphics);
    
    // 添加缓冲区提取
    GraphBuilder.QueueBufferExtraction(SourceBuffer, &OutPooledBuffer);
    
    // 直接使用ENQUEUE_RENDER_COMMAND来在RDG执行后创建SRV
    ENQUEUE_RENDER_COMMAND(CreateSRVFromExtractedBuffer)(
        [&OutPooledBuffer, &OutSRV](FRHICommandListImmediate& RHICmdList)
        {
            if (OutPooledBuffer.IsValid() && !OutSRV.IsValid())
            {
                FRHIBuffer* RHIBuffer = OutPooledBuffer->GetRHI();
                OutSRV = RHICreateShaderResourceView(RHIBuffer);
            }
        });
}