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

UCLASS()
class XRVIS_API AXVBarChart : public AXVChartBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text")
	TArray<FString> x_Text;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text")
	TArray<FString> y_Text;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text")
	TArray<FString> z_Text;
	
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
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

	virtual void ConstructMesh(double Rate = 1) override;

	virtual void UpdateOnMouseEnterOrLeft() override;
	
	virtual void SetValue(const FString& InValue) override;

	virtual void GenerateAllMeshInfo() override;
	
private:
	
	TMap<int, TMap<int, float>> XYZs;
	
	int MaxX, MinX, MaxY,MinY;
	float MaxZ, MinZ;
	int RowCounts, ColCounts;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<FColor> Colors;

	// 当前鼠标悬停位置
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	int HoveredIndex = -1;

	// 当前鼠标点击时选中的柱体下标
	UPROPERTY(BlueprintReadWrite, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	int ClickedIndex = -1;
	
	// X轴上的间距
	UPROPERTY(EditAnywhere, Category="Chart Property | Bar", meta=(AllowPrivateAccess = true, ClampMin="0"))
	int XAxisInterval;
	
	// Y轴上的间距
	UPROPERTY(EditAnywhere, Category="Chart Property | Bar", meta=(AllowPrivateAccess = true, ClampMin="0"))
	int YAxisInterval;

	// 柱体宽度
	UPROPERTY(EditAnywhere, Category="Chart Property | Bar", meta=(AllowPrivateAccess = true, ClampMin="0"))
	int Width;

	// 柱体长度
	UPROPERTY(EditAnywhere, Category="Chart Property | Bar", meta=(AllowPrivateAccess = true, ClampMin="0"))
	int Length;


};
