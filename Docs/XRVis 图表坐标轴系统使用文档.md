# XRVis 图表坐标轴系统使用文档

## 1. 概述

XRVis插件提供了一个强大而灵活的图表坐标轴系统，允许在3D空间中可视化数据。坐标轴系统围绕两个主要组件构建：

1. **属性映射**：一个将JSON数据属性映射到图表坐标轴（X, Y, Z）的系统
2. **图表坐标轴**：一个专用的actor类（`AXVChartAxis`），负责处理坐标轴、网格和标签的视觉表示

本文档提供了在XRVis可视化中设置和自定义图表坐标轴的综合指南。

## 2. 属性映射系统

### 2.1 属性映射结构

属性映射系统以`FXVChartPropertyMapping`结构为中心，该结构定义了如何将数据属性映射到图表坐标轴：

```cpp
struct FXVChartPropertyMapping
{
    FString XProperty;       // X轴属性名称
    FString YProperty;       // Y轴属性名称
    FString ZProperty;       // Z轴属性名称（值）
    FString CategoryProperty; // 分类属性名称（饼图）
    FString ValueProperty;    // 数值属性名称（饼图）
};
```

这个结构用于告诉图表应该使用JSON数据中的哪个属性作为每个坐标轴。例如，如果您的数据具有名为"date"、"category"和"value"的属性，您可以这样映射它们：

```cpp
YourChart->PropertyMapping.XProperty = "date";      // X轴
YourChart->PropertyMapping.YProperty = "category";  // Y轴
YourChart->PropertyMapping.ZProperty = "value";     // Z轴（高度/值）
```

### 2.2 自动坐标轴标签生成

使用属性映射系统时，图表会自动：

1. 从XProperty和YProperty字段中提取唯一值
2. 对这些值进行排序
3. 将它们分配为X轴和Y轴的标签
4. 将它们存储在`XAxisLabels`和`YAxisLabels`数组中

然后，这些标签会传递给任何附加的`AXVChartAxis` actor进行视觉渲染。

## 3. AXVChartAxis类

`AXVChartAxis`类负责可视化表示坐标轴系统，包括：

- 坐标轴（X, Y, Z）
- 网格线
- 坐标轴标签
- 刻度标记

### 3.1 主要属性

`AXVChartAxis`类提供了许多自定义属性：

#### 位置和布局
- `AxisDisplacement`：相对于父图表的坐标轴偏移向量
- `bAutoSwitch`：坐标轴是否应该根据摄像机视图自动调整方向
- `AxisLineLength`：坐标轴的长度

#### 线条外观
- `AxisColor`：坐标轴的颜色
- `AxisLineThickness`：坐标轴的粗细
- `GridLineThickness`：网格线的粗细

#### 网格配置
- `xAxisGridNum`, `yAxisGridNum`, `zAxisGridNum`：每个轴的网格部分数量
- `xAxisInterval`, `yAxisInterval`, `zAxisInterval`：网格线之间的间距

#### 文本配置
- `xTextSize`, `yTextSize`, `zTextSize`：坐标轴标签的大小
- `xAxisTextColor`, `yAxisTextColor`, `zAxisTextColor`：坐标轴标签的颜色
- `DistanceOfTextAndXAxis`, `DistanceOfTextAndYAxis`, `DistanceOfTextAndZAxis`：坐标轴与其标签之间的间距

### 3.2 设置坐标轴标签

`AXVChartAxis`类提供了设置坐标轴标签的方法：

```cpp
// 设置单个坐标轴标签
void SetXAxisText(const TArray<FString>& xText);
void SetYAxisText(const TArray<FString>& yText);
void SetZAxisText(const TArray<FString>& zText);

// 一次设置所有坐标轴标签
void SetAxisText(const TArray<FString>& xText, const TArray<FString>& yText, const TArray<FString>& zText);
```

### 3.3 设置坐标轴刻度

对于数值数据，您可以设置刻度标记：

```cpp
// 设置单个坐标轴刻度
void SetXAxisScaleText(const float& xMin, const float& xMax);
void SetYAxisScaleText(const float& yMin, const float& yMax);
void SetZAxisScaleText(const float& zMin, const float& zMax);

// 一次设置所有坐标轴刻度
void SetAxisScaleText(const float& xMin, const float& xMax, const float& yMin, const float& yMax, const float& zMin, const float& zMax);
```

## 4. 在编辑器中设置图表坐标轴

### 4.1 将坐标轴添加到图表

1. 创建您的图表actor（例如，`AXVBarChart`）
2. 创建一个`AXVChartAxis` actor
3. 在World Outliner中将坐标轴actor附加到图表actor
4. 在Details面板中配置坐标轴属性

### 4.2 配置属性映射

1. 选择图表actor
2. 在Details面板中找到"Chart Property | Data"部分
3. 设置PropertyMapping属性（XProperty, YProperty, ZProperty）
4. 将DataFilePath设置为您的JSON数据文件
5. 如果需要，启用bAutoLoadData

## 5. 在代码中设置图表坐标轴

### 5.1 基本图表与坐标轴设置

