// Fill out your copyright notice in the Description page of Project Settings.
#include "Charts/XVPieChart.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
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

	// 加载圆柱体网格用于引导线
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		CylinderMesh = CylinderMeshFinder.Object;
	}

	// 初始化标签配置
	LabelConfig.bShowLabels = true;
	LabelConfig.FontSize = 40.0f;
	LabelConfig.FontColor = FColor::Cyan;
	LabelConfig.LabelFormat = TEXT("{category}: {value}");
	LabelConfig.LabelPosition = EPieChartLabelPosition::Outside;
	LabelConfig.LabelOffset = 10.0f;
	LabelConfig.bShowLeaderLines = true;
	LabelConfig.LeaderLineColor = FColor::Black;
	LabelConfig.LeaderLineThickness = 2.0f;

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
	for (UTextRenderComponent* Label : LabelComponents)
	{
		if (Label)
		{
			Label->DestroyComponent();
		}
	}
	LabelComponents.Empty();

	// 清理旧的引导线
	for (UStaticMeshComponent* LeaderLine : LeaderLineComponents)
	{
		if (LeaderLine)
		{
			LeaderLine->DestroyComponent();
		}
	}
	LeaderLineComponents.Empty();

	AccumulatedValues.SetNum(TotalCountOfValue + 1);
	AccumulatedValues[0] = 0;
	SectionSelectStates.Init(false, TotalCountOfValue);

	// 保存类别和数值，用于标签显示
	SectionCategories.Empty(TotalCountOfValue);
	SectionValues.Empty(TotalCountOfValue);

	double TotalValue = 0.0;
	size_t CurrentIndex = 0;

	// 使用一个有序数组来存储数据，以确保顺序一致
	TArray<TPair<FString, float>> OrderedData;
	for (const auto& Elem : Data)
	{
		OrderedData.Add(TPair<FString, float>(Elem.Key, Elem.Value));
	}

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
	GenerateLOD();

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
	GenerateLOD();
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

void AXVPieChart::GenerateLOD()
{
	Super::GenerateLOD();
	if (AccumulatedValues.IsEmpty())
	{
		return;
	}
	check(GenerateLODCount);
	TArray<FLODInfo> CurrentLODInfo;
	CurrentLODInfo.SetNum(GenerateLODCount);
	
	size_t DataSize = AccumulatedValues.Num() - 1;
	for (int i = 0; i < GenerateLODCount; i++)
	{
		size_t CurrentSectionStartAngle = 0;
		CurrentLODInfo[i].LODOffset = i * DataSize;
		for (int CurrentIndex = 0; CurrentIndex < DataSize; ++CurrentIndex)
		{
			uint32_t SectionIndex = CurrentLODInfo[i].LODOffset + CurrentIndex;
			size_t CurrentSectionEndAngle = static_cast<size_t>(AccumulatedValues[CurrentIndex] * AngleConvertFactor);
			GeneratePieSectionInfo(CenterPosition, SectionIndex,
			                       CurrentSectionStartAngle, CurrentSectionEndAngle - FinalSectionGapAngle,
			                       FinalInternalDiameter, ExternalDiameter + CurrentIndex * FinalNightingaleOffset,
			                       SectionHeight,
			                       SectionColors[CurrentIndex],
			                       i * NumLODReduceFactor + 1);

			DynamicMaterialInstances[SectionIndex] = UMaterialInstanceDynamic::Create(Material, this);
			DynamicMaterialInstances[SectionIndex]->SetVectorParameterValue("EmissiveColor", EmissiveColor);
			ProceduralMeshComponent->SetMaterial(SectionIndex, DynamicMaterialInstances[SectionIndex]);
			DrawMeshSection(SectionIndex);
			ProceduralMeshComponent->SetMeshSectionVisible(SectionIndex,false);
			
			CurrentSectionStartAngle = CurrentSectionEndAngle;
		}
		CurrentLODInfo[i].LODCount = DataSize;
	}
	TimedSectionInfos.Add(0.0f, {0, std::move(CurrentLODInfo)});
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

	// 确保标签和引导线在游戏开始时正确显示
	if (LabelConfig.bShowLabels && TotalCountOfValue > 0)
	{
		// 在下一帧更新标签，确保所有组件都已初始化
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			UpdateLabels();
			// 只有在悬停模式下才隐藏所有标签
			if (LabelConfig.bHoverToShowLabels)
			{
				HideAllLabels();
			}
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
						// 只在悬停模式下隐藏之前悬停区块的标签
						if (LabelConfig.bHoverToShowLabels)
						{
							HideLabel(HoveredSectionIndex);
						}
						
						UpdateSection(HoveredSectionIndex, SectionSelectStates[HoveredSectionIndex],
						              NewInternalDiameter,
						              NewExternalDiameter + HoveredSectionIndex * FinalNightingaleOffset, 0);
					}
					NewExternalDiameter += i * FinalNightingaleOffset;
					
					//绘制新选中的块
					HoveredSectionIndex = i;
					
					// 只在悬停模式下显示当前悬停区块的标签
					if (LabelConfig.bHoverToShowLabels)
					{
						ShowLabel(HoveredSectionIndex);
					}
					
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
		// 只在悬停模式下隐藏悬停区块的标签
		if (LabelConfig.bHoverToShowLabels)
		{
			HideLabel(HoveredSectionIndex);
		}
		
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
	
	if(bEnableEnterAnimation && !bAnimationFinished)
	{
		if (CurrentBuildTime < BuildTime && !IsHidden())
		{
			double Rate = CurrentBuildTime / BuildTime;
			UpdateLOD(Rate);
			CurrentBuildTime += DeltaTime;
		}
		else
		{
			bAnimationFinished = true;
		}
	}
	else 
	{
		UpdateLOD();
	}
}

