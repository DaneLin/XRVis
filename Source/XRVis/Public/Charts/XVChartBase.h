#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "XVChartUtils.h"
#include "DataProcessing/XVDataManager.h"
#include "Rendering/XRVisGeometryRenderer.h"
#include "XVChartBase.generated.h"

class FXRVisSceneViewExtension;

UENUM(BlueprintType)
enum EReferenceComparisonType
{
	Greater UMETA(DisplayName="大于参考值"),
	Less UMETA(DisplayName="小于参考值"),
	Equal UMETA(DisplayName="等于参考值"),
	GreaterOrEqual UMETA(DisplayName="大于等于参考值"),
	LessOrEqual UMETA(DisplayName="小于等于参考值"),
	NotEqual UMETA(DisplayName="不等于参考值")
};

// 添加统计轴线类型枚举
UENUM(BlueprintType)
enum class EStatisticalLineType : uint8
{
	None UMETA(DisplayName="无"),
	Mean UMETA(DisplayName="平均值"),
	Median UMETA(DisplayName="中位数"),
	Max UMETA(DisplayName="最大值"),
	Min UMETA(DisplayName="最小值"),
	Custom UMETA(DisplayName="自定义值")
};

// 添加统计轴线结构体
USTRUCT(BlueprintType)
struct FXVStatisticalLine
{
	GENERATED_BODY()
	
	// 轴线类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Statistical Line")
	EStatisticalLineType LineType = EStatisticalLineType::None;
	
	// 轴线值（如果是Custom类型则使用此值）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Statistical Line", meta=(EditCondition="LineType==EStatisticalLineType::Custom"))
	float CustomValue = 0.0f;
	
	// 实际值（计算后的值）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Statistical Line")
	float ActualValue = 0.0f;
	
	// 轴线颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Statistical Line")
	FLinearColor LineColor = FLinearColor::White;
	
	// 轴线宽度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Statistical Line", meta=(ClampMin="0.1"))
	float LineWidth = 2.0f;
	
	// 是否显示标签
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Statistical Line")
	bool bShowLabel = true;
	
	// 标签文本格式（使用{value}作为值的占位符）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Statistical Line")
	FString LabelFormat = TEXT("{value}");
};

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

