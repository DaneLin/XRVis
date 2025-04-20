#include "Charts/XVChartBase.h"


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

float AXVChartBase::GetCursorHitAngle(const FHitResult& HitResult) const
{
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
		return AngleDegrees;
	}
	return -1;
}

FVector AXVChartBase::GetCursorHitRowAndColAndHeight(const FHitResult& HitResult) const
{
	FVector Result(-1, -1, -1);
	if (HitResult.GetActor())
	{
		FVector ForwardVector = HitResult.GetActor()->GetActorForwardVector()* GetActorScale();
		FVector UpwardVector = HitResult.GetActor()->GetActorUpVector() *GetActorScale();
		FVector RightwardVector = HitResult.GetActor()->GetActorRightVector()* GetActorScale();
		
		FVector ProjectionP = HitResult.Location - GetActorLocation();

		float X = ProjectionP.Dot(ForwardVector) / ForwardVector.Length();
		float Y = ProjectionP.Dot(RightwardVector) / RightwardVector.Length();
		float Z = ProjectionP.Dot(UpwardVector) / UpwardVector.Length();
		
		Result = FVector{ X,Y,Z};
	}
	return Result;
}

const FHitResult AXVChartBase::GetCursorHitResult() const
{
	return XVChartUtils::GetCursorHitResult(GetWorld());
}

void AXVChartBase::NotifyActorBeginCursorOver()
{
	Super::NotifyActorBeginCursorOver();
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		PlayerController->CurrentMouseCursor = EMouseCursor::Hand;
	}
	bIsMouseEntered = true;
}

void AXVChartBase::NotifyActorEndCursorOver()
{
	Super::NotifyActorEndCursorOver();
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		PlayerController->CurrentMouseCursor = EMouseCursor::Type::Default;
	}
	bIsMouseEntered = false;
}
