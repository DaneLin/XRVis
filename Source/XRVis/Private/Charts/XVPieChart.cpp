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

	XVChartUtils::LoadResourceFromPath(TEXT("/Script/Engine.Material'/XRVis/Materials/M_BaseVertexColor.M_BaseVertexColor'"), Material);

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

void AXVPieChart::Create3DPieChart(const TMap<FString, float>& Data, EPieChartStyle ChartStyle,const TArray<FColor>& PieChartColor,EPieShape Shape)
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
	DynamicMaterialInstances.SetNum(TotalCountOfValue + 1);

	// 设置图表样式和颜色
	Set3DPieChart(ChartStyle, PieChartColor, Shape);
	
	// 确保中心位置已设置，如果是零向量，设置为当前Actor的位置
	if (CenterPosition.IsZero())
	{
		CenterPosition = GetActorLocation();
	}
	
	// 创建标签 - 移动到ConstructMesh之后调用，确保网格已经构建完成
}

void AXVPieChart::Set3DPieChart(EPieChartStyle ChartStyle, const TArray<FColor>& PieChartColor, EPieShape Shape)
{
	// Only for editor panel
	PieChartStyle = ChartStyle;
	PieShape = Shape;
	SectionColors = PieChartColor;

	while(SectionColors.Num() < TotalCountOfValue)
	{
		SectionColors.Push(FColor::Black);
	}

	ProcessTypeAndShapeInfo();

#if WITH_EDITOR
	ConstructMesh(1);
#endif
}

void AXVPieChart::ColorModify(int ModifyIndex, const FColor& Color)
{
	if(ModifyIndex >= TotalCountOfValue)
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
	if(ModifyIndex >= TotalCountOfValue)
	{
		UE_LOG(LogTemp, Warning, TEXT("Modify Index out of data range"));
		return;
	}
	UpdateSection(ModifyIndex, SectionSelectStates[ModifyIndex],FinalInternalDiameter,ExternalDiameter, HighlightIntensity);
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
		UE_LOG(LogTemp, Warning, TEXT("AXVPieChart: 累积值数组为空，无法构建网格"));
		return;
	}

	PrepareMeshSections();

	// 记录中心位置
	CenterPosition = GetActorLocation();
	UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 构建网格，中心位置: (%f, %f, %f)"), 
		CenterPosition.X, CenterPosition.Y, CenterPosition.Z);

	size_t DataSize = AccumulatedValues.Num() - 1;
	size_t CurrentSectionStartAngle = 0;
	for (int CurrentIndex = 0; CurrentIndex < FMath::CeilToInt(DataSize * Rate); ++CurrentIndex)
	{
		size_t CurrentSectionEndAngle = static_cast<size_t>((AccumulatedValues[CurrentIndex]) * AngleConvertFactor *
			Rate);

		// 日志输出每个区块的角度信息
		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 区块 %d, 开始角度: %d, 结束角度: %d"), 
			CurrentIndex, CurrentSectionStartAngle, CurrentSectionEndAngle - FinalSectionGapAngle);

		GeneratePieSectionInfo(CenterPosition, CurrentIndex,
		                       CurrentSectionStartAngle, CurrentSectionEndAngle - FinalSectionGapAngle,
		                       FinalInternalDiameter, ExternalDiameter + CurrentIndex * FinalNightingaleOffset,
		                       SectionHeight,
		                       SectionColors[CurrentIndex]);


		DynamicMaterialInstances[CurrentIndex] = UMaterialInstanceDynamic::Create(Material, this);
		DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue("EmissiveColor", EmissiveColor);
		ProceduralMeshComponent->SetMaterial(CurrentIndex, DynamicMaterialInstances[CurrentIndex]);
		ProceduralMeshComponent->CreateMeshSection_LinearColor(CurrentIndex,
		                                                       SectionInfos[CurrentIndex].Vertices,
		                                                       SectionInfos[CurrentIndex].Indices,
		                                                       SectionInfos[CurrentIndex].Normals,
		                                                       SectionInfos[CurrentIndex].UVs,
		                                                       SectionInfos[CurrentIndex].VertexColors,
		                                                       SectionInfos[CurrentIndex].Tangents,
		                                                       true);

		CurrentSectionStartAngle = CurrentSectionEndAngle;
	}

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
	ConstructMesh();
	
	// 如果在编辑器中，确保标签可见
#if WITH_EDITOR
	if (LabelConfig.bShowLabels && TotalCountOfValue > 0)
	{
		// 在下一帧更新标签，确保所有组件都已初始化
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			UpdateLabels();
		});
	}