void AXVPieChart::SetLabelConfig(const FXVPieChartLabelConfig& InLabelConfig)
{
	// 更新标签配置
	LabelConfig = InLabelConfig;

	// 更新标签显示
	UpdateLabels();
	
	// 根据新配置设置标签显示状态
	if (LabelConfig.bShowLabels)
	{
		if (LabelConfig.bHoverToShowLabels)
		{
			// 悬停模式：隐藏所有标签
			HideAllLabels();
		}
		// 如果不是悬停模式，标签已经在CreateSectionLabels中设置为可见
	}
}

void AXVPieChart::UpdateLabels()
{
	// 清理现有标签组件 - 参考LineChart的方式
	for (auto* Label : LabelComponents)
	{
		if (Label)
		{
			Label->DestroyComponent();
		}
	}
	LabelComponents.Empty();

	// 清理现有引导线组件
	for (auto* LeaderLine : LeaderLineComponents)
	{
		if (LeaderLine)
		{
			LeaderLine->DestroyComponent();
		}
	}
	LeaderLineComponents.Empty();

	// 如果不显示标签或没有数据，直接返回
	if (!LabelConfig.bShowLabels || TotalCountOfValue <= 0)
	{
		return;
	}

	// 直接创建新标签和引导线
	CreateSectionLabels();
	if (LabelConfig.bShowLeaderLines)
	{
		CreateLeaderLines();
	}
}

