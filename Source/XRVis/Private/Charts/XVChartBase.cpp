#include "Charts/XVChartBase.h"

#include "SceneViewExtension.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Charts/XVBarChart.h"
#include "Charts/XVLineChart.h"
#include "Rendering/XRVisBoxGeometryGenerator.h"
#include "Rendering/XRVisBoxGeometryRenderer.h"
#include "Rendering/XRVisSceneViewExtension.h"


// Sets default values
AXVChartBase::AXVChartBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Procedural Mesh Component"));
	ProceduralMeshComponent->SetCastShadow(false);
	RootComponent = ProceduralMeshComponent;

	Tags.Add(FName("Chart"));
	TotalCountOfValue = 0;

	EmissiveIntensity = 10.f;
	EmissiveColor = FColor::White;

	// 初始化参考值相关属性
	ReferenceValue = 0.0f;
	ReferenceHighlightColor = FColor::Yellow;
	ReferenceComparisonType = EReferenceComparisonType::Greater;
	bEnableReferenceHighlight = false;

	// 初始化触发条件相关属性
	bEnableValueTriggers = false;

	// 初始化统计轴线相关属性
	bEnableStatisticalLines = false;

	// 初始化Z轴控制相关属性
	bForceZeroBase = true;
	MinZAxisValue = 0.0f;
	ZAxisScale = 1.0f;
	bAutoAdjustZAxis = false;
	ZAxisMarginPercent = 0.1f;

	bAnimationFinished = false;
	CurrentBuildTime = 0.f;
	BuildTime = 1.5f;

	// 初始化数据管理器
	ChartDataManager = CreateDefaultSubobject<UXVDataManager>(TEXT("ChartDataManager"));

	// 默认设置
	bAutoLoadData = false;
	bEnableGPU = false;

	// GPU
	GeometryGenerator = new FXRVisBoxGeometryGenerator();
	GeometryRenderer = new FXRVisBoxGeometryRenderer();

	// LOD
	LODSwitchDis.Push(0);
	LODSwitchDis.Push(500);
	LODSwitchDis.Push(1000);
	LODSwitchDis.Push(20000);
	LODSwitchSize.Push(0.0);
	LODSwitchSize.Push(0.2);
	LODSwitchSize.Push(0.4);
	LODSwitchSize.Push(0.8);

	PrepareMeshSections();
}

AXVChartBase::~AXVChartBase()
{
	if (GeometryGenerator)
	{
		delete GeometryGenerator;
		GeometryGenerator = nullptr;
	}
	if (GeometryRenderer)
	{
		delete GeometryRenderer;
		GeometryRenderer = nullptr;
	}
}


void AXVChartBase::UpdateOnMouseEnterOrLeft()
{
}

uint32_t AXVChartBase::GetSectionIndexOfLOD(uint32_t SectionIndex)
{
	return SectionIndex + LODInfos[CurrentLOD].LODOffset;
}

void AXVChartBase::SetValue(const FString& InValue)
{
}

void AXVChartBase::SetStyle()
{
}

void AXVChartBase::PrepareMeshSections()
{
	ProceduralMeshComponent->ClearAllMeshSections();
	ProceduralMeshComponent->ClearCollisionConvexMeshes();
	LODInfos.Empty();
	LODInfos.SetNum(GenerateLODCount);
	SectionInfos.Empty();
	SectionInfos.SetNum(GenerateLODCount * TotalCountOfValue + 1);
	LabelComponents.Empty();
	LabelComponents.SetNum(TotalCountOfValue);
	VerticesBackup.Empty();
}

void AXVChartBase::DrawMeshLOD(int LODLevel, double Rate)
{
	if (bEnableGPU)
	{
		return;
	}
	check(LODLevel < GenerateLODCount);
	bool bUpdateLOD = false;
	if (CurrentLOD == -1 || CurrentLOD != LODLevel)
	{
		if (CurrentLOD != -1)
		{
			for (int Index = 0; Index < LODInfos[CurrentLOD].LODCount * Rate; ++Index)
			{
				int SectionIndex = Index + LODInfos[CurrentLOD].LODOffset;
				ProceduralMeshComponent->SetMeshSectionVisible(SectionIndex, false);
			}
		}
		CurrentLOD = LODLevel;
		bUpdateLOD = true;
	}
	if(bUpdateLOD || Rate < 1)
	{
		for (int Index = 0; Index < LODInfos[CurrentLOD].LODCount * Rate; ++Index)
		{
			int SectionIndex = Index + LODInfos[CurrentLOD].LODOffset;
			ProceduralMeshComponent->SetMeshSectionVisible(SectionIndex, true);
		}
	}
}

void AXVChartBase::ClearSelectedSection(const int& SectionIndex)
{
	SectionInfos[SectionIndex].Vertices.Empty();
	SectionInfos[SectionIndex].Indices.Empty();
	SectionInfos[SectionIndex].Normals.Empty();
	SectionInfos[SectionIndex].UVs.Empty();
	SectionInfos[SectionIndex].Tangents.Empty();
	SectionInfos[SectionIndex].VertexColors.Empty();
}

void AXVChartBase::GenerateLOD()
{
	PrepareMeshSections();
}

