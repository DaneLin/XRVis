#include "Charts/XVChartBase.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"


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
	
	// 根据文件后缀自动选择加载方法
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
	
	bool bSuccess = false;
	
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
