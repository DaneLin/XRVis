// Fill out your copyright notice in the Description page of Project Settings.


#include "Charts/XVLineChart.h"
#include "Charts/XVChartAxis.h"
#include "ProceduralMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetTextLibrary.h"
#include "PhysicsEngine/ShapeElem.h"


// Sets default values
AXVLineChart::AXVLineChart()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	XAxisInterval = 13;
	YAxisInterval = 13;
	Width = 10;
	
	MaxX = std::numeric_limits<int>::min();
	MinX = std::numeric_limits<int>::max();
	MaxY = std::numeric_limits<int>::min();
	MinY = std::numeric_limits<int>::max();
	MaxZ = std::numeric_limits<float>::min();
	MinZ = std::numeric_limits<float>::max();

	SphereRadius = 10.f;
	NumSphereSlices = 32;
	NumSphereStacks = 16;
	
	// 默认顶点颜色数组
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

	// 初始化统计轴线相关数组
	StatisticalLineMeshes.Empty();
	StatisticalLineLabels.Empty();
}

void AXVLineChart::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);

	FHitResult HitResult = XVChartUtils::GetCursorHitResult(GetWorld());

	if (HitResult.GetActor())
	{
		FVector Result = GetCursorHitRowAndColAndHeight(HitResult);
			
		int CurrentRow = Result.Y / (YAxisInterval * GetActorScale().Y);
		LineSelection[CurrentRow] = !LineSelection[CurrentRow];

		for (size_t col = 0; col < ColCounts; col++)
		{
			int CurrentIndex = CurrentRow * ColCounts + col;
			if (CurrentIndex < TotalCountOfValue)
			{
				if (!LineSelection[CurrentRow])
				{
					ProceduralMeshComponent->ClearMeshSection(CurrentIndex);
					DynamicMaterialInstances[CurrentIndex]->SetScalarParameterValue("EmissiveIntensity", 0);
					ProceduralMeshComponent->SetMaterial(CurrentIndex,DynamicMaterialInstances[CurrentIndex] );
					ProceduralMeshComponent->CreateMeshSection_LinearColor(
						CurrentIndex,
						SectionInfos[CurrentIndex].Vertices,
						SectionInfos[CurrentIndex].Indices,
						SectionInfos[CurrentIndex].Normals,
						SectionInfos[CurrentIndex].UVs,
						SectionInfos[CurrentIndex].VertexColors,
						SectionInfos[CurrentIndex].Tangents,
						true);
					LabelComponents[CurrentIndex]->SetVisibility(false);
					LabelComponents[CurrentIndex]->MarkRenderStateDirty();
					TotalSelection[CurrentIndex] = false;
				}
				else
				{
					ProceduralMeshComponent->ClearMeshSection(CurrentIndex);
					// TODO: Update hover material
					ProceduralMeshComponent->CreateMeshSection_LinearColor(
						CurrentIndex,
						SectionInfos[CurrentIndex].Vertices,
						SectionInfos[CurrentIndex].Indices,
						SectionInfos[CurrentIndex].Normals,
						SectionInfos[CurrentIndex].UVs,
						SectionInfos[CurrentIndex].VertexColors,
						SectionInfos[CurrentIndex].Tangents,
						true);
					TotalSelection[CurrentIndex] = true;
				}
			}
		}
	}
}


