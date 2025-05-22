// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "XVChartBase.h"
#include "GameFramework/Actor.h"
#include "XVLineChart.generated.h"

/**
 * 时间数据点结构，用于存储折线图时间轴播放的数据
 */
USTRUCT()
struct FXVTimeDataPoint
{
	GENERATED_BODY()
	
	// 行索引
	int32 RowIndex;
	
	// 列索引
	int32 ColIndex;
	
	// 数据值
	float Value;
	
	// 实际时间值（从数据中解析）
	float TimeValue;
	
	// 用于排序的键值
	int32 SortKey;
};

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

	/**
	 * 应用参考值高亮到线图
	 */
	virtual void ApplyReferenceHighlight() override;
	
	/**
	 * 应用统计轴线到线图
	 */
	virtual void ApplyStatisticalLines() override;
	
	/**
	 * 应用值触发条件到折线图
	 */
	virtual void ApplyValueTriggerConditions() override;
	
	/**
	 * 获取所有数据值
	 */
	virtual TArray<float> GetAllDataValues() const override;
	
	/**
	 * 创建一条统计轴线
	 */
	void CreateStatisticalLine(const FXVStatisticalLine& LineInfo);
	
	/**
	 * 根据时间轴进度更新折线图
	 * @param Progress - 时间轴进度，范围0-1
	 */
	void UpdateMeshBasedOnTimeProgress(double Progress);
	
	/**
	 * 解析具有命名属性的数据，包括时间属性
	 * @param NamedData - 命名属性数据数组
	 */
	void ParseNamedDataWithTime(const TArray<TSharedPtr<FJsonObject>>& NamedData);
	
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

	/** X轴的间隔 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Line")
	int XAxisInterval;
	
	/** Y轴的间隔 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Line")
	int YAxisInterval;
	
	/** Z轴的间隔 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Line")
	int ZAxisInterval;
	
	/** 宽度 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Line")
	int Width;
	
	/** 长度 */
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
	int CurColCount;
	int RowCounts, ColCounts;

	int HoveredIndex = -1;

	// 统计轴线相关
	UPROPERTY()
	TArray<UProceduralMeshComponent*> StatisticalLineMeshes;
	
	UPROPERTY()
	TArray<UTextRenderComponent*> StatisticalLineLabels;
	
	// 时间轴数据
	TArray<FXVTimeDataPoint> TimeData;
	
};
