// Fill out your copyright notice in the Description page of Project Settings.

#include "Charts/XVLineChart.h"

#include "Charts/XVChartAxis.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetTextLibrary.h"
#include "PhysicsEngine/ShapeElem.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AXVLineChart::AXVLineChart()
{
	// Set this actor to call Tick() every frame.  You can turn this off to
	// improve performance if you don't need it.
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
	Colors.Add(FColor::FromHex("#313695"));
	Colors.Add(FColor::FromHex("#4575b4"));
	Colors.Add(FColor::FromHex("#74add1"));
	Colors.Add(FColor::FromHex("#abd9e9"));
	Colors.Add(FColor::FromHex("#e0f3f8"));
	Colors.Add(FColor::FromHex("#ffffbf"));
	Colors.Add(FColor::FromHex("#fee090"));
	Colors.Add(FColor::FromHex("#fdae61"));
	Colors.Add(FColor::FromHex("#f46d43"));
	Colors.Add(FColor::FromHex("#d73027"));
	Colors.Add(FColor::FromHex("#a50026"));

	XVChartUtils::LoadResourceFromPath(
		TEXT("Material'/XRVis/Materials/M_BaseVertexColor.M_BaseVertexColor'"),
		BaseMaterial);

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
					DynamicMaterialInstances[CurrentIndex]->SetScalarParameterValue(
						"EmissiveIntensity", 0);
					ProceduralMeshComponent->SetMaterial(
						CurrentIndex, DynamicMaterialInstances[CurrentIndex]);
					ProceduralMeshComponent->CreateMeshSection_LinearColor(
						CurrentIndex, SectionInfos[CurrentIndex].Vertices,
						SectionInfos[CurrentIndex].Indices,
						SectionInfos[CurrentIndex].Normals,
						SectionInfos[CurrentIndex].UVs,
						SectionInfos[CurrentIndex].VertexColors,
						SectionInfos[CurrentIndex].Tangents, true);
					LabelComponents[CurrentIndex]->SetVisibility(false);
					LabelComponents[CurrentIndex]->MarkRenderStateDirty();
					TotalSelection[CurrentIndex] = false;
				}
				else
				{
					ProceduralMeshComponent->ClearMeshSection(CurrentIndex);
					ProceduralMeshComponent->CreateMeshSection_LinearColor(
						CurrentIndex, SectionInfos[CurrentIndex].Vertices,
						SectionInfos[CurrentIndex].Indices,
						SectionInfos[CurrentIndex].Normals,
						SectionInfos[CurrentIndex].UVs,
						SectionInfos[CurrentIndex].VertexColors,
						SectionInfos[CurrentIndex].Tangents, true);
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

			int CurrentRow = Result.Y / (YAxisInterval * GetActorScale().Y);
			int CurrentCol = Result.X / (XAxisInterval * GetActorScale().X);
			int CurrentIndex = CurrentRow * ColCounts + CurrentCol;

			if (CurrentIndex < TotalCountOfValue)
			{
				if (HoveredIndex != -1 && HoveredIndex != CurrentIndex &&
					!TotalSelection[HoveredIndex])
				{
					DynamicMaterialInstances[HoveredIndex]->SetScalarParameterValue(
						"EmissiveIntensity", 0);
					UpdateMeshSection(HoveredIndex);
					LabelComponents[HoveredIndex]->SetVisibility(false);
					LabelComponents[HoveredIndex]->MarkRenderStateDirty();
				}
				if (HoveredIndex != CurrentIndex && !TotalSelection[CurrentIndex])
				{
					HoveredIndex = CurrentIndex;
					DynamicMaterialInstances[HoveredIndex]->SetScalarParameterValue(
						"EmissiveIntensity", EmissiveIntensity);
					UpdateMeshSection(HoveredIndex);

					FRotator CamRotation = GetWorld()
					                       ->GetFirstPlayerController()
					                       ->PlayerCameraManager->GetCameraRotation();
					CamRotation.Yaw += 180;
					CamRotation.Pitch *= -1;
					FRotator TextRotation(CamRotation);
					LabelComponents[HoveredIndex]->SetWorldRotation(TextRotation);

					FQuat QuatRotation = FQuat(GetActorRotation());
					FVector Position(XAxisInterval * CurrentCol,
					                 YAxisInterval * CurrentRow, 0);
					FVector NewLocation =
						GetActorLocation() +
						QuatRotation.RotateVector(
							Position + FVector(Width * .5, YAxisInterval * .5,
							                   SectionsHeight[HoveredIndex] + 5)) *
						GetActorScale3D();
					LabelComponents[HoveredIndex]->SetWorldScale3D(GetActorScale3D() *
						0.5f);
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
			DynamicMaterialInstances[HoveredIndex]->SetScalarParameterValue(
				"EmissiveIntensity", 0);
			ProceduralMeshComponent->SetMaterial(
				HoveredIndex, DynamicMaterialInstances[HoveredIndex]);

			ProceduralMeshComponent->CreateMeshSection_LinearColor(
				HoveredIndex, SectionInfos[HoveredIndex].Vertices,
				SectionInfos[HoveredIndex].Indices,
				SectionInfos[HoveredIndex].Normals, SectionInfos[HoveredIndex].UVs,
				SectionInfos[HoveredIndex].VertexColors,
				SectionInfos[HoveredIndex].Tangents, true);
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

	// 只有在没有启用时间轴功能或时间轴没有自动播放时才执行入场动画
	if (!bEnableTimelinePlayback || !bAutoPlayTimeline)
	{
		if (bEnableEnterAnimation)
		{
			if (CurrentBuildTime < BuildTime && !IsHidden())
			{
				ConstructMesh(CurrentBuildTime / BuildTime);
				CurrentBuildTime += DeltaTime;
			}
		}
		else
		{
			if (!bAnimationFinished)
			{
				ConstructMesh(1);
				bAnimationFinished = true;
			}
		}
	}
	
	UpdateOnMouseEnterOrLeft();
}

void AXVLineChart::Create3DLineChart(const FString& Data,
                                     ELineChartStyle ChartStyle,
                                     FColor LineChartColor)
{
	Set3DLineChart(ChartStyle, LineChartColor);

	SetValue(Data);
	
}

void AXVLineChart::SetValue(const FString& InValue)
{
	if (InValue.IsEmpty()) return;
	XYZs.Empty();
	// 清空时间数据
	TimeData.Empty();

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InValue);

	TArray<TSharedPtr<FJsonValue>> Value3DJsonValueArray;

	TotalCountOfValue = 0;
	bool bHasTimeProperty = false;
	
	// 尝试判断数据格式
	TSharedPtr<FJsonValue> JsonValue;
	if (FJsonSerializer::Deserialize(Reader, JsonValue) && JsonValue.IsValid())
	{
		// 检查是否为数组格式
		if (JsonValue->Type == EJson::Array)
		{
			TArray<TSharedPtr<FJsonValue>> JsonArray = JsonValue->AsArray();
			
			// 如果数组为空，直接返回
			if (JsonArray.Num() == 0)
				return;
			
			// 检查第一个元素以确定数据格式
			TSharedPtr<FJsonValue> FirstElement = JsonArray[0];
			
			// 检查是否为对象格式 [{"x": ..., "y": ..., "z": ..., "time": ...}, ...]
			if (FirstElement->Type == EJson::Object)
			{
				// 使用命名数据处理方式
				TArray<TSharedPtr<FJsonObject>> NamedData;
				for (const auto& Item : JsonArray)
				{
					if (Item->Type == EJson::Object)
					{
						NamedData.Add(Item->AsObject());
					}
				}
				
				// 解析命名数据
				ParseNamedDataWithTime(NamedData);
				return;
			}
		}
	}
	
	// 如果不是命名对象格式，回退到原始格式处理 [[y,x,z], ...]
	Reader = TJsonReaderFactory<>::Create(InValue);
	if (FJsonSerializer::Deserialize(Reader, Value3DJsonValueArray))
	{
		for (const TSharedPtr<FJsonValue>& Value3DJsonValue :
		     Value3DJsonValueArray)
		{
			TArray<TSharedPtr<FJsonValue>> Values = Value3DJsonValue->AsArray();

			if (Values.Num() < 3) return;
			int Y = Values[0]->AsNumber();
			int X = Values[1]->AsNumber();
			int V = Values[2]->AsNumber();
			
			// 检查是否有第四个值作为时间值
			float Time = 0.0f;
			if (Values.Num() > 3)
			{
				Time = Values[3]->AsNumber();
				bHasTimeProperty = true;
			}
			else
			{
				// 如果没有时间值，使用索引作为默认时间
				Time = TotalCountOfValue;
			}

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
			
			// 保存数据点的索引和值，用于时间轴功能
			FXVTimeDataPoint TimePoint;
			TimePoint.RowIndex = Y;
			TimePoint.ColIndex = X;
			TimePoint.Value = V;
			TimePoint.TimeValue = Time;
			TimePoint.SortKey = bHasTimeProperty ? 0 : (Y * 1000 + X); // 有时间值时不使用排序键
			TimeData.Add(TimePoint);
		}
		
		// 对时间数据进行排序
		if (bHasTimeProperty)
		{
			// 如果有时间属性，按时间值排序
			TimeData.Sort([](const FXVTimeDataPoint& A, const FXVTimeDataPoint& B) {
				return A.TimeValue < B.TimeValue;
			});
		}
		else
		{
			// 否则按排序键排序
			TimeData.Sort([](const FXVTimeDataPoint& A, const FXVTimeDataPoint& B) {
				return A.SortKey < B.SortKey;
			});
		}
	}
	RowCounts = XYZs.Num();
	LineSelection.SetNum(RowCounts);
	TotalSelection.SetNum(TotalCountOfValue);

	DynamicMaterialInstances.Empty();
	DynamicMaterialInstances.SetNum(TotalCountOfValue + 1);
	SectionSelectStates.Init(false, TotalCountOfValue + 1);
	SectionsHeight.SetNum(TotalCountOfValue + 1);

	GenerateAllMeshInfo();
}

/**
 * 解析具有命名属性的数据，包括时间属性
 */
void AXVLineChart::ParseNamedDataWithTime(const TArray<TSharedPtr<FJsonObject>>& NamedData)
{
	if (NamedData.IsEmpty())
		return;
		
	// 检查是否有时间属性
	bool bHasTimeProperty = false;
	
	// 收集所有唯一的X和Y值
	TSet<FString> UniqueXValues;
	TSet<FString> UniqueYValues;
	
	// 第一轮扫描收集所有唯一值
	for (const auto& DataItem : NamedData)
	{
		// 检查时间属性
		if (DataItem->HasField(PropertyMapping.TimeProperty))
		{
			bHasTimeProperty = true;
		}
		
		// 检查X和Y属性
		if (DataItem->HasField(PropertyMapping.XProperty) &&
			DataItem->HasField(PropertyMapping.YProperty))
		{
			// 获取X值
			FString XValue;
			if (DataItem->TryGetStringField(PropertyMapping.XProperty, XValue))
			{
				UniqueXValues.Add(XValue);
			}
			else
			{
				double XNumeric = 0;
				if (DataItem->TryGetNumberField(PropertyMapping.XProperty, XNumeric))
				{
					XValue = FString::Printf(TEXT("%g"), XNumeric);
					UniqueXValues.Add(XValue);
				}
			}

			// 获取Y值
			FString YValue;
			if (DataItem->TryGetStringField(PropertyMapping.YProperty, YValue))
			{
				UniqueYValues.Add(YValue);
			}
			else
			{
				double YNumeric = 0;
				if (DataItem->TryGetNumberField(PropertyMapping.YProperty, YNumeric))
				{
					YValue = FString::Printf(TEXT("%g"), YNumeric);
					UniqueYValues.Add(YValue);
				}
			}
		}
	}
	
	// 转换为排序数组
	TArray<FString> SortedXValues = UniqueXValues.Array();
	TArray<FString> SortedYValues = UniqueYValues.Array();
	
	// 自定义排序逻辑：如果是纯数字则按照数值排序，否则按照字符串排序
	auto NumericSort = [](const FString& A, const FString& B) -> bool
	{
		// 检查是否都是数字
		bool bAIsNumeric = true;
		bool bBIsNumeric = true;
		double ANumeric = 0.0;
		double BNumeric = 0.0;

		// 尝试将字符串转换为数字
		if (A.IsNumeric() && B.IsNumeric())
		{
			ANumeric = FCString::Atod(*A);
			BNumeric = FCString::Atod(*B);
			// 按数值排序
			return ANumeric < BNumeric;
		}
		// 否则使用默认的字符串排序
		return A < B;
	};
	
	// 使用自定义排序逻辑
	SortedXValues.Sort(NumericSort);
	SortedYValues.Sort(NumericSort);
	
	// 创建映射字典
	TMap<FString, int32> XValueToIndex;
	TMap<FString, int32> YValueToIndex;

	for (int32 i = 0; i < SortedXValues.Num(); i++)
	{
		XValueToIndex.Add(SortedXValues[i], i);
	}

	for (int32 i = 0; i < SortedYValues.Num(); i++)
	{
		YValueToIndex.Add(SortedYValues[i], i);
	}
	
	// 存储轴标签
	XText = SortedXValues;
	YText = SortedYValues;
	
	// 第二轮扫描处理数据
	for (const auto& DataItem : NamedData)
	{
		if (DataItem->HasField(PropertyMapping.XProperty) &&
			DataItem->HasField(PropertyMapping.YProperty) &&
			DataItem->HasField(PropertyMapping.ZProperty))
		{
			// 获取X值和索引
			FString XValue;
			int32 XIndex = 0;

			if (DataItem->TryGetStringField(PropertyMapping.XProperty, XValue))
			{
				XIndex = XValueToIndex.FindRef(XValue);
			}
			else
			{
				double XNumeric = 0;
				if (DataItem->TryGetNumberField(PropertyMapping.XProperty, XNumeric))
				{
					XValue = FString::Printf(TEXT("%g"), XNumeric);
					XIndex = XValueToIndex.FindRef(XValue);
				}
			}

			// 获取Y值和索引
			FString YValue;
			int32 YIndex = 0;

			if (DataItem->TryGetStringField(PropertyMapping.YProperty, YValue))
			{
				YIndex = YValueToIndex.FindRef(YValue);
			}
			else
			{
				double YNumeric = 0;
				if (DataItem->TryGetNumberField(PropertyMapping.YProperty, YNumeric))
				{
					YValue = FString::Printf(TEXT("%g"), YNumeric);
					YIndex = YValueToIndex.FindRef(YValue);
				}
			}

			// 获取Z值
			double ZValue = 0;
			DataItem->TryGetNumberField(PropertyMapping.ZProperty, ZValue);
			
			// 获取时间值
			float TimeValue = TotalCountOfValue; // 默认使用计数作为时间
			if (bHasTimeProperty)
			{
				double TimeNumeric = 0;
				if (DataItem->TryGetNumberField(PropertyMapping.TimeProperty, TimeNumeric))
				{
					TimeValue = TimeNumeric;
				}
				else
				{
					FString TimeString;
					if (DataItem->TryGetStringField(PropertyMapping.TimeProperty, TimeString))
					{
						// 尝试将字符串转换为数字（简单处理）
						if (TimeString.IsNumeric())
						{
							TimeValue = FCString::Atof(*TimeString);
						}
						// 可以在这里添加更复杂的时间字符串解析
					}
				}
			}
			
			// 更新数据范围
			MaxX = FMath::Max(MaxX, XIndex);
			MinX = FMath::Min(MinX, XIndex);
			MaxY = FMath::Max(MaxY, YIndex);
			MinY = FMath::Min(MinY, YIndex);
			MaxZ = FMath::Max(MaxZ, ZValue);
			MinZ = FMath::Min(MinZ, ZValue);
			
			// 添加到XYZs数据结构
			if (XYZs.Contains(YIndex))
			{
				XYZs[YIndex].Add(XIndex, ZValue);
			}
			else
			{
				TMap<int, int> M;
				M.Add(XIndex, ZValue);
				XYZs.Add(YIndex, M);
			}
			
			ColCounts = FMath::Max(ColCounts, XYZs[YIndex].Num());
			TotalCountOfValue++;
			
			// 保存数据点的索引和值，用于时间轴功能
			FXVTimeDataPoint TimePoint;
			TimePoint.RowIndex = YIndex;
			TimePoint.ColIndex = XIndex;
			TimePoint.Value = ZValue;
			TimePoint.TimeValue = TimeValue;
			TimePoint.SortKey = bHasTimeProperty ? 0 : (YIndex * 1000 + XIndex); // 有时间值时不使用排序键
			TimeData.Add(TimePoint);
		}
	}
	
	// 对时间数据进行排序
	if (bHasTimeProperty)
	{
		// 如果有时间属性，按时间值排序
		TimeData.Sort([](const FXVTimeDataPoint& A, const FXVTimeDataPoint& B) {
			return A.TimeValue < B.TimeValue;
		});
	}
	else
	{
		// 否则按排序键排序
		TimeData.Sort([](const FXVTimeDataPoint& A, const FXVTimeDataPoint& B) {
			return A.SortKey < B.SortKey;
		});
	}
	
	RowCounts = XYZs.Num();
	LineSelection.SetNum(RowCounts);
	TotalSelection.SetNum(TotalCountOfValue);

	DynamicMaterialInstances.Empty();
	DynamicMaterialInstances.SetNum(TotalCountOfValue + 1);
	SectionSelectStates.Init(false, TotalCountOfValue + 1);
	SectionsHeight.SetNum(TotalCountOfValue + 1);

	GenerateAllMeshInfo();
}

void AXVLineChart::ConstructMesh(double Rate)
{
	Super::ConstructMesh(Rate);
	if (TotalCountOfValue == 0)
	{
		return;
	}

	Rate = FMath::Clamp<double>(Rate, 0.f, 1.f);

	// 如果启用了时间轴，应用特殊处理
	if (bEnableTimelinePlayback)
	{
		// 根据时间轴进度来决定显示哪些数据点
		UpdateMeshBasedOnTimeProgress(Rate);
	}
	else
	{
		// 原有的实现
		UpdateSectionVerticesOfZ(Rate);
	}

	UpdateLOD();
}

/**
 * 根据时间轴进度更新折线图
 */
void AXVLineChart::UpdateMeshBasedOnTimeProgress(double Progress)
{
	// 如果没有数据或时间数据为空，直接返回
	if (TotalCountOfValue == 0 || TimeData.Num() == 0)
	{
		return;
	}
	
	// 更新顶点高度
	BackupVertices();
	
	// 计算当前时间点应该显示的数据点数量
	int MaxPointsToShow = FMath::FloorToInt(TimeData.Num() * Progress);
	
	// 确保至少显示1个点
	MaxPointsToShow = FMath::Max(1, MaxPointsToShow);
	
	// 创建一个集合来记录当前应该显示的点
	TSet<int> VisiblePoints;
	for (int i = 0; i < MaxPointsToShow; ++i)
	{
		const FXVTimeDataPoint& Point = TimeData[i];
		int SectionIndex = Point.RowIndex * ColCounts + Point.ColIndex;
		VisiblePoints.Add(SectionIndex);
	}
	
	// 遍历所有数据点，更新其可见性
	for (int RowIndex = 0; RowIndex < RowCounts; RowIndex++)
	{
		const auto& Row = XYZs[RowIndex];
		for (const auto& XZPair : Row)
		{
			int SectionIndex = RowIndex * ColCounts + XZPair.Key;
			
			// 检查当前点是否应该显示
			bool bShouldShowPoint = VisiblePoints.Contains(SectionIndex);
			
			// 更新该点的所有顶点
			if (bShouldShowPoint)
			{
				FXVChartSectionInfo& XVChartSectionInfo = SectionInfos[SectionIndex];
				for (size_t VerticeIndex = 0; VerticeIndex < XVChartSectionInfo.Vertices.Num(); VerticeIndex++)
				{
					FVector& Vertice = XVChartSectionInfo.Vertices[VerticeIndex];
				
					if (bShouldShowPoint)
					{
						// 完全显示
						Vertice.Z = VerticesBackup[SectionIndex][VerticeIndex].Z;
					}
					
				}
				DrawMeshSection(SectionIndex);
			}
			else
			{
				ProceduralMeshComponent->ClearMeshSection(SectionIndex);
			}
			
			
		}
	}
}

void AXVLineChart::GenerateAllMeshInfo()
{
	if (TotalCountOfValue == 0)
	{
		return;
	}

	// 如果启用了自动调整Z轴，先进行自动调整
	if (bAutoAdjustZAxis)
	{
		AutoAdjustZAxis(ZAxisMarginPercent);
	}

	PrepareMeshSections();

	int LODOffset = 0;
	for (int LODIndex = 0; LODIndex < GenerateLODCount; ++LODIndex)
	{
		int CurrentIndex = 0;
		for (int RowIndex = 0; RowIndex < RowCounts; RowIndex++)
		{
			CurColCount = XYZs[RowIndex].Num();

			for (int ColIndex = 0; ColIndex < CurColCount; ColIndex += LODIndex + 1)
			{
				FVector Position(XAxisInterval * ColIndex, YAxisInterval * RowIndex, 0);

				int NewColIndex = FMath::Min(CurColCount - 1, ColIndex + LODIndex + 1);

				// 获取原始高度
				float RawHeight = XYZs[RowIndex][ColIndex];
				float RawNextHeight = XYZs[RowIndex][NewColIndex];

				// 应用Z轴调整
				float AdjustedHeight = CalculateAdjustedHeight(RawHeight);
				float AdjustedNextHeight = CalculateAdjustedHeight(RawNextHeight);

				if (LODIndex == 0)
				{
					SectionsHeight[CurrentIndex] =
						FMath::Max(AdjustedHeight, AdjustedNextHeight);
				}

				if (LineChartStyle != ELineChartStyle::Point)
				{
					XVChartUtils::CreateBox(SectionInfos, CurrentIndex, Position,
					                        YAxisInterval, Width, AdjustedHeight,
					                        AdjustedNextHeight,
					                        Colors[RowIndex % Colors.Num()]);
				}
				else
				{
					int LODNumSphereSlices = FMath::Max(3, NumSphereSlices - LODIndex);
					int LODNumSphereStacks = FMath::Max(2, NumSphereStacks - LODIndex);

					XVChartUtils::CreateSphere(SectionInfos, CurrentIndex,
					                           Position + FVector(0, 0, AdjustedHeight),
					                           SphereRadius, LODNumSphereSlices,
					                           LODNumSphereStacks,
					                           Colors[RowIndex % Colors.Num()]);
				}

				DynamicMaterialInstances[CurrentIndex] =
					UMaterialInstanceDynamic::Create(BaseMaterial, this);
				DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue(
					TEXT("EmissiveColor"), EmissiveColor);
				ProceduralMeshComponent->SetMaterial(
					CurrentIndex, DynamicMaterialInstances[CurrentIndex]);

				// 使用原始高度值作为标签文本
				LabelComponents[CurrentIndex] = XVChartUtils::CreateTextRenderComponent(
					this, FText::FromString(FString::Printf(TEXT("%.2f"), RawHeight)),
					FColor::Cyan, false);

				CurrentIndex++;
			}
		}
		LODInfos[LODIndex].LODCount = CurrentIndex;
		LODInfos[LODIndex].LODOffset = LODOffset;
		LODOffset += CurrentIndex;
	}

	// 如果启用了参考值高亮，应用高亮效果
	if (bEnableReferenceHighlight)
	{
		ApplyReferenceHighlight();
	}

	// 如果启用了值触发条件，应用触发条件
	if (bEnableValueTriggers)
	{
		ApplyValueTriggerConditions();
	}

	// 如果启用了统计轴线，应用统计轴线
	if (bEnableStatisticalLines)
	{
		UpdateStatisticalLineValues();
		ApplyStatisticalLines();
	}
}

#if WITH_EDITOR
void AXVLineChart::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void AXVLineChart::Set3DLineChart(ELineChartStyle InLineChartStyle,
                                  FColor LineChartColor)
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
	for (int RowIndex = 0; RowIndex < XYZs.Num(); RowIndex++)
	{
		const auto& Row = XYZs[RowIndex];
		for (const auto& XZPair : Row)
		{
			// 使用原始高度值进行比较，而不是调整后的高度
			int RawValue = XZPair.Value; // Z值

			// 检查值是否符合参考值条件
			bool bMatchesReference = CheckAgainstReference(RawValue);

			if (bMatchesReference)
			{
				// 符合条件，应用高亮颜色和发光效果
				DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue(
					"EmissiveColor", ReferenceHighlightColor);
				DynamicMaterialInstances[CurrentIndex]->SetScalarParameterValue(
					"EmissiveIntensity", EmissiveIntensity);
			}
			else
			{
				// 不符合条件，恢复默认颜色和发光效果
				DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue(
					"EmissiveColor", EmissiveColor);
				DynamicMaterialInstances[CurrentIndex]->SetScalarParameterValue(
					"EmissiveIntensity", 0);
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
	if (LineInfo.LineType == EStatisticalLineType::None ||
		LineInfo.ActualValue <= 0)
	{
		return;
	}

	// 创建一个新的程序化网格组件用于绘制轴线
	UProceduralMeshComponent* LineMesh =
		NewObject<UProceduralMeshComponent>(this);
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
	Triangles.Add(0);
	Triangles.Add(1);
	Triangles.Add(2);
	Triangles.Add(0);
	Triangles.Add(2);
	Triangles.Add(3);

	// 顶面
	Triangles.Add(4);
	Triangles.Add(6);
	Triangles.Add(5);
	Triangles.Add(4);
	Triangles.Add(7);
	Triangles.Add(6);

	// 侧面1
	Triangles.Add(0);
	Triangles.Add(4);
	Triangles.Add(1);
	Triangles.Add(1);
	Triangles.Add(4);
	Triangles.Add(5);

	// 侧面2
	Triangles.Add(1);
	Triangles.Add(5);
	Triangles.Add(2);
	Triangles.Add(2);
	Triangles.Add(5);
	Triangles.Add(6);

	// 侧面3
	Triangles.Add(2);
	Triangles.Add(6);
	Triangles.Add(3);
	Triangles.Add(3);
	Triangles.Add(6);
	Triangles.Add(7);

	// 侧面4
	Triangles.Add(3);
	Triangles.Add(7);
	Triangles.Add(0);
	Triangles.Add(0);
	Triangles.Add(7);
	Triangles.Add(4);

	// 设置法线、UV和颜色
	for (int32 i = 0; i < Vertices.Num(); ++i)
	{
		Normals.Add(FVector(0, 0, 1));
		UV0.Add(FVector2D(0, 0));
		VertexColors.Add(LineInfo.LineColor);
		Tangents.Add(FProcMeshTangent(1, 0, 0));
	}

	// 创建网格
	LineMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0,
	                                        VertexColors, Tangents, true);

	// 创建材质实例
	UMaterialInstanceDynamic* LineMaterial =
		UMaterialInstanceDynamic::Create(BaseMaterial, this);
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
		LabelText = LabelText.Replace(
			TEXT("{value}"), *FString::Printf(TEXT("%.2f"), RawLineHeight));
		Label->SetText(FText::FromString(LabelText));

		// 设置标签外观
		Label->SetTextRenderColor(LineInfo.LineColor.ToFColor(true));
		Label->SetWorldSize(10.0f); // 设置文本大小
		Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Left);
		Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextBottom);

		// 设置标签位置（对于线图，可能需要调整位置）
		Label->SetWorldLocation(
			FVector(0, ChartLength * 0.5f, LineHeight + LineThickness + 1.0f));
		Label->SetWorldRotation(FRotator(90.0f, 0.0f, 90.0f)); // 使文本朝向正确

		// 保存标签组件
		StatisticalLineLabels.Add(Label);
	}

	// 保存轴线网格组件
	StatisticalLineMeshes.Add(LineMesh);
}

