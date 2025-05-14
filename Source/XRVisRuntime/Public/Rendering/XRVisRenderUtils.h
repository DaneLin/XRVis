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
}