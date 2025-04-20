// Fill out your copyright notice in the Description page of Project Settings.


#include "Charts/XVChartAxis.h"
#include <Components/LineBatchComponent.h>

#include "Charts/XVChartUtils.h"
#include "Engine/Font.h"
// Sets default values
AXVChartAxis::AXVChartAxis()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	InitAxisFont();
}

// Called when the game starts or when spawned
void AXVChartAxis::BeginPlay()
{
	Super::BeginPlay();
	ParentActor = GetAttachParentActor();
}
// Called every frame
void AXVChartAxis::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (ParentActor)
	{
		UpdateAxisTextVisibility(ParentActor->IsHidden());
		if (!ParentActor->IsHidden())
		{
			DrawAxis(DeltaTime);
		}
	}
	
}

void AXVChartAxis::SetAxisGridNum(const int& xGridNum, const int& yGridNum, const int& zGridNum)
{
	// 设置各轴的网格数量
	xAxisGridNum = FMath::Max(1, xGridNum); // 确保至少有1个网格
	yAxisGridNum = FMath::Max(1, yGridNum);
	zAxisGridNum = FMath::Max(1, zGridNum);
	
	// 如果已经有刻度文本，则清除并重新生成
	if (!XScaleTexts.IsEmpty() || !YScaleTexts.IsEmpty() || !ZScaleTexts.IsEmpty())
	{
		XScaleTexts.Empty();
		YScaleTexts.Empty();
		ZScaleTexts.Empty();
		
		// 根据新的网格数重新获取刻度文本
		GetXAxisScaleText();
		GetYAxisScaleText();
		GetZAxisScaleText();
	}
}

void AXVChartAxis::UpdateAxisTransform()
{
	if (ParentActor)
	{
		SetActorLocation(ParentActor->GetActorLocation());
		AxisTransform.SetLocation(AxisDisplacement + ParentActor->GetActorLocation());
		AxisTransform.SetRotation(ParentActor->GetActorRotation().Quaternion());
		AxisTransform.SetScale3D(ParentActor->GetActorScale());
	}
}

void AXVChartAxis::InitAxisFont()
{
	XVChartUtils::LoadResourceFromPath(TEXT("Font'/XRVis/Materials/Common_Usage_Font.Common_Usage_Font'"), Font);
	XVChartUtils::LoadResourceFromPath(TEXT("/Script/Engine.Material'/XRVis/Materials/M_AxisTextFont.M_AxisTextFont'"), FontMaterial);
}

