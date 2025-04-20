#include "DataProcessing/XVDataManager.h"
#include "DataProcessing/XVDataConverter.h"

UXVDataManager::UXVDataManager(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 使用CreateDefaultSubobject创建子对象
    JsonReader = ObjectInitializer.CreateDefaultSubobject<UXVJsonDataReader>(this, TEXT("JsonReader"));
    CsvReader = ObjectInitializer.CreateDefaultSubobject<UXVCsvDataReader>(this, TEXT("CsvReader"));
    ActiveReader = nullptr;
}

bool UXVDataManager::LoadFromJsonFile(const FString& FilePath)
{
    LastError.Empty();
    if (!JsonReader)
    {
        LastError = TEXT("JSON读取器未初始化");
        return false;
    }

    if (JsonReader->ReadFromFile(FilePath))
    {
        ActiveReader = JsonReader;
        return true;
    }
    else
    {
        LastError = JsonReader->GetLastError();
        return false;
    }
}

bool UXVDataManager::LoadFromCsvFile(const FString& FilePath)
{
    LastError.Empty();
    if (!CsvReader)
    {
        LastError = TEXT("CSV读取器未初始化");
        return false;
    }

    if (CsvReader->ReadFromFile(FilePath))
    {
        ActiveReader = CsvReader;
        return true;
    }
    else
    {
        LastError = CsvReader->GetLastError();
        return false;
    }
}

bool UXVDataManager::LoadFromJsonString(const FString& JsonString)
{
    LastError.Empty();
    if (!JsonReader)
    {
        LastError = TEXT("JSON读取器未初始化");
        return false;
    }

    if (JsonReader->ReadFromString(JsonString))
    {
        ActiveReader = JsonReader;
        return true;
    }
    else
    {
        LastError = JsonReader->GetLastError();
        return false;
    }
}

bool UXVDataManager::LoadFromCsvString(const FString& CsvString)
{
    LastError.Empty();
    if (!CsvReader)
    {
        LastError = TEXT("CSV读取器未初始化");
        return false;
    }

    if (CsvReader->ReadFromString(CsvString))
    {
        ActiveReader = CsvReader;
        return true;
    }
    else
    {
        LastError = CsvReader->GetLastError();
        return false;
    }
}

const FXVDataTable& UXVDataManager::GetDataTable() const
{
    static FXVDataTable EmptyTable;
    
    if (ActiveReader)
    {
        return ActiveReader->GetDataTable();
    }
    
    return EmptyTable;
}

FString UXVDataManager::GetLastError() const
{
    return LastError;
}

FString UXVDataManager::ConvertToBarChartData(const FString& XColumn, const FString& YColumn, const FString& ZColumn)
{
    LastError.Empty();
    
    if (!ActiveReader)
    {
        LastError = TEXT("未加载数据");
        return TEXT("");
    }
    
    return UXVDataConverter::ConvertToBarChartFormat(ActiveReader->GetDataTable(), XColumn, YColumn, ZColumn);
}

FString UXVDataManager::ConvertToLineChartData(const FString& XColumn, const FString& YColumn, const FString& ZColumn)
{
    LastError.Empty();
    
    if (!ActiveReader)
    {
        LastError = TEXT("未加载数据");
        return TEXT("");
    }
    
    return UXVDataConverter::ConvertToLineChartFormat(ActiveReader->GetDataTable(), XColumn, YColumn, ZColumn);
}

TMap<FString, float> UXVDataManager::ConvertToPieChartData(const FString& LabelColumn, const FString& ValueColumn)
{
    LastError.Empty();
    
    if (!ActiveReader)
    {
        LastError = TEXT("未加载数据");
        return TMap<FString, float>();
    }
    
    return UXVDataConverter::ConvertToPieChartFormat(ActiveReader->GetDataTable(), LabelColumn, ValueColumn);
} 