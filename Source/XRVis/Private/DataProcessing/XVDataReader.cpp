#include "DataProcessing/XVDataReader.h"
#include "Misc/FileHelper.h"

UXVDataReader::UXVDataReader()
{
    // 默认构造函数
}

bool UXVDataReader::ReadFromFile(const FString& FilePath)
{
    FString FileContent;
    if (FFileHelper::LoadFileToString(FileContent, *FilePath))
    {
        return ReadFromString(FileContent);
    }
    else
    {
        LastError = FString::Printf(TEXT("无法读取文件: %s"), *FilePath);
        return false;
    }
} 