#endif
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
					//UE_LOG(LogTemp, Warning, TEXT("UpdateOnMouseEnterOrLeft Current Angle: %f %f"), StartAngle, EndAngle);
					//之前移动到或者已经点选中了
					if (HoveredSectionIndex == i || (HoveredSectionIndex != i && SectionSelectStates[i])) break;
					//FString text = DataArray[i].Label;
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
					if(bEnableZoomAnimation)
					{
						NewExternalDiameter += ZoomOffset;
					}
					if(bEnablePopAnimation)
					{
						NewInternalDiameter += PopOffset;
						NewExternalDiameter += PopOffset + bEnableZoomAnimation ? ZoomOffset:0;
					}
					UpdateSection(HoveredSectionIndex, SectionSelectStates[HoveredSectionIndex],NewInternalDiameter,NewExternalDiameter, EmissiveIntensity);
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
	ClearSelectedSection(UpdateSectionIndex);

	int StartAngle = UpdateSectionIndex == 0 ? 0 : AccumulatedValues[UpdateSectionIndex - 1] * AngleConvertFactor;
	int EndAngle = AccumulatedValues[UpdateSectionIndex] * AngleConvertFactor - FinalSectionGapAngle;

	SectionSelectStates[UpdateSectionIndex] = InSelected;
	GeneratePieSectionInfo(CenterPosition, UpdateSectionIndex, StartAngle, EndAngle, InIR, InER, SectionHeight,
	                       SectionColors[UpdateSectionIndex]);
	DynamicMaterialInstances[UpdateSectionIndex]->SetScalarParameterValue("EmissiveIntensity", InEmissiveIntensity);

	ProceduralMeshComponent->CreateMeshSection_LinearColor(UpdateSectionIndex,
	                                                       SectionInfos[UpdateSectionIndex].Vertices,
	                                                       SectionInfos[UpdateSectionIndex].Indices,
	                                                       SectionInfos[UpdateSectionIndex].Normals,
	                                                       SectionInfos[UpdateSectionIndex].UVs,
	                                                       SectionInfos[UpdateSectionIndex].VertexColors,
	                                                       SectionInfos[UpdateSectionIndex].Tangents,
	                                                       true);
}

// Called every frame
void AXVPieChart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
		CreateSectionLabels();
	}
}

void AXVPieChart::CreateSectionLabels()
{
	// 确保数组是空的
	SectionLabels.Empty();
	
	// 如果没有数据或字体，则退出
	if (TotalCountOfValue <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVPieChart: 没有数据，无法创建标签"));
		return;
	}
	
	// 如果字体为空，尝试加载默认字体
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
	
	// 创建每个区块的标签
	for (int32 i = 0; i < TotalCountOfValue; ++i)
	{
		// 安全检查
		if (i >= SectionCategories.Num() || i >= SectionValues.Num())
		{
			UE_LOG(LogTemp, Error, TEXT("AXVPieChart: 索引超出范围，标签创建失败: %d"), i);
			continue;
		}
		
		// 创建文本渲染组件
		UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
		Label->SetupAttachment(RootComponent);
		Label->RegisterComponent();
		
		// 设置字体
		Label->SetFont(LabelConfig.Font);
		Label->SetTextRenderColor(LabelConfig.FontColor);
		Label->SetWorldSize(LabelConfig.FontSize);
		
		// 设置水平和垂直对齐方式
		Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		
		// 格式化标签文本
		FString LabelText = LabelConfig.LabelFormat;
		LabelText = LabelText.Replace(TEXT("{category}"), *SectionCategories[i]);
		LabelText = LabelText.Replace(TEXT("{value}"), *FString::Printf(TEXT("%.2f"), SectionValues[i]));
		Label->SetText(FText::FromString(LabelText));
		
		// 计算标签位置
		float StartAngle = 0;
		if (i > 0)
		{
			StartAngle = AccumulatedValues[i-1] * AngleConvertFactor + (i > 0 ? FinalSectionGapAngle : 0);
		}
		float EndAngle = AccumulatedValues[i] * AngleConvertFactor - FinalSectionGapAngle;
		
		FVector LabelPosition;
		FRotator LabelRotation;
		FVector LeaderLineStart;
		FVector LeaderLineEnd;
		
		// 计算标签位置和引线位置
		CalculateLabelPosition(i, StartAngle, EndAngle, LabelPosition, LabelRotation, LeaderLineStart, LeaderLineEnd);
		
		// 设置标签位置和旋转
		Label->SetWorldLocation(LabelPosition);
		Label->SetWorldRotation(LabelRotation);
		
		// 设置可见性
		Label->SetVisibility(true);
		Label->SetHiddenInGame(false);
		
		// 记录日志
		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 创建标签 %d, 文本: %s, 位置: (%f, %f, %f)"), 
			i, *LabelText, LabelPosition.X, LabelPosition.Y, LabelPosition.Z);
		
		// 保存到数组
		SectionLabels.Add(Label);
	}
	
	// 如果启用了引线，创建引线
	if (LabelConfig.LabelPosition == EPieChartLabelPosition::Outside && LabelConfig.bShowLeaderLine)
	{
		CreateLabelLeaderLines();
	}
}