void AXVChartBase::UpdateSectionVerticesOfZ(const double& Scale)
{
	BackupVertices();
	for (size_t SectionIndex = 0; SectionIndex < SectionInfos.Num(); SectionIndex++)
	{
		FXVChartSectionInfo& XVChartSectionInfo = SectionInfos[SectionIndex];
		for (size_t VerticeIndex = 0; VerticeIndex < XVChartSectionInfo.Vertices.Num(); VerticeIndex++)
		{
			FVector& Vertice = XVChartSectionInfo.Vertices[VerticeIndex];
			Vertice.Z = VerticesBackup[SectionIndex][VerticeIndex].Z * FMath::Clamp(Scale, 0.1f, 1.0f);
		}
	}
}

void AXVChartBase::DrawMeshSection(int SectionIndex, bool bCreateCollision)
{
	ProceduralMeshComponent->CreateMeshSection_LinearColor(SectionIndex,
	                                                       SectionInfos[SectionIndex].Vertices,
	                                                       SectionInfos[SectionIndex].Indices,
	                                                       SectionInfos[SectionIndex].Normals,
	                                                       SectionInfos[SectionIndex].UVs,
	                                                       SectionInfos[SectionIndex].VertexColors,
	                                                       SectionInfos[SectionIndex].Tangents,
	                                                       bCreateCollision);
}

void AXVChartBase::UpdateMeshSection(int SectionIndex, bool bSRGBConversion)
{
	ProceduralMeshComponent->UpdateMeshSection_LinearColor(
		SectionIndex,
		SectionInfos[SectionIndex].Vertices,
		SectionInfos[SectionIndex].Normals,
		SectionInfos[SectionIndex].UVs,
		SectionInfos[SectionIndex].VertexColors,
		SectionInfos[SectionIndex].Tangents,
		bSRGBConversion);
}

void AXVChartBase::DrawWithGPU()
{
	if (!bEnableGPU)
	{
		return;
	}
}


// Called when the game starts or when spawned
void AXVChartBase::BeginPlay()
{
	Super::BeginPlay();

	if (bEnableGPU)
	{
		SceneViewExtension = FSceneViewExtensions::NewExtension<FXRVisSceneViewExtension>();
		SceneViewExtension->RegisterGeometryGenerator(GeometryGenerator);
		SceneViewExtension->RegisterGeometryRenderer(GeometryRenderer);
	}
	// 如果启用了自动加载数据并且设置了有效的数据路径
	if (bAutoLoadData && !DataFilePath.IsEmpty())
	{
		LoadDataFromFile(DataFilePath);
	}

	// 如果启用了参考值高亮，应用高亮效果
	if (bEnableReferenceHighlight)
	{
		ApplyReferenceHighlight();
	}

	// 如果启用了触发条件，应用触发条件
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

void AXVChartBase::BackupVertices()
{
	// 只备份一次
	if (VerticesBackup.IsEmpty())
	{
		VerticesBackup.SetNum(SectionInfos.Num());
		for (size_t SectionIndex = 0; SectionIndex < SectionInfos.Num(); SectionIndex++)
		{
			FXVChartSectionInfo& XVChartSectionInfo = SectionInfos[SectionIndex];
			VerticesBackup[SectionIndex].SetNum(XVChartSectionInfo.Vertices.Num());
			for (size_t VerticeIndex = 0; VerticeIndex < XVChartSectionInfo.Vertices.Num(); VerticeIndex++)
			{
				VerticesBackup[SectionIndex][VerticeIndex] = XVChartSectionInfo.Vertices[VerticeIndex];
			}
		}
	}
}

// Called every frame
void AXVChartBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsHidden())
	{
		CurrentBuildTime = 0.f;
	}

	if (bEnableGPU)
	{
		GeometryRenderer->SetModelMatrix(static_cast<FMatrix44f>(GetActorTransform().ToMatrixWithScale()));
	}
}

// 数据加载相关方法实现

bool AXVChartBase::LoadDataFromFile(const FString& FilePath)
{
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 文件路径为空"));
		return false;
	}

	if (!ChartDataManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 数据管理器未初始化,现在进行初始化"));
		ChartDataManager = NewObject<UXVDataManager>(this, TEXT("ChartDataManager"));
	}

	// 保存当前文件路径
	DataFilePath = FilePath;

	// 读取文件内容
	FString FileContent;
	if (FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		// 判断文件类型
		FString Extension = FPaths::GetExtension(FilePath);

		if (Extension.Equals(TEXT("json"), ESearchCase::IgnoreCase))
		{
			// 首先尝试直接解析JSON
			if (SetValueFromJson(FileContent))
			{
				return true;
			}
		}
	}

	// 如果直接解析失败，使用之前的数据管理器方法
	bool bSuccess = LoadDataByFileExtension(FilePath);

	if (bSuccess)
	{
		// 格式化数据
		FString FormattedData = GetFormattedDataForChart();

		// 设置图表值
		if (!FormattedData.IsEmpty())
		{
			SetValue(FormattedData);
			return true;
		}
	}

	return false;
}

