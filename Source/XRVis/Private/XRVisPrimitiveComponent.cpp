// Fill out your copyright notice in the Description page of Project Settings.


#include "XRVisPrimitiveComponent.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "DynamicMeshBuilder.h"
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"
#include "Rendering/XRVisBoxGeometryGenerator.h"
#include "Rendering/XRVisBoxGeometryRenderer.h"
#include "Rendering/XRVisGeometryGenerator.h"
#include "Rendering/XRVisSceneViewExtension.h"

#define USING_CUSTOM_VERTEXFACTORY 1

#if USING_CUSTOM_VERTEXFACTORY

/** Index Buffer */
class FXRVisMeshIndexBuffer : public FIndexBuffer
{
public:
	void SetPooledIndexBuffer(const TRefCountPtr<FRDGPooledBuffer>& InPooledBuffer)
	{
		IndexBufferRHI = InPooledBuffer.GetReference()->GetRHI();
	}
};

/**
 * Uniform buffer for mesh vertex factories.
 */
BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FXRVisPrimitiveVertexFactoryParameters,)
	SHADER_PARAMETER(FIntVector4, VertexFetch_Parameters)
	SHADER_PARAMETER_SRV(Buffer<float2>, VertexFetch_TexCoordBuffer)
	SHADER_PARAMETER_SRV(Buffer<float4>, VertexFetch_InstanceOriginBuffer)
	SHADER_PARAMETER_SRV(Buffer<float4>, VertexFetch_InstanceTransformBuffer)
END_GLOBAL_SHADER_PARAMETER_STRUCT()
typedef TUniformBufferRef<FXRVisPrimitiveVertexFactoryParameters> FXRVisPrimitiveVertexFactoryBufferRef;
IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FXRVisPrimitiveVertexFactoryParameters, "XRVisVF");

/**
 * 自定义顶点工厂，直接从GPU计算得到的SRV读取数据
 */
class FXRVisPrimitiveVertexFactory : public FLocalVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FXRVisPrimitiveVertexFactory);
public:
	FXRVisPrimitiveVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, const char* InDebugName)
        : FLocalVertexFactory(InFeatureLevel, InDebugName)
    {
    }

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
	{
		return false;
	}
	
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment )
	{
		FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	virtual void InitRHI(FRHICommandListBase& RHICmdList) override
	{
		check(Streams.Num() == 0);
		
		FVertexStream PositionVertexStream;
		PositionVertexStream.VertexBuffer = VertexBuffer;
		PositionVertexStream.Stride = sizeof(FVector3f);
		PositionVertexStream.Offset = 0;
		PositionVertexStream.VertexStreamUsage = EVertexStreamUsage::Default;
		
		FVertexStream TangentXVertexStream;
		TangentXVertexStream.VertexBuffer = TangentVertexBuffer;
		TangentXVertexStream.Stride = 8;
		TangentXVertexStream.Offset = 0;
		TangentXVertexStream.VertexStreamUsage = EVertexStreamUsage::Default;
		
		FVertexStream TangentZVertexStream;
		TangentZVertexStream.VertexBuffer = TangentVertexBuffer;
		TangentZVertexStream.Stride = 8;
		TangentZVertexStream.Offset = 4;
		TangentZVertexStream.VertexStreamUsage = EVertexStreamUsage::Default;
		
		FVertexStream ColorVertexStream;
		ColorVertexStream.VertexBuffer = ColorVertexBuffer;
		ColorVertexStream.Stride = sizeof(FColor);
		ColorVertexStream.Offset = 0;
		ColorVertexStream.VertexStreamUsage = EVertexStreamUsage::Default;
		
		// same with VertexFactoryInput
		const FVertexElement VertexPositionElement(Streams.Add(PositionVertexStream),0, VET_Float3, 0, PositionVertexStream.Stride, false);
		const FVertexElement VertexTangentXElement(Streams.Add(TangentXVertexStream),0, VET_PackedNormal, 1, TangentXVertexStream.Stride, false);
		const FVertexElement VertexTangentZElement(Streams.Add(TangentZVertexStream),0, VET_PackedNormal, 2, TangentZVertexStream.Stride, false);
		const FVertexElement VertexColorElement(Streams.Add(ColorVertexStream), 0,VET_Color, 3, ColorVertexStream.Stride, false);
		
		// Vertex declaration
		FVertexDeclarationElementList Elements;
		Elements.Add(VertexPositionElement);
		Elements.Add(VertexTangentXElement);
		Elements.Add(VertexTangentZElement);
		Elements.Add(VertexColorElement);
		
		InitDeclaration(Elements);
	}

	inline const FUniformBufferRHIRef GetVertexFactoryUniformBuffer() const
	{
		return UniformBuffer;
	}

	
public:
	FVertexBuffer* VertexBuffer;
	FVertexBuffer* TangentVertexBuffer = nullptr;
	FColorVertexBuffer* ColorVertexBuffer = nullptr;

	mutable FRHIShaderResourceView* PositionSRV = nullptr;
};

