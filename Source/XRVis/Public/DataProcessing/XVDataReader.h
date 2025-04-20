#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "XVDataReader.generated.h"

/**
 * 表示数据表格中的一行
 */
USTRUCT(BlueprintType)
struct XRVIS_API FXVDataRow
{
    GENERATED_BODY()

    /** 行中的单元格数据 */
    UPROPERTY(BlueprintReadOnly, Category = "Data")
    TArray<FString> Cells;
};

/**
 * 表示数据读取后的通用格式
 */
USTRUCT(BlueprintType)
struct XRVIS_API FXVDataTable
{
    GENERATED_BODY()

    /** 数据表格的列名 */
    UPROPERTY(BlueprintReadOnly, Category = "Data")
    TArray<FString> ColumnNames;

    /** 数据表格内容（行） */
    UPROPERTY(BlueprintReadOnly, Category = "Data")
    TArray<FXVDataRow> Rows;

    /** 获取行数 */
    int32 GetRowCount() const { return Rows.Num(); }

    /** 获取列数 */
    int32 GetColumnCount() const { return ColumnNames.Num(); }

    /** 清空数据 */
    void Clear()
    {
        ColumnNames.Empty();
        Rows.Empty();
    }
};

/**
 * 数据读取基类 - 提供从不同格式读取数据的通用接口
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class XRVIS_API UXVDataReader : public UObject
{
    GENERATED_BODY()

public:
    UXVDataReader();

    /** 从文件路径读取数据 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    virtual bool ReadFromFile(const FString& FilePath);

    /** 从字符串内容读取数据 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    virtual bool ReadFromString(const FString& Content) PURE_VIRTUAL(UXVDataReader::ReadFromString, return false;);

    /** 获取解析后的数据表格 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    const FXVDataTable& GetDataTable() const { return DataTable; }

    /** 获取最后一次错误信息 */
    UFUNCTION(BlueprintCallable, Category = "XRVis|Data")
    FString GetLastError() const { return LastError; }

protected:
    /** 解析后的数据表 */
    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FXVDataTable DataTable;

    /** 最后一次错误信息 */
    UPROPERTY(BlueprintReadOnly, Category = "Data")
    FString LastError;
}; 