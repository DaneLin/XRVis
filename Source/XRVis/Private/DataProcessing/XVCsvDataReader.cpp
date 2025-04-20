#include "DataProcessing/XVCsvDataReader.h"

UXVCsvDataReader::UXVCsvDataReader()
{
    Delimiter = TEXT(",");
    bHasHeaderRow = true;
    bAutoDetectDelimiter = true;
}

bool UXVCsvDataReader::ReadFromString(const FString& Content)
{
    // 清除之前的数据
    DataTable.Clear();
    LastError.Empty();

    if (Content.IsEmpty())
    {
        LastError = TEXT("CSV内容为空");
        return false;
    }

    // 如果需要，自动检测分隔符
    FString CurrentDelimiter = Delimiter;
    if (bAutoDetectDelimiter)
    {
        CurrentDelimiter = DetectDelimiter(Content);
    }

    // 按行分割
    TArray<FString> Lines;
    Content.ParseIntoArrayLines(Lines, false);

    if (Lines.Num() == 0)
    {
        LastError = TEXT("CSV没有有效行");
        return false;
    }

    // 处理标题行
    int32 StartIndex = 0;
    if (bHasHeaderRow)
    {
        TArray<FString> Headers;
        ParseCSVRow(Lines[0], Headers);
        DataTable.ColumnNames = Headers;
        StartIndex = 1;
    }
    else
    {
        // 如果没有标题行，生成默认列名
        TArray<FString> FirstRow;
        ParseCSVRow(Lines[0], FirstRow);
        
        for (int32 i = 0; i < FirstRow.Num(); ++i)
        {
            DataTable.ColumnNames.Add(FString::Printf(TEXT("Column%d"), i));
        }
    }

    // 处理数据行
    for (int32 i = StartIndex; i < Lines.Num(); ++i)
    {
        if (!Lines[i].IsEmpty())
        {
            TArray<FString> RowCells;
            ParseCSVRow(Lines[i], RowCells);
            
            // 创建数据行
            FXVDataRow RowData;
            
            // 确保行长度与标题数匹配
            while (RowCells.Num() < DataTable.ColumnNames.Num())
            {
                RowCells.Add(TEXT(""));
            }
            
            // 如果行过长，截断
            if (RowCells.Num() > DataTable.ColumnNames.Num())
            {
                RowCells.SetNum(DataTable.ColumnNames.Num());
            }
            
            // 设置单元格数据
            RowData.Cells = RowCells;
            
            DataTable.Rows.Add(RowData);
        }
    }

    return true;
}

void UXVCsvDataReader::ParseCSVRow(const FString& InRow, TArray<FString>& OutRow)
{
    const TCHAR* Start = *InRow;
    const TCHAR* End = Start + InRow.Len();
    const TCHAR* Current = Start;
    bool bInQuotes = false;
    FString Field;

    while (Current < End)
    {
        if (*Current == TEXT('"'))
        {
            bInQuotes = !bInQuotes;
        }
        else if (!bInQuotes && Delimiter.Len() == 1 && Delimiter[0] != 0 && *Current == Delimiter[0])
        {
            // 简单分隔符处理
            OutRow.Add(Field);
            Field.Empty();
        }
        else if (!bInQuotes && Delimiter.Len() > 1 && (End - Current) >= Delimiter.Len() && 
                FCString::Strncmp(Current, *Delimiter, Delimiter.Len()) == 0)
        {
            // 多字符分隔符处理
            OutRow.Add(Field);
            Field.Empty();
            Current += (Delimiter.Len() - 1); // -1 因为之后会 ++Current
        }
        else
        {
            Field.AppendChar(*Current);
        }
        
        ++Current;
    }
    
    // 添加最后一个字段
    OutRow.Add(Field);
}

FString UXVCsvDataReader::DetectDelimiter(const FString& Content)
{
    // 常用的CSV分隔符
    TArray<FString> CommonDelimiters = { TEXT(","), TEXT(";"), TEXT("\t"), TEXT("|") };
    
    // 对每种分隔符，计算在前几行中出现的次数
    TMap<FString, int32> DelimiterCounts;
    
    // 只检查前几行
    TArray<FString> Lines;
    Content.ParseIntoArrayLines(Lines, false);
    int32 LinesToCheck = FMath::Min(10, Lines.Num());
    
    for (int32 i = 0; i < LinesToCheck; ++i)
    {
        for (const FString& Delim : CommonDelimiters)
        {
            int32 Count = 0;
            const TCHAR* Start = *Lines[i];
            const TCHAR* End = Start + Lines[i].Len();
            const TCHAR* Current = Start;
            bool bInQuotes = false;
            
            while (Current < End)
            {
                if (*Current == TEXT('"'))
                {
                    bInQuotes = !bInQuotes;
                }
                else if (!bInQuotes && Delim.Len() == 1 && Delim[0] != 0 && *Current == Delim[0])
                {
                    Count++;
                }
                else if (!bInQuotes && Delim.Len() > 1 && (End - Current) >= Delim.Len() &&
                        FCString::Strncmp(Current, *Delim, Delim.Len()) == 0)
                {
                    Count++;
                    Current += (Delim.Len() - 1);
                }
                
                ++Current;
            }
            
            DelimiterCounts.FindOrAdd(Delim) += Count;
        }
    }
    
    // 查找出现次数最多的分隔符
    FString BestDelimiter = Delimiter; // 默认
    int32 MaxCount = 0;
    
    for (const TPair<FString, int32>& Pair : DelimiterCounts)
    {
        if (Pair.Value > MaxCount)
        {
            MaxCount = Pair.Value;
            BestDelimiter = Pair.Key;
        }
    }
    
    return BestDelimiter;
} 