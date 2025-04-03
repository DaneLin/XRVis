// Fill out your copyright notice in the Description page of Project Settings.
#include "XVChartUtils.h"
#include "ProceduralMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetTextLibrary.h"


XVChartUtils::XVChartUtils()
{
}

XVChartUtils::~XVChartUtils()
{
}

void XVChartUtils::AddBaseTriangle(TArray<FXVChartSectionInfo>& SectionInfos, 
                                 const size_t SectionIndex, const FVector& InFirstPoint, const FVector& InSecondPoint,
                                 const FVector& InThirdPoint,
                                 const FVector& InFirstNormal, const FVector& InSecondNormal,
                                 const FVector& InThirdNormal,
                                 const FVector2D& InFirstUV, const FVector2D& InSecondUV, const FVector2D& InThirdUV,
                                 const FProcMeshTangent& Tangent, const FColor& TriangleColor)
{
	int FirstIndex = SectionInfos[SectionIndex].Vertices.Emplace(InFirstPoint);
	int SecondIndex = SectionInfos[SectionIndex].Vertices.Emplace(InSecondPoint);
	int ThirdIndex = SectionInfos[SectionIndex].Vertices.Emplace(InThirdPoint);
	// 逆时针顺序
	SectionInfos[SectionIndex].Indices.Emplace(FirstIndex);
	SectionInfos[SectionIndex].Indices.Emplace(SecondIndex);
	SectionInfos[SectionIndex].Indices.Emplace(ThirdIndex);
	
	SectionInfos[SectionIndex].Normals.Emplace(InFirstNormal);
	SectionInfos[SectionIndex].Normals.Emplace(InSecondNormal);
	SectionInfos[SectionIndex].Normals.Emplace(InThirdNormal);

	SectionInfos[SectionIndex].UVs.Emplace(InFirstUV.ClampAxes(0., 1.));
	SectionInfos[SectionIndex].UVs.Emplace(InSecondUV.ClampAxes(0., 1.));
	SectionInfos[SectionIndex].UVs.Emplace(InThirdUV.ClampAxes(0., 1.));

	SectionInfos[SectionIndex].Tangents.Emplace(Tangent);
	SectionInfos[SectionIndex].Tangents.Emplace(Tangent);
	SectionInfos[SectionIndex].Tangents.Emplace(Tangent);

	FLinearColor LinearColor = FLinearColor::FromSRGBColor(TriangleColor);
	SectionInfos[SectionIndex].VertexColors.Emplace(LinearColor);
	SectionInfos[SectionIndex].VertexColors.Emplace(LinearColor);
	SectionInfos[SectionIndex].VertexColors.Emplace(LinearColor);
}

void XVChartUtils::AddBaseQuad(TArray<FXVChartSectionInfo>& SectionInfos, 
                             const size_t SectionIndex, const FVector& InFirstPoint, const FVector& InSecondPoint,
                             const FVector& InThirdPoint,
                             const FVector& InFouthPoint, const FVector& InQuadNormal, const FVector2D& InFirstUV,
                             const FVector2D& InSecondUV,
                             const FVector2D& InThirdUV, const FVector2D& InFouthUV, const FProcMeshTangent& Tangent,
                             const FColor& TriangleColor)
{
	AddBaseTriangle(SectionInfos, SectionIndex,
		InFirstPoint, InSecondPoint, InThirdPoint,
		InQuadNormal,InQuadNormal, InQuadNormal,
		InFirstUV, InSecondUV, InThirdUV,
		Tangent,
		TriangleColor);
	AddBaseTriangle(SectionInfos, SectionIndex,
		InThirdPoint, InFouthPoint, InFirstPoint,
		InQuadNormal,InQuadNormal, InQuadNormal,
		InThirdUV, InFouthUV, InFirstUV,
		Tangent,
		TriangleColor);
}