IMPLEMENT_VERTEX_FACTORY_TYPE(FXRVisPrimitiveVertexFactory,"/Engine/Private/LocalVertexFactory.ush",
	  EVertexFactoryFlags::UsedWithMaterials
	// | EVertexFactoryFlags::SupportsStaticLighting
	// | EVertexFactoryFlags::SupportsDynamicLighting
	// | EVertexFactoryFlags::SupportsPrecisePrevWorldPos
);

class FXRVisPrimitiveVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FXRVisPrimitiveVertexFactoryShaderParameters, NonVirtual);
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
	}

	void GetElementShaderBindings(
		const class FSceneInterface* Scene,
		const class FSceneView* View,
		const class FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const class FVertexFactory* InVertexFactory,
		const struct FMeshBatchElement& BatchElement,
		class FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const
	{
		FXRVisPrimitiveVertexFactory* VertexFactory = (FXRVisPrimitiveVertexFactory*)InVertexFactory; 
		ShaderBindings.Add(Shader->GetUniformBufferParameter<FXRVisPrimitiveVertexFactoryParameters>(), VertexFactory->GetVertexFactoryUniformBuffer());
	}
};

IMPLEMENT_TYPE_LAYOUT(FXRVisPrimitiveVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FXRVisPrimitiveVertexFactory, SF_Vertex, FXRVisPrimitiveVertexFactoryShaderParameters);

#endif 
/**
 * 
 */
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
		, GeometryGenerator(InComponent->GetGeometryGenerator())
		, bDrawIndirect(InComponent->bDrawIndirect)
		, bGPUGenerate(InComponent->bGPUGenerate)
		, VertexFactory(GetScene().GetFeatureLevel(), "FXRVisPrimitiveSceneProxy")
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
			{0,  1, 0},  // Front
			{0, -1, 0}, // Back
			{-1, 0, 0}, // Left
			{1,  0, 0},  // Right
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
		{
			for (size_t i = 0; i < 6; i++)
			{
				size_t VertexStart =  i * 4;
				size_t IndexStart = i * 6;

				// every face with 4 vertices
				for (size_t j = 0; j < 4; j++)
				{
					UploadVertices[VertexStart + j].Position = Positions[Indices[i * 4 + j]] ;
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
		}
		

			
		IndexBuffer.Indices = UploadIndices;
		BeginInitResource(&IndexBuffer);

		if(bGPUGenerate)
		{
			VertexFactory.InitResource(GetImmediateCommandList_ForRenderCommand());
		}
		else 
		{
			VertexBuffers.InitFromDynamicVertex(&VertexFactory, UploadVertices);
			
		}

		Material = InComponent->GetMaterial(0);
		if (Material == nullptr)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}
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
		
	
		// TODO : remove this debug code
	
		TResourceArray<uint32> InitData;
		InitData.Add(36 ); // NumIndicesPerInstance
		InitData.Add(1); // InstanceCount;
		InitData.Add(0); // StartIndexLocation;
		InitData.Add(0); // BaseVertexLocation
		InitData.Add(0); // StartInstanceLocation

		DrawIndirectArgsBuffer.Initialize(RHICmdList,TEXT("XRVis_DrawIndirectBuffer"),sizeof(uint32),5,PF_R32_UINT,BUF_Static|BUF_DrawIndirect,&InitData);
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_XRVisPrimitiveSceneProxy_GetDynamicMeshElements);

		FMatrix EffectiveLocalToWorld = GetLocalToWorld();
		
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];

				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				if (bGPUGenerate && GPUIndexBuffer.IsInitialized())
				{
					BatchElement.IndexBuffer = &GPUIndexBuffer;
				}
				else
				{
					BatchElement.IndexBuffer = &IndexBuffer;
				}
				Mesh.bWireframe = false;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = Material->GetRenderProxy();

				FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();

				DynamicPrimitiveUniformBuffer.Set(
					EffectiveLocalToWorld,
					EffectiveLocalToWorld,
					GetBounds(),
					GetLocalBounds(),
					true,
					false,
					AlwaysHasVelocity());
				
