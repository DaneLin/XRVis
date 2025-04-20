// Fill out your copyright notice in the Description page of Project Settings.


#include "Charts/XVBarChart.h"

#include "Charts/XVChartAxis.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetTextLibrary.h"
#include "Rendering/XRVisBoxGeometryGenerator.h"
#include "Rendering/XRVisGeometryTypes.h"


// Sets default values
AXVBarChart::AXVBarChart()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	XAxisInterval = 13;
	YAxisInterval = 13;
	Width = 10;
	Length = 10;

	MinX = std::numeric_limits<int>::max();
	MaxX = std::numeric_limits<int>::min();
	MinY = std::numeric_limits<int>::max();
	MaxY = std::numeric_limits<int>::min();
	MinZ = std::numeric_limits<float>::max();
	MaxZ = std::numeric_limits<float>::min();
	

	// 默认颜色
	Colors.Add((FColor::FromHex("#313695")));
	Colors.Add((FColor::FromHex("#4575b4")));
	Colors.Add((FColor::FromHex("#74add1")));
	Colors.Add((FColor::FromHex("#abd9e9")));
	Colors.Add((FColor::FromHex("#e0f3f8")));
	Colors.Add((FColor::FromHex("#ffffbf")));
	Colors.Add((FColor::FromHex("#fee090")));
	Colors.Add((FColor::FromHex("#fdae61")));
	Colors.Add((FColor::FromHex("#f46d43")));
	Colors.Add((FColor::FromHex("#d73027")));
	Colors.Add((FColor::FromHex("#a50026")));
	
	XVChartUtils::LoadResourceFromPath(TEXT("Material'/XRVis/Materials/M_BaseVertexColor.M_BaseVertexColor'"), BaseMaterial);
		
}

// Called when the game starts or when spawned
void AXVBarChart::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> ChildActors;
	GetAttachedActors(ChildActors);
	for (AActor* Actor : ChildActors)
	{
		AXVChartAxis* ChartAxis = Cast<AXVChartAxis>(Actor);
		if (ChartAxis)
		{
			ChartAxis->SetXAxisText(XTextArrs);
			ChartAxis->SetYAxisText(YTextArrs);
			ChartAxis->SetZAxisText(ZTextArrs);
			ChartAxis->SetAxisGridNum(XAxisLabels.Num(), YAxisLabels.Num(),5);
		}
	}

	if (bAutoLoadData && bEnableGPU)
	{
		FXRVisBoxGeometryParams Params;
		Params.RowCount = XAxisLabels.Num();
		Params.ColumnCount = YAxisLabels.Num();
		Params.HeightValues = HeightValues;
		static_cast<FXRVisBoxGeometryGenerator*>(GeometryGenerator)->SetParameters(Params);
	}
}

// Called every frame
void AXVBarChart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(bEnableGPU)
	{
		return;
	}

	if(bEnableEnterAnimation)
	{
		if (CurrentBuildTime < BuildTime && !IsHidden())
		{
			ConstructMesh(CurrentBuildTime / BuildTime);
			CurrentBuildTime += DeltaTime;
		}
	}
	else
	{
		if(!bAnimationFinished)
		{
			ConstructMesh(1);
			bAnimationFinished = true;
		}
	}
	UpdateOnMouseEnterOrLeft();
}

void AXVBarChart::NotifyActorOnClicked(FKey ButtonPressed)
{
	
	FHitResult HitResult = XVChartUtils::GetCursorHitResult(GetWorld());

	if (HitResult.GetActor())
	{
		FVector Result = GetCursorHitRowAndColAndHeight(HitResult);

		int CurrentRow = Result.Y / (YAxisInterval * GetActorScale3D().Y);
		ClickedIndex = CurrentRow;
		Super::NotifyActorOnClicked(ButtonPressed);
	}
	else
	{
		ClickedIndex = -1;
	}
}

void AXVBarChart::Create3DHistogramChart(const FString& Data, EHistogramChartStyle InHistogramChartStyle, EHistogramChartShape InHistogramChartShape)
{
	Set3DHistogramChart(InHistogramChartStyle, InHistogramChartShape);
	SetValue(Data);
#if WITH_EDITOR
	ConstructMesh(1);
#endif
	
}