void XVChartUtils::CalcAnglePlaneInfo(const FVector& CenterPosition, const size_t& Angle, const float& PlaneNearDis,
                                    const float& PlaneFarDis, const float& Height,
                                    FXVPlaneInfo& OutPlaneInfo)
{
	float CurrentRadians = FMath::DegreesToRadians(Angle);
	OutPlaneInfo.RadiansAngle = CurrentRadians;
	OutPlaneInfo.NearTopNormal = FVector(FMath::Cos(CurrentRadians), FMath::Sin(CurrentRadians), 0);
	OutPlaneInfo.NearBottomNormal = OutPlaneInfo.NearTopNormal;
	OutPlaneInfo.FarTopNormal = OutPlaneInfo.NearTopNormal;
	OutPlaneInfo.FarBottomNormal = OutPlaneInfo.FarTopNormal;
	OutPlaneInfo.NearBottomVertexPosition = CenterPosition + OutPlaneInfo.NearBottomNormal * PlaneNearDis;
	OutPlaneInfo.NearTopVertexPosition = OutPlaneInfo.NearBottomVertexPosition + FVector(0, 0, Height);
	OutPlaneInfo.FarBottomVertexPosition = CenterPosition + OutPlaneInfo.FarBottomNormal * PlaneFarDis;
	OutPlaneInfo.FarTopVertexPosition = OutPlaneInfo.FarBottomVertexPosition + FVector(0, 0, Height);
}

void XVChartUtils::CreateBox(TArray<FXVChartSectionInfo>& SectionInfos,
						  const int& SectionIndex, const FVector& InPosition,
						  const float& InLength, const float& InWidth, const float& InHeight, const float& InNextHeight,const FColor& InColor)
{
	FVector Position0 = FVector(0, InWidth, 0) + InPosition;
	FVector Position1 = FVector(InLength, InWidth, 0) + InPosition;
	FVector Position2 = FVector(InLength, 0, 0) + InPosition;
	FVector Position3 = FVector(0, 0, 0) + InPosition;
	FVector Position4 = FVector(0, InWidth, InHeight) + InPosition;
	FVector Position5 = FVector(InLength, InWidth, InNextHeight) + InPosition;
	FVector Position6 = FVector(InLength, 0, InNextHeight) + InPosition;
	FVector Position7 = FVector(0, 0, InHeight) + InPosition;

	FVector UpNormal(0, 0, 1);
	FVector FrontNormal(0, 1, 0);
	FVector LeftNormal(-1, 0, 0);

	// front
	AddBaseQuad(SectionInfos, SectionIndex, Position4, Position0, Position1, Position5, FrontNormal,
	            {0, 0}, {0, 1}, {1, 1}, {1, 0}, FProcMeshTangent(1, 0, 0), InColor);
	// back
	AddBaseQuad(SectionInfos, SectionIndex, Position6, Position2, Position3, Position7, -FrontNormal,
	            {0, 0}, {0, 1}, {1, 1}, {1, 0}, FProcMeshTangent(1, 0, 0), InColor);
	// left
	AddBaseQuad(SectionInfos,SectionIndex, Position7, Position3, Position0, Position4, LeftNormal,
	            {0, 0}, {0, 1}, {1, 1}, {1, 0}, FProcMeshTangent(0, 0, 1), InColor);
	// right
	AddBaseQuad(SectionInfos, SectionIndex, Position5, Position1, Position2, Position6, -LeftNormal,
	            {0, 0}, {0, 1}, {1, 1}, {1, 0}, FProcMeshTangent(0, 0, 1), InColor);
	// up
	AddBaseQuad(SectionInfos,  SectionIndex, Position7, Position4, Position5, Position6, UpNormal, {0, 0},
	            {0, 1}, {1, 1}, {1, 0}, FProcMeshTangent(0, 1, 0), InColor);
	// down
	AddBaseQuad(SectionInfos, SectionIndex, Position3, Position0, Position1, Position2, -UpNormal, {0, 0},
				{0, 1}, {1, 1}, {1, 0}, FProcMeshTangent(0, 1, 0), InColor);
}