#if USING_CUSTOM_VERTEXFACTORY
				BatchElement.PrimitiveIdMode = PrimID_FromPrimitiveSceneInfo;
				BatchElement.PrimitiveUniformBuffer = DynamicPrimitiveUniformBuffer.UniformBuffer.GetUniformBufferRef();
#else
				BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;
#endif
				
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

	FXRVisGeometryGenerator* GeometryGenerator;

	FStaticMeshVertexBuffers VertexBuffers;
	FDynamicMeshIndexBuffer32 IndexBuffer;

	mutable FVertexBuffer GPUVertexBuffer;

	mutable FXRVisMeshIndexBuffer GPUIndexBuffer;

	bool bDrawIndirect;
	bool bGPUGenerate;
	
	FRWBuffer DrawIndirectArgsBuffer;

#if USING_CUSTOM_VERTEXFACTORY
	mutable FXRVisPrimitiveVertexFactory VertexFactory;
#else
	FLocalVertexFactory VertexFactory;
#endif
	
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

	bGPUGenerate = true;
	// ...
	if(bGPUGenerate)
	{
		UE_LOG(LogTemp, Log, TEXT("bGPUGenerate is true"));
		GeometryGenerator = new FXRVisBoxGeometryGenerator();
		FXRVisBoxGeometryParams Params;
		Params.HeightValues.SetNum(Params.RowCount * Params.ColumnCount);
		for (int x = 0; x < Params.RowCount; ++x)
		{
			for (int y = 0; y < Params.ColumnCount; ++y)
			{
				Params.HeightValues[x * Params.ColumnCount + y]= 1;
			}
		}
		static_cast<FXRVisBoxGeometryGenerator*>(GeometryGenerator)->SetParameters(Params);
		
		GeometryRenderer = new FXRVisBoxGeometryRenderer();
		GeometryGenerator->MarkForUpdate();
	}
}

UXRVisPrimitiveComponent::~UXRVisPrimitiveComponent()
{
	if(SceneViewExtension)
	{
		SceneViewExtension->UnregisterGeometryGenerator();
		SceneViewExtension->UnregisterGeometryRenderer();
	}
	if(GeometryGenerator)
	{
		delete GeometryGenerator;
	}
	if(GeometryRenderer)
	{
		delete GeometryRenderer;
	}
	
}

void UXRVisPrimitiveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}


void UXRVisPrimitiveComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial)
{
	Material = InMaterial;
}

void UXRVisPrimitiveComponent::SetGeometryGenerator(FXRVisGeometryGenerator* InGeometryGenerator)
{
	GeometryGenerator = InGeometryGenerator;
}

FXRVisGeometryGenerator* UXRVisPrimitiveComponent::GetGeometryGenerator() const
{
	return GeometryGenerator;
}

// Called when the game starts
void UXRVisPrimitiveComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...


	SceneViewExtension = FSceneViewExtensions::NewExtension<FXRVisSceneViewExtension>();
	SceneViewExtension->RegisterGeometryGenerator(GeometryGenerator);
	SceneViewExtension->RegisterGeometryRenderer(GeometryRenderer);
}

void UXRVisPrimitiveComponent::OnMaterialChanged()
{
	SetMaterial(0, Material);
	MarkRenderStateDirty();
}


// Called every frame
void UXRVisPrimitiveComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if(GeometryRenderer)
		GeometryRenderer->SetModelMatrix(static_cast<FMatrix44f>(GetComponentTransform().ToMatrixWithScale()));
}

void UXRVisPrimitiveComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials,
	bool bGetDebugMaterials) const
{
	if(Material)
	{
		OutMaterials.Add(Material);
	}
}
