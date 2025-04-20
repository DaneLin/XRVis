#pragma once

#include "CoreMinimal.h"
#include "DataProcessing/XVDataReader.h"
#include "XVCsvDataReader.generated.h"

/**
 * 用于读取CSV格式数据的读取器
 */
UCLASS(BlueprintType)
class XRVIS_API UXVCsvDataReader : public UXVDataReader
{
    GENERATED_BODY()

public:
    UXVCsvDataReader();

    /** 从字符串读取CSV数据 */
    virtual bool ReadFromString(const FString& Content) override;

    /** 分隔符 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XRVis|Data|CSV")
    FString Delimiter;

    /** 是否将第一行作为标题 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XRVis|Data|CSV")
    bool bHasHeaderRow;

    /** 是否自动检测分隔符 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XRVis|Data|CSV")
    bool bAutoDetectDelimiter;

private:
    /** 分析CSV行内容 */
    void ParseCSVRow(const FString& InRow, TArray<FString>& OutRow);

    /** 自动检测分隔符 */
    FString DetectDelimiter(const FString& Content);
}; 