void AXVPieChart::CreateLabelLeaderLines()
{
	// 清理旧的引线组件
	for (UProceduralMeshComponent* LineMesh : LeaderLineMeshes)
	{
		if (LineMesh)
		{
			LineMesh->DestroyComponent();
		}
	}
	LeaderLineMeshes.Empty();
	
	UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 开始创建引线，共 %d 个区块"), TotalCountOfValue);
	
	// 为每个区块创建引线
	for (int32 i = 0; i < TotalCountOfValue; ++i)
	{
		// 安全检查
		if (i >= SectionLabels.Num())
		{
			continue;
		}
		
		// 计算引线的起点和终点
		float StartAngle = 0;
		if (i > 0)
		{
			StartAngle = AccumulatedValues[i-1] * AngleConvertFactor + (i > 0 ? FinalSectionGapAngle : 0);
		}
		float EndAngle = AccumulatedValues[i] * AngleConvertFactor - FinalSectionGapAngle;
		
		FVector LabelPosition;
		FRotator LabelRotation;
		FVector LeaderLineStart;
		FVector LeaderLineEnd;
		
		// 计算标签位置和引线位置
		CalculateLabelPosition(i, StartAngle, EndAngle, LabelPosition, LabelRotation, LeaderLineStart, LeaderLineEnd);
		
		// 创建引线网格组件
		UProceduralMeshComponent* LineMesh = NewObject<UProceduralMeshComponent>(this);
		LineMesh->SetupAttachment(RootComponent);
		LineMesh->RegisterComponent();
		
		// 创建引线几何体 - 简单的线段
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UV0;
		TArray<FLinearColor> VertexColors;
		TArray<FProcMeshTangent> Tangents;
		
		// 计算引线的方向向量
		FVector LineDir = (LeaderLineEnd - LeaderLineStart).GetSafeNormal();
		FVector LineUp = FVector(0, 0, 1);
		FVector LineRight = FVector::CrossProduct(LineDir, LineUp).GetSafeNormal();
		
		// 引线的宽度（厚度）
		float LineWidth = LabelConfig.LeaderLineThickness * 0.5f;
		
		// 创建围绕线段的4个顶点
		FVector V1 = LeaderLineStart + LineRight * LineWidth;
		FVector V2 = LeaderLineStart - LineRight * LineWidth;
		FVector V3 = LeaderLineEnd + LineRight * LineWidth;
		FVector V4 = LeaderLineEnd - LineRight * LineWidth;
		
		// 添加顶点
		Vertices.Add(V1);
		Vertices.Add(V2);
		Vertices.Add(V3);
		Vertices.Add(V4);
		
		// 添加三角形索引 - 两个三角形组成一个四边形
		Triangles.Add(0);
		Triangles.Add(2);
		Triangles.Add(1);
		
		Triangles.Add(1);
		Triangles.Add(2);
		Triangles.Add(3);
		
		// 添加法线、UV和颜色
		for (int32 j = 0; j < 4; ++j)
		{
			Normals.Add(FVector(0, 0, 1));
			UV0.Add(FVector2D(0, 0));
			VertexColors.Add(FLinearColor(LabelConfig.LeaderLineColor));
			Tangents.Add(FProcMeshTangent(1, 0, 0));
		}
		
		// 创建网格
		LineMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
		
		// 创建材质实例并设置
		if (Material)
		{
			UMaterialInstanceDynamic* LineMaterial = UMaterialInstanceDynamic::Create(Material, this);
			if (LineMaterial)
			{
				LineMaterial->SetVectorParameterValue("EmissiveColor", FLinearColor(LabelConfig.LeaderLineColor));
				LineMaterial->SetScalarParameterValue("EmissiveIntensity", 1.0f);
				LineMesh->SetMaterial(0, LineMaterial);
			}
		}
		
		// 设置可见性
		LineMesh->SetVisibility(true);
		LineMesh->SetHiddenInGame(false);
		
		// 保存到数组
		LeaderLineMeshes.Add(LineMesh);
		
		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 创建引线 %d, 起点: (%f, %f, %f), 终点: (%f, %f, %f)"),
			i, LeaderLineStart.X, LeaderLineStart.Y, LeaderLineStart.Z,
			LeaderLineEnd.X, LeaderLineEnd.Y, LeaderLineEnd.Z);
	}
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
	
	// 检查中心点是否有效
	if (CenterPosition.IsZero())
	{
		CenterPosition = GetActorLocation();
		UE_LOG(LogTemp, Warning, TEXT("AXVPieChart: 中心点为零，使用Actor位置: (%f, %f, %f)"), 
			CenterPosition.X, CenterPosition.Y, CenterPosition.Z);
	}
	
	// 如果这个区块有特殊的半径（例如南丁格尔玫瑰图），计算正确的外径
	float ActualExternalRadius = ExternalDiameter;
	if (bIsShapeNightingale && SectionIndex < TotalCountOfValue)
	{
		ActualExternalRadius = ExternalDiameter + SectionIndex * FinalNightingaleOffset;
	}
	
	// 设置引线起点在区块外径处
	OutLeaderLineStart = CenterPosition + FVector(
		ActualExternalRadius * FMath::Cos(MidAngleRadians),
		ActualExternalRadius * FMath::Sin(MidAngleRadians),
		SectionHeight * 0.5f
	);
	
	// 记录引线起点
	UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 区块 %d 引线起点位置 (%f, %f, %f)"), 
		SectionIndex, OutLeaderLineStart.X, OutLeaderLineStart.Y, OutLeaderLineStart.Z);
	
	// 根据标签位置计算标签位置和引线终点
	if (LabelConfig.LabelPosition == EPieChartLabelPosition::Inside)
	{
		// 内部标签 - 放在饼图内部
		float LabelRadius = FinalInternalDiameter + (ActualExternalRadius - FinalInternalDiameter) * 0.5f;
		OutLabelPosition = CenterPosition + FVector(
			LabelRadius * FMath::Cos(MidAngleRadians),
			LabelRadius * FMath::Sin(MidAngleRadians),
			SectionHeight * 0.5f
		);
		
		// 标签朝向 - 内部标签朝向饼图中心
		FVector DirToCenter = (CenterPosition - OutLabelPosition).GetSafeNormal();
		if(!DirToCenter.IsNearlyZero())
		{
			OutLabelRotation = DirToCenter.Rotation();
		}
		else
		{
			// 默认朝向
			OutLabelRotation = FRotator(0, 0, 0);
		}
		
		// 内部标签不需要引线
		OutLeaderLineEnd = OutLeaderLineStart;
		
		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 标签内部位置 (%f, %f, %f), 旋转: (%f, %f, %f)"), 
			OutLabelPosition.X, OutLabelPosition.Y, OutLabelPosition.Z,
			OutLabelRotation.Pitch, OutLabelRotation.Yaw, OutLabelRotation.Roll);
	}
	else // EPieChartLabelPosition::Outside
	{
		// 外部标签 - 从引线起点向外偏移
		float LabelOffset = LabelConfig.LabelOffset + 5.0f; // 增加小偏移使标签更远离饼图边缘
		
		// 计算标签位置 - 从外径向外延伸LabelOffset距离
		OutLabelPosition = OutLeaderLineStart + FVector(
			LabelOffset * FMath::Cos(MidAngleRadians),
			LabelOffset * FMath::Sin(MidAngleRadians),
			0.0f  // 保持Z值不变
		);
		
		// 标签朝向 - 使标签正对着观察者（朝向Z轴）
		OutLabelRotation = FRotator(0, MidAngleDegrees + 90.0f, 0);
		
		// 设置引线终点 - 接近标签位置但不完全重叠
		if (LabelConfig.LeaderLineLength > 0)
		{
			// 使用指定的引线长度
			OutLeaderLineEnd = OutLeaderLineStart + FVector(
				LabelConfig.LeaderLineLength * FMath::Cos(MidAngleRadians),
				LabelConfig.LeaderLineLength * FMath::Sin(MidAngleRadians),
				0.0f
			);
		}
		else
		{
			// 自动计算引线长度 - 到标签位置前一小段距离
			OutLeaderLineEnd = OutLabelPosition - FVector(
				5.0f * FMath::Cos(MidAngleRadians),
				5.0f * FMath::Sin(MidAngleRadians),
				0.0f
			);
		}
		
		UE_LOG(LogTemp, Log, TEXT("AXVPieChart: 标签外部位置 (%f, %f, %f), 旋转: (%f, %f, %f), 引线终点: (%f, %f, %f)"), 
			OutLabelPosition.X, OutLabelPosition.Y, OutLabelPosition.Z,
			OutLabelRotation.Pitch, OutLabelRotation.Yaw, OutLabelRotation.Roll,
			OutLeaderLineEnd.X, OutLeaderLineEnd.Y, OutLeaderLineEnd.Z);
	}
}