// 应用值触发条件到折线图
void AXVLineChart::ApplyValueTriggerConditions()
{
	if (!bEnableValueTriggers || TotalCountOfValue == 0 || ValueTriggerConditions.Num() == 0)
	{
		return;
	}

	// 遍历所有线段/点，检查是否符合触发条件
	size_t CurrentIndex = 0;
	for (int RowIndex = 0; RowIndex < XYZs.Num(); RowIndex++)
	{
		const auto& Row = XYZs[RowIndex];
		for (const auto& XZPair : Row)
		{
			// 使用原始高度值进行比较
			int RawValue = XZPair.Value; // Z值
			
			// 检查值是否满足任何触发条件
			FLinearColor HighlightColor;
			bool bMatchesTrigger = CheckValueTriggerConditions(RawValue, HighlightColor);
			
			if (bMatchesTrigger)
			{
				// 符合条件，应用高亮颜色和发光效果
				DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue(
					"EmissiveColor", HighlightColor);
				DynamicMaterialInstances[CurrentIndex]->SetScalarParameterValue(
					"EmissiveIntensity", EmissiveIntensity);
			}
			else
			{
				// 不符合条件，恢复默认颜色和发光效果
				DynamicMaterialInstances[CurrentIndex]->SetVectorParameterValue(
					"EmissiveColor", EmissiveColor);
				DynamicMaterialInstances[CurrentIndex]->SetScalarParameterValue(
					"EmissiveIntensity", 0);
			}
			
			// 更新网格部分
			UpdateMeshSection(CurrentIndex);
			
			CurrentIndex++;
		}
	}
}
