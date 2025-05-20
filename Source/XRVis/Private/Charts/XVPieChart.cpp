// Fill out your copyright notice in the Description page of Project Settings.
#include "Charts/XVPieChart.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/TextRenderComponent.h"
#include "ProceduralMeshComponent.h"


// Sets default values
AXVPieChart::AXVPieChart(): TimeSinceLastUpdate(0), GradientFactor(0), AngleConvertFactor(0), FinalNightingaleOffset(0),
                            FinalInternalDiameter(0),
                            FinalSectionGapAngle(0)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	XVChartUtils::LoadResourceFromPath(
		TEXT("/Script/Engine.Material'/XRVis/Materials/M_BaseVertexColor.M_BaseVertexColor'"), Material);

	// 初始化标签配置
	LabelConfig.bShowLabels = true;
	LabelConfig.FontSize = 12.0f;
	LabelConfig.FontColor = FColor::White;
	LabelConfig.FontWeight = 1.0f;
	LabelConfig.LabelFormat = TEXT("{category}: {value}");
	LabelConfig.LabelPosition = EPieChartLabelPosition::Outside;
	LabelConfig.LabelOffset = 10.0f;
	LabelConfig.bShowLeaderLine = true;
	LabelConfig.LeaderLineColor = FColor::White;
	LabelConfig.LeaderLineThickness = 1.0f;
	LabelConfig.LeaderLineLength = 0.0f;


	static ConstructorHelpers::FObjectFinder<UFont> FontObject(TEXT("/Engine/EngineFonts/Roboto"));
	if (FontObject.Succeeded())
	{
		UFont* Font = FontObject.Object;
		LabelConfig.Font = Font;
	}


	ZoomOffset = 20.f;
}

void AXVPieChart::Create3DPieChart(const TMap<FString, float>& Data, EPieChartStyle ChartStyle,
                                   const TArray<FColor>& PieChartColor, EPieShape Shape)
{
	TotalCountOfValue = Data.Num();

	if (TotalCountOfValue <= 0 || InternalDiameter > ExternalDiameter)
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVPieChart: 数据为空或内径大于外径，无法创建饼图"));
		return;
	}

	// 清理旧的标签
	for (UTextRenderComponent* Label : SectionLabels)
	{
		if (Label)
		{
			Label->DestroyComponent();
		}
	}
	SectionLabels.Empty();

	AccumulatedValues.SetNum(TotalCountOfValue + 1);
	AccumulatedValues[0] = 0;
	SectionSelectStates.Init(false, TotalCountOfValue);

	// 保存类别和数值，用于标签显示
	SectionCategories.Empty(TotalCountOfValue);
	SectionValues.Empty(TotalCountOfValue);

	// 日志输出数据信息
	UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 创建饼图，数据项数量: %d"), TotalCountOfValue);

	double TotalValue = 0.0;
	size_t CurrentIndex = 0;

	// 使用一个有序数组来存储数据，以确保顺序一致
	TArray<TPair<FString, float>> OrderedData;
	for (const auto& Elem : Data)
	{
		OrderedData.Add(TPair<FString, float>(Elem.Key, Elem.Value));
	}

	// 按键排序(可选)
	// OrderedData.Sort([](const TPair<FString, float>& A, const TPair<FString, float>& B) { return A.Key < B.Key; });

	for (const auto& Pair : OrderedData)
	{
		TotalValue += Pair.Value;
		AccumulatedValues[CurrentIndex] = TotalValue;

		// 保存类别和值
		SectionCategories.Add(Pair.Key);
		SectionValues.Add(Pair.Value);

		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 数据项 %d, 类别: %s, 值: %f"),
		       CurrentIndex, *Pair.Key, Pair.Value);

		++CurrentIndex;
	}

	AccumulatedValues[TotalCountOfValue] = TotalValue;
	AngleConvertFactor = 360.0 / TotalValue;

	DynamicMaterialInstances.Empty();
	DynamicMaterialInstances.SetNum(GenerateLODCount * TotalCountOfValue + 1);

	// 设置图表样式和颜色
	Set3DPieChart(ChartStyle, PieChartColor, Shape);

	// 完成后构建网格
	ConstructMesh();

	// 更新标签
	if (LabelConfig.bShowLabels && TotalCountOfValue > 0)
	{
		UpdateLabels();
	}
}

void AXVPieChart::Set3DPieChart(EPieChartStyle ChartStyle, const TArray<FColor>& PieChartColor, EPieShape Shape)
{
	// Only for editor panel
	PieChartStyle = ChartStyle;
	PieShape = Shape;
	SectionColors = PieChartColor;

	while (SectionColors.Num() < TotalCountOfValue)
	{
		SectionColors.Push(FColor::Black);
	}

	ProcessTypeAndShapeInfo();
	ConstructMesh();
	// 更新标签
	if (LabelConfig.bShowLabels && TotalCountOfValue > 0)
	{
		UpdateLabels();
	}
}

