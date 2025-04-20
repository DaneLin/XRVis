// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Actor.h"
#include "XVChartAxis.generated.h"

UENUM()
enum ETextRenderState
{
	RenderText,
	RenderScaleText,
	RenderNone
};
UCLASS()
class XRVIS_API AXVChartAxis : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AXVChartAxis();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	//������λ��
	UPROPERTY(EditAnywhere,Category="Axis Property | Displacement")
	FVector AxisDisplacement = FVector(0, 0, 0);

	// �Ƿ����ӽǱ仯�Զ��仯������ĳ���
	UPROPERTY(EditAnywhere,Category="Axis Property | Displacement")
	bool bAutoSwitch = false;

	//�����ߴ�ϸ
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Thickness")
	float GridLineThickness = 1.f;

	//�������ߴ�ϸ
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Thickness")
	float AxisLineThickness = 1.5f;

	//x���߳��Ȳ���
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Length")
	float xAxisLineLengthParam = 0.5f;

	//y���߳��Ȳ���
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Length")
	float yAxisLineLengthParam = 0.5f;

	//z���߳��Ȳ���
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Length")
	float zAxisLineLengthParam = 0.5f;

	//�������߳���
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Length")
	float AxisLineLength = 100.f;

	//��������ɫ
	UPROPERTY(EditAnywhere,Category="Axis Property | Axis Color")
	FColor AxisColor = FColor::Black;

	//x��������
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Num")
	int xAxisGridNum = 12;

	//y��������
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Num")
	int yAxisGridNum = 3;

	//z��������
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Num")
	int zAxisGridNum = 5; 

	//x����
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Interval")
	float xAxisInterval = 13.f;

	//y����
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Interval")
	float yAxisInterval = 13.f;

	//z����
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Interval")
	float zAxisInterval = 30.f;

	//x�����ִ�С
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Size")
	float xTextSize = 5.0f;

	//y�����ִ�С
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Size")
	float yTextSize = 5.0f;

	//z�����ִ�С
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Size")
	float zTextSize = 5.0f;

	//������x��ľ���
	UPROPERTY(EditAnywhere,Category="Axis Property | Gap")
	float DistanceOfTextAndXAxis = 0.f; 

	//������y��ľ���
	UPROPERTY(EditAnywhere,Category="Axis Property | Gap")
	float DistanceOfTextAndYAxis = 0.f;

	//������z��ľ���
	UPROPERTY(EditAnywhere,Category="Axis Property | Gap")
	float DistanceOfTextAndZAxis = 0.f;

	//x��������ɫ
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Color")
	FColor xAxisTextColor = FColor::Black;

	//y��������ɫ
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Color")
	FColor yAxisTextColor = FColor::Black;

	//z��������ɫ
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Color")
	FColor zAxisTextColor = FColor::Black;

public:

	void UpdateAxisTextVisibility(bool bIsHidden);

	//��������������
	void SetXAxisText(const TArray<FString>& xText);

	void SetYAxisText(const TArray<FString>& yText);

	void SetZAxisText(const TArray<FString>& zText);

	void SetAxisText(const TArray<FString>& xText,const TArray<FString>& yText,const TArray<FString>& zText);

	//����������̶�����
	void SetXAxisScaleText(const float& xMin,const float& xMax);

	void SetYAxisScaleText(const float& yMin, const float& yMax);

	void SetZAxisScaleText(const float& zMin, const float& zMax);

	void SetAxisScaleText(const float& xMin,const float& xMax, const float& yMin, const float& yMax, const float& zMin, const float& zMax);

private:

	//����ͼ��
	AActor* ParentActor = nullptr;

	//������任
	FTransform AxisTransform;

	//x���������
	TArray<UTextRenderComponent*> XAxisTextComponents;

	//y���������
	TArray<UTextRenderComponent*> YAxisTextComponents;

	//z���������
	TArray<UTextRenderComponent*> ZAxisTextComponents;

	//����
	UFont* Font = nullptr;

	//�������
	UMaterial* FontMaterial = nullptr;

	//x������
	TArray<FString> XAxisTexts;

	//y������
	TArray<FString> YAxisTexts;

	//z������
	TArray<FString> ZAxisTexts;

	//x��̶�����
	TArray<FString> XScaleTexts;

	//y��̶�����
	TArray<FString> YScaleTexts;

	//z��̶�����
	TArray<FString> ZScaleTexts;

	//x�����ֻ���״̬
	ETextRenderState TextRenderStateX = ETextRenderState::RenderNone;

	//y�����ֻ���״̬
	ETextRenderState TextRenderStateY = ETextRenderState::RenderNone;

	//z�����ֻ���״̬
	ETextRenderState TextRenderStateZ = ETextRenderState::RenderNone;

	//x�����ֵ
	float AxisMaxX = 3;

	//x����Сֵ
	float AxisMinX = -3;

	//y�����ֵ
	float AxisMaxY = 2560.164646;

	//y����Сֵ
	float AxisMinY = 128.145555;

	//z�����ֵ
	float AxisMaxZ = 0.114654;

	//z����Сֵ
	float AxisMinZ = 0.091545;

	//�̶ȼ�������
	float MagicArray[26] = { 0.000001,0.00001,0.0001,0.001,0.01,0.1,1.,10.,100.,1000.,10000.,100000.,1000000.,10000000.,100000000.,1000000000.,10000000000.,100000000000.,1000000000000.,10000000000000., 100000000000000.,1000000000000000.,10000000000000000.,100000000000000000.,1000000000000000000.,10000000000000000000. };
private:

	//����������任
	void UpdateAxisTransform();

	//��ȡ����
	void InitAxisFont();

	//��������
	//LineStart���ֲ�����λ�����
	//LineEnd���ֲ�����λ���յ�
	void DrawLine(const UWorld* InWorld, FVector const& LineStart, FVector const& LineEnd, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f);

	//����������
	void DrawAxis(float DeltaTime);

	//����������
	void DrawGridLine(float duration);

	//������������
	void DrawAxisLine(float duration);

	//��������
	void UpdateText();

	//����X������
	void UpdateXAxisText();

	//����Y������
	void UpdateYAxisText();

	//����Z������
	void UpdateZAxisText();

	//����X��̶�����
	void UpdateXAxisScaleText();

	//����Y��̶�����
	void UpdateYAxisScaleText();

	//����Z��̶�����
	void UpdateZAxisScaleText();

	//��ȡX��̶�����
	void GetXAxisScaleText();

	//��ȡY��̶�����
	void GetYAxisScaleText();

	//��ȡZ��̶�����
	void GetZAxisScaleText();

	//�����µ�TextComponent
	UTextRenderComponent* CreateTextComponent(const FString& Text, const FColor& Color, const float& Size, const float& Scale);

	//���¾ɵ�TextComponent
	void UpdateTextComponent(UTextRenderComponent* TextComponent,const FString& Text, const FColor& Color,const float& Size, const float& Scale);

	//����TextComponent�任
	void UpdateTextComponentsTransform();

	//����TextComponent����,0Ϊx�ᣬ1Ϊy�ᣬ2Ϊz��
	void UpdateTextComponentRotation(UTextRenderComponent* TextComponent,const int& type);

	//��ȡ�����ж�����
	FVector GetJudgeVector();

};
