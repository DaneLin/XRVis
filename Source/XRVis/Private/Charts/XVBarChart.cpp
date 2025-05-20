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
	
	// 初始化统计轴线相关数组
	StatisticalLineMeshes.Empty();
	StatisticalLineLabels.Empty();
}

void AXVBarChart::UpdateAxis()
{
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
			if (bEnableGPU)
			{
				ChartAxis->SetAxisGridNum(XAxisLabels.Num(), YAxisLabels.Num(),std::max(5, int(MaxZ/ChartAxis->zAxisInterval)));
			}
		}
	}
}

// Called when the game starts or when spawned
void AXVBarChart::BeginPlay()
{
	Super::BeginPlay();
	
	// Z轴自动调整已移至GenerateAllMeshInfo方法中，确保只在数据加载完成后才进行调整

	if (bAutoLoadData && bEnableGPU)
	{
		DrawWithGPU();
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
// #if WITH_EDITOR
// 	ConstructMesh(1);
// #endif
	
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
		UpdateAxis();
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
	size_t ActualSectionInfoCount = 0;
	for (size_t LODIndex = 0; LODIndex < GenerateLODCount; ++LODIndex)
	{
		auto& LODInfo = LODInfos[LODIndex];
		LODInfo.LODOffset = ActualSectionInfoCount;
		size_t CurrentIndex = 0;
		for (size_t IndexOfY = 0; IndexOfY < RowCounts; IndexOfY += LODIndex + 1)
		{
			for (size_t IndexOfX = 0; IndexOfX < XYZs[IndexOfY].Num(); )
			{
				FVector Position(XAxisInterval * IndexOfX, YAxisInterval * IndexOfY, 0);

				float MergedHeight = 0;
				for (size_t StepX = 0; StepX <= LODIndex &&  IndexOfX < XYZs[IndexOfY].Num(); ++StepX,++IndexOfX)
				{
					for (size_t StepY = 0; StepY <= LODIndex && IndexOfY  + StepY< RowCounts; ++StepY)
					{
						MergedHeight += XYZs[IndexOfY + StepY][IndexOfX];
					}
				}
				
				// 获取原始高度
				float RawHeight = MergedHeight / ((LODIndex + 1) * (LODIndex + 1));
				
				// 应用Z轴调整
				float AdjustedHeight = CalculateAdjustedHeight(RawHeight) + 0.1;
				
				// 计算原始高度的百分比(相对于最大值)
				double Percentage = static_cast<double>(RawHeight) / static_cast<double>(MaxZ);
				int ColorIndex = FMath::Floor(Percentage * (Colors.Num() - 1));

				size_t CreatedSectionIndex = CurrentIndex + ActualSectionInfoCount;

				// TODO: Implement more styles
				switch (HistogramChartShape)
				{
				case EHistogramChartShape::Bar:
					XVChartUtils::CreateBox(SectionInfos, CreatedSectionIndex, Position, Length * (LODIndex + 1), Width * (LODIndex + 1), AdjustedHeight, AdjustedHeight, Colors[ColorIndex]);
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

				if (LODIndex == 0)
				{
					DynamicMaterialInstances[CurrentIndex] = UMaterialInstanceDynamic::Create(BaseMaterial, this);
					DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue("EmissiveColor", EmissiveColor);
				}
				ProceduralMeshComponent->SetMaterial(CreatedSectionIndex, DynamicMaterialInstances[CurrentIndex]);
				SectionsHeight[CurrentIndex] = AdjustedHeight;
				
				// 使用原始高度值作为标签文本
				LabelComponents[CurrentIndex] = XVChartUtils::CreateTextRenderComponent(this, FText::FromString(FString::Printf(TEXT("%.2f"), RawHeight)), FColor::Cyan, false);
			
				++CurrentIndex;
			}
		}
		ActualSectionInfoCount += CurrentIndex;
		LODInfo.LODCount = CurrentIndex;
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

	// 如果启用了自动调整Z轴，先进行自动调整
	if (bAutoAdjustZAxis)
	{
		AutoAdjustZAxis(ZAxisMarginPercent);
	}
}

void AXVBarChart::DrawWithGPU()
{
	Super::DrawWithGPU();
	UpdateAxis();
	if(!HeightValues.IsEmpty() && HeightValues.Num() == XAxisLabels.Num() * YAxisLabels.Num())
	{
		FXRVisBoxGeometryParams Params;
		Params.RowCount = XAxisLabels.Num();
		Params.ColumnCount = YAxisLabels.Num();
		Params.HeightValues = HeightValues;
		Params.ColorValues = Colors;
		static_cast<FXRVisBoxGeometryGenerator*>(GeometryGenerator)->SetParameters(Params);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No vaild height value array or row and column counts not equal to height value counts"));
	}
}

void AXVBarChart::GenerateLOD()
{
	Super::GenerateLOD();

	
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

	UpdateLOD();
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

// 添加ApplyReferenceHighlight方法实现
void AXVBarChart::ApplyReferenceHighlight()
{
	if (!bEnableReferenceHighlight || TotalCountOfValue == 0)
	{
		return;
	}

	// 遍历所有柱子，检查是否符合参考值条件
	size_t CurrentIndex = 0;
	for (size_t IndexOfY = 0; IndexOfY < RowCounts; IndexOfY++)
	{
		for (size_t IndexOfX = 0; IndexOfX < XYZs[IndexOfY].Num(); IndexOfX++)
		{
			// 使用原始高度值进行比较，而不是调整后的高度
			float RawValue = XYZs[IndexOfY][IndexOfX];
			
			// 检查值是否符合参考值条件
			bool bMatchesReference = CheckAgainstReference(RawValue);
			
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

// 应用统计轴线到柱状图
void AXVBarChart::ApplyStatisticalLines()
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
TArray<float> AXVBarChart::GetAllDataValues() const
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
void AXVBarChart::CreateStatisticalLine(const FXVStatisticalLine& LineInfo)
{
	if (LineInfo.LineType == EStatisticalLineType::None || LineInfo.ActualValue <= 0)
	{
		return;
	}
	
	// 创建一个新的程序化网格组件用于绘制轴线
	UProceduralMeshComponent* LineMesh = NewObject<UProceduralMeshComponent>(this);
	LineMesh->SetupAttachment(RootComponent);
	LineMesh->RegisterComponent();
	
	// 计算轴线位置 - 使用调整后的高度
	float RawLineHeight = LineInfo.ActualValue;
	float LineHeight = CalculateAdjustedHeight(RawLineHeight);
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
		
		// 设置标签文本 - 显示原始值，而非调整后的高度
		FString LabelText = LineInfo.LabelFormat;
		LabelText = LabelText.Replace(TEXT("{value}"), *FString::Printf(TEXT("%.2f"), RawLineHeight));
		Label->SetText(FText::FromString(LabelText));
		
		// 设置标签外观
		Label->SetTextRenderColor(LineInfo.LineColor.ToFColor(true));
		Label->SetWorldSize(10.0f);  // 设置文本大小
		Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Left);
		Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextBottom);
		
		// 设置标签位置
		Label->SetWorldLocation(FVector(0, ChartLength * 0.5f, LineHeight + LineThickness + 1.0f));
		Label->SetWorldRotation(FRotator(90.0f, 0.0f, 90.0f));  // 使文本朝向正确
		
		// 保存标签组件
		StatisticalLineLabels.Add(Label);
	}
	
	// 保存轴线网格组件
	StatisticalLineMeshes.Add(LineMesh);
}
