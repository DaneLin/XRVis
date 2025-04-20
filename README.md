# XRVis - 3D XR可视化组件插件

XRVis是一个为Unreal Engine开发的强大的3D XR数据可视化插件。该插件提供了丰富的3D图表组件，支持VR/AR/MR等XR环境下的数据可视化需求，帮助开发者轻松创建沉浸式数据可视化体验。


## 主要特性

- **多种图表类型**：支持3D柱状图、折线图、饼图等多种可视化图表
- **灵活的数据源**：支持JSON格式数据，包括原始格式和命名属性格式
- **自定义外观**：丰富的样式设置选项，包括材质、颜色、发光效果等
- **完整的坐标轴系统**：支持自定义网格、标签和刻度的3D坐标轴
- **交互式体验**：支持鼠标悬停、选择等交互功能
- **高性能渲染**：支持GPU加速的几何体生成和渲染
- **入场动画**：内置动态的图表生成动画效果
- **完全集成到编辑器**：易于使用的编辑器界面和属性面板

## 支持的图表类型

### 柱状图 (XVBarChart)

柱状图提供多种形状和样式选项：
- 形状：方柱、圆柱、环形柱
- 样式：基础、渐变、透明、动态样式1、动态样式2

适用于比较不同类别之间的数值大小，如产品销售比较、生产线产量比较等。

### 折线图 (XVLineChart)

折线图支持不同的显示样式：
- 线条模式：连接数据点的线条
- 点模式：使用可自定义的球体显示数据点

适用于显示数据的趋势变化，如温度变化、股票价格走势等。

### 饼图 (XVPieChart)

饼图用于展示数据中各部分占整体的比例，支持自定义扇区颜色和标签。

## 数据支持

XRVis支持两种主要的JSON数据格式：

### 原始格式

```json
[
  [0, 0, 74], 
  [0, 1, 65], 
  [1, 0, 25], 
  [1, 1, 31]
]
```

### 命名属性格式

```json
[
  {
    "x": "2022/01/01 01:01",
    "y": "生产1线",
    "z": 10
  },
  {
    "x": "2022/01/02 01:02",
    "y": "生产1线",
    "z": 5
  }
]
```

## 属性映射系统

每个图表都包含一个`PropertyMapping`属性，用于配置JSON属性与图表轴的映射关系：

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

## 坐标轴系统

XRVis提供了专门的`AXVChartAxis`类，用于在3D空间中创建完整的坐标轴系统：

- 自定义坐标轴颜色、粗细和长度
- 可配置的网格线密度和样式
- 支持自动和手动设置轴标签
- 支持数值刻度和分类标签
- 自动根据相机视角调整坐标轴方向

## 使用方法

### 在编辑器中使用

1. 从内容浏览器创建所需的图表Actor（如AXVBarChart、AXVLineChart、AXVPieChart）
2. 配置图表的样式和属性
3. 添加AXVChartAxis Actor并将其附加到图表
4. 配置PropertyMapping和数据源
5. 运行游戏或进入PIE模式查看结果

### 在代码中使用

```cpp
// 创建柱状图
AXVBarChart* BarChart = World->SpawnActor<AXVBarChart>(AXVBarChart::StaticClass(), Location, Rotation);

// 设置属性映射
BarChart->PropertyMapping.XProperty = "date";
BarChart->PropertyMapping.YProperty = "region";
BarChart->PropertyMapping.ZProperty = "sales";

// 创建并附加坐标轴
AXVChartAxis* ChartAxis = World->SpawnActor<AXVChartAxis>(AXVChartAxis::StaticClass());
ChartAxis->AttachToActor(BarChart, FAttachmentTransformRules::KeepRelativeTransform);

// 配置坐标轴
ChartAxis->AxisColor = FColor::Black;
ChartAxis->AxisLineThickness = 2.0f;
ChartAxis->xAxisGridNum = 10;
ChartAxis->yAxisGridNum = 5;

// 加载数据
BarChart->LoadDataFromFile("Path/To/YourData.json");
```

## 高级功能

### GPU加速渲染

XRVis提供了GPU加速的几何体生成和渲染功能，适用于处理大量数据点的场景：

```cpp
BarChart->bEnableGPU = true;  // 启用GPU加速
```

### 自定义材质

XRVis允许为图表设置自定义材质，支持多种视觉效果：

```cpp
BarChart->BaseMaterial = YourCustomMaterial;  // 设置基础材质
BarChart->HoverMaterial = YourHoverMaterial;  // 设置悬停效果材质
```

### 动态数据更新

XRVis支持在运行时动态更新图表数据：

```cpp
// 从字符串加载新数据
FString JsonData = R"([
  {"date": "2023-01", "region": "北区", "sales": 150},
  {"date": "2023-01", "region": "南区", "sales": 120}
])";

BarChart->SetValueFromJson(JsonData);
```


## 系统要求

- Unreal Engine 5.3+
- 启用"ProceduralMeshComponent"插件


## 文档

更多详细信息，请参考Docs目录下的文档：
- [图表坐标轴系统使用文档](Docs/XRVis%20图表坐标轴系统使用文档.md)
- [图表属性映射系统使用文档](Docs/XRVis%20图表属性映射系统使用文档.md)