void AXVPieChart::CreateSectionLabels()
{
	// 如果没有数据或不需要显示标签，直接返回
	if (TotalCountOfValue <= 0 || !LabelConfig.bShowLabels)
	{
		return;
	}

	// 确保LabelComponents数组大小正确 - 参考LineChart的方式
	LabelComponents.SetNum(TotalCountOfValue);

	// 为每个区块创建标签
	for (int32 i = 0; i < TotalCountOfValue; ++i)
	{
		// 安全检查
		if (i >= SectionCategories.Num() || i >= SectionValues.Num())
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

		// 计算标签位置 - 在区块外径的上方
		FVector LabelPosition;
		
		// 标签统一放在区块外径边缘的上方
		LabelPosition = GetActorLocation() + FVector(
			ActualExternalRadius * FMath::Cos(MidAngleRadians),
			ActualExternalRadius * FMath::Sin(MidAngleRadians),
			SectionHeight + 50.0f // 在区块高度上方5个单位
		);

		// 如果是外部标签模式，可以稍微向外偏移一点
		if (LabelConfig.LabelPosition == EPieChartLabelPosition::Outside)
		{
			float LabelOffset = LabelConfig.LabelOffset;
			LabelPosition = GetActorLocation() + FVector(
				(ActualExternalRadius + LabelOffset) * FMath::Cos(MidAngleRadians),
				(ActualExternalRadius + LabelOffset) * FMath::Sin(MidAngleRadians),
				SectionHeight + 50.0f // 在区块高度上方5个单位
			);
		}

		// 创建文本渲染组件 - 参考LineChart的方式
		UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
		Label->SetupAttachment(RootComponent);
		Label->RegisterComponent();

		// 格式化标签文本
		FString LabelText = LabelConfig.LabelFormat;
		LabelText = LabelText.Replace(TEXT("{category}"), *SectionCategories[i]);
		LabelText = LabelText.Replace(TEXT("{value}"), *FString::Printf(TEXT("%.2f"), SectionValues[i]));
		Label->SetText(FText::FromString(LabelText));

		Label->SetTextRenderColor(LabelConfig.FontColor);
		Label->SetWorldSize(LabelConfig.FontSize);
		Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);

		// 设置标签位置 - 先不设置旋转，后续动态调整朝向相机
		Label->SetWorldLocation(LabelPosition);

		// 根据显示模式设置初始可见性
		bool bInitiallyVisible = !LabelConfig.bHoverToShowLabels;
		Label->SetVisibility(bInitiallyVisible);

		// 如果标签初始可见，设置朝向相机
		if (bInitiallyVisible && GetWorld() && GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->PlayerCameraManager)
		{
			FRotator CamRotation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraRotation();
			CamRotation.Yaw += 180;
			CamRotation.Pitch *= -1;
			FRotator TextRotation(CamRotation);
			Label->SetWorldRotation(TextRotation);
		}

		// 保存标签组件 - 直接设置到对应索引而不是Add
		LabelComponents[i] = Label;
	}
}

void AXVPieChart::CreateLeaderLines()
{
	// 如果没有数据或不需要显示引导线，直接返回
	if (TotalCountOfValue <= 0 || !LabelConfig.bShowLabels || !LabelConfig.bShowLeaderLines)
	{
		return;
	}

	// 确保LeaderLineComponents数组大小正确
	LeaderLineComponents.SetNum(TotalCountOfValue);

	// 为每个区块创建引导线
	for (int32 i = 0; i < TotalCountOfValue; ++i)
	{
		// 安全检查
		if (i >= SectionCategories.Num() || i >= SectionValues.Num())
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

		// 计算引导线起始位置（饼图边缘）
		FVector StartPosition = GetActorLocation() + FVector(
			ActualExternalRadius * FMath::Cos(MidAngleRadians),
			ActualExternalRadius * FMath::Sin(MidAngleRadians),
			SectionHeight * 0.5f // 在区块中间高度
		);

		// 计算引导线结束位置（标签位置）
		FVector EndPosition;
		if (LabelConfig.LabelPosition == EPieChartLabelPosition::Outside)
		{
			float LabelOffset = LabelConfig.LabelOffset;
			EndPosition = GetActorLocation() + FVector(
				(ActualExternalRadius + LabelOffset) * FMath::Cos(MidAngleRadians),
				(ActualExternalRadius + LabelOffset) * FMath::Sin(MidAngleRadians),
				SectionHeight + 50.0f // 与标签高度一致
			);
		}
		else
		{
			EndPosition = GetActorLocation() + FVector(
				ActualExternalRadius * FMath::Cos(MidAngleRadians),
				ActualExternalRadius * FMath::Sin(MidAngleRadians),
				SectionHeight + 50.0f // 与标签高度一致
			);
		}

		// 创建引导线
		CreateSingleLeaderLine(i, StartPosition, EndPosition);
	}
}

