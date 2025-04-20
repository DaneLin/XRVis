#include "DataProcessing/XVDataConverter.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

FString UXVDataConverter::ConvertToBarChartFormat(const FXVDataTable& DataTable, const FString& XColumn, const FString& YColumn, const FString& ZColumn)
{
    // 查找列索引
    int32 XColIdx = FindColumnIndex(DataTable, XColumn);
    int32 YColIdx = FindColumnIndex(DataTable, YColumn);
    int32 ZColIdx = FindColumnIndex(DataTable, ZColumn);

    // 检查列索引是否有效
    if (XColIdx == INDEX_NONE || YColIdx == INDEX_NONE || ZColIdx == INDEX_NONE)
    {
        return TEXT("");
    }

    // 创建JSON数组
    TArray<TSharedPtr<FJsonValue>> JsonArray;

    for (int32 i = 0; i < DataTable.GetRowCount(); ++i)
    {
        const FXVDataRow& Row = DataTable.Rows[i];
        
        // 确保行有足够的列
        if (Row.Cells.Num() > FMath::Max3(XColIdx, YColIdx, ZColIdx))
        {
            // 创建一个包含三个值的数组 [Y, X, Z]
            TArray<TSharedPtr<FJsonValue>> RowArray;
            
            // 注意：柱状图的格式为 [Y, X, Z] - 确保顺序正确
            int32 Y = FCString::Atoi(*Row.Cells[YColIdx]);
            int32 X = FCString::Atoi(*Row.Cells[XColIdx]);
            float Z = FCString::Atof(*Row.Cells[ZColIdx]);
            
            RowArray.Add(MakeShared<FJsonValueNumber>(Y));
            RowArray.Add(MakeShared<FJsonValueNumber>(X));
            RowArray.Add(MakeShared<FJsonValueNumber>(Z));
            
            JsonArray.Add(MakeShared<FJsonValueArray>(RowArray));
        }
    }

    // 序列化JSON
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonArray, Writer);

    return OutputString;
}

FString UXVDataConverter::ConvertToLineChartFormat(const FXVDataTable& DataTable, const FString& XColumn, const FString& YColumn, const FString& ZColumn)
{
    // 查找列索引
    int32 XColIdx = FindColumnIndex(DataTable, XColumn);
    int32 YColIdx = FindColumnIndex(DataTable, YColumn);
    int32 ZColIdx = FindColumnIndex(DataTable, ZColumn);

    // 检查列索引是否有效
    if (XColIdx == INDEX_NONE || YColIdx == INDEX_NONE || ZColIdx == INDEX_NONE)
    {
        return TEXT("");
    }

    // 创建JSON数组
    TArray<TSharedPtr<FJsonValue>> JsonArray;

    for (int32 i = 0; i < DataTable.GetRowCount(); ++i)
    {
        const FXVDataRow& Row = DataTable.Rows[i];
        
        // 确保行有足够的列
        if (Row.Cells.Num() > FMath::Max3(XColIdx, YColIdx, ZColIdx))
        {
            // 创建一个包含三个值的数组 [Y, X, Z]
            TArray<TSharedPtr<FJsonValue>> RowArray;
            
            // 注意：折线图格式为 [Y, X, Z]
            int32 Y = FCString::Atoi(*Row.Cells[YColIdx]);
            int32 X = FCString::Atoi(*Row.Cells[XColIdx]);
            int32 Z = FCString::Atoi(*Row.Cells[ZColIdx]);
            
            RowArray.Add(MakeShared<FJsonValueNumber>(Y));
            RowArray.Add(MakeShared<FJsonValueNumber>(X));
            RowArray.Add(MakeShared<FJsonValueNumber>(Z));
            
            JsonArray.Add(MakeShared<FJsonValueArray>(RowArray));
        }
    }

    // 序列化JSON
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonArray, Writer);

    return OutputString;
}

TMap<FString, float> UXVDataConverter::ConvertToPieChartFormat(const FXVDataTable& DataTable, const FString& LabelColumn, const FString& ValueColumn)
{
    TMap<FString, float> Result;
    
    // 查找列索引
    int32 LabelColIdx = FindColumnIndex(DataTable, LabelColumn);
    int32 ValueColIdx = FindColumnIndex(DataTable, ValueColumn);

    // 检查列索引是否有效
    if (LabelColIdx == INDEX_NONE || ValueColIdx == INDEX_NONE)
    {
        return Result;
    }

    // 遍历数据表行
    for (int32 i = 0; i < DataTable.GetRowCount(); ++i)
    {
        const FXVDataRow& Row = DataTable.Rows[i];
        
        // 确保行有足够的列
        if (Row.Cells.Num() > FMath::Max(LabelColIdx, ValueColIdx))
        {
            FString Label = Row.Cells[LabelColIdx];
            float Value = FCString::Atof(*Row.Cells[ValueColIdx]);
            
            // 添加或更新值
            if (Result.Contains(Label))
            {
                Result[Label] += Value;
            }
            else
            {
                Result.Add(Label, Value);
            }
        }
    }

    return Result;
}

int32 UXVDataConverter::FindColumnIndex(const FXVDataTable& DataTable, const FString& ColumnName)
{
    return DataTable.ColumnNames.Find(ColumnName);
} 