void AXVChartAxis::DrawLine(const UWorld* InWorld, FVector const& LineStart, FVector const& LineEnd, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
{
	ULineBatchComponent* LineBatcher = InWorld ? InWorld->LineBatcher : nullptr;
	float lifeTime = (bPersistentLines ? -1.f : LifeTime);
	if (LineBatcher)
	{
		LineBatcher->DrawLine(AxisTransform.TransformPosition(LineStart), AxisTransform.TransformPosition(LineEnd), Color, DepthPriority, Thickness, lifeTime);
	}
}

void AXVChartAxis::DrawAxis(float DeltaTime)
{
	UpdateAxisTransform();
	DrawGridLine(DeltaTime * 2);
	DrawAxisLine(DeltaTime * 2);
	UpdateText();
}

void AXVChartAxis::DrawGridLine(float duration)
{
	FVector judgeVector = GetJudgeVector();
	UWorld* world = GetWorld();
	for (int i = 0; i <= xAxisGridNum; i++)
	{
		if (judgeVector[2] > 0)DrawLine(world, i * FVector::XAxisVector * xAxisInterval,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + i *
			FVector::XAxisVector * xAxisInterval, AxisColor, false, duration, 0,
			GridLineThickness * AxisTransform.GetScale3D().X);
		else DrawLine(
			world,
			FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::XAxisVector * xAxisInterval,
			FVector::ZAxisVector * zAxisGridNum * zAxisInterval + FVector::YAxisVector * yAxisGridNum *
			yAxisInterval + i * FVector::XAxisVector * xAxisInterval, AxisColor, false, duration, 0, GridLineThickness * AxisTransform.GetScale3D().X);
		if (judgeVector[1] > 0)DrawLine(world, i * FVector::XAxisVector * xAxisInterval,
			FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i *
			FVector::XAxisVector * xAxisInterval, AxisColor, false, duration, 0,
			GridLineThickness * AxisTransform.GetScale3D().X);
		else DrawLine(
			world,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + i * FVector::XAxisVector * xAxisInterval,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + FVector::ZAxisVector * zAxisGridNum *
			zAxisInterval + i * FVector::XAxisVector * xAxisInterval, AxisColor, false, duration, 0, GridLineThickness * AxisTransform.GetScale3D().X);
	}
	for (int i = 0; i <= yAxisGridNum; i++)
	{
		if (judgeVector[2] > 0)DrawLine(world, i * FVector::YAxisVector * yAxisInterval,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + i *
			FVector::YAxisVector * yAxisInterval, AxisColor, false, duration, 0,
			GridLineThickness * AxisTransform.GetScale3D().Y);
		else DrawLine(
			world,
			FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::YAxisVector * yAxisInterval,
			FVector::ZAxisVector * zAxisGridNum * zAxisInterval + FVector::XAxisVector * xAxisGridNum *
			xAxisInterval + i * FVector::YAxisVector * yAxisInterval, AxisColor, false, duration, 0, GridLineThickness * AxisTransform.GetScale3D().Y);
		if (judgeVector[0] > 0)DrawLine(world, i * FVector::YAxisVector * yAxisInterval,
			FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i *
			FVector::YAxisVector * yAxisInterval, AxisColor, false, duration, 0,
			GridLineThickness * AxisTransform.GetScale3D().Y);
		else DrawLine(
			world,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + i * FVector::YAxisVector * yAxisInterval,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + FVector::ZAxisVector * zAxisGridNum *
			zAxisInterval + i * FVector::YAxisVector * yAxisInterval, AxisColor, false, duration, 0, GridLineThickness * AxisTransform.GetScale3D().Y);
	}
	for (int i = 0; i <= zAxisGridNum; i++)
	{
		if (judgeVector[1] > 0)DrawLine(world, i * FVector::ZAxisVector * zAxisInterval,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + i *
			FVector::ZAxisVector * zAxisInterval, AxisColor, false, duration, 0,
			GridLineThickness * AxisTransform.GetScale3D().Z);
		else DrawLine(
			world,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + i * FVector::ZAxisVector * zAxisInterval,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + FVector::XAxisVector * xAxisGridNum *
			xAxisInterval + i * FVector::ZAxisVector * zAxisInterval, AxisColor, false, duration, 0,GridLineThickness * AxisTransform.GetScale3D().Z);
		if (judgeVector[0] > 0)DrawLine(world, i * FVector::ZAxisVector * zAxisInterval,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + i *
			FVector::ZAxisVector * zAxisInterval, AxisColor, false, duration, 0,
			GridLineThickness * AxisTransform.GetScale3D().Z);
		else DrawLine(
			world,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + i * FVector::ZAxisVector * zAxisInterval,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + FVector::YAxisVector * yAxisGridNum *
			yAxisInterval + i * FVector::ZAxisVector * zAxisInterval, AxisColor, false, duration, 0, GridLineThickness * AxisTransform.GetScale3D().Z);
	}
}

void AXVChartAxis::DrawAxisLine(float duration)
{
	FVector judgeVector = GetJudgeVector();
	UWorld* world = GetWorld();
	if (judgeVector[1] > 0 && judgeVector[2] > 0)
	{
		DrawLine(world, FVector::YAxisVector * yAxisGridNum * yAxisInterval,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + FVector::XAxisVector * xAxisGridNum *
			xAxisInterval, AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().X);
		for (int i = 0; i <= xAxisGridNum; i++)
		{
			DrawLine(
				world,
				i * FVector::XAxisVector * xAxisInterval + FVector::YAxisVector * yAxisGridNum *
				yAxisInterval,
				i * FVector::XAxisVector * xAxisInterval + FVector::YAxisVector * (yAxisGridNum *
					yAxisInterval + xAxisLineLengthParam * xAxisInterval), AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().X);
		}
	}
	if (judgeVector[1] > 0 && judgeVector[2] < 0)
	{
		DrawLine(
			world,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + FVector::ZAxisVector * zAxisGridNum *
			zAxisInterval,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + FVector::ZAxisVector * zAxisGridNum *
			zAxisInterval + FVector::XAxisVector * xAxisGridNum * xAxisInterval, AxisColor, false, duration, 0,
			AxisLineThickness * AxisTransform.GetScale3D().X);
		for (int i = 0; i <= xAxisGridNum; i++)
		{
			DrawLine(
				world,
				FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::XAxisVector *
				xAxisInterval + FVector::YAxisVector * yAxisGridNum * yAxisInterval,
				FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::XAxisVector *
				xAxisInterval + FVector::YAxisVector * (yAxisGridNum * yAxisInterval + xAxisLineLengthParam * xAxisInterval), AxisColor,
				false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().X);
		}
	}
	if (judgeVector[1] < 0 && judgeVector[2] > 0)
	{
		DrawLine(world, FVector(0, 0, 0), FVector::XAxisVector * xAxisGridNum * xAxisInterval, AxisColor, false,
			duration, 0, AxisLineThickness * AxisTransform.GetScale3D().X);
		for (int i = 0; i <= xAxisGridNum; i++)
		{
			DrawLine(world, i * FVector::XAxisVector * xAxisInterval,
				i * FVector::XAxisVector * xAxisInterval - FVector::YAxisVector * xAxisLineLengthParam * xAxisInterval,
				AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().X);
		}
	}
	if (judgeVector[1] < 0 && judgeVector[2] < 0)
	{
		DrawLine(world, FVector::ZAxisVector * zAxisGridNum * zAxisInterval,
			FVector::ZAxisVector * zAxisGridNum * zAxisInterval + FVector::XAxisVector * xAxisGridNum *
			xAxisInterval, AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().X);
		for (int i = 0; i <= xAxisGridNum; i++)
		{
			DrawLine(
				world,
				FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::XAxisVector *
				xAxisInterval,
				FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::XAxisVector *
				xAxisInterval - FVector::YAxisVector * xAxisLineLengthParam * xAxisInterval, AxisColor, false, duration, 0,
				AxisLineThickness * AxisTransform.GetScale3D().X);
		}
	}
	if (judgeVector[0] > 0 && judgeVector[2] > 0)
	{
		DrawLine(world, FVector::XAxisVector * xAxisGridNum * xAxisInterval,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + FVector::YAxisVector * yAxisGridNum *
			yAxisInterval, AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Y);
		for (int i = 0; i <= yAxisGridNum; i++)
		{
			DrawLine(
				world,
				i * FVector::YAxisVector * yAxisInterval + FVector::XAxisVector * xAxisGridNum *
				xAxisInterval,
				i * FVector::YAxisVector * yAxisInterval + FVector::XAxisVector * (xAxisGridNum *
					xAxisInterval + yAxisLineLengthParam * yAxisInterval), AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Y);
		}
	}
	if (judgeVector[0] > 0 && judgeVector[2] < 0)
	{
		DrawLine(
			world,
			FVector::ZAxisVector * zAxisGridNum * zAxisInterval + FVector::XAxisVector * xAxisGridNum *
			xAxisInterval,
			FVector::ZAxisVector * zAxisGridNum * zAxisInterval + FVector::XAxisVector * xAxisGridNum *
			xAxisInterval + FVector::YAxisVector * yAxisGridNum * yAxisInterval, AxisColor, false, duration, 0,
			AxisLineThickness * AxisTransform.GetScale3D().Y);
		for (int i = 0; i <= yAxisGridNum; i++)
		{
			DrawLine(
				world,
				i * FVector::YAxisVector * yAxisInterval + FVector::XAxisVector * xAxisGridNum *
				xAxisInterval + FVector::ZAxisVector * zAxisGridNum * zAxisInterval,
				i * FVector::YAxisVector * yAxisInterval + FVector::ZAxisVector * zAxisGridNum *
				zAxisInterval + FVector::XAxisVector * (xAxisGridNum * xAxisInterval + yAxisLineLengthParam * yAxisInterval), AxisColor,
				false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Y);
		}
	}
	if (judgeVector[0] < 0 && judgeVector[2] > 0)
	{
		DrawLine(world, FVector(0, 0, 0), FVector::YAxisVector * yAxisGridNum * yAxisInterval, AxisColor, false,
			duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Y);
		for (int i = 0; i <= yAxisGridNum; i++)
		{
			DrawLine(world, i * FVector::YAxisVector * yAxisInterval,
				i * FVector::YAxisVector * yAxisInterval - FVector::XAxisVector * yAxisLineLengthParam * yAxisInterval,
				AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Y);
		}
	}
	if (judgeVector[0] < 0 && judgeVector[2] < 0)
	{
		DrawLine(world, FVector::ZAxisVector * zAxisGridNum * zAxisInterval,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + FVector::ZAxisVector * zAxisGridNum *
			zAxisInterval, AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Y);
		for (int i = 0; i <= yAxisGridNum; i++)
		{
			DrawLine(
				world,
				i * FVector::YAxisVector * yAxisInterval + FVector::ZAxisVector * zAxisGridNum *
				zAxisInterval,
				i * FVector::YAxisVector * yAxisInterval - FVector::XAxisVector * yAxisLineLengthParam * yAxisInterval +
				FVector::ZAxisVector * zAxisGridNum * zAxisInterval, AxisColor, false, duration, 0,
				AxisLineThickness * AxisTransform.GetScale3D().Y);
		}
	}
	if (judgeVector[0] > 0 && judgeVector[1] > 0)
	{
		DrawLine(world, FVector::YAxisVector * yAxisGridNum * yAxisInterval,
			FVector::YAxisVector * yAxisGridNum * yAxisInterval + FVector::ZAxisVector * zAxisGridNum *
			zAxisInterval, AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Z);
		for (int i = 0; i <= zAxisGridNum; i++)
		{
			DrawLine(
				world,
				i * FVector::ZAxisVector * zAxisInterval + FVector::YAxisVector * yAxisGridNum *
				yAxisInterval,
				i * FVector::ZAxisVector * zAxisInterval + FVector::YAxisVector * (yAxisGridNum *
					yAxisInterval + zAxisLineLengthParam * zAxisInterval), AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Z);
		}
	}
	if (judgeVector[0] > 0 && judgeVector[1] < 0)
	{
		DrawLine(world, FVector(0, 0, 0), FVector::ZAxisVector * zAxisGridNum * zAxisInterval, AxisColor, false,
			duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Z);
		for (int i = 0; i <= zAxisGridNum; i++)
		{
			DrawLine(world, i * FVector::ZAxisVector * zAxisInterval,
				i * FVector::ZAxisVector * zAxisInterval - FVector::YAxisVector * zAxisLineLengthParam * zAxisInterval,
				AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Z);
		}
	}
	if (judgeVector[0] < 0 && judgeVector[1] > 0)
	{
		DrawLine(
			world,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + FVector::YAxisVector * yAxisGridNum *
			yAxisInterval,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + FVector::YAxisVector * yAxisGridNum *
			yAxisInterval + FVector::ZAxisVector * zAxisGridNum * zAxisInterval, AxisColor, false, duration, 0,
			AxisLineThickness * AxisTransform.GetScale3D().Z);
		for (int i = 0; i <= zAxisGridNum; i++)
		{
			DrawLine(
				world,
				i * FVector::ZAxisVector * zAxisInterval + FVector::YAxisVector * yAxisGridNum *
				yAxisInterval + FVector::XAxisVector * xAxisGridNum * xAxisInterval,
				i * FVector::ZAxisVector * zAxisInterval + FVector::YAxisVector * (yAxisGridNum *
					yAxisInterval + zAxisLineLengthParam * zAxisInterval) + FVector::XAxisVector * xAxisGridNum * xAxisInterval,
				AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Z);
		}
	}
	if (judgeVector[0] < 0 && judgeVector[1] < 0)
	{
		DrawLine(world, FVector::XAxisVector * xAxisGridNum * xAxisInterval,
			FVector::XAxisVector * xAxisGridNum * xAxisInterval + FVector::ZAxisVector * zAxisGridNum *
			zAxisInterval, AxisColor, false, duration, 0, AxisLineThickness * AxisTransform.GetScale3D().Z);
		for (int i = 0; i <= zAxisGridNum; i++)
		{
			DrawLine(
				world,
				i * FVector::ZAxisVector * zAxisInterval + FVector::XAxisVector * xAxisGridNum *
				xAxisInterval,
				i * FVector::ZAxisVector * zAxisInterval - FVector::YAxisVector * zAxisLineLengthParam * zAxisInterval +
				FVector::XAxisVector * xAxisGridNum * xAxisInterval, AxisColor, false, duration, 0,
				AxisLineThickness * AxisTransform.GetScale3D().Z);
		}

	}

}

void AXVChartAxis::UpdateText()
{
	//更新文字绘制状态
	if (XAxisTexts.Num() != 0)TextRenderStateX = ETextRenderState::RenderText;
	else if (XScaleTexts.Num() != 0)TextRenderStateX = ETextRenderState::RenderScaleText;
	else TextRenderStateX = ETextRenderState::RenderNone;

	if (YAxisTexts.Num() != 0)TextRenderStateY = ETextRenderState::RenderText;
	else if (YScaleTexts.Num() != 0)TextRenderStateY = ETextRenderState::RenderScaleText;
	else TextRenderStateY = ETextRenderState::RenderNone;

	if (ZAxisTexts.Num() != 0)TextRenderStateZ = ETextRenderState::RenderText;
	else if (ZScaleTexts.Num() != 0)TextRenderStateZ = ETextRenderState::RenderScaleText;
	else TextRenderStateZ = ETextRenderState::RenderNone;

	if (TextRenderStateX == ETextRenderState::RenderText)
	{
		UpdateXAxisText();
	}
	else if (TextRenderStateX == ETextRenderState::RenderScaleText)
	{
		UpdateXAxisScaleText();
	}
	else
	{
		for (int i = 0; i < XAxisTextComponents.Num(); i++)
		{
			XAxisTextComponents[i]->DestroyComponent();
		}
		XAxisTextComponents.Empty();

	}

	if (TextRenderStateY == ETextRenderState::RenderText)
	{
		UpdateYAxisText();
	}
	else if (TextRenderStateY == ETextRenderState::RenderScaleText)
	{
		UpdateYAxisScaleText();
	}
	else
	{
		for (int i = 0; i < YAxisTextComponents.Num(); i++)
		{
			YAxisTextComponents[i]->DestroyComponent();
		}
		YAxisTextComponents.Empty();
	}

	if (TextRenderStateZ == ETextRenderState::RenderText)
	{
		UpdateZAxisText();
	}
	else if (TextRenderStateZ == ETextRenderState::RenderScaleText)
	{
		UpdateZAxisScaleText();
	}
	else
	{
		for (int i = 0; i < ZAxisTextComponents.Num(); i++)
		{
			ZAxisTextComponents[i]->DestroyComponent();
		}
		ZAxisTextComponents.Empty();
	}
	UpdateTextComponentsTransform();
}

void AXVChartAxis::UpdateXAxisText()
{
	for (int i = 0; i < XAxisTexts.Num(); i++)
	{
		if (i >= XAxisTextComponents.Num())
		{
			XAxisTextComponents.Add(CreateTextComponent(XAxisTexts[i], xAxisTextColor, xTextSize , AxisTransform.GetScale3D().X));
		}
		else
		{
			if (XAxisTextComponents[i])
			{
				UpdateTextComponent(XAxisTextComponents[i], XAxisTexts[i], xAxisTextColor, xTextSize , AxisTransform.GetScale3D().X);
			}
		}
	}
	for (int i = XAxisTextComponents.Num() - 1; i >= XAxisTexts.Num(); i--) {
		XAxisTextComponents[i]->DestroyComponent();
		XAxisTextComponents.RemoveAt(i);
	}
}

void AXVChartAxis::UpdateYAxisText()
{
	for (int i = 0; i < YAxisTexts.Num(); i++)
	{
		if (i >= YAxisTextComponents.Num())
		{
			YAxisTextComponents.Add(CreateTextComponent(YAxisTexts[i], yAxisTextColor, yTextSize , AxisTransform.GetScale3D().Y));
		}
		else
		{
			if (YAxisTextComponents[i])
			{
				UpdateTextComponent(YAxisTextComponents[i], YAxisTexts[i], yAxisTextColor, yTextSize , AxisTransform.GetScale3D().Y);
			}
		}
	}
	for (int i = YAxisTextComponents.Num() - 1; i >= YAxisTexts.Num(); i--) {
		YAxisTextComponents[i]->DestroyComponent();
		YAxisTextComponents.RemoveAt(i);
	}

}

void AXVChartAxis::UpdateZAxisText()
{
	for (int i = 0; i < ZAxisTexts.Num(); i++)
	{
		if (i >= ZAxisTextComponents.Num())
		{
			ZAxisTextComponents.Add(CreateTextComponent(ZAxisTexts[i], zAxisTextColor, zTextSize , AxisTransform.GetScale3D().Z));
		}
		else
		{
			if (ZAxisTextComponents[i])
			{
				UpdateTextComponent(ZAxisTextComponents[i], ZAxisTexts[i], zAxisTextColor, zTextSize , AxisTransform.GetScale3D().Z);
			}
		}
	}
	for (int i = ZAxisTextComponents.Num() - 1; i >= ZAxisTexts.Num(); i--) {
		ZAxisTextComponents[i]->DestroyComponent();
		ZAxisTextComponents.RemoveAt(i);
	}
}

void AXVChartAxis::UpdateXAxisScaleText()
{
	for (int i = 0; i < XScaleTexts.Num(); i++)
	{
		if (i >= XAxisTextComponents.Num())
		{
			XAxisTextComponents.Add(CreateTextComponent(XScaleTexts[i], xAxisTextColor, xTextSize , AxisTransform.GetScale3D().X));
		}
		else
		{
			if (XAxisTextComponents[i])
			{
				UpdateTextComponent(XAxisTextComponents[i], XScaleTexts[i], xAxisTextColor, xTextSize , AxisTransform.GetScale3D().X);
			}
		}
	}
	for (int i = XAxisTextComponents.Num() - 1; i >= XScaleTexts.Num(); i--) {
		XAxisTextComponents[i]->DestroyComponent();
		XAxisTextComponents.RemoveAt(i);
	}
}

void AXVChartAxis::UpdateYAxisScaleText()
{
	for (int i = 0; i < YScaleTexts.Num(); i++)
	{
		if (i >= YAxisTextComponents.Num())
		{
			YAxisTextComponents.Add(CreateTextComponent(YScaleTexts[i], yAxisTextColor, yTextSize , AxisTransform.GetScale3D().Y));
		}
		else
		{
			if (YAxisTextComponents[i])
			{
				UpdateTextComponent(YAxisTextComponents[i], YScaleTexts[i], yAxisTextColor, yTextSize , AxisTransform.GetScale3D().Y);
			}
		}
	}
	for (int i = YAxisTextComponents.Num() - 1; i >= YScaleTexts.Num(); i--) {
		YAxisTextComponents[i]->DestroyComponent();
		YAxisTextComponents.RemoveAt(i);
	}
}

void AXVChartAxis::UpdateZAxisScaleText()
{
	for (int i = 0; i < ZScaleTexts.Num(); i++)
	{
		if (i >= ZAxisTextComponents.Num())
		{
			ZAxisTextComponents.Add(CreateTextComponent(ZScaleTexts[i], zAxisTextColor, zTextSize , AxisTransform.GetScale3D().Z));
		}
		else
		{
			if (ZAxisTextComponents[i])
			{
				UpdateTextComponent(ZAxisTextComponents[i], ZScaleTexts[i], zAxisTextColor, zTextSize , AxisTransform.GetScale3D().Z);
			}
		}
	}
	for (int i = ZAxisTextComponents.Num() - 1; i >= ZScaleTexts.Num(); i--) {
		ZAxisTextComponents[i]->DestroyComponent();
		ZAxisTextComponents.RemoveAt(i);
	}
}

void AXVChartAxis::GetXAxisScaleText()
{
	float xScale = (AxisMaxX - AxisMinX) / xAxisGridNum;
	int p = 0;
	for (; p < 25; p++)
	{
		if (MagicArray[p] < xScale && xScale <= MagicArray[p + 1])
		{
			break;
		}
	}
	int k = 1;
	if (p != 0)
	{
		while (MagicArray[p] + MagicArray[p - 1] * k < xScale)
		{
			k++;
		}
		xScale = MagicArray[p] + MagicArray[p - 1] * k;
	}
	else xScale = MagicArray[p];
	float minScale = ((floor)(AxisMinX / xScale)) * xScale;
	while (minScale + xScale * xAxisGridNum < AxisMaxX && abs(AxisMaxX - (minScale + xScale * xAxisGridNum)) >= 1e-6)minScale +=
		MagicArray[p - 1];
	for (int i = 0; i <= xAxisGridNum; i++, minScale += xScale)
	{
		int np = p;
		while (np - 7 < 0)
		{
			np++;
			minScale *= 10;
		}
		minScale = round(minScale);
		while (np != p)
		{
			np--;
			minScale /= 10;
		}
		XScaleTexts.Add(FString::FromInt((int)minScale));
	}
}

void AXVChartAxis::GetYAxisScaleText()
{
	float yScale = (AxisMaxY - AxisMinY) / yAxisGridNum;
	int p = 0;
	for (; p < 25; p++)
	{
		if (MagicArray[p] < yScale && yScale <= MagicArray[p + 1])
		{
			break;
		}
	}
	int k = 1;
	if (p != 0)
	{
		while (MagicArray[p] + MagicArray[p - 1] * k < yScale)
		{
			k++;
		}
		yScale = MagicArray[p] + MagicArray[p - 1] * k;
	}
	else yScale = MagicArray[p];
	float minScale = ((floor)(AxisMinY / yScale)) * yScale;
	while (minScale + yScale * yAxisGridNum < AxisMaxY && abs(AxisMaxY - (minScale + yScale * yAxisGridNum)) >= 1e-6)minScale +=
		MagicArray[p - 1];
	for (int i = 0; i <= yAxisGridNum; i++, minScale += yScale)
	{
		int np = p;
		while (np - 7 < 0)
		{
			np++;
			minScale *= 10;
		}
		minScale = round(minScale);
		while (np != p)
		{
			np--;
			minScale /= 10;
		}
		YScaleTexts.Add(FString::FromInt((int)minScale));
	}
}

void AXVChartAxis::GetZAxisScaleText()
{
	float zScale = (AxisMaxZ - AxisMinZ) / zAxisGridNum;
	int p = 0;
	for (; p < 25; p++)
	{
		if (MagicArray[p] < zScale && zScale <= MagicArray[p + 1])
		{
			break;
		}
	}
	int k = 1;
	if (p != 0)
	{
		while (MagicArray[p] + MagicArray[p - 1] * k < zScale)
		{
			k++;
		}
		zScale = MagicArray[p] + MagicArray[p - 1] * k;
	}
	else zScale = MagicArray[p];
	float minScale = ((floor)(AxisMinZ / zScale)) * zScale;
	while (minScale + zScale * zAxisGridNum < AxisMaxZ && abs(AxisMaxZ - (minScale + zScale * zAxisGridNum)) >= 1e-6)minScale +=
		MagicArray[p - 1];
	for (int i = 0; i <= zAxisGridNum; i++, minScale += zScale)
	{
		int np = p;
		while (np - 7 < 0)
		{
			np++;
			minScale *= 10;
		}
		minScale = round(minScale);
		while (np != p)
		{
			np--;
			minScale /= 10;
		}
		ZScaleTexts.Add(FString::FromInt((int)minScale));
	}
}

UTextRenderComponent* AXVChartAxis::CreateTextComponent(const FString& Text, const FColor& Color, const float& Size, const float& Scale)
{
	UTextRenderComponent* utext = NewObject<UTextRenderComponent>(this, UTextRenderComponent::StaticClass());
	FText text = FText::FromString(Text);
	utext->RegisterComponent();
	utext->SetText(text);
	utext->SetWorldSize(Size);
	utext->SetWorldScale3D(FVector(Scale));
	utext->SetTextRenderColor(Color);
	utext->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	utext->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	utext->SetTextMaterial(FontMaterial);
	utext->SetFont(Font);
	return utext;
}

void AXVChartAxis::UpdateTextComponent(UTextRenderComponent* TextComponent, const FString& Text, const FColor& Color, const float& Size, const float& Scale)
{
	TextComponent->SetText(FText::FromString(Text));
	TextComponent->SetTextRenderColor(Color);
	TextComponent->SetWorldSize(Size);
	TextComponent->SetWorldScale3D(FVector(Scale));
}

void AXVChartAxis::UpdateTextComponentsTransform()
{
	FVector judgeVector = GetJudgeVector();
	for (int i = 0; i < XAxisTextComponents.Num(); i++)
	{
		if (XAxisTextComponents[i])
		{
			FVector location;
			if (judgeVector[1] > 0 && judgeVector[2] > 0)
			{
				location = i * FVector::XAxisVector * xAxisInterval + FVector::YAxisVector * (yAxisGridNum *
					yAxisInterval + xAxisInterval + DistanceOfTextAndXAxis) - FVector::ZAxisVector * xTextSize / 2;
			}
			if (judgeVector[1] > 0 && judgeVector[2] < 0)
			{
				location = FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::XAxisVector *
					xAxisInterval + FVector::YAxisVector * (yAxisGridNum * yAxisInterval + xAxisInterval +
						DistanceOfTextAndXAxis) - FVector::ZAxisVector * xTextSize / 2;
			}
			if (judgeVector[1] < 0 && judgeVector[2] > 0)
			{
				location = i * FVector::XAxisVector * xAxisInterval - FVector::YAxisVector * (xAxisInterval +
					DistanceOfTextAndXAxis) - FVector::ZAxisVector * xTextSize / 2;
			}
			if (judgeVector[1] < 0 && judgeVector[2] < 0)
			{
				location = FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::XAxisVector *
					xAxisInterval - FVector::YAxisVector * (xAxisInterval + DistanceOfTextAndXAxis) -
					FVector::ZAxisVector * xTextSize / 2;
			}
			FVector vector = AxisTransform.TransformPosition(location);
			XAxisTextComponents[i]->SetWorldLocation(AxisTransform.TransformPosition(location));
			UpdateTextComponentRotation(XAxisTextComponents[i], 0);
		}
	}
	for (int i = 0; i < YAxisTextComponents.Num(); i++)
	{
		if (YAxisTextComponents[i])
		{
			FVector location;
			if (judgeVector[0] > 0 && judgeVector[2] > 0) {
				location = i * FVector::YAxisVector * yAxisInterval + FVector::XAxisVector * (xAxisGridNum *
					xAxisInterval + yAxisInterval + DistanceOfTextAndYAxis) - FVector::ZAxisVector * yTextSize / 2;
			}
			if (judgeVector[0] > 0 && judgeVector[2] < 0)
			{
				location = FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::YAxisVector *
					yAxisInterval + FVector::XAxisVector * (xAxisGridNum * xAxisInterval + yAxisInterval +
						DistanceOfTextAndYAxis) - FVector::ZAxisVector * yTextSize / 2;
			}
			if (judgeVector[0] < 0 && judgeVector[2] > 0)
			{
				location = i * FVector::YAxisVector * yAxisInterval - FVector::XAxisVector * (yAxisInterval +
					DistanceOfTextAndYAxis) - FVector::ZAxisVector * yTextSize / 2;
			}
			if (judgeVector[0] < 0 && judgeVector[2] < 0)
			{
				location = FVector::ZAxisVector * zAxisGridNum * zAxisInterval + i * FVector::YAxisVector *
					yAxisInterval - FVector::XAxisVector * (yAxisInterval + DistanceOfTextAndYAxis) -
					FVector::ZAxisVector * yTextSize / 2;
			}
			YAxisTextComponents[i]->SetWorldLocation(AxisTransform.TransformPosition(location));
			UpdateTextComponentRotation(YAxisTextComponents[i], 1);
		}
	}
	for (int i = 0; i < ZAxisTextComponents.Num(); i++)
	{
		if (ZAxisTextComponents[i])
		{
			FVector location;
			if (judgeVector[0] > 0 && judgeVector[1] > 0) {
				location = i * FVector::ZAxisVector * zAxisInterval + FVector::YAxisVector * (yAxisGridNum *
					yAxisInterval + zAxisInterval + DistanceOfTextAndZAxis) - FVector::XAxisVector * zTextSize / 2;
			}
			if (judgeVector[0] > 0 && judgeVector[1] < 0)
			{
				location = i * FVector::ZAxisVector * zAxisInterval - FVector::YAxisVector * (zAxisInterval +
					DistanceOfTextAndZAxis) - FVector::XAxisVector * zTextSize / 2;
			}
			if (judgeVector[0] < 0 && judgeVector[1] > 0)
			{
				location = FVector::XAxisVector * xAxisGridNum * xAxisInterval + i * FVector::ZAxisVector *
					zAxisInterval + FVector::YAxisVector * (yAxisGridNum * yAxisInterval + zAxisInterval +
						DistanceOfTextAndZAxis) - FVector::XAxisVector * zTextSize / 2;
			}
			if (judgeVector[0] < 0 && judgeVector[1] < 0)
			{
				location = FVector::XAxisVector * xAxisGridNum * xAxisInterval + i * FVector::ZAxisVector *
					zAxisInterval - FVector::YAxisVector * (zAxisInterval + DistanceOfTextAndZAxis) -
					FVector::XAxisVector * zTextSize / 2;
			}
			ZAxisTextComponents[i]->SetWorldLocation(AxisTransform.TransformPosition(location));
			UpdateTextComponentRotation(ZAxisTextComponents[i], 2);
		}
	}
}

void AXVChartAxis::UpdateTextComponentRotation(UTextRenderComponent* TextComponent, const int& type)
{
	FVector cameraLocation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	FVector judgeVector = GetJudgeVector();
	FVector relativeLocation;
	FVector componentLocation;
	if (type == 0)
	{
		relativeLocation = FVector::XAxisVector * xAxisGridNum * xAxisInterval / 2;
		if(judgeVector[1]>0)
		{
			relativeLocation += FVector::YAxisVector * DistanceOfTextAndXAxis;
		}
		else
		{
			relativeLocation -= FVector::YAxisVector * DistanceOfTextAndXAxis;
		}
		componentLocation = XAxisTextComponents[0]->GetComponentLocation();
	}
	else if (type == 1)
	{
		relativeLocation = FVector::YAxisVector * yAxisGridNum * yAxisInterval / 2;
		if(judgeVector[0] > 0)
		{
			relativeLocation += FVector::XAxisVector * DistanceOfTextAndYAxis;
		}
		else
		{
			relativeLocation -= FVector::XAxisVector * DistanceOfTextAndYAxis;
		}
		componentLocation = YAxisTextComponents[0]->GetComponentLocation();
	}
	else
	{
		relativeLocation = FVector::ZAxisVector * zAxisGridNum * zAxisInterval / 2;
		if(judgeVector[1] > 0)
		{
			relativeLocation += FVector::YAxisVector * DistanceOfTextAndZAxis;
		}
		else
		{
			relativeLocation -= FVector::YAxisVector * DistanceOfTextAndZAxis;
		}
		componentLocation = ZAxisTextComponents[0]->GetComponentLocation();
	}
	//TransformVector不会受Location影响
	FVector disLocation = cameraLocation - componentLocation - AxisTransform.TransformVector(relativeLocation);
	TextComponent->SetWorldRotation(FRotationMatrix::MakeFromX(disLocation).Rotator());
}

FVector AXVChartAxis::GetJudgeVector()
{
	FVector cameraLocation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	FVector relativeLocation = FVector::XAxisVector * xAxisGridNum * xAxisInterval / 2
		+ FVector::YAxisVector * yAxisInterval * yAxisGridNum / 2 + FVector::ZAxisVector * zAxisInterval * zAxisGridNum/ 2;
	//转为局部坐标判断
	FVector judgeVector;
	if (bAutoSwitch)
	{
		judgeVector = AxisTransform.InverseTransformPosition(cameraLocation) - relativeLocation;
	}
	return judgeVector;
}

void AXVChartAxis::UpdateAxisTextVisibility(bool bIsHidden)
{
	for (int idx = 0; idx < XAxisTextComponents.Num(); idx++)
	{
		XAxisTextComponents[idx]->SetHiddenInGame(bIsHidden);
	}
	for (int idx = 0; idx < YAxisTextComponents.Num(); idx++)
	{
		YAxisTextComponents[idx]->SetHiddenInGame(bIsHidden);
	}
	for (int idx = 0; idx < ZAxisTextComponents.Num(); idx++)
	{
		ZAxisTextComponents[idx]->SetHiddenInGame(bIsHidden);
	}
	
}

void AXVChartAxis::SetXAxisText(const TArray<FString>& xText)
{
	XAxisTexts = xText;
}

void AXVChartAxis::SetYAxisText(const TArray<FString>& yText)
{
	YAxisTexts = yText;
}

void AXVChartAxis::SetZAxisText(const TArray<FString>& zText)
{
	ZAxisTexts = zText;
}

void AXVChartAxis::SetAxisText(const TArray<FString>& xText, const TArray<FString>& yText, const TArray<FString>& zText)
{
	XAxisTexts = xText;
	YAxisTexts = yText;
	ZAxisTexts = zText;
}

void AXVChartAxis::SetXAxisScaleText(const float& xMin, const float& xMax)
{
	AxisMinX = xMin;
	AxisMaxX = xMax;
	GetXAxisScaleText();
}

void AXVChartAxis::SetYAxisScaleText(const float& yMin, const float& yMax)
{
	AxisMinY = yMin;
	AxisMaxY = yMax;
	GetYAxisScaleText();
}

void AXVChartAxis::SetZAxisScaleText(const float& zMin, const float& zMax)
{
	AxisMinZ = zMin;
	AxisMaxZ = zMax;
	GetZAxisScaleText();
}

void AXVChartAxis::SetAxisScaleText(const float& xMin, const float& xMax, const float& yMin, const float& yMax, const float& zMin, const float& zMax)
{
	AxisMinX = xMin;
	AxisMaxX = xMax;
	AxisMinY = yMin;
	AxisMaxY = yMax;
	AxisMinZ = zMin;
	AxisMaxZ = zMax;
	GetXAxisScaleText();
	GetYAxisScaleText();
	GetZAxisScaleText();
}
