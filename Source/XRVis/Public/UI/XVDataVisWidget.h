#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Charts/XVBarChart.h"
#include "XVDataVisWidget.generated.h"

/**
 * 数据可视化UI组件，提供文件加载和图表绘制功能
 */
UCLASS(Blueprintable)
class XRVIS_API UXVDataVisWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 构造函数
    UXVDataVisWidget(const FObjectInitializer& ObjectInitializer);

    // 蓝图可直接设置目标图表
    UFUNCTION(BlueprintCallable, Category = "Chart")
    void SetTargetChart(AXVBarChart* InChart);

protected:
    virtual void NativeConstruct() override;
    
    // UI组件
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UButton* LoadFileButton;
    
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UButton* DrawButton;
    
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UTextBlock* StatusText;
    
    // 图表引用
    UPROPERTY(BlueprintReadWrite, Category = "Chart")
    AXVBarChart* TargetChart;

    UPROPERTY(BlueprintReadWrite, Category = "Chart")
    bool bDataLoaded;
    
    // 文件加载函数
    UFUNCTION(BlueprintCallable, Category = "File Operations")
    void OnLoadFileClicked();
    
    // 绘制函数
    UFUNCTION(BlueprintCallable, Category = "Chart Operations")
    void OnDrawClicked();
    
    // 设置按钮状态
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateButtonStates();
}; 