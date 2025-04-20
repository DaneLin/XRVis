#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "XVChartUtils.h"
#include "DataProcessing/XVDataManager.h"
#include "XVChartBase.generated.h"

/**
 * 图表属性映射
 */
USTRUCT(BlueprintType)
struct FXVChartPropertyMapping
{
	GENERATED_BODY()

	/** X轴属性名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Data Mapping", meta=(ToolTip="X轴属性名称"))
	FString XProperty;

	/** Y轴属性名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Data Mapping", meta=(ToolTip="Y轴属性名称"))
	FString YProperty;

	/** Z轴属性名称（值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Data Mapping", meta=(ToolTip="Z轴属性名称（值）"))
	FString ZProperty;

	/** 分类属性名称（饼图） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Data Mapping", meta=(ToolTip="分类属性名称（饼图）"))
	FString CategoryProperty;

	/** 数值属性名称（饼图） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chart Property | Data Mapping", meta=(ToolTip="数值属性名称（饼图）"))
	FString ValueProperty;
};

UCLASS()
class XRVIS_API AXVChartBase : public AActor
{
	GENERATED_BODY()

public:
	/* 发光强度 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Style", meta=(ToolTip="发光强度"))
	float EmissiveIntensity;

	/* 发光颜色 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Style", meta=(ToolTip="发光颜色"))
	FLinearColor EmissiveColor;

	/* 是否开启入场动画 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Animation", meta=(ToolTip="是否开启入场动画"))
	bool bEnableEnterAnimation;

	/* 数据文件路径 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Data", meta=(ToolTip="数据文件路径"))
	FString DataFilePath;

	/* 属性映射 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Data", meta=(ToolTip="属性映射"))
	FXVChartPropertyMapping PropertyMapping;

	/* 是否自动加载数据 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Data", meta=(ToolTip="是否自动加载数据"))
	bool bAutoLoadData;

public:
	// Sets default values for this actor's properties
	AXVChartBase();
	virtual ~AXVChartBase();
	
	virtual void SetValue(const FString& InValue);
	virtual void SetStyle();
	virtual void ConstructMesh(double Rate = 1);
	void PrepareMeshSections();
	virtual void ClearSelectedSection(const int& SectionIndex);
	virtual void GenerateAllMeshInfo();
	virtual void UpdateSectionVerticesOfZ(const double& Scale);

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
	
	/* 标志鼠标是否进入该组件 */
	virtual void NotifyActorBeginCursorOver() override;
	virtual void NotifyActorEndCursorOver() override;
	virtual void UpdateOnMouseEnterOrLeft();

	// 数据加载相关函数
	UFUNCTION(BlueprintCallable, Category = "Chart Property | Data")
	virtual bool LoadDataFromFile(const FString& FilePath);
	
	UFUNCTION(BlueprintCallable, Category = "Chart Property | Data")
	virtual bool LoadDataFromString(const FString& Content, const FString& FileExtension);
	
	UFUNCTION(BlueprintCallable, Category = "Chart Property | Data")
	virtual FString GetFormattedDataForChart();
	
	UFUNCTION(BlueprintCallable, Category = "Chart Property | Data")
	UXVDataManager* GetDataManager() const { return ChartDataManager; }

	// 从JSON字符串设置图表数据（自动识别格式）
	UFUNCTION(BlueprintCallable, Category = "Chart Property | Data")
	virtual bool SetValueFromJson(const FString& JsonString);

	// 从命名数据设置图表值（使用现有的PropertyMapping）
	virtual void SetValueFromNamedData(const TArray<TSharedPtr<FJsonObject>>& NamedData);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void BackupVertices();
	
	/* 根据文件扩展名自动选择合适的加载方法 */
	virtual bool LoadDataByFileExtension(const FString& FilePath);
	
	/* 根据图表类型和属性映射转换数据为合适的格式 */
	virtual FString FormatDataByChartType();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	/* 程序化网格组件，作为根组件 */
	UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess= true))
	UProceduralMeshComponent* ProceduralMeshComponent;

	/* 存储所有信息的数组 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<FXVChartSectionInfo> SectionInfos;

	/* 顶点备份 */
	TArray<TArray<FVector>> VerticesBackup;

	/* 区块高度，即Z轴的值 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<float> SectionsHeight;

	/* 动态材质实例 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<UMaterialInstanceDynamic*> DynamicMaterialInstances;

	/* 文本组件 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<UTextRenderComponent*> LabelComponents;

	/* 区块的选择情况数组 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	TArray<bool> SectionSelectStates;

	/* 输入的所有数据计数 */
	UPROPERTY(VisibleDefaultsOnly, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	int TotalCountOfValue;

	/* 当前构建时间 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float CurrentBuildTime;

	/* 总体构建时间，即动画时间 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	float BuildTime;

	/* 当前鼠标是否进入该组件 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	bool bIsMouseEntered;

	/* 是否构造LOD */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	bool bGenerateLOD;

	/* 动画是否结束标记 */
	UPROPERTY(EditAnywhere, Category="Chart Property | Debugging", meta=(AllowPrivateAccess = true))
	bool bAnimationFinished;

	/* 图表数据管理器 */
	UPROPERTY()
	UXVDataManager* ChartDataManager;

	// 存储轴标签
	UPROPERTY(EditAnywhere, Category="Chart Property | Axis")
	TArray<FString> XAxisLabels;
    
	UPROPERTY(EditAnywhere, Category="Chart Property | Axis")
	TArray<FString> YAxisLabels;

	/* GPU生成相关 */
	UPROPERTY(EditAnywhere, Category="Chart Property | GPU")
	bool bEnableGPU;
	
	TSharedPtr<FXRVisSceneViewExtension, ESPMode::ThreadSafe > SceneViewExtension;
	FXRVisGeometryGenerator* GeometryGenerator;
	FXRVisGeometryRenderer* GeometryRenderer;
};