bool AXVChartBase::LoadDataFromString(const FString& Content, const FString& FileExtension)
{
	if (Content.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 内容为空"));
		return false;
	}

	if (!ChartDataManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 数据管理器未初始化"));
		return false;
	}

	// 如果是JSON，首先尝试直接解析
	if (FileExtension.Equals(TEXT("json"), ESearchCase::IgnoreCase))
	{
		if (SetValueFromJson(Content))
		{
			return true;
		}
	}

	bool bSuccess = false;

	// 如果直接解析失败，使用之前的数据管理器方法
	// 根据文件扩展名选择合适的加载方法
	if (FileExtension.Equals(TEXT("json"), ESearchCase::IgnoreCase))
	{
		bSuccess = ChartDataManager->LoadFromJsonString(Content);
	}
	else if (FileExtension.Equals(TEXT("csv"), ESearchCase::IgnoreCase))
	{
		bSuccess = ChartDataManager->LoadFromCsvString(Content);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 不支持的文件扩展名 %s"), *FileExtension);
		return false;
	}

	if (bSuccess)
	{
		// 格式化数据
		FString FormattedData = GetFormattedDataForChart();

		// 设置图表值
		if (!FormattedData.IsEmpty())
		{
			SetValue(FormattedData);
			return true;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 数据加载失败 - %s"), *ChartDataManager->GetLastError());
	}

	return false;
}

bool AXVChartBase::LoadDataByFileExtension(const FString& FilePath)
{
	// 获取文件扩展名
	FString Extension = FPaths::GetExtension(FilePath);

	// 根据文件扩展名选择合适的加载方法
	if (Extension.Equals(TEXT("json"), ESearchCase::IgnoreCase))
	{
		return ChartDataManager->LoadFromJsonFile(FilePath);
	}
	else if (Extension.Equals(TEXT("csv"), ESearchCase::IgnoreCase))
	{
		return ChartDataManager->LoadFromCsvFile(FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 不支持的文件扩展名 %s"), *Extension);
		return false;
	}
}

FString AXVChartBase::GetFormattedDataForChart()
{
	// 格式化数据为图表可用的格式
	return FormatDataByChartType();
}

FString AXVChartBase::FormatDataByChartType()
{
	// 默认实现，子类可重写以提供特定图表类型的实现
	// 检查图表类型（通过类名判断）
	FString ClassName = GetClass()->GetName();

	if (ClassName.Contains(TEXT("BarChart")))
	{
		// 柱状图
		return ChartDataManager->ConvertToBarChartData(
			PropertyMapping.XProperty,
			PropertyMapping.YProperty,
			PropertyMapping.ZProperty
		);
	}
	else if (ClassName.Contains(TEXT("LineChart")))
	{
		// 折线图
		return ChartDataManager->ConvertToLineChartData(
			PropertyMapping.XProperty,
			PropertyMapping.YProperty,
			PropertyMapping.ZProperty
		);
	}
	else if (ClassName.Contains(TEXT("PieChart")))
	{
		// 饼图 - 特殊处理（饼图需要返回TMap）
		TMap<FString, float> PieData = ChartDataManager->ConvertToPieChartData(
			PropertyMapping.CategoryProperty,
			PropertyMapping.ValueProperty
		);

		// 将TMap转换为JSON字符串
		FString ResultJson;
		TArray<TSharedPtr<FJsonValue>> JsonArray;

		for (const auto& Pair : PieData)
		{
			TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
			JsonObject->SetStringField("name", Pair.Key);
			JsonObject->SetNumberField("value", Pair.Value);

			JsonArray.Add(MakeShareable(new FJsonValueObject(JsonObject)));
		}

		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultJson);
		FJsonSerializer::Serialize(JsonArray, Writer);

		return ResultJson;
	}

	// 其他未知类型
	UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 未知图表类型 %s，无法格式化数据"), *ClassName);
	return TEXT("");
}

bool AXVChartBase::SetValueFromJson(const FString& JsonString)
{
	if (JsonString.IsEmpty())
		return false;

	// 尝试解析JSON
	TSharedPtr<FJsonValue> JsonValue;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonValue) || !JsonValue.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 无法解析JSON数据"));
		return false;
	}

	// 检查是数组格式
	if (JsonValue->Type == EJson::Array)
	{
		TArray<TSharedPtr<FJsonValue>> JsonArray = JsonValue->AsArray();

		if (JsonArray.Num() == 0)
			return false;

		// 检查第一个元素以确定数据格式
		TSharedPtr<FJsonValue> FirstElement = JsonArray[0];

		// 原始格式 [[x,y,z], ...]
		if (FirstElement->Type == EJson::Array)
		{
			// 使用原始SetValue方法
			SetValue(JsonString);
			return true;
		}
		// 对象格式 [{"x": ..., "y": ..., "z": ...}, ...]
		else if (FirstElement->Type == EJson::Object)
		{
			// 收集所有对象
			TArray<TSharedPtr<FJsonObject>> NamedData;
			for (const auto& Item : JsonArray)
			{
				if (Item->Type == EJson::Object)
				{
					NamedData.Add(Item->AsObject());
				}
			}

			// 使用命名数据方法
			if (NamedData.Num() > 0)
			{
				SetValueFromNamedData(NamedData);
				return true;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 不支持的JSON数据格式"));
	return false;
}

void AXVChartBase::SetValueFromNamedData(const TArray<TSharedPtr<FJsonObject>>& NamedData)
{
	if (NamedData.IsEmpty())
		return;

	// 验证是否设置了必要的属性映射
	if (PropertyMapping.XProperty.IsEmpty() || PropertyMapping.YProperty.IsEmpty() || PropertyMapping.ZProperty.
		IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 缺少必要的属性映射，请设置PropertyMapping中的XProperty、YProperty和ZProperty"));
		return;
	}

	// 收集所有唯一的X和Y值
	TSet<FString> UniqueXValues;
	TSet<FString> UniqueYValues;

	// 遍历所有数据项，提取X和Y值
	for (const auto& DataItem : NamedData)
	{
		// 检查所需的字段是否存在
		if (DataItem->HasField(PropertyMapping.XProperty) &&
			DataItem->HasField(PropertyMapping.YProperty) &&
			DataItem->HasField(PropertyMapping.ZProperty))
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
	XAxisLabels = SortedXValues;
	YAxisLabels = SortedYValues;

	// 设置图表轴标签（根据图表类型）
	FString ClassName = GetClass()->GetName();
	if (ClassName.Contains(TEXT("BarChart")))
	{
		AXVBarChart* BarChart = Cast<AXVBarChart>(this);
		if (BarChart)
		{
			BarChart->XTextArrs = SortedXValues;
			BarChart->YTextArrs = SortedYValues;
		}
	}
	else if (ClassName.Contains(TEXT("LineChart")))
	{
		AXVLineChart* LineChart = Cast<AXVLineChart>(this);
		if (LineChart)
		{
			LineChart->XText = SortedXValues;
			LineChart->YText = SortedYValues;
		}
	}

	// 将命名数据转换为图表所需的格式
	TArray<TSharedPtr<FJsonValue>> ChartData;

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

			// 创建图表数据项 [YIndex, XIndex, ZValue]
			TArray<TSharedPtr<FJsonValue>> InnerArray;
			InnerArray.Add(MakeShareable(new FJsonValueNumber(YIndex)));
			InnerArray.Add(MakeShareable(new FJsonValueNumber(XIndex)));
			InnerArray.Add(MakeShareable(new FJsonValueNumber(ZValue)));

			ChartData.Add(MakeShareable(new FJsonValueArray(InnerArray)));
		}
	}

	// 将转换后的数据序列化为JSON字符串
	FString FormattedData;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&FormattedData);
	FJsonSerializer::Serialize(ChartData, Writer);

	// 使用原始SetValue方法处理
	SetValue(FormattedData);
}

