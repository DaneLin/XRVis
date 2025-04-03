// Fill out your copyright notice in the Description page of Project Settings.


#include "XRVisPrimitiveComponent.h"

#include "DynamicMeshBuilder.h"
#include "MaterialDomain.h"

class FXRVisPrimitiveSceneProxy : public FPrimitiveSceneProxy
{
public:
	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}
	
	FXRVisPrimitiveSceneProxy(UXRVisPrimitiveComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
		, bDrawIndirect(InComponent->bDrawIndirect)
		, VertexFactory(GetScene().GetFeatureLevel(), "FMyPrimitiveSceneProxy")
	{
		bWillEverBeLit = false;

		TArray<FDynamicMeshVertex> UploadVertices;
		TArray<uint32> UploadIndices;

		UploadVertices.SetNum(24);
		UploadIndices.SetNum(36);

		FVector3f Positions[8] = {
        {0, 100, 0},
        {100, 100, 0},
        {100, 000, 0},
        {0, 0, 0},
        {0, 100, 100},
        {100, 100, 100},
        {100, 0, 100},
        {0, 0, 100}};

	    FVector3f Normals[6] = {
	        {0, 1, 0},  // Front
	        {0, -1, 0}, // Back
	        {-1, 0, 0}, // Left
	        {1, 0, 0},  // Right
	        {0, 0, 1},  // Top
	        {0, 0, -1}  // Bottom
	    };

	    int Indices[24] = {
	        // Front face
	        0, 1, 5, 4,
	        // Back face
	        2, 3, 7,  6,
	        // Left face
	        3, 0, 4, 7,
	        // Right face
	        1, 2, 6, 5,
	        // Top face
	        4, 5, 6, 7,
	        // Bottom face
	        0, 3, 2, 1
	    };

	    FVector2f UVs[4] = {
	        {0, 1},
	        {1, 1},
	        {1, 0},
	        {0, 0},
	    };
	    // box for six faces
	    for (size_t i = 0; i < 6; i++)
	    {
	        size_t VertexStart =  i * 4;
	        size_t IndexStart =  i * 6;

	        // every face with 4 vertices
	        for (size_t j = 0; j < 4; j++)
	        {
	            UploadVertices[VertexStart + j].Position = Positions[Indices[i * 4 + j]];
	            UploadVertices[VertexStart + j].TangentZ = Normals[i];
	            // OutputUVs[VertexStart + j] = UVs[j];
	            UploadVertices[VertexStart + j].Color = FColor(1.0, 1.0, 1.0, 1.0);
	        }
	    	UploadIndices[IndexStart + 0] = VertexStart + 0;
	    	UploadIndices[IndexStart + 1] = VertexStart + 1;
	    	UploadIndices[IndexStart + 2] = VertexStart + 2;
	    	UploadIndices[IndexStart + 3] = VertexStart + 0;
	    	UploadIndices[IndexStart + 4] = VertexStart + 2;
	    	UploadIndices[IndexStart + 5] = VertexStart + 3;
	    }

		Material = InComponent->GetMaterial(0);
		if (Material == nullptr)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
		IndexBuffer.Indices = UploadIndices;

		VertexBuffers.InitFromDynamicVertex(&VertexFactory, UploadVertices);
		BeginInitResource(&IndexBuffer);
		
	}

	virtual ~FXRVisPrimitiveSceneProxy()
	{
		VertexBuffers.PositionVertexBuffer.ReleaseResource();
		VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		VertexBuffers.ColorVertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
		DrawIndirectArgsBuffer.Release();
	}

	virtual void CreateRenderThreadResources() override
	{
		FRHICommandListBase& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
		
		TResourceArray<uint32> InitData;
		InitData.Add(36); // NumIndicesPerInstance
		InitData.Add(1); // InstanceCount;
		InitData.Add(0); // StartIndexLocation;
		InitData.Add(0); // BaseVertexLocation
		InitData.Add(0); // StartInstanceLocation

		DrawIndirectArgsBuffer.Initialize(RHICmdList,TEXT("Test_DrawIndirectBuffer"),sizeof(uint32),5,PF_R32_UINT,BUF_Static|BUF_DrawIndirect,&InitData);
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_CustomSceneProxy_GetDynamicMeshElements);

		FMatrix EffectiveLocalToWorld = GetLocalToWorld();
		
		
		
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];

				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				Mesh.bWireframe = false;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = Material->GetRenderProxy();

				FMatrix ScaleMatrix = FScaleMatrix(FVector(1.0f)).RemoveTranslation();
				FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();

				DynamicPrimitiveUniformBuffer.Set(
					EffectiveLocalToWorld,
					EffectiveLocalToWorld,
					GetBounds(),
					GetLocalBounds(),
					true,
					false,
					AlwaysHasVelocity());
				
				BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

				BatchElement.FirstIndex = 0;
				BatchElement.MinVertexIndex = 0;
				if(!bDrawIndirect)
				{
					BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
					BatchElement.MaxVertexIndex = VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
					BatchElement.NumInstances = 1;
				}
				else
				{
					BatchElement.NumPrimitives = 0;
					BatchElement.IndirectArgsOffset = 0;
					BatchElement.IndirectArgsBuffer = DrawIndirectArgsBuffer.Buffer;
				}
				
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;
				Mesh.CastShadow = true;
				Collector.AddMesh(ViewIndex, Mesh);
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && (View->Family->EngineShowFlags.BillboardSprites);
		Result.bDynamicRelevance = true;
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
		return Result;
	}

	virtual void OnTransformChanged() override
	{
		Origin = GetLocalToWorld().GetOrigin();
	}

	virtual uint32 GetMemoryFootprint() const override
	{
		return sizeof(*this) + GetAllocatedSize();
	}

private:
	UMaterialInterface* Material;

	FStaticMeshVertexBuffers VertexBuffers;
	FDynamicMeshIndexBuffer32 IndexBuffer;

	bool bDrawIndirect;
	FRWBuffer DrawIndirectArgsBuffer;

	FLocalVertexFactory VertexFactory;

	FVector Origin;
};



FPrimitiveSceneProxy* UXRVisPrimitiveComponent::CreateSceneProxy()
{
	return new FXRVisPrimitiveSceneProxy(this);
}

FBoxSphereBounds UXRVisPrimitiveComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FBox(FVector(0, 0, 0), FVector(100, 100, 1))).TransformBy(LocalToWorld);
}

// Sets default values for this component's properties
UXRVisPrimitiveComponent::UXRVisPrimitiveComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UXRVisPrimitiveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}


void UXRVisPrimitiveComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial)
{
	Material = InMaterial;
}

// Called when the game starts
void UXRVisPrimitiveComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UXRVisPrimitiveComponent::OnMaterialChanged()
{
	SetMaterial(0, Material);
}


// Called every frame
void UXRVisPrimitiveComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UXRVisPrimitiveComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials,
	bool bGetDebugMaterials) const
{
	if(Material)
	{
		OutMaterials.Add(Material);
	}
}