void AXVPieChart::CreateSingleLeaderLine(int32 SectionIndex, const FVector& StartPosition, const FVector& EndPosition)
{
	// 创建静态网格组件作为引导线
	UStaticMeshComponent* LeaderLine = NewObject<UStaticMeshComponent>(this);
	LeaderLine->SetupAttachment(RootComponent);

	// 使用构造函数中加载的圆柱体网格作为线条
	if (CylinderMesh)
	{
		LeaderLine->SetStaticMesh(CylinderMesh);
	}

	// 计算线条的长度、位置和旋转
	FVector LineDirection = EndPosition - StartPosition;
	float LineLength = LineDirection.Size();
	FVector LineMidpoint = (StartPosition + EndPosition) * 0.5f;
	
	// 计算旋转角度，使圆柱体沿着线条方向
	FRotator LineRotation = FRotationMatrix::MakeFromZ(LineDirection.GetSafeNormal()).Rotator();
	
	// 设置位置和旋转
	LeaderLine->SetWorldLocation(LineMidpoint);
	LeaderLine->SetWorldRotation(LineRotation);
	
	// 设置缩放：长度为线条长度，粗细为配置的厚度
	float Thickness = LabelConfig.LeaderLineThickness * 0.01f; // 调整比例
	LeaderLine->SetWorldScale3D(FVector(Thickness, Thickness, LineLength * 0.01f));

	// 创建动态材质实例并设置颜色
	if (Material)
	{
		UMaterialInstanceDynamic* LineMaterial = UMaterialInstanceDynamic::Create(Material, this);
		LineMaterial->SetVectorParameterValue("BaseColor", FLinearColor(LabelConfig.LeaderLineColor));
		LineMaterial->SetScalarParameterValue("EmissiveIntensity", 0.5f);
		LeaderLine->SetMaterial(0, LineMaterial);
	}

	// 注册组件
	LeaderLine->RegisterComponent();

	// 根据显示模式设置初始可见性
	bool bInitiallyVisible = !LabelConfig.bHoverToShowLabels;
	LeaderLine->SetVisibility(bInitiallyVisible);

	// 保存引导线组件
	LeaderLineComponents[SectionIndex] = LeaderLine;
}

// 应用值触发条件到饼图
void AXVPieChart::ApplyValueTriggerConditions()
{
	if (!bEnableValueTriggers || TotalCountOfValue == 0 || ValueTriggerConditions.Num() == 0)
	{
		return;
	}

	// 遍历所有饼图区块，检查是否符合触发条件
	for (int32 i = 0; i < TotalCountOfValue; ++i)
	{
		// 获取区块值
		float SectionValue = SectionValues[i];
		
		// 检查值是否满足任何触发条件
		FLinearColor HighlightColor;
		bool bMatchesTrigger = CheckValueTriggerConditions(SectionValue, HighlightColor);
		
		if (bMatchesTrigger)
		{
			// 符合条件，高亮显示该区块
			Highlight(i, EmissiveIntensity);
			
			// 更改区块颜色（可选）
			if (DynamicMaterialInstances.IsValidIndex(i))
			{
				DynamicMaterialInstances[i]->SetVectorParameterValue("EmissiveColor", HighlightColor);
			}
		}
		else
		{
			// 不符合条件，重置区块状态
			Highlight(i, 0.0f);
			
			// 恢复区块原始颜色（可选）
			if (DynamicMaterialInstances.IsValidIndex(i))
			{
				DynamicMaterialInstances[i]->SetVectorParameterValue("EmissiveColor", EmissiveColor);
			}
		}
	}
}

