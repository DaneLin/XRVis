// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "XVChartBase.h"
#include "XVPieChart.generated.h"


UENUM(BlueprintType)
enum class EPieChartStyle : uint8
{
	Base,
	Gradient
};

UENUM(BlueprintType)
enum class EPieShape : uint8
{
	Round,
	Circular,
	Nightingale,
	SectorGap
};

UCLASS()
class XRVIS_API AXVPieChart : public AXVChartBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AXVPieChart();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/**
	 * 基于三维数据在指定坐标系内创建饼状图。
	 * @param Data - 用于生成三维图的数据，键值对形式。
	 * @param ChartStyle - 设置生成的三维饼状图的样式。
	 * @param PieChartColor - 设置生成的三维饼状图每个区域的颜色。
	 * @param Shape - 设置饼状图形状。
	 */
	UFUNCTION(BlueprintCallable)
	void Create3DPieChart(const TMap<FString, float>& Data, EPieChartStyle ChartStyle, const TArray<FColor>& PieChartColor,
	                      EPieShape Shape);

	/**
	 * 设置饼状图的样式和颜色。
	 * @param ChartStyle - 设置生成的三维饼状图的样式。
	 * @param PieChartColor - 设置生成的三维饼状图每个区域的颜色。
	 * @param Shape - 设置饼状图形状。
	 */
	UFUNCTION(BlueprintCallable)
	void Set3DPieChart(EPieChartStyle ChartStyle, const TArray<FColor>& PieChartColor, EPieShape Shape);

	/**
	 * 修改区块颜色
	 */
	UFUNCTION(BlueprintCallable)
	void ColorModify(int ModifyIndex, const FColor& Color);
	
	/**
	 * 高亮区块
	 */
	UFUNCTION(BlueprintCallable)
	void Highlight(int ModifyIndex, float HighlightIntensity);	

	/**
	 * 获取区块索引
	 */
	UFUNCTION(BlueprintCallable)
	int GetSectionIndex();

	virtual void ConstructMesh(double Rate = 1) override;

	virtual void UpdateOnMouseEnterOrLeft() override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	/**
	 * 处理类型和形状信息
	 */
	void ProcessTypeAndShapeInfo();
	
	/**
	 * 更新区块
	 */
	void UpdateSection(const size_t& UpdateSectionIndex, const bool& InSelected, const float& InIR,
	                   const float& InER, const float& InEmissiveIntensity);

public:
	
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;

private:
	TArray<FColor> SectionColors;

	float TimeSinceLastUpdate;
	float SectionHoverCooldown = 0.5f;

	int HoveredSectionIndex = -1;

	/**
	 * 是否启用缩放动画
	 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true))
	bool bEnableZoomAnimation;

	/**
	 * 缩放偏移量
	 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true,EditCondition="bEnableZoomAnimation", EditConditionHides))
	float ZoomOffset;

	/**
	 * 是否启用弹跳动画
	 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true))
	bool bEnablePopAnimation;

	/**
	 * 是否启用弹跳动画
	 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true,EditCondition="bEnablePopAnimation", EditConditionHides))
	float PopOffset;

	float GradientFactor;

	double AngleConvertFactor;

	FVector CenterPosition = FVector::ZeroVector;

	/**
	 * 饼状图样式
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	EPieChartStyle PieChartStyle = EPieChartStyle::Base;

	/**
	 * 饼状图形状
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	EPieShape PieShape = EPieShape::Round;

	/**
	 * 是否具有内径
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	bool bHasInternalDiameter = false;
	
	/**
	 * 内径
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float InternalDiameter = 110.f;

	/**
	 * 外径
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float ExternalDiameter = 185.f;

	/**
	 * 区块高度
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float SectionHeight = 50.f;

	/**
	 * 是否具有区块间距
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	bool bIsSectionGaped = false;

	/**
	 * 区块间距角度
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float SectionGapAngle = 5.0f;

	/**
	 * 是否为南丁格尔玫瑰样式
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	bool bIsShapeNightingale = false;

	/**
	 * 南丁格尔玫瑰样式偏移量
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float NightingaleOffset = 10.f;
	
	/**
	 * 累积值
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<double> AccumulatedValues;

	/**
	 * 最终南丁格尔玫瑰样式偏移量
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float FinalNightingaleOffset;

	/**
	 * 最终内径
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float FinalInternalDiameter;

	/**
	 * 最终区块间距角度
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float FinalSectionGapAngle;
};
