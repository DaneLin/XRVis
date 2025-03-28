﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XVChartBase.h"
#include "GameFramework/Actor.h"
#include "XVLineChart.generated.h"

UENUM()
enum class ELineChartStyle : uint8
{
	Base,
	Gradient,
	Transparent,
	Point
};

UCLASS()
class XRVIS_API AXVLineChart : public AXVChartBase
{
	GENERATED_BODY()

	
public:
	// Sets default values for this actor's properties
	AXVLineChart();
	
	virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

	virtual void UpdateOnMouseEnterOrLeft() override;

	virtual void SetValue(const FString& InValue) override;

	virtual void ConstructMesh(double Rate = 1) override;

	virtual void GenerateAllMeshInfo() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	void Create3DLineChart(const FString& Data, ELineChartStyle ChartStyle, FColor LineChartColor);
	
	UFUNCTION(BlueprintCallable)
	void Set3DLineChart(ELineChartStyle InLineChartStyle, FColor LineChartColor);

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	/** 是否启用坐标轴 */
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text")
	bool bEnableAxis;
	
	/** X轴的文本内容，启用坐标轴后可编辑 */
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text", meta=(EditCondition="bEnableAxis"))
	TArray<FString> XText;

	/** Y轴的文本内容，启用坐标轴后可编辑 */
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text", meta=(EditCondition="bEnableAxis"))
	TArray<FString> YText;

	/** Z轴的文本内容，启用坐标轴后可编辑 */
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Chart Property | Axis Text", meta=(EditCondition="bEnableAxis"))
	TArray<FString> ZText;

	UPROPERTY(EditAnywhere, Category="Chart Property | Line")
	int XAxisInterval;
	
	UPROPERTY(EditAnywhere, Category="Chart Property | Line")
	int YAxisInterval;
	
	UPROPERTY(EditAnywhere, Category="Chart Property | Line")
	int Width;
	
	UPROPERTY(EditAnywhere, Category="Chart Property | Line")
	int Length;

	// 球半径，只有当HistogramChartShape为Point时才可以修改
	UPROPERTY(EditAnywhere, Category="Chart Property | Sphere", meta=(ClampMin="0", EditCondition="LineChartStyle==ELineChartStyle::Point", EditConditionHides))
	float SphereRadius;

	// 球细分数，只有当HistogramChartShape为Point时才可以修改
	UPROPERTY(EditAnywhere, Category="Chart Property | Sphere", meta=(ClampMin="3", EditCondition="LineChartStyle==ELineChartStyle::Point", EditConditionHides))
	int NumSphereSlices;

	// 球栈数，只有当HistogramChartShape为Point时才可以修改
	UPROPERTY(EditAnywhere, Category="Chart Property | Sphere", meta=(ClampMin="2", EditCondition="LineChartStyle==ELineChartStyle::Point", EditConditionHides))
	int NumSphereStacks;
	
	// 输入的数据内容
	UPROPERTY(EditAnywhere,  BlueprintReadWrite,Category = "Chart Property | Value")
	FString InValues;
	
	UPROPERTY(EditAnywhere, Category = "Chart Property | Style")
	ELineChartStyle LineChartStyle;
private:

	UPROPERTY(VisibleDefaultsOnly,Category = "Chart Property | Material", meta=(AllowPrivateAccess = true))
	UMaterialInterface* BaseMaterial;

	TArray<FColor> Colors;
	
	TArray<bool> LineSelection;
	TArray<bool> TotalSelection;
	
	TMap<int, TMap<int, int>> XYZs;
	int MaxX, MinX, MaxY,MinY, MaxZ, MinZ;
	int CurrentRolCounts;
	int RowCounts, ColCounts;

	int HoveredIndex = -1;

	
};
