// Fill out your copyright notice in the Description page of Project Settings.


#include "XRVisPrimitiveComponent.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "DynamicMeshBuilder.h"
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#include "MeshMaterialShader.h"

#define USING_CUSTOM_VERTEXFACTORY 1

#if USING_CUSTOM_VERTEXFACTORY
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
		return RHISupportsManualVertexFetch(Parameters.Platform);
	}
	
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment )
	{
		FVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	/**
     * 使用计算着色器输出的SRV初始化顶点工厂
     */
   void InitWithComputeOutput(FRHIShaderResourceView* InVertexBufferSRV, uint32 VertexStride)
    {
        ENQUEUE_RENDER_COMMAND(InitXRVisGPUVertexFactory)(
        [this, InVertexBufferSRV, VertexStride](FRHICommandListImmediate& RHICmdList)
        {
            FDataType Data;
            
            // 使用正确的构造方法，注意这里需要使用SRV专用的方法
            Data.PositionComponent = FVertexStreamComponent(
                nullptr,  // 不使用VertexBuffer，而使用SRV
                0,        // 位置在结构中的偏移
                VertexStride,  // 每个顶点的大小
                VET_Float3,    // 位置数据类型
                EVertexStreamUsage::Default
            );
            // 单独设置SRV
            Data.PositionComponentSRV = InVertexBufferSRV;
        	
            
            // 法线组件
            Data.TangentBasisComponents[0] = FVertexStreamComponent(
                nullptr,
                sizeof(FVector3f),
                VertexStride,
                VET_PackedNormal,
                EVertexStreamUsage::Default
            );
            
            // 切线组件
            Data.TangentBasisComponents[1] = FVertexStreamComponent(
                nullptr,
                sizeof(FVector3f) + sizeof(FPackedNormal),
                VertexStride,
                VET_PackedNormal,
                EVertexStreamUsage::Default
            );
            
            // UV坐标
            Data.TextureCoordinates.Add(FVertexStreamComponent(
                nullptr,
                sizeof(FVector3f) + sizeof(FPackedNormal) * 2,
                VertexStride,
                VET_Float2,
                EVertexStreamUsage::Default
            ));
            
            // 颜色组件
            Data.ColorComponent = FVertexStreamComponent(
                nullptr,
                sizeof(FVector3f) + sizeof(FPackedNormal) * 2 + sizeof(FVector2f),
                VertexStride,
                VET_Color,
                EVertexStreamUsage::Default
            );
            
            // 设置SRV
            // 我们需要为所有组件设置相同的SRV
            Data.TangentsSRV = InVertexBufferSRV;
            Data.TextureCoordinatesSRV = InVertexBufferSRV;
            Data.ColorComponentsSRV = InVertexBufferSRV;
            
            // 设置数据到顶点工厂
            SetData(Data);
            
            // 初始化资源
            InitResource(RHICmdList);
        });
    }

	inline const FUniformBufferRHIRef GetVertexFactoryUniformBuffer() const
	{
		return UniformBuffer;
	}
	
};

IMPLEMENT_VERTEX_FACTORY_TYPE(FXRVisPrimitiveVertexFactory,"/Engine/Private/LocalVertexFactory.ush",
	  EVertexFactoryFlags::UsedWithMaterials
	| EVertexFactoryFlags::SupportsStaticLighting
	| EVertexFactoryFlags::SupportsDynamicLighting
	| EVertexFactoryFlags::SupportsPrecisePrevWorldPos
	| EVertexFactoryFlags::SupportsCachingMeshDrawCommands
	| EVertexFactoryFlags::DoesNotSupportNullPixelShader
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
		// TODO : remove this debug code
		FRHICommandListBase& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
		
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

	bool bDrawIndirect;
	FRWBuffer DrawIndirectArgsBuffer;

#if USING_CUSTOM_VERTEXFACTORY
	FXRVisPrimitiveVertexFactory VertexFactory;
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
}

void UXRVisPrimitiveComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials,
	bool bGetDebugMaterials) const
{
	if(Material)
	{
		OutMaterials.Add(Material);
	}
}
