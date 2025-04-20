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
	 * ������ά������ָ������ϵ�ڴ�����״ͼ��
	 * @param Data - ����������άͼ�����ݣ���ֵ����ʽ��
	 * @param ChartStyle - �������ɵ���ά��״ͼ����ʽ��
	 * @param PieChartColor - �������ɵ���ά��״ͼÿ���������ɫ��
	 * @param Shape - ���ñ�״ͼ��״��
	 */
	UFUNCTION(BlueprintCallable)
	void Create3DPieChart(const TMap<FString, float>& Data, EPieChartStyle ChartStyle, const TArray<FColor>& PieChartColor,
	                      EPieShape Shape);

	/**
	 * ���ñ�״ͼ����ʽ����ɫ��
	 * @param ChartStyle - �������ɵ���ά��״ͼ����ʽ��
	 * @param PieChartColor - �������ɵ���ά��״ͼÿ���������ɫ��
	 * @param Shape - ���ñ�״ͼ��״��
	 */
	UFUNCTION(BlueprintCallable)
	void Set3DPieChart(EPieChartStyle ChartStyle, const TArray<FColor>& PieChartColor, EPieShape Shape);

	/**
	 * �޸�������ɫ
	 */
	UFUNCTION(BlueprintCallable)
	void ColorModify(int ModifyIndex, const FColor& Color);
	
	/**
	 * ��������
	 */
	UFUNCTION(BlueprintCallable)
	void Highlight(int ModifyIndex, float HighlightIntensity);	

	/**
	 * ��ȡ��������
	 */
	UFUNCTION(BlueprintCallable)
	int GetSectionIndex();

	virtual void ConstructMesh(double Rate = 1) override;

	virtual void UpdateOnMouseEnterOrLeft() override;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	/**
	 * �������ͺ���״��Ϣ
	 */
	void ProcessTypeAndShapeInfo();
	
	/**
	 * ��������
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
	 * �Ƿ��������Ŷ���
	 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true))
	bool bEnableZoomAnimation;

	/**
	 * ����ƫ����
	 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true,EditCondition="bEnableZoomAnimation", EditConditionHides))
	float ZoomOffset;

	/**
	 * �Ƿ����õ�������
	 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true))
	bool bEnablePopAnimation;

	/**
	 * �Ƿ����õ�������
	 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Animation", meta=(AllowPrivateAccess = true,EditCondition="bEnablePopAnimation", EditConditionHides))
	float PopOffset;

	float GradientFactor;

	double AngleConvertFactor;

	FVector CenterPosition = FVector::ZeroVector;

	/**
	 * ��״ͼ��ʽ
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	EPieChartStyle PieChartStyle = EPieChartStyle::Base;

	/**
	 * ��״ͼ��״
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	EPieShape PieShape = EPieShape::Round;

	/**
	 * �Ƿ�����ھ�
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	bool bHasInternalDiameter = false;
	
	/**
	 * �ھ�
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float InternalDiameter = 110.f;

	/**
	 * �⾶
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float ExternalDiameter = 185.f;

	/**
	 * ����߶�
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float SectionHeight = 50.f;

	/**
	 * �Ƿ����������
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	bool bIsSectionGaped = false;

	/**
	 * ������Ƕ�
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float SectionGapAngle = 5.0f;

	/**
	 * �Ƿ�Ϊ�϶����õ����ʽ
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	bool bIsShapeNightingale = false;

	/**
	 * �϶����õ����ʽƫ����
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Style", meta=(AllowPrivateAccess = true))
	float NightingaleOffset = 10.f;
	
	/**
	 * �ۻ�ֵ
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<double> AccumulatedValues;

	/**
	 * �����϶����õ����ʽƫ����
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float FinalNightingaleOffset;

	/**
	 * �����ھ�
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float FinalInternalDiameter;

	/**
	 * ����������Ƕ�
	 */
	UPROPERTY(VisibleAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float FinalSectionGapAngle;
};
