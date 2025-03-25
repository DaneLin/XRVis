#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "XVChartUtils.h"
#include "XVChartBase.generated.h"

UCLASS()
class XRVIS_API AXVChartBase : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Chart Property | Style")
	float EmissiveIntensity;

	UPROPERTY(EditAnywhere, Category = "Chart Property | Style")
	FLinearColor EmissiveColor;

	/* 是否开启入场动画 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Animation")
	bool bEnableEnterAnimation;

public:
	// Sets default values for this actor's properties
	AXVChartBase();
	
	virtual void SetValue(const FString& InValue);
	virtual void SetStyle();
	virtual void ConstructMesh(double Rate = 1);
	virtual void PrepareMeshSections();
	virtual void ClearSelectedSection(const int& SectionIndex);
	virtual void GenerateAllMeshInfo();
	virtual void UpdateSectionVerticesOfZ(const double& scale);

	virtual void DrawMeshSection(int SectionIndex, bool bCreateCollision = true);
	virtual void UpdateMeshSection(int SectionIndex, bool bSRGBConversion = false);

	void GeneratePieSectionInfo(const FVector& CenterPosition, const size_t& SectionIndex,
	                            const size_t& StartAngle, const size_t& EndAngle, const float& NearDis,
	                            const float& FarDis, const int& Height, const FColor& SectionColor);

	// Mouse Event
	UFUNCTION(BlueprintCallable)
	const FHitResult GetCursorHitResult() const;

	virtual float GetCursorHitAngle(const FHitResult& HitResult) const;

	virtual FVector GetCursorHitRowAndColAndHeight(const FHitResult& HitResult) const;
	
	virtual void UpdateOnMouseEnterOrLeft();
	
	virtual void NotifyActorBeginCursorOver() override;
	
	virtual void NotifyActorEndCursorOver() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void BackupVertices();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess= true))
	UProceduralMeshComponent* ProceduralMeshComponent;

	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<FXVChartSectionInfo> SectionInfos;

	TArray<TArray<FVector>> VerticesBackup;
	
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<float> SectionsHeight;
	
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<UMaterialInstanceDynamic*> DynamicMaterialInstances;

	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<class UTextRenderComponent*> LabelComponents;

	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<bool> SectionSelectStates;

	UPROPERTY(VisibleDefaultsOnly, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	int TotalCountOfValue;

	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float CurrentBuildTime;

	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float BuildTime;

	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	bool bIsMouseEntered;

	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	bool bGenerateLOD;

	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	bool bAnimationFinished;

};