void AXVLineChart::UpdateOnMouseEnterOrLeft()
{
	if (bIsMouseEntered)
	{
		FHitResult HitResult = XVChartUtils::GetCursorHitResult(GetWorld());

		if (HitResult.GetActor())
		{
			FVector Result = GetCursorHitRowAndColAndHeight(HitResult);
			
			int CurrentRow = Result.Y / (YAxisInterval * GetActorScale().Y) ;
			int CurrentCol = Result.X / (XAxisInterval * GetActorScale().X) ;
			int CurrentIndex = CurrentRow * ColCounts + CurrentCol;
	
			if (CurrentIndex < TotalCountOfValue)
			{
				if (HoveredIndex != -1 && HoveredIndex != CurrentIndex && !TotalSelection[HoveredIndex])
				{
					DynamicMaterialInstances[HoveredIndex]->SetScalarParameterValue("EmissiveIntensity", 0);
					UpdateMeshSection(HoveredIndex);
					LabelComponents[HoveredIndex]->SetVisibility(false);
					LabelComponents[HoveredIndex]->MarkRenderStateDirty();
				}
				if (HoveredIndex != CurrentIndex && !TotalSelection[CurrentIndex])
				{
					HoveredIndex = CurrentIndex;
					DynamicMaterialInstances[HoveredIndex]->SetScalarParameterValue("EmissiveIntensity", EmissiveIntensity);
					UpdateMeshSection(HoveredIndex);

					FRotator CamRotation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraRotation();
					CamRotation.Yaw += 180;
					CamRotation.Pitch *= -1;
					FRotator TextRotation(CamRotation);
					LabelComponents[HoveredIndex]->SetWorldRotation(TextRotation);

					FQuat QuatRotation = FQuat(GetActorRotation());
					FVector Position(XAxisInterval * CurrentCol , YAxisInterval * CurrentRow, 0);
					FVector NewLocation =GetActorLocation() + QuatRotation.RotateVector(Position + FVector(Width * .5, YAxisInterval*.5,  SectionsHeight[HoveredIndex] + 5))*GetActorScale3D();
					LabelComponents[HoveredIndex]->SetWorldScale3D(GetActorScale3D() * 0.5f);
					LabelComponents[HoveredIndex]->SetWorldLocation(NewLocation);
					
					LabelComponents[HoveredIndex]->SetVisibility(true);
					LabelComponents[HoveredIndex]->MarkRenderStateDirty();
				}
			}
		}
	}
	else
	{
		if (HoveredIndex != -1 && !TotalSelection[HoveredIndex])
		{
			ProceduralMeshComponent->ClearMeshSection(HoveredIndex);
			DynamicMaterialInstances[HoveredIndex]->SetScalarParameterValue("EmissiveIntensity", 0);
			ProceduralMeshComponent->SetMaterial(HoveredIndex,DynamicMaterialInstances[HoveredIndex] );
			
			ProceduralMeshComponent->CreateMeshSection_LinearColor(
				HoveredIndex,
				SectionInfos[HoveredIndex].Vertices,
				SectionInfos[HoveredIndex].Indices,
				SectionInfos[HoveredIndex].Normals,
				SectionInfos[HoveredIndex].UVs,
				SectionInfos[HoveredIndex].VertexColors,
				SectionInfos[HoveredIndex].Tangents,
				true);
			LabelComponents[HoveredIndex]->SetVisibility(false);
			LabelComponents[HoveredIndex]->MarkRenderStateDirty();
			HoveredIndex = -1;
		}
	}
}

// Called when the game starts or when spawned
void AXVLineChart::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> ChildActors;
	GetAttachedActors(ChildActors);
	for (AActor* Actor : ChildActors)
	{
		AXVChartAxis* ChartAxis = Cast<AXVChartAxis>(Actor);
		if (ChartAxis)
		{
			ChartAxis->SetXAxisText(XText);
			ChartAxis->SetYAxisText(YText);
			ChartAxis->SetZAxisText(ZText);
		}
	}
}

// Called every frame
void AXVLineChart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

void AXVLineChart::Create3DLineChart(const FString& Data, ELineChartStyle ChartStyle, FColor LineChartColor)
{
	Set3DLineChart(ChartStyle, LineChartColor);

	SetValue(Data);

#if WITH_EDITOR
	ConstructMesh(1);
#endif
}

