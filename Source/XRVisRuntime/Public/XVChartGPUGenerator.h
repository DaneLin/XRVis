// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "XVChartGPUGenerator.generated.h"

struct FProcMeshTangent;

// GPU 盒体分发参数
USTRUCT()
struct XRVISRUNTIME_API FXVGPUBoxDispatchParams
{
	GENERATED_BODY()
    
	uint32 RowsNum;
	uint32 ColsNum;
	float WidthX;
	float WidthY;
	float SpaceX;
	float SpaceY;
	TArray<float> Heights;
};

// GPU 盒体输出参数
USTRUCT()
struct XRVISRUNTIME_API FGPUBoxOutputParams
{
	GENERATED_BODY()
	
	TArray<FVector3f> Vertices;
	TArray<int32> Indices;
	TArray<FVector3f> Normals;
	TArray<FVector2f> UVs;
	TArray<FVector3f> Tangents;
	TArray<FLinearColor> VertexColors;
};

using FGPUBoxOutputPtr = TSharedPtr < FGPUBoxOutputParams > ;

// 公共接口类
class XRVISRUNTIME_API FXVGPUBoxInterface
{
public:
	
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnXVGPUBoxGenerator_AsyncExecutionCompleted, const float, Value);

/**
 * 
 */
UCLASS()
class XRVISRUNTIME_API UXVChartGPUGenerator : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:

	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FXVGPUBoxDispatchParams Params,
		TFunction<void(FGPUBoxOutputPtr)> AsyncCallback);

	static void DispatchGameThread(FXVGPUBoxDispatchParams Params, TFunction<void(FGPUBoxOutputPtr)> AsyncCallback)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
			[Params, AsyncCallback](FRHICommandListImmediate& RHICmdList)
			{
				DispatchRenderThread(RHICmdList, Params, AsyncCallback);
			});
	}

	static void Dispatch(FXVGPUBoxDispatchParams Params, const TFunction<void(FGPUBoxOutputPtr)>& AsyncCallback)
	{
		if (IsInRenderingThread())
		{
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}
		else
		{
			DispatchGameThread(Params, AsyncCallback);
		}
	}
	
	virtual void Activate() override;
	static UXVChartGPUGenerator* ExecuteBoxGenerator(UObject* WorldContextObject, const FXVGPUBoxDispatchParams& DispatchParams);

	UPROPERTY(BlueprintAssignable)
	FOnXVGPUBoxGenerator_AsyncExecutionCompleted Completed;

	UPROPERTY(VisibleAnywhere)
	FXVGPUBoxDispatchParams BoxDispatchParams;
};
