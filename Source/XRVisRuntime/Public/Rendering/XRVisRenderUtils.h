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
	checkf(IsInRenderingThread() || IsInRHIThread(), TEXT("Cannot create from outside the rendering thread"));
        
	OutPooledBuffer = GraphBuilder.ConvertToExternalBuffer(SourceBuffer);

	GraphBuilder.QueueBufferExtraction(SourceBuffer, &OutPooledBuffer);

	OutSRV = OutPooledBuffer->GetOrCreateSRV(GraphBuilder.RHICmdList, FRHIBufferSRVCreateInfo(PF_R32G32B32F));
    // // 直接使用ENQUEUE_RENDER_COMMAND来在RDG执行后创建SRV
    // ENQUEUE_RENDER_COMMAND(CreateSRVFromExtractedBuffer)(
    //     [&OutPooledBuffer, &OutSRV](FRHICommandListImmediate& RHICmdList)
    //     {
    //         if (OutPooledBuffer.IsValid() && !OutSRV.IsValid())
    //         {
    //             FRHIBuffer* RHIBuffer = OutPooledBuffer->GetRHI();
    //             OutSRV = RHICreateShaderResourceView(RHIBuffer);
    //         }
    //     });
}