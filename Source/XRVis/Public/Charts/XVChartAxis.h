// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
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
	//坐标轴位移
	UPROPERTY(EditAnywhere,Category="Axis Property | Displacement")
	FVector AxisDisplacement = FVector(0, 0, 0);

	// 是否随视角变化自动变化坐标轴的朝向。
	UPROPERTY(EditAnywhere,Category="Axis Property | Displacement")
	bool bAutoSwitch = false;

	//网格线粗细
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Thickness")
	float GridLineThickness = 1.f;

	//坐标轴线粗细
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Thickness")
	float AxisLineThickness = 1.5f;

	//x轴线长度参数
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Length")
	float xAxisLineLengthParam = 0.5f;

	//y轴线长度参数
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Length")
	float yAxisLineLengthParam = 0.5f;

	//z轴线长度参数
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Length")
	float zAxisLineLengthParam = 0.5f;

	//坐标轴线长度
	UPROPERTY(EditAnywhere,Category="Axis Property | Line Length")
	float AxisLineLength = 100.f;

	//坐标轴颜色
	UPROPERTY(EditAnywhere,Category="Axis Property | Axis Color")
	FColor AxisColor = FColor::Black;

	//x轴网格数
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Num")
	int xAxisGridNum = 12;

	//y轴网格数
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Num")
	int yAxisGridNum = 3;

	//z轴网格数
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Num")
	int zAxisGridNum = 5; 

	//x轴间隔
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Interval")
	float xAxisInterval = 13.f;

	//y轴间隔
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Interval")
	float yAxisInterval = 13.f;

	//z轴间隔
	UPROPERTY(EditAnywhere,Category="Axis Property | Grid Interval")
	float zAxisInterval = 30.f;

	//x轴文字大小
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Size")
	float xTextSize = 5.0f;

	//y轴文字大小
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Size")
	float yTextSize = 5.0f;

	//z轴文字大小
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Size")
	float zTextSize = 5.0f;

	//文字与x轴的距离
	UPROPERTY(EditAnywhere,Category="Axis Property | Gap")
	float DistanceOfTextAndXAxis = 0.f; 

	//文字与y轴的距离
	UPROPERTY(EditAnywhere,Category="Axis Property | Gap")
	float DistanceOfTextAndYAxis = 0.f;

	//文字与z轴的距离
	UPROPERTY(EditAnywhere,Category="Axis Property | Gap")
	float DistanceOfTextAndZAxis = 0.f;

	//x轴文字颜色
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Color")
	FColor xAxisTextColor = FColor::Black;

	//y轴文字颜色
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Color")
	FColor yAxisTextColor = FColor::Black;

	//z轴文字颜色
	UPROPERTY(EditAnywhere,Category="Axis Property | Text Color")
	FColor zAxisTextColor = FColor::Black;

public:

	void UpdateAxisTextVisibility(bool bIsHidden);

	//设置坐标轴文字
	void SetXAxisText(const TArray<FString>& xText);

	void SetYAxisText(const TArray<FString>& yText);

	void SetZAxisText(const TArray<FString>& zText);

	void SetAxisText(const TArray<FString>& xText,const TArray<FString>& yText,const TArray<FString>& zText);

	//设置坐标轴刻度文字
	void SetXAxisScaleText(const float& xMin,const float& xMax);

	void SetYAxisScaleText(const float& yMin, const float& yMax);

	void SetZAxisScaleText(const float& zMin, const float& zMax);

	void SetAxisScaleText(const float& xMin,const float& xMax, const float& yMin, const float& yMax, const float& zMin, const float& zMax);

	//设置所有坐标轴网格数
	UFUNCTION(BlueprintCallable, Category="Axis Property | Grid Num")
	void SetAxisGridNum(const int& xGridNum, const int& yGridNum, const int& zGridNum);

private:

	//父级图表
	AActor* ParentActor = nullptr;

	//坐标轴变换
	FTransform AxisTransform;

	//x轴文字组件
	TArray<UTextRenderComponent*> XAxisTextComponents;

	//y轴文字组件
	TArray<UTextRenderComponent*> YAxisTextComponents;

	//z轴文字组件
	TArray<UTextRenderComponent*> ZAxisTextComponents;

	//字体
	UFont* Font = nullptr;

	//字体材质
	UMaterial* FontMaterial = nullptr;

	//x轴文字
	TArray<FString> XAxisTexts;

	//y轴文字
	TArray<FString> YAxisTexts;

	//z轴文字
	TArray<FString> ZAxisTexts;

	//x轴刻度文字
	TArray<FString> XScaleTexts;

	//y轴刻度文字
	TArray<FString> YScaleTexts;

	//z轴刻度文字
	TArray<FString> ZScaleTexts;

	//x轴文字绘制状态
	ETextRenderState TextRenderStateX = ETextRenderState::RenderNone;

	//y轴文字绘制状态
	ETextRenderState TextRenderStateY = ETextRenderState::RenderNone;

	//z轴文字绘制状态
	ETextRenderState TextRenderStateZ = ETextRenderState::RenderNone;

	//x轴最大值
	float AxisMaxX = 3;

	//x轴最小值
	float AxisMinX = -3;

	//y轴最大值
	float AxisMaxY = 2560.164646;

	//y轴最小值
	float AxisMinY = 128.145555;

	//z轴最大值
	float AxisMaxZ = 0.114654;

	//z轴最小值
	float AxisMinZ = 0.091545;

	//刻度计算数组
	float MagicArray[26] = { 0.000001,0.00001,0.0001,0.001,0.01,0.1,1.,10.,100.,1000.,10000.,100000.,1000000.,10000000.,100000000.,1000000000.,10000000000.,100000000000.,1000000000000.,10000000000000., 100000000000000.,1000000000000000.,10000000000000000.,100000000000000000.,1000000000000000000.,10000000000000000000. };

private:

	//更新坐标轴变换
	void UpdateAxisTransform();

	//获取字体
	void InitAxisFont();

	//绘制线条
	//LineStart：局部坐标位置起点
	//LineEnd：局部坐标位置终点
	void DrawLine(const UWorld* InWorld, FVector const& LineStart, FVector const& LineEnd, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f);

	//绘制坐标轴
	void DrawAxis(float DeltaTime);

	//绘制网格线
	void DrawGridLine(float duration);

	//绘制坐标轴线
	void DrawAxisLine(float duration);

	//更新文字
	void UpdateText();

	//更新X轴文字
	void UpdateXAxisText();

	//更新Y轴文字
	void UpdateYAxisText();

	//更新Z轴文字
	void UpdateZAxisText();

	//更新X轴刻度文字
	void UpdateXAxisScaleText();

	//更新Y轴刻度文字
	void UpdateYAxisScaleText();

	//更新Z轴刻度文字
	void UpdateZAxisScaleText();

	//获取X轴刻度文字
	void GetXAxisScaleText();

	//获取Y轴刻度文字
	void GetYAxisScaleText();

	//获取Z轴刻度文字
	void GetZAxisScaleText();

	//创建新的TextComponent
	UTextRenderComponent* CreateTextComponent(const FString& Text, const FColor& Color, const float& Size, const float& Scale);

	//更新旧的TextComponent
	void UpdateTextComponent(UTextRenderComponent* TextComponent,const FString& Text, const FColor& Color,const float& Size, const float& Scale);

	//更新TextComponent变换
	void UpdateTextComponentsTransform();

	//更新TextComponent朝向,0为x轴，1为y轴，2为z轴
	void UpdateTextComponentRotation(UTextRenderComponent* TextComponent,const int& type);

	//获取绘制判断向量
	FVector GetJudgeVector();

};
