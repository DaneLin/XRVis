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

// 饼图标签位置枚举
UENUM(BlueprintType)
enum class EPieChartLabelPosition : uint8
{
	Inside,    // 标签在饼图内部
	Outside    // 标签在饼图外部
};

// 饼图标签配置结构体
USTRUCT(BlueprintType)
struct FXVPieChartLabelConfig
{
	GENERATED_BODY()

	/** 是否显示标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label")
	bool bShowLabels = true;

	/** 标签字体 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels"))
	UFont* Font = nullptr;

	/** 标签字体大小 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels"))
	float FontSize = 12.0f;

	/** 标签字体颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels"))
	FColor FontColor = FColor::White;

	/** 标签字体粗细（材质强度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels", ClampMin="0.0", ClampMax="3.0"))
	float FontWeight = 1.0f;

	/** 标签格式 - 使用{category}表示类别，{value}表示数值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels"))
	FString LabelFormat = "{category}: {value}";

	/** 标签位置 - 内部或外部 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels"))
	EPieChartLabelPosition LabelPosition = EPieChartLabelPosition::Outside;

	/** 标签距离饼图表面的偏移距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels"))
	float LabelOffset = 10.0f;
    
	/** 是否显示引线 - 仅当标签位置为外部时有效 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels && LabelPosition==EPieChartLabelPosition::Outside"))
	bool bShowLeaderLine = true;
    
	/** 引线颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels && bShowLeaderLine && LabelPosition==EPieChartLabelPosition::Outside"))
	FColor LeaderLineColor = FColor::White;
    
	/** 引线粗细 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels && bShowLeaderLine && LabelPosition==EPieChartLabelPosition::Outside", ClampMin="0.5", ClampMax="5.0"))
	float LeaderLineThickness = 1.0f;
    
	/** 引线长度 - 如果为0，则自动计算 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Label", meta=(EditCondition="bShowLabels && bShowLeaderLine && LabelPosition==EPieChartLabelPosition::Outside"))
	float LeaderLineLength = 0.0f;
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

	/**
	 * 设置饼图标签配置
	 * @param LabelConfig - 标签配置
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Label")
	void SetLabelConfig(const FXVPieChartLabelConfig& InLabelConfig);

	/**
	 * 更新标签显示
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Label")
	void UpdateLabels();

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

	/**
	 * 创建区块标签
	 */
	void CreateSectionLabels();

	/**
	 * 创建标签引线
	 */
	void CreateLabelLeaderLines();

	/**
	 * 计算标签位置
	 * @param SectionIndex - 区块索引
	 * @param StartAngle - 区块起始角度
	 * @param EndAngle - 区块结束角度
	 * @param OutLabelPosition - 计算得到的标签位置
	 * @param OutLabelRotation - 计算得到的标签旋转
	 * @param OutLeaderLineStart - 引线起点(如果需要引线)
	 * @param OutLeaderLineEnd - 引线终点(如果需要引线)
	 */
	void CalculateLabelPosition(
		int32 SectionIndex, 
		float StartAngle, 
		float EndAngle, 
		FVector& OutLabelPosition, 
		FRotator& OutLabelRotation,
		FVector& OutLeaderLineStart,
		FVector& OutLeaderLineEnd);

public:
	
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;

	/**
	 * 饼图标签配置
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chart Property | Label")
	FXVPieChartLabelConfig LabelConfig;

private:
	TArray<FColor> SectionColors;

	TArray<FString> SectionCategories;
	TArray<float> SectionValues;
	
	// 标签组件数组
	TArray<UTextRenderComponent*> SectionLabels;
	
	// 引线组件数组
	TArray<UProceduralMeshComponent*> LeaderLineMeshes;

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