void AXVPieChart::ColorModify(int ModifyIndex, const FColor& Color)
{
	if (ModifyIndex >= TotalCountOfValue)
	{
		UE_LOG(LogTemp, Warning, TEXT("Modify Index out of data range"));
		return;
	}

	SectionColors[ModifyIndex] = Color;
	// TODO: maybe memcpy or something to boost this 
	for (int i = 0; i < SectionInfos[ModifyIndex].VertexColors.Num(); ++i)
	{
		SectionInfos[ModifyIndex].VertexColors[i] = Color;
	}
}

void AXVPieChart::Highlight(int ModifyIndex, float HighlightIntensity)
{
	if (ModifyIndex >= TotalCountOfValue)
	{
		UE_LOG(LogTemp, Warning, TEXT("Modify Index out of data range"));
		return;
	}
	UpdateSection(ModifyIndex, SectionSelectStates[ModifyIndex], FinalInternalDiameter, ExternalDiameter,
	              HighlightIntensity);
}

int AXVPieChart::GetSectionIndex()
{
	const FHitResult HitResult = XVChartUtils::GetCursorHitResult(GetWorld());
	float AngleDegrees = GetCursorHitAngle(HitResult);
	float StartAngle = 0;

	for (size_t i = 0; i < AccumulatedValues.Num() - 1; i++)
	{
		float EndAngle = AccumulatedValues[i] * AngleConvertFactor - FinalSectionGapAngle;

		//判断是否在当前块的角度范围内
		if (AngleDegrees >= StartAngle && AngleDegrees <= EndAngle)
		{
			//绘制新选中的块
			return i;
		}
		StartAngle = EndAngle + FinalSectionGapAngle;
	}
	return -1;
}

void AXVPieChart::ConstructMesh(double Rate)
{
	Super::ConstructMesh(Rate);
	if (AccumulatedValues.IsEmpty())
	{
		return;
	}

	PrepareMeshSections();

	GenerateLOD();

	// 网格构建完成后创建标签
	// 先清理旧的标签
	for (UTextRenderComponent* Label : SectionLabels)
	{
		if (Label)
		{
			Label->DestroyComponent();
		}
	}
	SectionLabels.Empty();

	// 清除现有引线
	for (UProceduralMeshComponent* LineMesh : LeaderLineMeshes)
	{
		if (LineMesh)
		{
			LineMesh->DestroyComponent();
		}
	}
	LeaderLineMeshes.Empty();

	// 如果启用了标签，则创建新标签
	if (LabelConfig.bShowLabels && TotalCountOfValue > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 网格构建完成，开始创建标签"));
		// 给一个小延迟，确保网格已完全构建
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			CreateSectionLabels();
		});
	}
}

void AXVPieChart::GenerateLOD()
{
	Super::GenerateLOD();

	check(GenerateLODCount);
	LODInfos.SetNum(GenerateLODCount);
	size_t DataSize = AccumulatedValues.Num() - 1;
	for (int i = 0; i < GenerateLODCount; i++)
	{
		size_t CurrentSectionStartAngle = 0;
		LODInfos[i].LODOffset = i * DataSize;
		for (int CurrentIndex = 0; CurrentIndex < DataSize; ++CurrentIndex)
		{
			uint32_t SectionIndex = LODInfos[i].LODOffset + CurrentIndex;
			size_t CurrentSectionEndAngle = static_cast<size_t>(AccumulatedValues[CurrentIndex] * AngleConvertFactor);
			GeneratePieSectionInfo(CenterPosition, SectionIndex,
			                       CurrentSectionStartAngle, CurrentSectionEndAngle - FinalSectionGapAngle,
			                       FinalInternalDiameter, ExternalDiameter + CurrentIndex * FinalNightingaleOffset,
			                       SectionHeight,
			                       SectionColors[CurrentIndex],
			                       i + 1);

			DynamicMaterialInstances[SectionIndex] = UMaterialInstanceDynamic::Create(Material, this);
			DynamicMaterialInstances[SectionIndex]->SetVectorParameterValue("EmissiveColor", EmissiveColor);
			ProceduralMeshComponent->SetMaterial(SectionIndex, DynamicMaterialInstances[SectionIndex]);

			CurrentSectionStartAngle = CurrentSectionEndAngle;
		}
		LODInfos[i].LODCount = DataSize;
	}
}

void AXVPieChart::ProcessTypeAndShapeInfo()
{
	switch (PieChartStyle)
	{
	case EPieChartStyle::Base:
		GradientFactor = 0.f;
		break;
	case EPieChartStyle::Gradient:
		GradientFactor = 1.f;
		break;
	default:
		GradientFactor = 0.f;
		break;
	}

	switch (PieShape)
	{
	case EPieShape::Round:
		bIsShapeNightingale = false;
		bIsSectionGaped = false;
		bHasInternalDiameter = false;
		break;
	case EPieShape::Circular:
		bIsShapeNightingale = false;
		bIsSectionGaped = false;
		bHasInternalDiameter = true;
		break;
	case EPieShape::Nightingale:
		bIsShapeNightingale = true;
		bIsSectionGaped = false;
		bHasInternalDiameter = true;
		break;
	case EPieShape::SectorGap:
		bIsShapeNightingale = false;
		bIsSectionGaped = true;
		bHasInternalDiameter = true;
		break;
	default:
		break;
	}

	FinalNightingaleOffset = bIsShapeNightingale ? NightingaleOffset : 0;
	FinalInternalDiameter = bHasInternalDiameter ? InternalDiameter : 0;
	FinalSectionGapAngle = bIsSectionGaped ? SectionGapAngle : 0;
}