void AXVChartBase::GeneratePieSectionInfo(const FVector& CenterPosition, const size_t& SectionIndex,
                                          const size_t& StartAngle, const size_t& EndAngle, const float& NearDis,
                                          const float& FarDis, const int& Height,
                                          const FColor& SectionColor, const int& Step)
{
	if (NearDis > FarDis || StartAngle >= EndAngle)
	{
		UE_LOG(LogTemp, Warning, TEXT("Error distance range of internal and external circle or Angle"));
		return;
	}

	size_t CurrentAngle = StartAngle;

	FXVPlaneInfo CurrentPlaneInfo;
	XVChartUtils::CalcAnglePlaneInfo(CenterPosition, CurrentAngle, NearDis, FarDis, Height, CurrentPlaneInfo);

	// 单独处理一下左面
	FVector LeftFaceNormal = (CurrentPlaneInfo.NearBottomVertexPosition - CurrentPlaneInfo.FarTopVertexPosition) ^ (
		CurrentPlaneInfo.NearBottomVertexPosition - CurrentPlaneInfo.NearTopVertexPosition);
	XVChartUtils::AddBaseTriangle(
		SectionInfos,
		SectionIndex,
		CurrentPlaneInfo.NearBottomVertexPosition, CurrentPlaneInfo.NearTopVertexPosition,
		CurrentPlaneInfo.FarTopVertexPosition,
		LeftFaceNormal, LeftFaceNormal, LeftFaceNormal,
		FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, 0),
		FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, Height),
		FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, Height),
		FProcMeshTangent(0, 1, 0),
		SectionColor);
	XVChartUtils::AddBaseTriangle(
		SectionInfos, SectionIndex,
		CurrentPlaneInfo.NearBottomVertexPosition, CurrentPlaneInfo.FarTopVertexPosition,
		CurrentPlaneInfo.FarBottomVertexPosition,
		LeftFaceNormal, LeftFaceNormal, LeftFaceNormal,
		FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, 0),
		FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, 0),
		FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, Height),
		FProcMeshTangent(0, 1, 0),
		SectionColor);

	for (; CurrentAngle < EndAngle;)
	{
		FXVPlaneInfo NextPlaneInfo;
		CurrentAngle += Step;
		if (CurrentAngle > EndAngle)
		{
			CurrentAngle = EndAngle;
		}
		XVChartUtils::CalcAnglePlaneInfo(CenterPosition, CurrentAngle, NearDis, FarDis, Height, NextPlaneInfo);

		// near front face
		XVChartUtils::AddBaseTriangle(
			SectionInfos, SectionIndex,
			NextPlaneInfo.NearBottomVertexPosition, NextPlaneInfo.NearTopVertexPosition,
			CurrentPlaneInfo.NearTopVertexPosition,
			-NextPlaneInfo.NearBottomNormal, -CurrentPlaneInfo.NearTopNormal, -NextPlaneInfo.NearTopNormal,
			FVector2D(NextPlaneInfo.RadiansAngle * NearDis, 0),
			FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, Height),
			FVector2D(NextPlaneInfo.RadiansAngle * NearDis, Height),
			FProcMeshTangent(0, 0, 1),
			SectionColor);
		XVChartUtils::AddBaseTriangle(
			SectionInfos, SectionIndex,
			CurrentPlaneInfo.NearTopVertexPosition, CurrentPlaneInfo.NearBottomVertexPosition,
			NextPlaneInfo.NearBottomVertexPosition,
			-NextPlaneInfo.NearBottomNormal, -CurrentPlaneInfo.NearBottomNormal, -CurrentPlaneInfo.NearTopNormal,
			FVector2D(NextPlaneInfo.RadiansAngle * NearDis, 0),
			FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, 0),
			FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, Height),
			FProcMeshTangent(0, 0, 1),
			SectionColor);

		// far back face
		XVChartUtils::AddBaseTriangle(
			SectionInfos, SectionIndex,
			CurrentPlaneInfo.FarBottomVertexPosition, CurrentPlaneInfo.FarTopVertexPosition,
			NextPlaneInfo.FarTopVertexPosition,
			CurrentPlaneInfo.FarBottomNormal, NextPlaneInfo.FarBottomNormal, CurrentPlaneInfo.FarTopNormal,
			FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, 0),
			FVector2D(NextPlaneInfo.RadiansAngle * FarDis, 0),
			FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, Height),
			FProcMeshTangent(0, 0, 1),
			SectionColor);
		XVChartUtils::AddBaseTriangle(
			SectionInfos, SectionIndex,
			CurrentPlaneInfo.FarBottomVertexPosition, NextPlaneInfo.FarTopVertexPosition,
			NextPlaneInfo.FarBottomVertexPosition,
			NextPlaneInfo.FarBottomNormal, NextPlaneInfo.FarTopNormal, CurrentPlaneInfo.FarTopNormal,
			FVector2D(NextPlaneInfo.RadiansAngle * FarDis, 0),
			FVector2D(NextPlaneInfo.RadiansAngle * FarDis, Height),
			FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, Height),
			FProcMeshTangent(0, 0, 1),
			SectionColor);

		// Top Face
		XVChartUtils::AddBaseTriangle(
			SectionInfos, SectionIndex,
			CurrentPlaneInfo.NearTopVertexPosition, NextPlaneInfo.NearTopVertexPosition,
			CurrentPlaneInfo.FarTopVertexPosition,
			FVector(0, Height, 0), FVector(0, Height, 0), FVector(0, Height, 0),
			FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, Height),
			FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, Height),
			FVector2D(NextPlaneInfo.RadiansAngle * NearDis, Height),
			FProcMeshTangent(1, 0, 0),
			SectionColor);
		XVChartUtils::AddBaseTriangle(
			SectionInfos, SectionIndex,
			NextPlaneInfo.NearTopVertexPosition, NextPlaneInfo.FarTopVertexPosition,
			CurrentPlaneInfo.FarTopVertexPosition,
			FVector(0, Height, 0), FVector(0, Height, 0), FVector(0, Height, 0),
			FVector2D(NextPlaneInfo.RadiansAngle * NearDis, Height),
			FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, Height),
			FVector2D(NextPlaneInfo.RadiansAngle * FarDis, Height),
			FProcMeshTangent(1, 0, 0),
			SectionColor);

		// Bottom Face
		XVChartUtils::AddBaseTriangle(
			SectionInfos, SectionIndex,
			CurrentPlaneInfo.NearBottomVertexPosition, NextPlaneInfo.FarBottomVertexPosition,
			NextPlaneInfo.NearBottomVertexPosition,
			FVector(0, -Height, 0), FVector(0, -Height, 0), FVector(0, -Height, 0),
			FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, 0),
			FVector2D(NextPlaneInfo.RadiansAngle * NearDis, 0),
			FVector2D(NextPlaneInfo.RadiansAngle * FarDis, 0),
			FProcMeshTangent(1, 0, 0),
			SectionColor);
		XVChartUtils::AddBaseTriangle(
			SectionInfos, SectionIndex,
			CurrentPlaneInfo.NearBottomVertexPosition, CurrentPlaneInfo.FarBottomVertexPosition,
			NextPlaneInfo.FarBottomVertexPosition,
			FVector(0, -Height, 0), FVector(0, -Height, 0), FVector(0, -Height, 0),
			FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, 0),
			FVector2D(NextPlaneInfo.RadiansAngle * FarDis, 0),
			FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, 0),
			FProcMeshTangent(1, 0, 0),
			SectionColor);

		CurrentPlaneInfo = NextPlaneInfo;
	}

	// 特殊处理右边
	FVector RightFaceNormal = (CurrentPlaneInfo.NearBottomVertexPosition - CurrentPlaneInfo.NearTopVertexPosition) ^ (
		CurrentPlaneInfo.NearBottomVertexPosition - CurrentPlaneInfo.FarTopVertexPosition);
	XVChartUtils::AddBaseTriangle(
		SectionInfos, SectionIndex,
		CurrentPlaneInfo.NearBottomVertexPosition, CurrentPlaneInfo.FarTopVertexPosition,
		CurrentPlaneInfo.NearTopVertexPosition,
		RightFaceNormal, RightFaceNormal, RightFaceNormal,
		FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, 0),
		FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, Height),
		FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, Height),
		FProcMeshTangent(0, 1, 0),
		SectionColor);
	XVChartUtils::AddBaseTriangle(
		SectionInfos, SectionIndex,
		CurrentPlaneInfo.NearBottomVertexPosition, CurrentPlaneInfo.FarBottomVertexPosition,
		CurrentPlaneInfo.FarTopVertexPosition,
		RightFaceNormal, RightFaceNormal, RightFaceNormal,
		FVector2D(CurrentPlaneInfo.RadiansAngle * NearDis, 0),
		FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, Height),
		FVector2D(CurrentPlaneInfo.RadiansAngle * FarDis, 0),
		FProcMeshTangent(0, 1, 0),
		SectionColor);
}