UCLASS(Blueprintable)
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

	/* 参考值 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Reference", meta=(ToolTip="用于比较的参考值"))
	float ReferenceValue;

	/* 高亮颜色 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Reference", meta=(ToolTip="符合参考条件的区域高亮颜色"))
	FLinearColor ReferenceHighlightColor;

	/* 参考值比较类型 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Reference", meta=(ToolTip="选择与参考值的比较方式"))
	TEnumAsByte<enum EReferenceComparisonType> ReferenceComparisonType;

	/* 是否启用参考值高亮 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Reference", meta=(ToolTip="是否启用参考值高亮"))
	bool bEnableReferenceHighlight;

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

	/* Z轴控制 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Z-Axis", meta=(ToolTip="是否强制Z轴从0开始"))
	bool bForceZeroBase = true;

	/* Z轴最小值(仅当不强制从0开始时生效) */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Z-Axis", meta=(ToolTip="Z轴最小值(仅当不强制从0开始时生效)", EditCondition="!bForceZeroBase"))
	float MinZAxisValue = 0.0f;

	/* Z轴缩放比例 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Z-Axis", meta=(ToolTip="Z轴缩放比例", ClampMin="0.1", ClampMax="10.0"))
	float ZAxisScale = 1.0f;

	/* 自动调整Z轴范围 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Z-Axis", meta=(ToolTip="自动调整Z轴范围以最佳显示数据差异"))
	bool bAutoAdjustZAxis = false;

	/* 自动调整Z轴时的边距百分比(0-0.5) */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Z-Axis", meta=(ToolTip="自动调整Z轴时的边距百分比", ClampMin="0.0", ClampMax="0.5", EditCondition="bAutoAdjustZAxis"))
	float ZAxisMarginPercent = 0.1f;

	/* 统计轴线 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Statistical Lines", meta=(ToolTip="显示统计数据的轴线"))
	TArray<FXVStatisticalLine> StatisticalLines;

	/* 是否启用统计轴线 */
	UPROPERTY(EditAnywhere, Category = "Chart Property | Statistical Lines", meta=(ToolTip="是否启用统计轴线"))
	bool bEnableStatisticalLines = false;

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
	
	virtual void DrawWithGPU();

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

	/**
	 * 应用参考值高亮到图表
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Reference")
	virtual void ApplyReferenceHighlight();

	/**
	 * 设置参考值
	 * @param InReferenceValue - 新的参考值
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Reference")
	virtual void SetReferenceValue(float InReferenceValue);

	/**
	 * 设置参考值比较类型
	 * @param InComparisonType - 新的比较类型
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Reference")
	virtual void SetReferenceComparisonType(TEnumAsByte<enum EReferenceComparisonType> InComparisonType);

	/**
	 * 检查某个值是否符合参考值条件
	 * @param ValueToCheck - 要检查的值
	 * @return 如果符合条件返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Reference")
	virtual bool CheckAgainstReference(float ValueToCheck) const;

	/**
	 * 设置是否启用参考值高亮
	 * @param bEnable - 是否启用参考值高亮
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Reference")
	virtual void SetEnableReferenceHighlight(bool bEnable);
	
	/**
	 * 设置参考值高亮颜色
	 * @param InColor - 新的高亮颜色
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Reference")
	virtual void SetReferenceHighlightColor(FLinearColor InColor);

	/**
	 * 更新所有统计轴线的值
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Statistical Lines")
	virtual void UpdateStatisticalLineValues();

	/**
	 * 应用统计轴线到图表
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Statistical Lines")
	virtual void ApplyStatisticalLines();

	/**
	 * 添加统计轴线
	 * @param LineType - 轴线类型
	 * @param LineColor - 轴线颜色
	 * @param CustomValue - 如果是自定义类型，则使用此值
	 * @return 新添加的轴线的索引
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Statistical Lines")
	virtual int32 AddStatisticalLine(EStatisticalLineType LineType, FLinearColor LineColor, float CustomValue = 0.0f);

	/**
	 * 移除统计轴线
	 * @param Index - 要移除的轴线索引
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Statistical Lines")
	virtual void RemoveStatisticalLine(int32 Index);

	/**
	 * 计算数据平均值
	 * @return 数据的平均值
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Statistical Lines")
	virtual float CalculateMean() const;

	/**
	 * 计算数据中位数
	 * @return 数据的中位数
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Statistical Lines")
	virtual float CalculateMedian() const;

	/**
	 * 计算数据最大值
	 * @return 数据的最大值
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Statistical Lines")
	virtual float CalculateMax() const;

	/**
	 * 计算数据最小值
	 * @return 数据的最小值
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Statistical Lines")
	virtual float CalculateMin() const;

	/**
	 * 获取所有数据值
	 * @return 包含所有数据值的数组
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Statistical Lines")
	virtual TArray<float> GetAllDataValues() const;

	/**
	 * 设置Z轴范围
	 * @param bFromZero - 是否从0开始
	 * @param MinValue - 最小值(仅当不从0开始时生效)
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Z-Axis")
	virtual void SetZAxisRange(bool bFromZero, float MinValue = 0.0f);

	/**
	 * 设置Z轴缩放比例
	 * @param Scale - 缩放比例
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Z-Axis")
	virtual void SetZAxisScale(float Scale);

	/**
	 * 自动调整Z轴范围
	 * @param MarginPercent - 边距百分比(0-0.5)
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Z-Axis")
	virtual void AutoAdjustZAxis(float MarginPercent = 0.1f);

	/**
	 * 根据Z轴设置计算实际高度
	 * @param RawHeight - 原始高度
	 * @return 调整后的高度
	 */
	UFUNCTION(BlueprintCallable, Category="Chart Property | Z-Axis")
	virtual float CalculateAdjustedHeight(float RawHeight) const;

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