// Called when the game starts or when spawned
void AXVPieChart::BeginPlay()
{
	Super::BeginPlay();
	TimeSinceLastUpdate = 0.f;

	// 确保标签和引导线可见
	if (LabelConfig.bShowLabels && TotalCountOfValue > 0)
	{
		// 在下一帧更新标签，确保所有组件都已初始化
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			UpdateLabels();
		});
	}
}

void AXVPieChart::UpdateOnMouseEnterOrLeft()
{
	if (bIsMouseEntered)
	{
		const FHitResult HitResult = GetCursorHitResult();
		if (HitResult.GetActor())
		{
			FVector UpwardVector = HitResult.GetActor()->GetActorForwardVector();
			FVector ForwardVector = HitResult.GetActor()->GetActorUpVector();
			FVector CurPosition = this->GetActorLocation();

			float Distance = FVector::DotProduct(ForwardVector, HitResult.Location - CurPosition);
			FVector ProjectionP = HitResult.Location - ForwardVector * Distance;
			FVector Origin2Projection = ProjectionP - GetActorLocation();
			Origin2Projection.Normalize();

			float DotProduct = FVector::DotProduct(UpwardVector, Origin2Projection);
			float AngleCos = FMath::Clamp(DotProduct, -1.0f, 1.0f); // 限制范围以避免可能的计算误差
			float AngleRadians = acos(AngleCos); // 计算弧度
			float AngleDegrees = FMath::RadiansToDegrees(AngleRadians); // 将弧度转换为度

			FVector CrossProduct = FVector::CrossProduct(UpwardVector, Origin2Projection);
			if (FVector::DotProduct(CrossProduct, ForwardVector) < 0)
			{
				AngleDegrees = 360 - AngleDegrees; // 如果需要的话，调整角度值
			}

			float StartAngle = 0;

			for (size_t i = 0; i < AccumulatedValues.Num() - 1; i++)
			{
				float EndAngle = AccumulatedValues[i] * AngleConvertFactor - FinalSectionGapAngle;

				//判断是否在当前块的角度范围内
				if (AngleDegrees >= StartAngle && AngleDegrees <= EndAngle)
				{
					//之前移动到或者已经点选中了
					if (HoveredSectionIndex == i || (HoveredSectionIndex != i && SectionSelectStates[i])) break;
					//清空之前选中
					float NewInternalDiameter = FinalInternalDiameter;
					float NewExternalDiameter = ExternalDiameter;
					if (HoveredSectionIndex != -1 && !SectionSelectStates[HoveredSectionIndex])
					{
						UpdateSection(HoveredSectionIndex, SectionSelectStates[HoveredSectionIndex],
						              NewInternalDiameter,
						              NewExternalDiameter + HoveredSectionIndex * FinalNightingaleOffset, 0);
					}
					NewExternalDiameter += i * FinalNightingaleOffset;
					//绘制新选中的块
					HoveredSectionIndex = i;
					if (bEnableZoomAnimation)
					{
						NewExternalDiameter += ZoomOffset;
					}
					if (bEnablePopAnimation)
					{
						NewInternalDiameter += PopOffset;
						NewExternalDiameter += PopOffset + bEnableZoomAnimation ? ZoomOffset : 0;
					}
					UpdateSection(HoveredSectionIndex, SectionSelectStates[HoveredSectionIndex], NewInternalDiameter,
					              NewExternalDiameter, EmissiveIntensity);
					break;
				}
				StartAngle = EndAngle + FinalSectionGapAngle;
			}
		}
	}
	else if (!bIsMouseEntered && HoveredSectionIndex != -1 && !SectionSelectStates[HoveredSectionIndex])
	{
		UpdateSection(HoveredSectionIndex, SectionSelectStates[HoveredSectionIndex], FinalInternalDiameter,
		              ExternalDiameter + HoveredSectionIndex * FinalNightingaleOffset,
		              0);
		HoveredSectionIndex = -1;
	}
}


void AXVPieChart::UpdateSection(const size_t& UpdateSectionIndex, const bool& InSelected, const float& InIR,
                                const float& InER, const float& InEmissiveIntensity)
{
	uint32_t OffsetedSectionIndex = GetSectionIndexOfLOD(UpdateSectionIndex);
	ClearSelectedSection(UpdateSectionIndex);

	int StartAngle = UpdateSectionIndex == 0 ? 0 : AccumulatedValues[UpdateSectionIndex - 1] * AngleConvertFactor;
	int EndAngle = AccumulatedValues[UpdateSectionIndex] * AngleConvertFactor - FinalSectionGapAngle;

	SectionSelectStates[UpdateSectionIndex] = InSelected;
	GeneratePieSectionInfo(CenterPosition, OffsetedSectionIndex, StartAngle, EndAngle, InIR, InER, SectionHeight,
	                       SectionColors[UpdateSectionIndex], CurrentLOD + 1);
	DynamicMaterialInstances[OffsetedSectionIndex]->SetScalarParameterValue("EmissiveIntensity", InEmissiveIntensity);

	DrawMeshSection(OffsetedSectionIndex);
}