void AXVChartBase::NotifyActorBeginCursorOver()
{
	Super::NotifyActorBeginCursorOver();
	bIsMouseEntered = true;
	UpdateOnMouseEnterOrLeft();
}

void AXVChartBase::NotifyActorEndCursorOver()
{
	Super::NotifyActorEndCursorOver();
	bIsMouseEntered = false;
	UpdateOnMouseEnterOrLeft();
}

const FHitResult AXVChartBase::GetCursorHitResult() const
{
	return XVChartUtils::GetCursorHitResult(GetWorld());
}

float AXVChartBase::GetCursorHitAngle(const FHitResult& HitResult) const
{
	// 计算鼠标点击的角度，默认实现
	if (HitResult.bBlockingHit)
	{
		// 计算点击位置到Actor中心的向量
		FVector HitLocation = HitResult.Location;
		FVector CenterLocation = GetActorLocation();

		// 计算在XY平面上的角度
		FVector Direction = HitLocation - CenterLocation;
		Direction.Z = 0; // 忽略Z轴方向
		Direction.Normalize();

		// 计算角度（弧度）
		float Angle = FMath::Atan2(Direction.Y, Direction.X);

		// 将角度转换为0-360度
		if (Angle < 0)
		{
			Angle += 2 * PI;
		}

		return Angle;
	}

	return 0.0f;
}