// 获取所有数据值
TArray<float> AXVPieChart::GetAllDataValues() const
{
	return SectionValues;
}

void AXVPieChart::ShowLabel(int32 SectionIndex)
{
	// 安全检查
	if (SectionIndex < 0 || SectionIndex >= LabelComponents.Num() || !LabelComponents[SectionIndex])
	{
		return;
	}

	// 设置标签朝向相机 - 参考LineChart的旋转设置
	if (GetWorld() && GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->PlayerCameraManager)
	{
		FRotator CamRotation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraRotation();
		CamRotation.Yaw += 180;
		CamRotation.Pitch *= -1;
		FRotator TextRotation(CamRotation);
		LabelComponents[SectionIndex]->SetWorldRotation(TextRotation);
	}

	// 显示标签
	LabelComponents[SectionIndex]->SetVisibility(true);
	
	// 同时显示对应的引导线
	if (LabelConfig.bShowLeaderLines)
	{
		ShowLeaderLine(SectionIndex);
	}
}

void AXVPieChart::HideLabel(int32 SectionIndex)
{
	// 安全检查
	if (SectionIndex < 0 || SectionIndex >= LabelComponents.Num() || !LabelComponents[SectionIndex])
	{
		return;
	}

	// 隐藏标签
	LabelComponents[SectionIndex]->SetVisibility(false);
	
	// 同时隐藏对应的引导线
	HideLeaderLine(SectionIndex);
}

void AXVPieChart::HideAllLabels()
{
	// 隐藏所有标签
	for (int32 i = 0; i < LabelComponents.Num(); ++i)
	{
		HideLabel(i);
	}
	
	// 同时隐藏所有引导线
	HideAllLeaderLines();
}

void AXVPieChart::ShowAllLabels()
{
	// 获取相机旋转角度 - 一次获取，应用到所有标签
	FRotator TextRotation = FRotator::ZeroRotator;
	if (GetWorld() && GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->PlayerCameraManager)
	{
		FRotator CamRotation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraRotation();
		CamRotation.Yaw += 180;
		CamRotation.Pitch *= -1;
		TextRotation = FRotator(CamRotation);
	}

	// 显示所有标签并设置朝向相机
	for (int32 i = 0; i < LabelComponents.Num(); ++i)
	{
		if (LabelComponents[i])
		{
			LabelComponents[i]->SetWorldRotation(TextRotation);
			LabelComponents[i]->SetVisibility(true);
		}
	}
	
	// 同时显示所有引导线
	if (LabelConfig.bShowLeaderLines)
	{
		ShowAllLeaderLines();
	}
}

void AXVPieChart::ShowLeaderLine(int32 SectionIndex)
{
	// 安全检查
	if (SectionIndex < 0 || SectionIndex >= LeaderLineComponents.Num() || !LeaderLineComponents[SectionIndex])
	{
		return;
	}

	// 显示引导线
	LeaderLineComponents[SectionIndex]->SetVisibility(true);
}

void AXVPieChart::HideLeaderLine(int32 SectionIndex)
{
	// 安全检查
	if (SectionIndex < 0 || SectionIndex >= LeaderLineComponents.Num() || !LeaderLineComponents[SectionIndex])
	{
		return;
	}

	// 隐藏引导线
	LeaderLineComponents[SectionIndex]->SetVisibility(false);
}

void AXVPieChart::HideAllLeaderLines()
{
	// 隐藏所有引导线
	for (int32 i = 0; i < LeaderLineComponents.Num(); ++i)
	{
		HideLeaderLine(i);
	}
}

void AXVPieChart::ShowAllLeaderLines()
{
	// 显示所有引导线
	for (int32 i = 0; i < LeaderLineComponents.Num(); ++i)
	{
		if (LeaderLineComponents[i])
		{
			LeaderLineComponents[i]->SetVisibility(true);
		}
	}
}