// Called every frame
void AXVPieChart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateLOD();
	{
		TimeSinceLastUpdate += DeltaTime;
		if (TimeSinceLastUpdate < SectionHoverCooldown)
		{
			return;
		}
		TimeSinceLastUpdate = 0.f;
		UpdateOnMouseEnterOrLeft();
	}
}

void AXVPieChart::SetLabelConfig(const FXVPieChartLabelConfig& InLabelConfig)
{
	// 更新标签配置
	LabelConfig = InLabelConfig;

	// 更新标签显示
	UpdateLabels();
}

void AXVPieChart::UpdateLabels()
{
	// 清除现有标签
	for (UTextRenderComponent* Label : SectionLabels)
	{
		if (Label)
		{
			Label->DestroyComponent();
		}
	}
	SectionLabels.Empty();

	// 清除现有引线
	for (UProceduralMeshComponent* LineMesh : LeaderLineMeshes)
	{
		if (LineMesh)
		{
			LineMesh->DestroyComponent();
		}
	}
	LeaderLineMeshes.Empty();

	// 如果启用了标签且有数据，创建新标签
	if (LabelConfig.bShowLabels && TotalCountOfValue > 0)
	{
		// 在下一帧创建标签，确保所有组件都已初始化
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			CreateSectionLabels();
		});
	}
}

