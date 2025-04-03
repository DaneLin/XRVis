// Fill out your copyright notice in the Description page of Project Settings.
#include "XVPieChart.h"
#include "UObject/ConstructorHelpers.h"


// Sets default values
AXVPieChart::AXVPieChart(): TimeSinceLastUpdate(0), GradientFactor(0), AngleConvertFactor(0), FinalNightingaleOffset(0),
                            FinalInternalDiameter(0),
                            FinalSectionGapAngle(0)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	XVChartUtils::LoadResourceFromPath(TEXT("/Script/Engine.Material'/XRVis/Materials/M_BaseVertexColor.M_BaseVertexColor'"), Material);


	ZoomOffset = 20.f;
}

void AXVPieChart::Create3DPieChart(const TMap<FString, float>& Data, EPieChartStyle ChartStyle,const TArray<FColor>& PieChartColor,EPieShape Shape)
{
	TotalCountOfValue = Data.Num();

	if (TotalCountOfValue <= 0 || InternalDiameter > ExternalDiameter)
	{
		return;
	}
	AccumulatedValues.SetNum(TotalCountOfValue + 1);
	AccumulatedValues[0] = 0;
	SectionSelectStates.Init(false, TotalCountOfValue);

	double TotalValue = 0.0;
	size_t CurrentIndex = 0;
	for (auto& Elem : Data)
	{
		TotalValue = TotalValue + Elem.Value;
		AccumulatedValues[CurrentIndex] = TotalValue;
		++CurrentIndex;
	}
	AccumulatedValues[TotalCountOfValue] = TotalValue;
	AngleConvertFactor = 360.0 / TotalValue;

	DynamicMaterialInstances.Empty();
	DynamicMaterialInstances.SetNum(TotalCountOfValue + 1);

	// 完整性检查，避免出现颜色数据少于实际数据
	
	Set3DPieChart(ChartStyle, PieChartColor, Shape);
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
		return;
	}

	PrepareMeshSections();

	size_t DataSize = AccumulatedValues.Num() - 1;
	size_t CurrentSectionStartAngle = 0;
	for (int CurrentIndex = 0; CurrentIndex < FMath::CeilToInt(DataSize * Rate); ++CurrentIndex)
	{
		size_t CurrentSectionEndAngle = static_cast<size_t>((AccumulatedValues[CurrentIndex]) * AngleConvertFactor *
			Rate);

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
