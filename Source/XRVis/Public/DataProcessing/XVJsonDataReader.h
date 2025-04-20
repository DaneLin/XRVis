#pragma once

#include "CoreMinimal.h"
#include "DataProcessing/XVDataReader.h"
#include "Dom/JsonObject.h"
#include "XVJsonDataReader.generated.h"

/**
 * 用于读取Json格式数据的读取器
 */
UCLASS(BlueprintType)
class XRVIS_API UXVJsonDataReader : public UXVDataReader
{
    GENERATED_BODY()

public:
    UXVJsonDataReader();

    /** 从字符串读取JSON数据 */
    virtual bool ReadFromString(const FString& Content) override;

    /** 从数组型JSON读取 - 注意：此方法不暴露给蓝图 */
    bool ReadFromJsonArray(const TArray<TSharedPtr<FJsonValue>>& JsonArray, bool bHasHeaderRow = false);

    /** 从对象型JSON读取 - 注意：此方法不暴露给蓝图 */
    bool ReadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject);

    /** 是否解析为层次式列名 (对象型JSON) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XRVis|Data|Json")
    bool bFlattenObjectKeys;

    /** 扁平化键的分隔符 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XRVis|Data|Json")
    FString KeySeparator;

private:
    /** 处理嵌套JSON对象，生成扁平化键 */
    void ProcessJsonObject(const TSharedPtr<FJsonObject>& JsonObject, const FString& KeyPrefix, TMap<FString, FString>& OutValues);
}; 