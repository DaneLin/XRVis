#include "UI/XVDataVisWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"

UXVDataVisWidget::UXVDataVisWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bDataLoaded = false;
}

void UXVDataVisWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // 绑定按钮点击事件
    if (LoadFileButton)
    {
        LoadFileButton->OnClicked.AddDynamic(this, &UXVDataVisWidget::OnLoadFileClicked);
    }
    
    if (DrawButton)
    {
        DrawButton->OnClicked.AddDynamic(this, &UXVDataVisWidget::OnDrawClicked);
    }
    
    // 初始状态下绘制按钮不可用
    UpdateButtonStates();
}

void UXVDataVisWidget::SetTargetChart(AXVBarChart* InChart)
{
    TargetChart = InChart;
    
    // 更新按钮状态
    UpdateButtonStates();
}


void UXVDataVisWidget::OnLoadFileClicked()
{
    TArray<FString> OutFilenames;
    FString DefaultPath = FPaths::ProjectSavedDir();
    
    // 获取桌面平台接口
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        void* ParentWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
        
        // 弹出文件选择对话框
        bool bSuccess = DesktopPlatform->OpenFileDialog(
            ParentWindowHandle,
            TEXT("选择数据文件"),
            DefaultPath,
            TEXT(""),
            TEXT("文本文件|*.json|CSV文件|*.csv|所有文件|*.*"),
            EFileDialogFlags::None,
            OutFilenames
        );
        
        // 如果用户选择了文件
        if (bSuccess && OutFilenames.Num() > 0)
        {
            FString SelectedFile = OutFilenames[0];
            
            // 读取文件内容
            if (TargetChart->LoadDataFromFile(*SelectedFile))
            {
                bDataLoaded = true;
                if (StatusText)
                {
                    StatusText->SetText(FText::FromString(FString::Printf(TEXT("已加载文件: %s"), *FPaths::GetCleanFilename(SelectedFile))));
                }
                
                // 更新按钮状态
                UpdateButtonStates();
            }
            else
            {
                if (StatusText)
                {
                    StatusText->SetText(FText::FromString(TEXT("文件加载失败")));
                }
            }
        }
    }
}

void UXVDataVisWidget::OnDrawClicked()
{
    // 检查是否有图表引用和加载的数据
    if (TargetChart)
    {
        TargetChart->DrawWithGPU();
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(TEXT("绘制完成")));
        }
    }
    else
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(TEXT("绘制失败：未找到图表引用或数据为空")));
        }
    }
}

void UXVDataVisWidget::UpdateButtonStates()
{
    // 加载按钮总是可用
    if (LoadFileButton)
    {
        LoadFileButton->SetIsEnabled(true);
    }
    
    // 绘制按钮只有在加载了数据后才可用
    if (DrawButton)
    {
        DrawButton->SetIsEnabled(bDataLoaded && TargetChart != nullptr);
    }
} 