void AXVLineChart::SetValue(const FString& InValue)
{
	if (InValue.IsEmpty())
		return;
	XYZs.Empty();

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
			int V = Values[2]->AsNumber();

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
				TMap<int, int> M;
				M.Add(X, V);
				XYZs.Add(Y, M);
			}
			ColCounts = FMath::Max(ColCounts, XYZs[Y].Num());
			TotalCountOfValue++;
		}
	}
	RowCounts = XYZs.Num();
	LineSelection.SetNum(RowCounts);
	TotalSelection.SetNum(TotalCountOfValue);
	
	DynamicMaterialInstances.Empty();
	DynamicMaterialInstances.SetNum(TotalCountOfValue + 1);
	SectionSelectStates.Init(false, TotalCountOfValue + 1);
	SectionsHeight.SetNum(TotalCountOfValue + 1);

	GenerateAllMeshInfo()
;}

void AXVLineChart::ConstructMesh(double Rate)
{
	Super::ConstructMesh(Rate);
	if (TotalCountOfValue == 0)
	{
		return;
	}

	Rate = FMath::Clamp<double>(Rate, 0.f, 1.f);

	UpdateSectionVerticesOfZ(Rate);

	for (size_t CurrentIndex =0; CurrentIndex < TotalCountOfValue; CurrentIndex++)
	{
		ProceduralMeshComponent->CreateMeshSection_LinearColor(CurrentIndex,
																   SectionInfos[CurrentIndex].Vertices,
																   SectionInfos[CurrentIndex].Indices,
																   SectionInfos[CurrentIndex].Normals,
																   SectionInfos[CurrentIndex].UVs,
																   SectionInfos[CurrentIndex].VertexColors,
																   SectionInfos[CurrentIndex].Tangents,
																   true);
	}
}

void AXVLineChart::GenerateAllMeshInfo()
{
	if (TotalCountOfValue == 0)
	{
		return;
	}
	PrepareMeshSections();
	
	int CurrentIndex = 0;
	for (int IndexOfY = 0; IndexOfY < RowCounts; IndexOfY++)
	{
		CurrentRolCounts = XYZs[IndexOfY].Num();

		for (int IndexOfX = 0; IndexOfX < CurrentRolCounts; ++IndexOfX)
		{
			FVector Position(XAxisInterval * IndexOfX, YAxisInterval * IndexOfY, 0);

			int NewIndexOfX = FMath::Min(CurrentRolCounts - 1, IndexOfX + 1);

			float Height = XYZs[IndexOfY][IndexOfX];
			float NextHeight = XYZs[IndexOfY][NewIndexOfX];
			SectionsHeight[CurrentIndex] = FMath::Max(Height,NextHeight);

			if(LineChartStyle != ELineChartStyle::Point)
			{
				XVChartUtils::CreateBox(SectionInfos, CurrentIndex, Position, YAxisInterval, Width, Height , NextHeight, Colors[IndexOfY % Colors.Num()]);
			}
			else
			{
				XVChartUtils::CreateSphere(SectionInfos, CurrentIndex, Position + FVector(0,0,Height), SphereRadius, NumSphereSlices, NumSphereStacks, Colors[IndexOfY % Colors.Num()]);
			}
			DynamicMaterialInstances[CurrentIndex] = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
			ProceduralMeshComponent->SetMaterial(CurrentIndex, DynamicMaterialInstances[CurrentIndex]);
			// Create Text Component
			LabelComponents[CurrentIndex] = XVChartUtils::CreateTextRenderComponent(this,UKismetTextLibrary::Conv_IntToText(SectionsHeight[CurrentIndex]),FColor::Cyan, false );
			
			CurrentIndex++;
		}
	}

	// 如果启用了参考值高亮，应用高亮效果
	if (bEnableReferenceHighlight)
	{
		ApplyReferenceHighlight();
	}
	
	// 如果启用了统计轴线，应用统计轴线
	if (bEnableStatisticalLines)
	{
		UpdateStatisticalLineValues();
		ApplyStatisticalLines();
	}
}