FVector AXVChartBase::GetCursorHitRowAndColAndHeight(const FHitResult& HitResult) const
{
	// 计算鼠标点击的行列和高度，默认实现，子类可重写
	if (HitResult.bBlockingHit)
	{
		// 转换世界坐标到局部坐标
		FVector LocalHitLocation = GetActorTransform().InverseTransformPosition(HitResult.Location);
		return LocalHitLocation;
	}

	return FVector::ZeroVector;
}

void AXVChartBase::ApplyReferenceHighlight()
{
	// 基类实现为空，由子类具体实现
}

void AXVChartBase::SetReferenceValue(float InReferenceValue)
{
	ReferenceValue = InReferenceValue;

	// 如果启用了参考值高亮，则应用高亮
	if (bEnableReferenceHighlight)
	{
		ApplyReferenceHighlight();
	}
}

void AXVChartBase::SetReferenceComparisonType(TEnumAsByte<enum EReferenceComparisonType> InComparisonType)
{
	ReferenceComparisonType = InComparisonType;

	// 如果启用了参考值高亮，则应用高亮
	if (bEnableReferenceHighlight)
	{
		ApplyReferenceHighlight();
	}
}

bool AXVChartBase::CheckAgainstReference(float ValueToCheck) const
{
	switch (ReferenceComparisonType.GetValue())
	{
	case EReferenceComparisonType::Greater:
		return ValueToCheck > ReferenceValue;
	case EReferenceComparisonType::Less:
		return ValueToCheck < ReferenceValue;
	case EReferenceComparisonType::Equal:
		return FMath::IsNearlyEqual(ValueToCheck, ReferenceValue);
	case EReferenceComparisonType::GreaterOrEqual:
		return ValueToCheck >= ReferenceValue;
	case EReferenceComparisonType::LessOrEqual:
		return ValueToCheck <= ReferenceValue;
	case EReferenceComparisonType::NotEqual:
		return !FMath::IsNearlyEqual(ValueToCheck, ReferenceValue);
	default:
		return false;
	}
}

void AXVChartBase::SetEnableReferenceHighlight(bool bEnable)
{
	bEnableReferenceHighlight = bEnable;

	// 根据新的启用状态应用或清除高亮
	if (bEnableReferenceHighlight)
	{
		ApplyReferenceHighlight();
	}
	else
	{
		// 禁用高亮时，恢复所有区域的默认颜色
		for (int32 i = 0; i < DynamicMaterialInstances.Num(); i++)
		{
			if (DynamicMaterialInstances[i])
			{
				DynamicMaterialInstances[i]->SetVectorParameterValue("EmissiveColor", EmissiveColor);
				DynamicMaterialInstances[i]->SetScalarParameterValue("EmissiveIntensity", 0);
				UpdateMeshSection(i);
			}
		}
	}
}

void AXVChartBase::SetReferenceHighlightColor(FLinearColor InColor)
{
	ReferenceHighlightColor = InColor;

	// 如果启用了参考值高亮，更新高亮颜色
	if (bEnableReferenceHighlight)
	{
		ApplyReferenceHighlight();
	}
}

void AXVChartBase::UpdateStatisticalLineValues()
{
	// 更新每条统计轴线的实际值
	for (auto& Line : StatisticalLines)
	{
		switch (Line.LineType)
		{
		case EStatisticalLineType::Mean:
			Line.ActualValue = CalculateMean();
			break;
		case EStatisticalLineType::Median:
			Line.ActualValue = CalculateMedian();
			break;
		case EStatisticalLineType::Max:
			Line.ActualValue = CalculateMax();
			break;
		case EStatisticalLineType::Min:
			Line.ActualValue = CalculateMin();
			break;
		case EStatisticalLineType::Custom:
			Line.ActualValue = Line.CustomValue;
			break;
		default:
			Line.ActualValue = 0.0f;
			break;
		}
	}
}