void AXVPieChart::CreateSectionLabels()
{
	// 清理旧的标签
	for (UTextRenderComponent* Label : SectionLabels)
	{
		if (Label)
		{
			Label->DestroyComponent();
		}
	}
	SectionLabels.Empty();

	// 如果没有数据或不需要显示标签，直接返回
	if (TotalCountOfValue <= 0 || !LabelConfig.bShowLabels)
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVPieChart: 没有数据或不需要显示标签"));
		return;
	}

	// 确保有可用的字体
	if (LabelConfig.Font == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVPieChart: 标签字体为空，尝试加载默认字体"));
		LabelConfig.Font = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		if (LabelConfig.Font == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("AXVPieChart: 无法加载默认字体，标签创建失败"));
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 开始创建标签，共 %d 个区块"), TotalCountOfValue);

	// 获取相机方向（用于标签面向）
	FVector CameraDirection = FVector(1, 0, 0); // 默认方向

	// 获取主相机方向
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		FRotator CameraRotation;
		PC->GetPlayerViewPoint(/*out*/ CameraDirection, /*out*/ CameraRotation);
		CameraDirection = CameraRotation.Vector();
	}

	// 为每个区块创建标签
	for (int32 i = 0; i < TotalCountOfValue; ++i)
	{
		// 安全检查
		if (i >= SectionCategories.Num() || i >= SectionValues.Num())
		{
			UE_LOG(LogTemp, Error, TEXT("AXVPieChart: 索引超出范围，标签创建失败: %d"), i);
			continue;
		}

		// 计算区块的角度
		float StartAngle = 0;
		if (i > 0)
		{
			StartAngle = AccumulatedValues[i - 1] * AngleConvertFactor + (i > 0 ? FinalSectionGapAngle : 0);
		}
		float EndAngle = AccumulatedValues[i] * AngleConvertFactor - FinalSectionGapAngle;
		float MidAngleDegrees = (StartAngle + EndAngle) * 0.5f;
		float MidAngleRadians = FMath::DegreesToRadians(MidAngleDegrees);

		// 计算饼图半径
		float ActualExternalRadius = ExternalDiameter;
		if (bIsShapeNightingale && i < TotalCountOfValue)
		{
			ActualExternalRadius = ExternalDiameter + i * FinalNightingaleOffset;
		}

		// 确定标签位置和旋转
		FVector LabelPosition;
		FRotator LabelRotation;

		// 根据标签类型计算位置
		if (LabelConfig.LabelPosition == EPieChartLabelPosition::Inside)
		{
			// 内部标签 - 放在区块中间位置
			float LabelRadius = FinalInternalDiameter + (ActualExternalRadius - FinalInternalDiameter) * 0.5f;
			LabelPosition = CenterPosition + FVector(
				LabelRadius * FMath::Cos(MidAngleRadians),
				LabelRadius * FMath::Sin(MidAngleRadians),
				SectionHeight * 0.5f + 1.0f // 增加高度，确保可见
			);

			// 内部标签朝向（始终朝向相机）
			FVector LabelDirection = (LabelPosition - CenterPosition).GetSafeNormal();
			// 计算标签看向相机的旋转
			LabelRotation = FRotationMatrix::MakeFromX(LabelDirection).Rotator();

			// 如果在下半圆，需要翻转文本方向
			if (MidAngleDegrees > 90 && MidAngleDegrees < 270)
			{
				LabelRotation.Yaw += 180.0f;
			}
		}
		else // EPieChartLabelPosition::Outside
		{
			// 外部标签 - 在区块边缘向外偏移
			float LabelOffset = LabelConfig.LabelOffset + 30.0f; // 增加偏移，确保标签足够远
			LabelPosition = CenterPosition + FVector(
				(ActualExternalRadius + LabelOffset) * FMath::Cos(MidAngleRadians),
				(ActualExternalRadius + LabelOffset) * FMath::Sin(MidAngleRadians),
				SectionHeight * 0.5f + 1.0f // 增加高度，确保可见
			);

			// 外部标签朝向 - 使用水平方向
			if (MidAngleDegrees >= 270 || MidAngleDegrees <= 90)
			{
				// 右侧标签
				LabelRotation = FRotator(0, 270.0f, 0);
			}
			else
			{
				// 左侧标签
				LabelRotation = FRotator(0, 90.0f, 0);
			}
		}

		// 创建文本渲染组件
		UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
		Label->SetupAttachment(RootComponent);
		Label->RegisterComponent();

		// 设置标签属性
		Label->SetFont(LabelConfig.Font);
		Label->SetTextRenderColor(LabelConfig.FontColor);
		Label->SetWorldSize(LabelConfig.FontSize);

		// 设置水平对齐方式
		if (LabelConfig.LabelPosition == EPieChartLabelPosition::Inside)
		{
			// 内部标签居中对齐
			Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		}
		else
		{
			// 外部标签：右侧标签左对齐，左侧标签右对齐
			if (MidAngleDegrees >= 270 || MidAngleDegrees <= 90)
			{
				Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Left);
			}
			else
			{
				Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Right);
			}
		}

		// 设置垂直对齐方式
		Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);

		// 格式化标签文本
		FString LabelText = LabelConfig.LabelFormat;
		LabelText = LabelText.Replace(TEXT("{category}"), *SectionCategories[i]);
		LabelText = LabelText.Replace(TEXT("{value}"), *FString::Printf(TEXT("%.2f"), SectionValues[i]));
		Label->SetText(FText::FromString(LabelText));

		// 设置标签位置和旋转
		Label->SetWorldLocation(LabelPosition);
		Label->SetWorldRotation(LabelRotation);

		// 应用高亮材质
		UMaterialInstanceDynamic* TextMaterial = UMaterialInstanceDynamic::Create(Material, this);
		if (TextMaterial)
		{
			TextMaterial->SetVectorParameterValue("EmissiveColor", FLinearColor(LabelConfig.FontColor));
			TextMaterial->SetScalarParameterValue("EmissiveIntensity", 5.0f);
			Label->SetTextMaterial(TextMaterial);
		}

		// 设置标签渲染特性
		Label->SetVisibility(true);
		Label->SetHiddenInGame(false);
		Label->bAlwaysRenderAsText = true; // 确保始终渲染为文本
		Label->bRenderCustomDepth = true; // 开启自定义深度
		Label->SetCastShadow(false); // 不投射阴影
		Label->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 禁用碰撞
		Label->bUseAsOccluder = false; // 不作为遮挡物

		// 记录标签信息
		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 创建标签 %d, 文本: %s, 位置: (%f, %f, %f), 角度: %f"),
		       i, *LabelText, LabelPosition.X, LabelPosition.Y, LabelPosition.Z, MidAngleDegrees);

		// 保存到标签数组
		SectionLabels.Add(Label);
	}

	// 如果需要显示引导线且标签在外部，则创建引导线
	if (LabelConfig.bShowLeaderLine && LabelConfig.LabelPosition == EPieChartLabelPosition::Outside && TotalCountOfValue
		> 0)
	{
		// 在下一帧创建引导线，确保标签已经正确创建
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			CreateLabelLeaderLines();
		});
	}
}