#if WITH_EDITOR
void AXVLineChart::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	
}
#endif


void AXVLineChart::Set3DLineChart(ELineChartStyle InLineChartStyle, FColor LineChartColor)
{
	LineChartStyle = InLineChartStyle;
	switch (LineChartStyle)
	{
	case ELineChartStyle::Base:
		
		break;
	case ELineChartStyle::Gradient:
		
		break;
	case ELineChartStyle::Transparent:
		
		break;
	default:
		
		break;
	}
}

// 添加ApplyReferenceHighlight方法实现
void AXVLineChart::ApplyReferenceHighlight()
{
	if (!bEnableReferenceHighlight || TotalCountOfValue == 0)
	{
		return;
	}

	// 遍历所有线段/点，检查是否符合参考值条件
	size_t CurrentIndex = 0;
	for (int32 RowIndex = 0; RowIndex < XYZs.Num(); RowIndex++)
	{
		const auto& Row = XYZs[RowIndex];
		for (const auto& XZPair : Row)
		{
			int Value = XZPair.Value; // Z值
			
			// 检查值是否符合参考值条件
			bool bMatchesReference = CheckAgainstReference(Value);
			
			if (bMatchesReference)
			{
				// 符合条件，应用高亮颜色和发光效果
				DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue("EmissiveColor", ReferenceHighlightColor);
				DynamicMaterialInstances[CurrentIndex]->SetScalarParameterValue("EmissiveIntensity", EmissiveIntensity);
			}
			else
			{
				// 不符合条件，恢复默认颜色和发光效果
				DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue("EmissiveColor", EmissiveColor);
				DynamicMaterialInstances[CurrentIndex]->SetScalarParameterValue("EmissiveIntensity", 0);
			}
			
			// 更新网格部分
			UpdateMeshSection(CurrentIndex);
			
			CurrentIndex++;
		}
	}
}

// 应用统计轴线到线图
void AXVLineChart::ApplyStatisticalLines()
{
	// 清除现有的统计轴线
	for (auto* Mesh : StatisticalLineMeshes)
	{
		if (Mesh)
		{
			Mesh->DestroyComponent();
		}
	}
	StatisticalLineMeshes.Empty();
	
	for (auto* Label : StatisticalLineLabels)
	{
		if (Label)
		{
			Label->DestroyComponent();
		}
	}
	StatisticalLineLabels.Empty();
	
	if (!bEnableStatisticalLines || StatisticalLines.Num() == 0)
	{
		return;
	}
	
	// 计算图表的总宽度和高度
	float ChartWidth = MaxX * XAxisInterval;
	float ChartHeight = MaxY * YAxisInterval;
	
	// 为每条统计轴线创建可视化效果
	for (const auto& Line : StatisticalLines)
	{
		CreateStatisticalLine(Line);
	}
}

// 获取所有数据值
TArray<float> AXVLineChart::GetAllDataValues() const
{
	TArray<float> Values;
	
	// 收集所有Z值（高度值）
	for (const auto& Row : XYZs)
	{
		for (const auto& Item : Row.Value)
		{
			Values.Add(Item.Value);
		}
	}
	
	return Values;
}