void AXVChartBase::ApplyStatisticalLines()
{
	// 基类实现为空，由子类具体实现绘制逻辑
}

int32 AXVChartBase::AddStatisticalLine(EStatisticalLineType LineType, FLinearColor LineColor, float CustomValue)
{
	FXVStatisticalLine NewLine;
	NewLine.LineType = LineType;
	NewLine.LineColor = LineColor;
	NewLine.CustomValue = CustomValue;

	// 设置默认标签格式
	switch (LineType)
	{
	case EStatisticalLineType::Mean:
		NewLine.LabelFormat = TEXT("平均值: {value}");
		break;
	case EStatisticalLineType::Median:
		NewLine.LabelFormat = TEXT("中位数: {value}");
		break;
	case EStatisticalLineType::Max:
		NewLine.LabelFormat = TEXT("最大值: {value}");
		break;
	case EStatisticalLineType::Min:
		NewLine.LabelFormat = TEXT("最小值: {value}");
		break;
	case EStatisticalLineType::Custom:
		NewLine.LabelFormat = TEXT("自定义值: {value}");
		break;
	default:
		break;
	}

	// 添加到数组并返回索引
	return StatisticalLines.Add(NewLine);
}

void AXVChartBase::RemoveStatisticalLine(int32 Index)
{
	if (StatisticalLines.IsValidIndex(Index))
	{
		StatisticalLines.RemoveAt(Index);
	}
}

float AXVChartBase::CalculateMean() const
{
	TArray<float> Values = GetAllDataValues();
	if (Values.Num() == 0)
	{
		return 0.0f;
	}

	float Sum = 0.0f;
	for (float Value : Values)
	{
		Sum += Value;
	}

	return Sum / Values.Num();
}

float AXVChartBase::CalculateMedian() const
{
	TArray<float> Values = GetAllDataValues();
	if (Values.Num() == 0)
	{
		return 0.0f;
	}

	// 排序数组
	Values.Sort();

	// 计算中位数
	if (Values.Num() % 2 == 0)
	{
		// 偶数个元素，取中间两个值的平均
		int32 MiddleIndex = Values.Num() / 2;
		return (Values[MiddleIndex - 1] + Values[MiddleIndex]) / 2.0f;
	}
	else
	{
		// 奇数个元素，直接返回中间值
		return Values[Values.Num() / 2];
	}
}

float AXVChartBase::CalculateMax() const
{
	TArray<float> Values = GetAllDataValues();
	if (Values.Num() == 0)
	{
		return 0.0f;
	}

	return FMath::Max(Values);
}

float AXVChartBase::CalculateMin() const
{
	TArray<float> Values = GetAllDataValues();
	if (Values.Num() == 0)
	{
		return 0.0f;
	}

	return FMath::Min(Values);
}

TArray<float> AXVChartBase::GetAllDataValues() const
{
	// 基类中返回空数组，由子类具体实现
	return TArray<float>();
}

void AXVChartBase::SetZAxisRange(bool bFromZero, float MinValue)
{
	bForceZeroBase = bFromZero;
	MinZAxisValue = bFromZero ? 0.0f : MinValue;
}

void AXVChartBase::SetZAxisScale(float Scale)
{
	ZAxisScale = FMath::Clamp(Scale, 0.1f, 10.0f);
	
}

void AXVChartBase::AutoAdjustZAxis(float MarginPercent)
{
	ZAxisMarginPercent = FMath::Clamp(MarginPercent, 0.0f, 0.5f);
	bAutoAdjustZAxis = true;

	// 获取所有数据值
	TArray<float> Values = GetAllDataValues();
	if (Values.Num() == 0)
	{
		return;
	}

	// 计算数据范围
	float MinVal = FMath::Min(Values);
	float MaxVal = FMath::Max(Values);

	// 如果最小值和最大值几乎相等，则进行特殊处理
	if (FMath::IsNearlyEqual(MinVal, MaxVal, 0.001f))
	{
		// 数据基本没有波动，创建一个人工范围
		MinVal = MaxVal * 0.9f;
	}

	// 计算范围和边距
	float Range = MaxVal - MinVal;
	float Margin = Range * ZAxisMarginPercent;

	// 设置Z轴范围，考虑边距
	MinZAxisValue = MinVal - Margin;

	// 如果最小值接近0或小于0，则从0开始
	if (MinZAxisValue <= 0.01f * MaxVal)
	{
		bForceZeroBase = true;
		MinZAxisValue = 0.0f;
	}
	else
	{
		bForceZeroBase = false;
	}
}

float AXVChartBase::CalculateAdjustedHeight(float RawHeight) const
{
	if (bForceZeroBase || RawHeight <= 0.0f)
	{
		return RawHeight * ZAxisScale;
	}

	float AdjustedHeight = (RawHeight - MinZAxisValue) * ZAxisScale;

	return FMath::Max(0.1f, AdjustedHeight);
}

