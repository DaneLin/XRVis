﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "XRVisPrimitiveComponent.generated.h"

class FPrimitiveSceneProxy;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class XRVIS_API UXRVisPrimitiveComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.

public:
	// Sets default values for this component's properties
	UXRVisPrimitiveComponent();
	
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const override;

	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override
	{
		if(Material)
		{
			Material->ConditionalPostLoad();
		}
		return Material.Get();
	}

	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnMaterialChanged();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnMaterialChanged)
	TObjectPtr<UMaterialInterface> Material;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDrawIndirect;
};