void AXVPieChart::CreateLabelLeaderLines()
{
	// 清理旧的引导线组件
	for (UProceduralMeshComponent* LineMesh : LeaderLineMeshes)
	{
		if (LineMesh)
		{
			LineMesh->DestroyComponent();
		}
	}
	LeaderLineMeshes.Empty();

	// 如果标签不在外部或者没有启用引导线，直接返回
	if (LabelConfig.LabelPosition != EPieChartLabelPosition::Outside || !LabelConfig.bShowLeaderLine)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 开始创建引导线，共 %d 个区块"), TotalCountOfValue);

	// 确保材质可用
	if (!Material)
	{
		// 如果没有找到材质，加载默认材质
		XVChartUtils::LoadResourceFromPath(
			TEXT("/Script/Engine.Material'/XRVis/Materials/M_BaseVertexColor.M_BaseVertexColor'"), Material);
		if (!Material)
		{
			UE_LOG(LogTemp, Error, TEXT("AXVPieChart: 无法加载默认材质，引导线创建失败"));
			return;
		}
	}

	// 为每个标签创建引导线
	for (int32 i = 0; i < TotalCountOfValue; ++i)
	{
		// 安全检查
		if (i >= SectionLabels.Num())
		{
			continue;
		}

		// 计算区块的角度
		float StartAngle = 0;
		if (i > 0)
		{
			StartAngle = AccumulatedValues[i - 1] * AngleConvertFactor + (i > 0 ? FinalSectionGapAngle : 0);
		}
		float EndAngle = AccumulatedValues[i] * AngleConvertFactor - FinalSectionGapAngle;
		float MidAngleDegrees = (StartAngle + EndAngle) * 0.5f;
		float MidAngleRadians = FMath::DegreesToRadians(MidAngleDegrees);

		// 计算饼图半径
		float ActualExternalRadius = ExternalDiameter;
		if (bIsShapeNightingale && i < TotalCountOfValue)
		{
			ActualExternalRadius = ExternalDiameter + i * FinalNightingaleOffset;
		}

		// 计算引导线的起点（饼图边缘）
		FVector LineStart = CenterPosition + FVector(
			ActualExternalRadius * FMath::Cos(MidAngleRadians),
			ActualExternalRadius * FMath::Sin(MidAngleRadians),
			SectionHeight * 0.5f + 1.0f // 与标签同高
		);

		// 获取标签位置作为终点
		FVector LabelPosition = SectionLabels[i]->GetComponentLocation();

		// 计算朝向标签的方向
		FVector LabelDirection = (LabelPosition - LineStart).GetSafeNormal();

		// 引导线终点 - 距离标签一小段距离
		FVector LineEnd = LabelPosition - LabelDirection * 10.0f;

		// 创建两段引导线（一段从饼图边缘出发，一段连接到标签）
		// 计算中间点
		FVector MidPoint = LineStart + LabelDirection * ((LineEnd - LineStart).Size() * 0.5f);

		// 第一段引导线
		UProceduralMeshComponent* LineMesh1 = NewObject<UProceduralMeshComponent>(this);
		LineMesh1->SetupAttachment(RootComponent);
		LineMesh1->RegisterComponent();
		CreateSingleLeaderLineSegment(LineMesh1, LineStart, MidPoint, LabelConfig.LeaderLineColor,
		                              LabelConfig.LeaderLineThickness);

		// 第二段引导线
		UProceduralMeshComponent* LineMesh2 = NewObject<UProceduralMeshComponent>(this);
		LineMesh2->SetupAttachment(RootComponent);
		LineMesh2->RegisterComponent();
		CreateSingleLeaderLineSegment(LineMesh2, MidPoint, LineEnd, LabelConfig.LeaderLineColor,
		                              LabelConfig.LeaderLineThickness);

		// 添加到引导线数组
		LeaderLineMeshes.Add(LineMesh1);
		LeaderLineMeshes.Add(LineMesh2);

		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 创建引导线 %d, 起点: (%f, %f, %f), 终点: (%f, %f, %f)"),
		       i, LineStart.X, LineStart.Y, LineStart.Z, LineEnd.X, LineEnd.Y, LineEnd.Z);
	}
}

