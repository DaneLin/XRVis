#include "DataProcessing/XVJsonDataReader.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UXVJsonDataReader::UXVJsonDataReader()
{
    bFlattenObjectKeys = true;
    KeySeparator = TEXT(".");
}

bool UXVJsonDataReader::ReadFromString(const FString& Content)
{
    // 清除之前的数据
    DataTable.Clear();
    LastError.Empty();

    if (Content.IsEmpty())
    {
        LastError = TEXT("JSON内容为空");
        return false;
    }

    // 尝试解析JSON
    TSharedPtr<FJsonValue> JsonValue;
    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Content);
    if (!FJsonSerializer::Deserialize(JsonReader, JsonValue) || !JsonValue.IsValid())
    {
        LastError = FString::Printf(TEXT("JSON解析失败: %s"), *JsonReader->GetErrorMessage());
        return false;
    }

    // 根据JSON类型选择处理方式
    if (JsonValue->Type == EJson::Array)
    {
        // 处理JSON数组
        TArray<TSharedPtr<FJsonValue>> JsonArray = JsonValue->AsArray();
        return ReadFromJsonArray(JsonArray);
    }
    else if (JsonValue->Type == EJson::Object)
    {
        // 处理JSON对象
        TSharedPtr<FJsonObject> JsonObject = JsonValue->AsObject();
        return ReadFromJsonObject(JsonObject);
    }
    else
    {
        LastError = TEXT("不支持的JSON格式，必须是数组或对象");
        return false;
    }
}

bool UXVJsonDataReader::ReadFromJsonArray(const TArray<TSharedPtr<FJsonValue>>& JsonArray, bool bHasHeaderRow)
{
    if (JsonArray.Num() == 0)
    {
        LastError = TEXT("JSON数组为空");
        return false;
    }

    // 清除之前的数据
    DataTable.Clear();
    
    // 处理第一行（可能是标题行）
    TArray<FString> Headers;
    int32 StartIndex = 0;
    
    if (bHasHeaderRow)
    {
        // 将第一行作为标题
        if (JsonArray[0]->Type == EJson::Array)
        {
            // 第一行是数组，直接用作标题
            TArray<TSharedPtr<FJsonValue>> HeaderRow = JsonArray[0]->AsArray();
            for (const TSharedPtr<FJsonValue>& HeaderValue : HeaderRow)
            {
                Headers.Add(HeaderValue->AsString());
            }
        }
        else if (JsonArray[0]->Type == EJson::Object)
        {
            // 第一行是对象，使用键作为标题
            TSharedPtr<FJsonObject> FirstRowObj = JsonArray[0]->AsObject();
            FirstRowObj->Values.GetKeys(Headers);
        }
        StartIndex = 1;
    }
    else
    {
        // 如果没有标题行，则生成默认标题（列索引）
        if (JsonArray[0]->Type == EJson::Array)
        {
            TArray<TSharedPtr<FJsonValue>> FirstRow = JsonArray[0]->AsArray();
            for (int32 i = 0; i < FirstRow.Num(); ++i)
            {
                Headers.Add(FString::Printf(TEXT("Column%d"), i));
            }
        }
        else if (JsonArray[0]->Type == EJson::Object)
        {
            TSharedPtr<FJsonObject> FirstRowObj = JsonArray[0]->AsObject();
            FirstRowObj->Values.GetKeys(Headers);
        }
    }
    
    // 设置列名
    DataTable.ColumnNames = Headers;

    // 处理数据行
    for (int32 i = StartIndex; i < JsonArray.Num(); ++i)
    {
        if (JsonArray[i]->Type == EJson::Array)
        {
            // 处理数组格式的行
            TArray<TSharedPtr<FJsonValue>> Row = JsonArray[i]->AsArray();
            FXVDataRow RowData;
            
            for (int32 j = 0; j < Row.Num(); ++j)
            {
                if (j < Headers.Num())
                {
                    RowData.Cells.Add(Row[j]->AsString());
                }
            }
            
            // 如果行长度小于标题数，用空字符串填充
            while (RowData.Cells.Num() < Headers.Num())
            {
                RowData.Cells.Add(TEXT(""));
            }
            
            DataTable.Rows.Add(RowData);
        }
        else if (JsonArray[i]->Type == EJson::Object)
        {
            // 处理对象格式的行
            TSharedPtr<FJsonObject> RowObj = JsonArray[i]->AsObject();
            FXVDataRow RowData;
            
            for (const FString& Header : Headers)
            {
                if (RowObj->HasField(Header))
                {
                    TSharedPtr<FJsonValue> Value = RowObj->Values.FindRef(Header);
                    RowData.Cells.Add(Value->AsString());
                }
                else
                {
                    RowData.Cells.Add(TEXT(""));
                }
            }
            
            DataTable.Rows.Add(RowData);
        }
    }
    
    return true;
}

