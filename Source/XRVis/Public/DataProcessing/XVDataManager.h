#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DataProcessing/XVDataReader.h"
#include "DataProcessing/XVJsonDataReader.h"
#include "DataProcessing/XVCsvDataReader.h"
#include "XVDataManager.generated.h"

/**
 * 数据管理器 - 负责创建和管理各种数据读取器和数据转换
 */
UCLASS(BlueprintType, Blueprintable)
class XRVIS_API UXVDataManager : public UObject
{
    GENERATED_BODY()

public:
    // 构造函数使用FObjectInitializer创建子对象
    UXVDataManager(const FObjectInitializer& ObjectInitializer);

    /** 从JSON文件读取数据 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    bool LoadFromJsonFile(const FString& FilePath);

    /** 从CSV文件读取数据 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    bool LoadFromCsvFile(const FString& FilePath);

    /** 从JSON字符串读取数据 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    bool LoadFromJsonString(const FString& JsonString);

    /** 从CSV字符串读取数据 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    bool LoadFromCsvString(const FString& CsvString);

    /** 获取当前加载的数据表 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    const FXVDataTable& GetDataTable() const;

    /** 获取最后一次错误信息 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    FString GetLastError() const;

    /** 转换为柱状图数据 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data|Conversion")
    FString ConvertToBarChartData(const FString& XColumn, const FString& YColumn, const FString& ZColumn);

    /** 转换为折线图数据 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data|Conversion")
    FString ConvertToLineChartData(const FString& XColumn, const FString& YColumn, const FString& ZColumn);

    /** 转换为饼图数据 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data|Conversion")
    TMap<FString, float> ConvertToPieChartData(const FString& LabelColumn, const FString& ValueColumn);

    /** 获取CSV数据读取器实例 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    UXVCsvDataReader* GetCsvReader() { return CsvReader; }

    /** 获取JSON数据读取器实例 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    UXVJsonDataReader* GetJsonReader() { return JsonReader; }

private:
    /** JSON数据读取器 */
    UPROPERTY()
    UXVJsonDataReader* JsonReader;

    /** CSV数据读取器 */
    UPROPERTY()
    UXVCsvDataReader* CsvReader;

    /** 当前活动的数据读取器 */
    UPROPERTY()
    UXVDataReader* ActiveReader;

    /** 最后一次错误信息 */
    FString LastError;
}; 