// 创建单段引导线
void AXVPieChart::CreateSingleLeaderLineSegment(UProceduralMeshComponent* LineMesh, const FVector& StartPoint,
                                                const FVector& EndPoint, const FColor& Color, float Thickness)
{
	// 计算方向和长度
	FVector Direction = (EndPoint - StartPoint).GetSafeNormal();
	float Length = FVector::Dist(StartPoint, EndPoint);

	// 使用两个交叉的平面创建"十字"形状的线条，确保在任何视角都能看到

	// 平面1：垂直于Z轴
	FVector Right1 = FVector(0, 0, 1).Cross(Direction).GetSafeNormal();
	if (Right1.IsNearlyZero())
	{
		Right1 = FVector(0, 1, 0);
	}
	FVector Up1 = Direction.Cross(Right1).GetSafeNormal();

	// 平面2：垂直于水平面上的一个向量
	FVector Right2 = FVector(0, 1, 0).Cross(Direction).GetSafeNormal();
	if (Right2.IsNearlyZero())
	{
		Right2 = FVector(1, 0, 0);
	}
	FVector Up2 = Direction.Cross(Right2).GetSafeNormal();

	// 创建线条几何体
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	// 线条宽度
	float HalfWidth = Thickness * 0.5f;

	// 创建第一个平面（8个顶点，形成一个交叉的十字形）
	// 平面1：垂直平面
	FVector V1_1 = StartPoint + Right1 * HalfWidth;
	FVector V1_2 = StartPoint - Right1 * HalfWidth;
	FVector V1_3 = EndPoint - Right1 * HalfWidth;
	FVector V1_4 = EndPoint + Right1 * HalfWidth;

	// 平面2：水平平面
	FVector V2_1 = StartPoint + Right2 * HalfWidth;
	FVector V2_2 = StartPoint - Right2 * HalfWidth;
	FVector V2_3 = EndPoint - Right2 * HalfWidth;
	FVector V2_4 = EndPoint + Right2 * HalfWidth;

	// 添加平面1的顶点
	int BaseIndex = Vertices.Num();
	Vertices.Add(V1_1); // 0
	Vertices.Add(V1_2); // 1
	Vertices.Add(V1_3); // 2
	Vertices.Add(V1_4); // 3

	// 添加平面1的两个三角形（正面）
	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 1);
	Triangles.Add(BaseIndex + 2);

	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 2);
	Triangles.Add(BaseIndex + 3);

	// 添加平面1的两个三角形（背面）
	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 2);
	Triangles.Add(BaseIndex + 1);

	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 3);
	Triangles.Add(BaseIndex + 2);

	// 添加平面2的顶点
	BaseIndex = Vertices.Num();
	Vertices.Add(V2_1); // 4
	Vertices.Add(V2_2); // 5
	Vertices.Add(V2_3); // 6
	Vertices.Add(V2_4); // 7

	// 添加平面2的两个三角形（正面）
	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 1);
	Triangles.Add(BaseIndex + 2);

	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 2);
	Triangles.Add(BaseIndex + 3);

	// 添加平面2的两个三角形（背面）
	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 2);
	Triangles.Add(BaseIndex + 1);

	Triangles.Add(BaseIndex + 0);
	Triangles.Add(BaseIndex + 3);
	Triangles.Add(BaseIndex + 2);

	// 设置法线、UV和颜色
	FLinearColor LineColor = FLinearColor(Color);

	// 平面1的法线
	for (int32 i = 0; i < 4; ++i)
	{
		// 我们需要为正面和背面设置不同的法线
		if (i % 2 == 0)
		{
			Normals.Add(Up1); // 正面法线
		}
		else
		{
			Normals.Add(-Up1); // 背面法线
		}
		UVs.Add(FVector2D(0, 0));
		VertexColors.Add(LineColor);
		Tangents.Add(FProcMeshTangent(Right1, false));
	}

	// 平面2的法线
	for (int32 i = 0; i < 4; ++i)
	{
		if (i % 2 == 0)
		{
			Normals.Add(Up2); // 正面法线
		}
		else
		{
			Normals.Add(-Up2); // 背面法线
		}
		UVs.Add(FVector2D(0, 0));
		VertexColors.Add(LineColor);
		Tangents.Add(FProcMeshTangent(Right2, false));
	}

	// 创建线条网格
	LineMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);

	// 创建并应用双面材质
	UMaterialInstanceDynamic* LineMaterial = UMaterialInstanceDynamic::Create(Material, this);
	if (LineMaterial)
	{
		LineMaterial->SetVectorParameterValue("EmissiveColor", LineColor);
		LineMaterial->SetScalarParameterValue("EmissiveIntensity", 20.0f); // 高发光强度

		// 尝试设置材质为双面渲染
		LineMaterial->SetScalarParameterValue("TwoSided", 1.0f); // 如果材质支持此参数

		LineMesh->SetMaterial(0, LineMaterial);
	}

	// 设置线条渲染属性 - 强制启用双面渲染和深度测试
	LineMesh->SetVisibility(true);
	LineMesh->SetHiddenInGame(false);
	LineMesh->SetRenderCustomDepth(true);
	LineMesh->SetCustomDepthStencilValue(1);
	LineMesh->SetCastShadow(false);
	LineMesh->bUseAsOccluder = false; // 不作为遮挡物
	LineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 禁用碰撞

	// 这里我们不使用bDisableDepthTest，因为用户移除了这个设置
	// 但我们确保其他所有可见性相关设置都是最优的
}