void AXVChartBase::UpdateLOD(double Rate)
{
	switch (LODType)
	{
	case ELODType::Distance:
		{
			FVector CameraLocation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
			FVector ActorLocation = GetActorLocation();
			float Distance = (CameraLocation - ActorLocation).Length();
			int NewLODLevel = Algo::LowerBound(LODSwitchDis, Distance) - 1;
			DrawMeshLOD(NewLODLevel, Rate);
		}
		break;
	case ELODType::ScreenSize:
		{
			FVector Origin, BoxExtent;
			GetActorBounds(false, Origin, BoxExtent);
			FVector Origin2BoxExtent = BoxExtent - Origin;
			float Dis = Origin2BoxExtent.Length();
			FMinimalViewInfo CameraView;
			FMinimalViewInfo ViewInfo = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraCacheView();
			FMatrix ProjectionMatrix = ViewInfo.CalculateProjectionMatrix();
			const float Dist = FVector::Dist(GetActorLocation(),
			                                 GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation());
			const float ScreenMultiple = FMath::Max(0.5f * ProjectionMatrix.M[0][0], 0.5f * ProjectionMatrix.M[1][1]);
			const float ScreenRadius = ScreenMultiple * Dis / FMath::Max(1.0f, Dist);
			float CurrentScreenSize = ScreenRadius * 2.f;

			int NewLODLevel = Algo::LowerBound(LODSwitchSize, CurrentScreenSize);
			DrawMeshLOD(GenerateLODCount - NewLODLevel, Rate);
		}
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("Please select correct lod type"));
		break;
	}
}

bool AXVChartBase::CheckValueTriggerConditions(float ValueToCheck, FLinearColor& OutColor) const
{
	if (!bEnableValueTriggers)
	{
		return false;
	}

	for (const auto& Condition : ValueTriggerConditions)
	{
		if (!Condition.bEnabled)
		{
			continue;
		}

		bool bConditionMet = false;

		switch (Condition.ConditionType)
		{
		case EValueTriggerConditionType::Equal:
			bConditionMet = FMath::IsNearlyEqual(ValueToCheck, Condition.ReferenceValue);
			break;
		case EValueTriggerConditionType::NotEqual:
			bConditionMet = !FMath::IsNearlyEqual(ValueToCheck, Condition.ReferenceValue);
			break;
		case EValueTriggerConditionType::Greater:
			bConditionMet = ValueToCheck > Condition.ReferenceValue;
			break;
		case EValueTriggerConditionType::Less:
			bConditionMet = ValueToCheck < Condition.ReferenceValue;
			break;
		case EValueTriggerConditionType::GreaterOrEqual:
			bConditionMet = ValueToCheck >= Condition.ReferenceValue;
			break;
		case EValueTriggerConditionType::LessOrEqual:
			bConditionMet = ValueToCheck <= Condition.ReferenceValue;
			break;
		case EValueTriggerConditionType::Range:
			bConditionMet = (ValueToCheck >= Condition.ReferenceValue && ValueToCheck <= Condition.UpperBoundValue);
			break;
		case EValueTriggerConditionType::NotInRange:
			bConditionMet = (ValueToCheck < Condition.ReferenceValue || ValueToCheck > Condition.UpperBoundValue);
			break;
		default:
			bConditionMet = false;
			break;
		}

		if (bConditionMet)
		{
			OutColor = Condition.HighlightColor;
			return true;
		}
	}

	return false;
}

int AXVChartBase::AddValueTriggerCondition(EValueTriggerConditionType ConditionType, float ReferenceValue_,
                                           FLinearColor HighlightColor, float UpperBoundValue,
                                           const FString& ConditionName)
{
	FValueTriggerCondition NewCondition;
	NewCondition.ConditionType = ConditionType;
	NewCondition.ReferenceValue = ReferenceValue;
	NewCondition.UpperBoundValue = UpperBoundValue;
	NewCondition.HighlightColor = HighlightColor;
	NewCondition.ConditionName = ConditionName;
	NewCondition.bEnabled = true;

	// 添加到数组并返回索引
	int32 Index = ValueTriggerConditions.Add(NewCondition);

	// 如果触发条件已启用，立即应用
	if (bEnableValueTriggers)
	{
		ApplyValueTriggerConditions();
	}

	return Index;
}

void AXVChartBase::RemoveValueTriggerCondition(int32 Index)
{
	if (ValueTriggerConditions.IsValidIndex(Index))
	{
		ValueTriggerConditions.RemoveAt(Index);

		// 如果触发条件已启用，立即应用更改
		if (bEnableValueTriggers)
		{
			ApplyValueTriggerConditions();
		}
	}
}

void AXVChartBase::ApplyValueTriggerConditions()
{
	// 基类实现为空，由子类具体实现
	// 这个函数应该遍历图表中的所有数据点，检查它们的值是否满足任何触发条件
	// 然后根据需要更新它们的颜色
}

void AXVChartBase::SetEnableValueTriggers(bool bEnable)
{
	bEnableValueTriggers = bEnable;

	// 根据新的启用状态应用或清除触发条件
	if (bEnableValueTriggers)
	{
		ApplyValueTriggerConditions();
	}
	else
	{
		// 禁用触发条件时，恢复所有区域的默认颜色
		for (int32 i = 0; i < DynamicMaterialInstances.Num(); i++)
		{
			if (DynamicMaterialInstances[i])
			{
				DynamicMaterialInstances[i]->SetVectorParameterValue("EmissiveColor", EmissiveColor);
				DynamicMaterialInstances[i]->SetScalarParameterValue("EmissiveIntensity", 0);
				UpdateMeshSection(i);
			}
		}
	}
}
