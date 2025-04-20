// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "XVChartUtils.generated.h"

/**
 * 
 */

class UTextRenderComponent;
struct FProcMeshTangent;

/**
 * 图表区块信息
 */
USTRUCT()
struct FXVChartSectionInfo
{
	GENERATED_BODY()
	
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> VertexColors;
};

/**
 * 平面信息
 */
USTRUCT()
struct FXVPlaneInfo
{
	GENERATED_BODY()

	float RadiansAngle;

	FVector NearTopNormal;
	FVector NearBottomNormal;
	FVector FarTopNormal;
	FVector FarBottomNormal;

	FVector NearTopVertexPosition;
	FVector NearBottomVertexPosition;
	FVector FarTopVertexPosition;
	FVector FarBottomVertexPosition;
};

/**
 * 图表工具类
 */
class XRVIS_API XVChartUtils
{
public:
	XVChartUtils();
	~XVChartUtils();

	// 根据顶点信息、颜色信息、法线信息等添加ProceduralMeshComponent需要形式的三角形
	static void AddBaseTriangle(
		TArray<FXVChartSectionInfo>& SectionInfos,
		const size_t SectionIndex,
		const FVector& InFirstPoint,
		const FVector& InSecondPoint,
		const FVector& InThirdPoint,
		const FVector& InFirstNormal,
		const FVector& InSecondNormal,
		const FVector& InThirdNormal,
		const FVector2D& InFirstUV,
		const FVector2D& InSecondUV,
		const FVector2D& InThirdUV,
		const FProcMeshTangent& Tangent,
		const FColor& TriangleColor);

	/**
	 * 添加平面
	 */
	static void AddBaseQuad(
		TArray<FXVChartSectionInfo>& SectionInfos,
		const size_t SectionIndex,
		const FVector& InFirstPoint,
		const FVector& InSecondPoint,
		const FVector& InThirdPoint,
		const FVector& InFouthPoint,
		const FVector& InQuadNormal,
		const FVector2D& InFirstUV,
		const FVector2D& InSecondUV,
		const FVector2D& InThirdUV,
		const FVector2D& InFouthUV,
		const FProcMeshTangent& Tangent,
		const FColor& TriangleColor);

	/**
	 * 计算平面信息
	 */
	static void CalcAnglePlaneInfo(const FVector& CenterPosition, const size_t& Angle, const float& PlaneNearDis,
	                               const float& PlaneFarDis, const float& Height,
	                               struct FXVPlaneInfo& OutPlaneInfo);

	/**
	 * 创建长方体
	 */
	static void CreateBox(TArray<FXVChartSectionInfo>& SectionInfos,
	                      const int& SectionIndex, const FVector& InPosition,
	                      const float& InLength, const float& InWidth, const float& InHeight,const float& InNextHeight, const FColor& InColor);

	/**
	 * 创建球体
	 */
	static void CreateSphere(TArray<FXVChartSectionInfo>& SectionInfos,
						  const int& SectionIndex, const FVector& InPosition,
						  const float& SphereRadius, const int& NumSphereSlices, const int& NumSphereStacks, const FColor& InColor);

	/**
	 * 创建文本渲染组件
	 */
	static UTextRenderComponent* CreateTextRenderComponent(UObject* Outer, const FText& Text, FColor Color, bool bVisible);
	
	/**
	 * 获取鼠标点击结果
	 */
	static FHitResult GetCursorHitResult(const UWorld* World);

	/**
	 * 辅助函数，从相关路径加载资源
	 */
	template<typename T>
	static void LoadResourceFromPath(const TCHAR* ObjectToFind, T*& OutResource )
	{
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectToFind);
		check(Finder.Succeeded() && "Failed to find Resource");
		OutResource = Finder.Object;
	}

};