void AXVPieChart::CalculateLabelPosition(
	int32 SectionIndex,
	float StartAngle,
	float EndAngle,
	FVector& OutLabelPosition,
	FRotator& OutLabelRotation,
	FVector& OutLeaderLineStart,
	FVector& OutLeaderLineEnd)
{
	// 计算区块中点角度（注意：输入角度是度数，需要转换为弧度）
	float MidAngleDegrees = (StartAngle + EndAngle) * 0.5f;
	float MidAngleRadians = FMath::DegreesToRadians(MidAngleDegrees);

	// 记录角度值
	UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 区块 %d 角度计算 - 开始角度: %f, 结束角度: %f, 中点角度: %f度/%f弧度"),
	       SectionIndex, StartAngle, EndAngle, MidAngleDegrees, MidAngleRadians);

	// 如果这个区块有特殊的半径（例如南丁格尔玫瑰图），计算正确的外径
	float ActualExternalRadius = ExternalDiameter;
	if (bIsShapeNightingale && SectionIndex < TotalCountOfValue)
	{
		ActualExternalRadius = ExternalDiameter + SectionIndex * FinalNightingaleOffset;
	}

	// 设置引导线起点在区块外径处
	OutLeaderLineStart = CenterPosition + FVector(
		ActualExternalRadius * FMath::Cos(MidAngleRadians),
		ActualExternalRadius * FMath::Sin(MidAngleRadians),
		SectionHeight * 0.5f
	);

	// 记录引导线起点
	UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 区块 %d 引导线起点位置 (%f, %f, %f)"),
	       SectionIndex, OutLeaderLineStart.X, OutLeaderLineStart.Y, OutLeaderLineStart.Z);

	// 根据标签位置计算标签位置和引导线终点
	if (LabelConfig.LabelPosition == EPieChartLabelPosition::Inside)
	{
		// 内部标签 - 放在饼图内部
		float LabelRadius = FinalInternalDiameter + (ActualExternalRadius - FinalInternalDiameter) * 0.5f;
		OutLabelPosition = CenterPosition + FVector(
			LabelRadius * FMath::Cos(MidAngleRadians),
			LabelRadius * FMath::Sin(MidAngleRadians),
			SectionHeight * 0.5f
		);

		// 标签朝向 - 始终垂直于饼图半径方向
		FVector ToCenter = (CenterPosition - OutLabelPosition).GetSafeNormal();
		if (!ToCenter.IsNearlyZero())
		{
			// 计算旋转，使标签垂直于半径
			FVector WorldUp = FVector(0, 0, 1);
			FVector RightVector = FVector::CrossProduct(WorldUp, ToCenter).GetSafeNormal();

			// 如果右向量接近零，使用另一个参考方向
			if (RightVector.IsNearlyZero())
			{
				RightVector = FVector(1, 0, 0);
			}

			// 计算向上方向，确保三个向量互相垂直
			FVector UpVector = FVector::CrossProduct(ToCenter, RightVector).GetSafeNormal();

			// 创建旋转矩阵并转换为旋转器
			OutLabelRotation = FRotationMatrix::MakeFromXY(ToCenter, RightVector).Rotator();

			// 调整角度，使文本从上到下阅读（而不是倒立）
			if (MidAngleDegrees > 90 && MidAngleDegrees < 270)
			{
				OutLabelRotation.Yaw += 180.0f;
			}
		}
		else
		{
			// 默认朝向
			OutLabelRotation = FRotator(0, 0, 0);
		}

		// 内部标签不需要引导线
		OutLeaderLineEnd = OutLeaderLineStart;

		// 略微抬高标签，确保不会被饼图表面遮挡
		OutLabelPosition.Z += 0.5f;

		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 标签内部位置 (%f, %f, %f), 旋转: (%f, %f, %f)"),
		       OutLabelPosition.X, OutLabelPosition.Y, OutLabelPosition.Z,
		       OutLabelRotation.Pitch, OutLabelRotation.Yaw, OutLabelRotation.Roll);
	}
	else // EPieChartLabelPosition::Outside
	{
		// 外部标签 - 从引导线起点向外偏移
		float LabelOffset = LabelConfig.LabelOffset + 25.0f; // 增加偏移使标签更远离饼图边缘

		// 计算标签位置 - 每个象限使用不同的策略
		// 将圆周分为四个象限
		bool bIsRightSide = (MidAngleDegrees <= 90 || MidAngleDegrees >= 270);
		bool bIsTopSide = (MidAngleDegrees >= 0 && MidAngleDegrees <= 180);

		// 基础位置 - 从饼图中心向外
		OutLabelPosition = CenterPosition + FVector(
			(ActualExternalRadius + LabelOffset) * FMath::Cos(MidAngleRadians),
			(ActualExternalRadius + LabelOffset) * FMath::Sin(MidAngleRadians),
			SectionHeight * 0.5f + 0.5f // 略微抬高，确保可见
		);

		// 调整标签旋转，使其总是水平显示
		if (bIsRightSide)
		{
			// 右侧标签 - 水平向右
			OutLabelRotation = FRotator(0, 270, 0);
		}
		else
		{
			// 左侧标签 - 水平向左
			OutLabelRotation = FRotator(0, 90, 0);
		}

		// 计算引导线终点
		if (LabelConfig.LeaderLineLength > 0)
		{
			// 使用指定的引导线长度
			OutLeaderLineEnd = OutLeaderLineStart + FVector(
				LabelConfig.LeaderLineLength * FMath::Cos(MidAngleRadians),
				LabelConfig.LeaderLineLength * FMath::Sin(MidAngleRadians),
				0.0f
			);
		}
		else
		{
			// 自动计算引导线长度 - 从饼图边缘到标签前一段距离
			float AutoLineLength = LabelOffset * 0.8f;

			OutLeaderLineEnd = OutLeaderLineStart + FVector(
				AutoLineLength * FMath::Cos(MidAngleRadians),
				AutoLineLength * FMath::Sin(MidAngleRadians),
				0.0f
			);
		}

		// 确保引导线和标签在同一Z平面上
		OutLeaderLineEnd.Z = OutLabelPosition.Z - 0.1f; // 稍微低于标签，避免重叠

		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 标签外部位置 (%f, %f, %f), 旋转: (%f, %f, %f), 引导线终点: (%f, %f, %f)"),
		       OutLabelPosition.X, OutLabelPosition.Y, OutLabelPosition.Z,
		       OutLabelRotation.Pitch, OutLabelRotation.Yaw, OutLabelRotation.Roll,
		       OutLeaderLineEnd.X, OutLeaderLineEnd.Y, OutLeaderLineEnd.Z);
	}
}
