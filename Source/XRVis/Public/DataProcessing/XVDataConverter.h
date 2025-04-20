#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DataProcessing/XVDataReader.h"
#include "XVDataConverter.generated.h"

/**
 * 数据转换工具类 - 将通用数据格式转换为各种图表所需的格式
 */
UCLASS(BlueprintType, Blueprintable)
class XRVIS_API UXVDataConverter : public UObject
{
    GENERATED_BODY()

public:
    /** 将数据表格转换为柱状图所需的格式 
     * X/Y轴分别使用的列名，Z值（高度）使用的列名 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data|Conversion")
    static FString ConvertToBarChartFormat(const FXVDataTable& DataTable, const FString& XColumn, const FString& YColumn, const FString& ZColumn);

    /** 将数据表格转换为折线图所需的格式 
     * X/Y轴分别使用的列名，Z值使用的列名 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data|Conversion")
    static FString ConvertToLineChartFormat(const FXVDataTable& DataTable, const FString& XColumn, const FString& YColumn, const FString& ZColumn);

    /** 将数据表格转换为饼图所需的格式 
     * 类别名称列和值列 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data|Conversion")
    static TMap<FString, float> ConvertToPieChartFormat(const FXVDataTable& DataTable, const FString& LabelColumn, const FString& ValueColumn);

private:
    /** 查找列索引 */
    static int32 FindColumnIndex(const FXVDataTable& DataTable, const FString& ColumnName);
}; 