```cpp
// 创建一个柱状图
AXVBarChart* BarChart = World->SpawnActor<AXVBarChart>(AXVBarChart::StaticClass(), Location, Rotation);

// 设置属性映射
BarChart->PropertyMapping.XProperty = "x";
BarChart->PropertyMapping.YProperty = "y";
BarChart->PropertyMapping.ZProperty = "z";

// 创建并附加一个坐标轴actor
AXVChartAxis* ChartAxis = World->SpawnActor<AXVChartAxis>(AXVChartAxis::StaticClass());
ChartAxis->AttachToActor(BarChart, FAttachmentTransformRules::KeepRelativeTransform);

// 配置坐标轴属性
ChartAxis->AxisColor = FColor::Black;
ChartAxis->AxisLineThickness = 2.0f;
ChartAxis->xAxisGridNum = 10;
ChartAxis->yAxisGridNum = 5;

// 加载数据，坐标轴将自动更新
BarChart->LoadDataFromFile("Path/To/YourData.json");
```

### 5.2 手动设置坐标轴标签

如果您需要手动设置坐标轴标签（而不是使用自动标签生成）：

```cpp
// 定义自定义标签
TArray<FString> XLabels = { "一月", "二月", "三月", "四月", "五月", "六月" };
TArray<FString> YLabels = { "产品A", "产品B", "产品C" };
TArray<FString> ZLabels = { "0%", "25%", "50%", "75%", "100%" };

// 在坐标轴上设置标签
ChartAxis->SetAxisText(XLabels, YLabels, ZLabels);
```

## 6. 示例：配置带有动态数据的图表

这个例子展示了如何设置一个带有动态加载数据的图表：

```cpp
// 创建图表和坐标轴
AXVBarChart* BarChart = World->SpawnActor<AXVBarChart>(...);
AXVChartAxis* ChartAxis = World->SpawnActor<AXVChartAxis>(...);
ChartAxis->AttachToActor(BarChart, FAttachmentTransformRules::KeepRelativeTransform);

// 配置坐标轴外观
ChartAxis->AxisColor = FColor::Black;
ChartAxis->xAxisTextColor = FColor::Blue;
ChartAxis->yAxisTextColor = FColor::Green;
ChartAxis->xTextSize = 8.0f;
ChartAxis->yTextSize = 8.0f;

// 设置属性映射
BarChart->PropertyMapping.XProperty = "date";
BarChart->PropertyMapping.YProperty = "region";
BarChart->PropertyMapping.ZProperty = "sales";

// 从字符串加载数据
FString JsonData = R"([
  {"date": "2023-01", "region": "北区", "sales": 150},
  {"date": "2023-01", "region": "南区", "sales": 120},
  {"date": "2023-02", "region": "北区", "sales": 180},
  {"date": "2023-02", "region": "南区", "sales": 160}
])";

BarChart->SetValueFromJson(JsonData);
```

## 7. 自定义坐标轴外观

### 7.1 网格线

要自定义网格线：

```cpp
ChartAxis->GridLineThickness = 0.5f;  // 更细的网格线
ChartAxis->xAxisGridNum = 20;         // X轴上更多的网格线
ChartAxis->xAxisInterval = 10.0f;     // X网格线之间更小的间隔
```

### 7.2 文本渲染

要自定义文本外观：

```cpp
ChartAxis->xAxisTextColor = FColor::Red;    // 红色X轴标签
ChartAxis->yTextSize = 10.0f;               // 更大的Y轴标签
ChartAxis->DistanceOfTextAndXAxis = 5.0f;   // X轴和标签之间更多的空间
```

### 7.3 坐标轴位置

要调整相对于图表的坐标轴位置：

```cpp
ChartAxis->AxisDisplacement = FVector(-10.0f, -10.0f, -5.0f);  // 移动坐标轴
```

## 8. 高级：使用数值刻度文本

对于带有数值数据的图表，您可以配置刻度文本，而不是或除了分类标签：

```cpp
// 设置刻度范围
ChartAxis->SetAxisScaleText(0, 100, 0, 500, 0, 1000);  // X: 0-100, Y: 0-500, Z: 0-1000
```

## 9. 故障排除

### 9.1 常见问题

1. **标签不显示**
   - 确保您的图表已加载有效数据
   - 检查PropertyMapping是否正确配置
   - 验证坐标轴actor是否正确附加到图表

2. **标签重叠**
   - 调整文本大小（xTextSize, yTextSize, zTextSize）
   - 增加坐标轴间隔（xAxisInterval, yAxisInterval, zAxisInterval）
   - 减少网格线数量（xAxisGridNum, yAxisGridNum, zAxisGridNum）

3. **坐标轴不可见**
   - 检查AxisColor是否与背景不同
   - 增加AxisLineThickness
   - 确保AxisLineLength适合您的图表比例

### 9.2 调试技巧

1. 检查图表上的XAxisLabels和YAxisLabels，确保它们包含预期的值
2. 使用日志记录来验证标签数组是否正确传递：
   ```cpp
   for (const FString& Label : BarChart->XAxisLabels)
   {
       UE_LOG(LogTemp, Log, TEXT("X标签: %s"), *Label);
   }
   ```

## 10. 总结

XRVis图表坐标轴系统提供了一种灵活的方式来显示和标记您的图表数据。通过将属性映射与AXVChartAxis类结合使用，您可以创建具有正确标记的坐标轴、网格线和刻度标记的丰富可视化。该系统同时支持从数据自动生成标签和手动配置，让您完全控制图表的外观。 