// 创建一条统计轴线
void AXVLineChart::CreateStatisticalLine(const FXVStatisticalLine& LineInfo)
{
	if (LineInfo.LineType == EStatisticalLineType::None || LineInfo.ActualValue <= 0)
	{
		return;
	}
	
	// 创建一个新的程序化网格组件用于绘制轴线
	UProceduralMeshComponent* LineMesh = NewObject<UProceduralMeshComponent>(this);
	LineMesh->SetupAttachment(RootComponent);
	LineMesh->RegisterComponent();
	
	// 计算轴线位置
	float LineHeight = LineInfo.ActualValue;
	float ChartWidth = (MaxX + 1) * XAxisInterval;
	float ChartLength = (MaxY + 1) * YAxisInterval;
	
	// 创建轴线几何体
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	
	// 轴线的厚度
	float LineThickness = LineInfo.LineWidth;
	
	// 创建一个水平平面作为轴线
	Vertices.Add(FVector(0, 0, LineHeight));
	Vertices.Add(FVector(ChartWidth, 0, LineHeight));
	Vertices.Add(FVector(ChartWidth, ChartLength, LineHeight));
	Vertices.Add(FVector(0, ChartLength, LineHeight));
	
	Vertices.Add(FVector(0, 0, LineHeight + LineThickness));
	Vertices.Add(FVector(ChartWidth, 0, LineHeight + LineThickness));
	Vertices.Add(FVector(ChartWidth, ChartLength, LineHeight + LineThickness));
	Vertices.Add(FVector(0, ChartLength, LineHeight + LineThickness));
	
	// 添加三角形（两个面）
	// 底面
	Triangles.Add(0); Triangles.Add(1); Triangles.Add(2);
	Triangles.Add(0); Triangles.Add(2); Triangles.Add(3);
	
	// 顶面
	Triangles.Add(4); Triangles.Add(6); Triangles.Add(5);
	Triangles.Add(4); Triangles.Add(7); Triangles.Add(6);
	
	// 侧面1
	Triangles.Add(0); Triangles.Add(4); Triangles.Add(1);
	Triangles.Add(1); Triangles.Add(4); Triangles.Add(5);
	
	// 侧面2
	Triangles.Add(1); Triangles.Add(5); Triangles.Add(2);
	Triangles.Add(2); Triangles.Add(5); Triangles.Add(6);
	
	// 侧面3
	Triangles.Add(2); Triangles.Add(6); Triangles.Add(3);
	Triangles.Add(3); Triangles.Add(6); Triangles.Add(7);
	
	// 侧面4
	Triangles.Add(3); Triangles.Add(7); Triangles.Add(0);
	Triangles.Add(0); Triangles.Add(7); Triangles.Add(4);
	
	// 设置法线、UV和颜色
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		Normals.Add(FVector(0, 0, 1));
		UV0.Add(FVector2D(0, 0));
		VertexColors.Add(LineInfo.LineColor);
		Tangents.Add(FProcMeshTangent(1, 0, 0));
	}
	
	// 创建网格
	LineMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
	
	// 创建材质实例
	UMaterialInstanceDynamic* LineMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	LineMaterial->SetVectorParameterValue("EmissiveColor", LineInfo.LineColor);
	LineMaterial->SetScalarParameterValue("EmissiveIntensity", 1.0f);
	LineMesh->SetMaterial(0, LineMaterial);
	
	// 如果需要标签，则创建文本组件
	if (LineInfo.bShowLabel)
	{
		UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this);
		Label->SetupAttachment(RootComponent);
		Label->RegisterComponent();
		
		// 设置标签文本
		FString LabelText = LineInfo.LabelFormat;
		LabelText = LabelText.Replace(TEXT("{value}"), *FString::Printf(TEXT("%.2f"), LineInfo.ActualValue));
		Label->SetText(FText::FromString(LabelText));
		
		// 设置标签外观
		Label->SetTextRenderColor(LineInfo.LineColor.ToFColor(true));
		Label->SetWorldSize(10.0f);  // 设置文本大小
		Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Left);
		Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextBottom);
		
		// 设置标签位置（对于线图，可能需要调整位置）
		Label->SetWorldLocation(FVector(0, ChartLength * 0.5f, LineHeight + LineThickness + 1.0f));
		Label->SetWorldRotation(FRotator(90.0f, 0.0f, 90.0f));  // 使文本朝向正确
		
		// 保存标签组件
		StatisticalLineLabels.Add(Label);
	}
	
	// 保存轴线网格组件
	StatisticalLineMeshes.Add(LineMesh);
}