void AXVBarChart::Set3DHistogramChart(EHistogramChartStyle InHistogramChartStyle, EHistogramChartShape InHistogramChartShape)
{
	HistogramChartStyle = InHistogramChartStyle;
	HistogramChartShape = InHistogramChartShape;

	switch (HistogramChartStyle)
	{
	case EHistogramChartStyle::Gradient:
		FinalMaterial = GradientMaterial;
		break;
	case EHistogramChartStyle::Transparent:
		FinalMaterial = TransparentMaterial;
		break;
	case EHistogramChartStyle::Dynamic1:
		FinalMaterial = DynamicMaterial1;
		break;
	case EHistogramChartStyle::Dynamic2:
		FinalMaterial = DynamicMaterial2;
		break;
	default:
		FinalMaterial = BaseMaterial;
		break;
	}
}

void AXVBarChart::SetValue(const FString& InValue)
{
	if (InValue.IsEmpty())
		return;
	XYZs.Empty();
	HeightValues.Empty();
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InValue);

	TArray<TSharedPtr<FJsonValue>> Value3DJsonValueArray;
	TotalCountOfValue = 0;
	if (FJsonSerializer::Deserialize(Reader, Value3DJsonValueArray))
	{
		for (const TSharedPtr<FJsonValue>& Value3DJsonValue : Value3DJsonValueArray)
		{
			TArray<TSharedPtr<FJsonValue>> Values = Value3DJsonValue->AsArray();

			if (Values.Num() != 3)
				return;
			int Y = Values[0]->AsNumber();
			int X = Values[1]->AsNumber();
			float V = Values[2]->AsNumber();
			HeightValues.Add(V);
			MaxX = FMath::Max(MaxX, X);
			MinX = FMath::Min(MinX, X);
			MaxY = FMath::Max(MaxY, Y);
			MinY = FMath::Min(MinY, Y);
			MaxZ = FMath::Max(MaxZ, V);
			MinZ = FMath::Min(MinZ, V);

			if (XYZs.Contains(Y))
			{
				XYZs[Y].Add(X, V);
			}
			else
			{
				TMap<int, float> M;
				M.Add(X, V);
				XYZs.Add(Y, M);
			}

			ColCounts = FMath::Max(ColCounts, XYZs[Y].Num());
			TotalCountOfValue++;
		}
	}

	RowCounts = XYZs.Num();
	DynamicMaterialInstances.Empty();
	DynamicMaterialInstances.SetNum(TotalCountOfValue + 1);
	SectionSelectStates.Init(false, TotalCountOfValue);
	SectionsHeight.SetNum(TotalCountOfValue + 1);

	// 只有不启用GPU生成的时候才生成CPU数据
	if(!bEnableGPU)
	{
		GenerateAllMeshInfo();
	}
}

void AXVBarChart::GenerateAllMeshInfo()
{
	if (TotalCountOfValue == 0)
	{
		return;
	}

	PrepareMeshSections();
	// 创建对应柱体
	{
		size_t CurrentIndex = 0;
		for (size_t IndexOfY = 0; IndexOfY < RowCounts; IndexOfY++)
		{
			for (size_t IndexOfX = 0; IndexOfX < XYZs[IndexOfY].Num(); IndexOfX++)
			{
				FVector Position(XAxisInterval * IndexOfX, YAxisInterval * IndexOfY, 0);
				float Height = XYZs[IndexOfY][IndexOfX] + 0.1;

				double Percentage = static_cast<double>(XYZs[IndexOfY][IndexOfX]) / static_cast<double>(MaxZ);
				int ColorIndex = FMath::Floor(Percentage * (Colors.Num() - 1));
				
				switch (HistogramChartShape)
				{
				case EHistogramChartShape::Bar:
					XVChartUtils::CreateBox(SectionInfos, CurrentIndex, Position, Length, Width, Height, Height,Colors[ColorIndex]);
					break;
				case EHistogramChartShape::Circle:
					UE_LOG(LogTemp, Warning, TEXT("Circle shaped not implemented!"));
					break;
				case EHistogramChartShape::Round:
					UE_LOG(LogTemp, Warning, TEXT("Round shaped not implemented!"));
					break;
				default:
					UE_LOG(LogTemp, Error, TEXT("Error HistogramChartShape!"));	
					break;
				}

				DynamicMaterialInstances[CurrentIndex] = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue("EmissiveColor", EmissiveColor);
				ProceduralMeshComponent->SetMaterial(CurrentIndex, DynamicMaterialInstances[CurrentIndex]);
				SectionsHeight[CurrentIndex] = Height;
				LabelComponents[CurrentIndex] = XVChartUtils::CreateTextRenderComponent(this,UKismetTextLibrary::Conv_IntToText(SectionsHeight[CurrentIndex]),FColor::Cyan, false );
			
				CurrentIndex++;
			}
		}
	}
}

