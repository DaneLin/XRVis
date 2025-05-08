// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XVChartBase.h"
#include "GameFramework/Actor.h"
#include "XVBarChart.generated.h"

class AXVChartAxis;

USTRUCT()
struct FBarChartSectionInfo
{
	GENERATED_BODY()
	TArray<FVector> Vertices;
	TArray<int> Indices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> VertexColors;
};

UENUM(BlueprintType)
enum class EHistogramChartShape : uint8
{
	Bar = 0,
	Round,
	Circle,
};

UENUM(BlueprintType)
enum class EHistogramChartStyle : uint8
{
	Base,
	Gradient,
	Transparent,
	Dynamic1,
	Dynamic2
};

UCLASS(Blueprintable)
class XRVIS_API AXVBarChart : public AXVChartBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text")
	TArray<FString> XTextArrs;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text")
	TArray<FString> YTextArrs;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text")
	TArray<FString> ZTextArrs;
	
	UPROPERTY(EditAnywhere,  BlueprintReadWrite,Category = "Chart Property | Value")
	FString InValues;
	
	UPROPERTY(EditAnywhere,  BlueprintReadWrite,Category = "Chart Property | Material")
	UMaterialInterface* HoverMaterial;
	
	UPROPERTY(EditAnywhere,  BlueprintReadWrite,Category = "Chart Property | Material")
	UMaterialInterface* FinalMaterial;

	UPROPERTY(EditAnywhere,  BlueprintReadWrite,Category = "Chart Property | Material")
	UMaterialInterface* BaseMaterial;

	UPROPERTY(EditAnywhere,  BlueprintReadWrite,Category = "Chart Property | Material")
	UMaterialInterface* GradientMaterial;

	UPROPERTY(EditAnywhere,  BlueprintReadWrite,Category = "Chart Property | Material")
	UMaterialInterface* TransparentMaterial;

	UPROPERTY(EditAnywhere,  BlueprintReadWrite,Category = "Chart Property | Material")
	UMaterialInterface* DynamicMaterial1;

	UPROPERTY(EditAnywhere,  BlueprintReadWrite,Category = "Chart Property | Material")
	UMaterialInterface* DynamicMaterial2;

	UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category="Chart Property | Style")
	EHistogramChartStyle HistogramChartStyle;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style")
	EHistogramChartShape HistogramChartShape;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void Create3DHistogramChart(const FString& Data,EHistogramChartStyle InHistogramChartStyle, EHistogramChartShape InHistogramChartShape);

	UFUNCTION(BlueprintCallable)
	void Set3DHistogramChart(EHistogramChartStyle InHistogramChartStyle, EHistogramChartShape InHistogramChartShape);
	
public:
	// Sets default values for this actor's properties
	AXVBarChart();
	void UpdateAxis();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

	UFUNCTION(BlueprintCallable, Category = "Chart Property | Data")
	virtual void ConstructMesh(double Rate = 1) override;

	virtual void UpdateOnMouseEnterOrLeft() override;
	
	UFUNCTION(BlueprintCallable, Category = "Chart Property | Data")
	virtual void SetValue(const FString& InValue) override;

	virtual void GenerateAllMeshInfo() override;

	virtual void DrawWithGPU() override;
	
	/**
	 * 应用参考值高亮到柱状图
	 */
	virtual void ApplyReferenceHighlight() override;
	
	/**
	 * 应用统计轴线到柱状图
	 */
	virtual void ApplyStatisticalLines() override;
	
	/**
	 * 获取所有数据值
	 */
	virtual TArray<float> GetAllDataValues() const override;
	
	/**
	 * 创建一条统计轴线
	 */
	void CreateStatisticalLine(const FXVStatisticalLine& LineInfo);
	
private:
	
	TMap<int, TMap<int, float>> XYZs;
	
	int MaxX, MinX, MaxY,MinY;
	float MaxZ, MinZ;
	int RowCounts, ColCounts;

	TArray<float> HeightValues;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<FColor> Colors;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true, ToolTip="当前鼠标悬停位置"))
	int HoveredIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true, ToolTip="当前鼠标点击时选中的柱体下标"))
	int ClickedIndex = -1;
	
	UPROPERTY(EditAnywhere, Category="Chart Property | Bar", meta=(AllowPrivateAccess = true, ClampMin="0", ToolTip="X轴上的间距"))
	int XAxisInterval;
	
	UPROPERTY(EditAnywhere, Category="Chart Property | Bar", meta=(AllowPrivateAccess = true, ClampMin="0", ToolTip="Y轴上的间距"))
	int YAxisInterval;

	UPROPERTY(EditAnywhere, Category="Chart Property | Bar", meta=(AllowPrivateAccess = true, ClampMin="0", ToolTip="柱体宽度"))
	int Width;

	UPROPERTY(EditAnywhere, Category="Chart Property | Bar", meta=(AllowPrivateAccess = true, ClampMin="0", ToolTip="柱体长度"))
	int Length;

	// 统计轴线相关
	UPROPERTY()
	TArray<UProceduralMeshComponent*> StatisticalLineMeshes;
	
	UPROPERTY()
	TArray<UTextRenderComponent*> StatisticalLineLabels;

};
