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
	bGenerateLOD = false;
	TotalCountOfValue = 0;
	
	EmissiveIntensity = 10.f;
	EmissiveColor = FColor::White;

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

void AXVChartBase::SetValue(const FString& InValue)
{
}

void AXVChartBase::SetStyle()
{
}

void AXVChartBase::ConstructMesh(double Rate)
{
	ProceduralMeshComponent->ClearAllMeshSections();
	ProceduralMeshComponent->ClearCollisionConvexMeshes();
}

void AXVChartBase::PrepareMeshSections()
{
	ProceduralMeshComponent->ClearAllMeshSections();
	ProceduralMeshComponent->ClearCollisionConvexMeshes();
	SectionInfos.Empty();
	SectionInfos.SetNum(TotalCountOfValue + 1);
	LabelComponents.Empty();
	LabelComponents.SetNum(TotalCountOfValue);
	VerticesBackup.Empty();
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

void AXVChartBase::GenerateAllMeshInfo()
{
}

void AXVChartBase::UpdateSectionVerticesOfZ(const double& Scale)
{
	BackupVertices();
	for (size_t SectionIndex =0; SectionIndex <SectionInfos.Num(); SectionIndex++)
	{
		FXVChartSectionInfo& XVChartSectionInfo = SectionInfos[SectionIndex];
		for (size_t VerticeIndex = 0; VerticeIndex < XVChartSectionInfo.Vertices.Num(); VerticeIndex++)
		{
			FVector& Vertice = XVChartSectionInfo.Vertices[VerticeIndex];
			// 这里限制比例的大小，避免出现高度为0的柱体，导致错误提示
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
	
}

void AXVChartBase::BackupVertices()
{
	// 只备份一次
	if(VerticesBackup.IsEmpty())
	{
		VerticesBackup.SetNum(SectionInfos.Num());
		for (size_t SectionIndex =0; SectionIndex <SectionInfos.Num(); SectionIndex++)
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

	if(bEnableGPU)
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
		UE_LOG(LogTemp, Warning, TEXT("AXVChartBase: 数据管理器未初始化"));
		return false;
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
    if (PropertyMapping.XProperty.IsEmpty() || PropertyMapping.YProperty.IsEmpty() || PropertyMapping.ZProperty.IsEmpty())
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
    SortedXValues.Sort();
    SortedYValues.Sort();
    
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
                                          const FColor& SectionColor)
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
		CurrentAngle += 1;
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