void AXVBarChart::ConstructMesh(double Rate)
{
	Super::ConstructMesh(Rate);

	if (TotalCountOfValue == 0)
	{
		return;
	}

	Rate = FMath::Clamp<double>(Rate, 0.f, 1.f);

	UpdateSectionVerticesOfZ(Rate);

	for (size_t CurrentIndex = 0; CurrentIndex < TotalCountOfValue; CurrentIndex++)
	{
		DrawMeshSection(CurrentIndex);
	}
}

void AXVBarChart::UpdateOnMouseEnterOrLeft()
{
	if (bIsMouseEntered)
	{
		FHitResult HitResult = XVChartUtils::GetCursorHitResult(GetWorld());

		if (HitResult.GetActor())
		{
			FVector Result = GetCursorHitRowAndColAndHeight(HitResult);

			int CurrentRow = Result.Y / (YAxisInterval * GetActorScale3D().Y);
			int CurrentCol = Result.X / (XAxisInterval * GetActorScale3D().X);
			int CurrentIndex = CurrentRow * ColCounts + CurrentCol;
			if (CurrentIndex < TotalCountOfValue)
			{
				if (HoveredIndex != -1 && HoveredIndex != CurrentIndex)
				{
					DynamicMaterialInstances[HoveredIndex]->SetScalarParameterValue("EmissiveIntensity", 0);
					UpdateMeshSection(HoveredIndex);
					LabelComponents[HoveredIndex]->SetVisibility(false);
					LabelComponents[HoveredIndex]->MarkRenderStateDirty();
				}
				if (HoveredIndex != CurrentIndex)
				{
					HoveredIndex = CurrentIndex;
					DynamicMaterialInstances[HoveredIndex]->SetScalarParameterValue("EmissiveIntensity", EmissiveIntensity);
					UpdateMeshSection(HoveredIndex);

					FQuat QuatRotation = FQuat(GetActorRotation());
					FVector Position(XAxisInterval * CurrentCol, YAxisInterval * CurrentRow, 0);
					FVector NewLocation = GetActorLocation() + QuatRotation.RotateVector(Position + FVector(Width * .5, Length * .5, SectionsHeight[HoveredIndex] + 5)) * GetActorScale3D();
					LabelComponents[HoveredIndex]->SetWorldScale3D(GetActorScale3D() * 0.5f);
					LabelComponents[HoveredIndex]->SetWorldLocation(NewLocation);

					FRotator CamRotation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraRotation();
					CamRotation.Yaw += 180;
					CamRotation.Pitch *= -1;
					FRotator TextRotation(CamRotation);
					LabelComponents[HoveredIndex]->SetWorldRotation(TextRotation);
					LabelComponents[HoveredIndex]->SetVisibility(true);
					LabelComponents[HoveredIndex]->MarkRenderStateDirty();
				}
			}
		}
	}
	else
	{
		if (HoveredIndex != -1)
		{
			DynamicMaterialInstances[HoveredIndex]->SetScalarParameterValue("EmissiveIntensity", 0);
			LabelComponents[HoveredIndex]->SetVisibility(false);
			LabelComponents[HoveredIndex]->MarkRenderStateDirty();
			HoveredIndex = -1;
		}
	}
}