bool UXVJsonDataReader::ReadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject)
{
    if (!JsonObject.IsValid())
    {
        LastError = TEXT("JSON对象无效");
        return false;
    }

    // 清除之前的数据
    DataTable.Clear();
    
    if (bFlattenObjectKeys)
    {
        // 扁平化处理JSON对象
        TArray<TMap<FString, FString>> FlattenedRows;
        
        // 处理每个顶层字段
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : JsonObject->Values)
        {
            if (Pair.Value->Type == EJson::Object)
            {
                TMap<FString, FString> FlatValues;
                ProcessJsonObject(Pair.Value->AsObject(), Pair.Key, FlatValues);
                
                if (FlattenedRows.Num() == 0)
                {
                    FlattenedRows.Add(FlatValues);
                }
                else
                {
                    // 合并到现有行
                    for (auto& ExistingValues : FlattenedRows)
                    {
                        for (const TPair<FString, FString>& FlatPair : FlatValues)
                        {
                            ExistingValues.Add(FlatPair.Key, FlatPair.Value);
                        }
                    }
                }
            }
            else if (Pair.Value->Type == EJson::Array)
            {
                // 处理数组，每个数组元素作为单独的行
                TArray<TSharedPtr<FJsonValue>> Array = Pair.Value->AsArray();
                
                if (Array.Num() > 0)
                {
                    if (FlattenedRows.Num() == 0)
                    {
                        // 首次添加行
                        for (int32 i = 0; i < Array.Num(); ++i)
                        {
                            TMap<FString, FString> RowValues;
                            
                            if (Array[i]->Type == EJson::Object)
                            {
                                ProcessJsonObject(Array[i]->AsObject(), Pair.Key, RowValues);
                            }
                            else
                            {
                                RowValues.Add(Pair.Key, Array[i]->AsString());
                            }
                            
                            FlattenedRows.Add(RowValues);
                        }
                    }
                    else
                    {
                        // 已有行，需要将数组元素分配到现有行
                        // 这里简化处理，只使用与现有行数量相同的元素
                        int32 MinNum = FMath::Min(FlattenedRows.Num(), Array.Num());
                        
                        for (int32 i = 0; i < MinNum; ++i)
                        {
                            if (Array[i]->Type == EJson::Object)
                            {
                                ProcessJsonObject(Array[i]->AsObject(), Pair.Key, FlattenedRows[i]);
                            }
                            else
                            {
                                FlattenedRows[i].Add(Pair.Key, Array[i]->AsString());
                            }
                        }
                    }
                }
            }
            else
            {
                // 简单值类型
                if (FlattenedRows.Num() == 0)
                {
                    TMap<FString, FString> RowValues;
                    RowValues.Add(Pair.Key, Pair.Value->AsString());
                    FlattenedRows.Add(RowValues);
                }
                else
                {
                    // 将值添加到所有现有行
                    for (auto& ExistingValues : FlattenedRows)
                    {
                        ExistingValues.Add(Pair.Key, Pair.Value->AsString());
                    }
                }
            }
        }
        
        // 收集所有可能的列名
        TSet<FString> AllKeys;
        for (const TMap<FString, FString>& Row : FlattenedRows)
        {
            for (const TPair<FString, FString>& Pair : Row)
            {
                AllKeys.Add(Pair.Key);
            }
        }
        
        // 设置列名
        DataTable.ColumnNames = AllKeys.Array();
        
        // 构建行数据
        for (const TMap<FString, FString>& Row : FlattenedRows)
        {
            FXVDataRow RowData;
            
            for (const FString& Key : DataTable.ColumnNames)
            {
                if (Row.Contains(Key))
                {
                    RowData.Cells.Add(Row.FindRef(Key));
                }
                else
                {
                    RowData.Cells.Add(TEXT(""));
                }
            }
            
            DataTable.Rows.Add(RowData);
        }
    }
    else
    {
        // 简单处理：将JSON对象的顶层键作为列名
        TArray<FString> Keys;
        JsonObject->Values.GetKeys(Keys);
        
        DataTable.ColumnNames = Keys;
        
        // 只创建一行数据
        FXVDataRow RowData;
        
        for (const FString& Key : Keys)
        {
            TSharedPtr<FJsonValue> Value = JsonObject->Values.FindRef(Key);
            RowData.Cells.Add(Value->AsString());
        }
        
        DataTable.Rows.Add(RowData);
    }
    
    return true;
}

void UXVJsonDataReader::ProcessJsonObject(const TSharedPtr<FJsonObject>& JsonObject, const FString& KeyPrefix, TMap<FString, FString>& OutValues)
{
    for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : JsonObject->Values)
    {
        FString NewKey = KeyPrefix.IsEmpty() ? Pair.Key : KeyPrefix + KeySeparator + Pair.Key;
        
        if (Pair.Value->Type == EJson::Object)
        {
            // 递归处理嵌套对象
            ProcessJsonObject(Pair.Value->AsObject(), NewKey, OutValues);
        }
        else if (Pair.Value->Type == EJson::Array)
        {
            // 数组简化处理为字符串，仅用于显示
            FString ArrayStr;
            TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&ArrayStr);
            FJsonSerializer::Serialize(Pair.Value->AsArray(), JsonWriter);
            OutValues.Add(NewKey, ArrayStr);
        }
        else
        {
            // 简单类型直接转为字符串
            OutValues.Add(NewKey, Pair.Value->AsString());
        }
    }
} 