void XVChartUtils::CreateSphere(TArray<FXVChartSectionInfo>& SectionInfos, const int& SectionIndex,
	const FVector& InPosition, const float& SphereRadius, const int& NumSphereSlices, const int& NumSphereStacks,
	const FColor& InColor)
{
	// 至少需要3个切片和2个堆栈才能形成有效的球体
	const int32 ActualSlices = FMath::Max(3, NumSphereSlices);
	const int32 ActualStacks = FMath::Max(2, NumSphereStacks);
	
	FProcMeshTangent Tangent(0, 1, 0); // 默认切线
	FLinearColor LinearColor = FLinearColor::FromSRGBColor(InColor);
	
	// 计算球体顶点
	for (int32 StackIndex = 0; StackIndex <= ActualStacks; ++StackIndex)
	{
		// 计算phi角 (0 到 Pi，从北极到南极)
		const float Phi = StackIndex * PI / ActualStacks;
		const float SinPhi = FMath::Sin(Phi);
		const float CosPhi = FMath::Cos(Phi);
		
		for (int32 SliceIndex = 0; SliceIndex <= ActualSlices; ++SliceIndex)
		{
			// 计算theta角 (0 到 2*Pi，围绕赤道)
			const float Theta = SliceIndex * 2.0f * PI / ActualSlices;
			const float SinTheta = FMath::Sin(Theta);
			const float CosTheta = FMath::Cos(Theta);
			
			// 计算球面上的点
			FVector VertexPosition(SphereRadius * SinPhi * CosTheta,SphereRadius * SinPhi * SinTheta,SphereRadius * CosPhi);
			
			// 添加世界位置偏移
			VertexPosition += InPosition;
			
			// 计算该点的法线 (从球心指向顶点的单位向量)
			FVector Normal = (VertexPosition - InPosition).GetSafeNormal();
			
			// 计算UV坐标
			FVector2D UV(static_cast<float>(SliceIndex) / ActualSlices,static_cast<float>(StackIndex) / ActualStacks);
			
			// 添加顶点
			int32 VertexIndex = SectionInfos[SectionIndex].Vertices.Emplace(VertexPosition);
			SectionInfos[SectionIndex].Normals.Emplace(Normal);
			SectionInfos[SectionIndex].UVs.Emplace(UV);
			SectionInfos[SectionIndex].Tangents.Emplace(Tangent);
			SectionInfos[SectionIndex].VertexColors.Emplace(LinearColor);
		}
	}
	
	// 生成三角形索引
	for (int32 StackIndex = 0; StackIndex < ActualStacks; ++StackIndex)
	{
		for (int32 SliceIndex = 0; SliceIndex < ActualSlices; ++SliceIndex)
		{
			// 计算当前栈中此切片的顶点索引
			const int32 CurrentRow = StackIndex * (ActualSlices + 1);
			const int32 NextRow = (StackIndex + 1) * (ActualSlices + 1);
			
			const int32 CurrentVertex = CurrentRow + SliceIndex;
			const int32 NextRowVertex = NextRow + SliceIndex;
			
			// 添加两个三角形以形成四边形面
			// 三角形1
			SectionInfos[SectionIndex].Indices.Add(CurrentVertex);
			SectionInfos[SectionIndex].Indices.Add(NextRowVertex);
			SectionInfos[SectionIndex].Indices.Add(NextRowVertex + 1);
			
			// 三角形2
			SectionInfos[SectionIndex].Indices.Add(CurrentVertex);
			SectionInfos[SectionIndex].Indices.Add(NextRowVertex + 1);
			SectionInfos[SectionIndex].Indices.Add(CurrentVertex + 1);
		}
	}
}

UTextRenderComponent* XVChartUtils::CreateTextRenderComponent(UObject* Outer, const FText& Text, FColor Color, bool bVisible)
{
	UTextRenderComponent* Label = NewObject<UTextRenderComponent>(Outer, UTextRenderComponent::StaticClass());
	Label->SetText(Text);
	Label->SetTextRenderColor(Color);
	Label->SetRenderCustomDepth(true);
	Label->CustomDepthStencilValue = 3;
	Label->RegisterComponent();
	Label->MarkRenderStateDirty();
	Label->SetVisibility(bVisible);
	return Label;
}

FHitResult XVChartUtils::GetCursorHitResult(const UWorld* World)
{
	{
		FVector WorldLocation, WorldDirection;

		APlayerController* PlayerController = World->GetFirstPlayerController();
		PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
		
		FHitResult HitResult;
		World->LineTraceSingleByChannel(HitResult, WorldLocation,WorldLocation + WorldDirection * PlayerController->HitResultTraceDistance,ECC_Visibility);
		return HitResult;
	}
}
