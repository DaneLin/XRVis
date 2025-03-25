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

	UFUNCTION(BlueprintCallable)
	void ColorModify(int ModifyIndex, const FColor& Color);
	
	UFUNCTION(BlueprintCallable)
	void Highlight(int ModifyIndex, float HighlightIntensity);	

	UFUNCTION(BlueprintCallable)
	int GetSectionIndex();

	virtual void ConstructMesh(double Rate = 1) override;

	virtual void UpdateOnMouseEnterOrLeft() override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	void ProcessTypeAndShapeInfo();
	
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

	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true))
	bool bEnableZoomAnimation;

	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true,EditCondition="bEnableZoomAnimation", EditConditionHides))
	float ZoomOffset;

	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true))
	bool bEnablePopAnimation;
	
	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true,EditCondition="bEnablePopAnimation", EditConditionHides))
	float PopOffset;

	float GradientFactor;

	double AngleConvertFactor;

	FVector CenterPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	EPieChartStyle PieChartStyle = EPieChartStyle::Base;
	
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	EPieShape PieShape = EPieShape::Round;
	
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	bool bHasInternalDiameter = false;
	
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float InternalDiameter = 110.f;
	
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float ExternalDiameter = 185.f;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float SectionHeight = 50.f;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	bool bIsSectionGaped = false;
	
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float SectionGapAngle = 5.0f;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	bool bIsShapeNightingale = false;
	
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float NightingaleOffset = 10.f;
	
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<double> AccumulatedValues;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float FinalNightingaleOffset;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float FinalInternalDiameter;

	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float FinalSectionGapAngle;
};
