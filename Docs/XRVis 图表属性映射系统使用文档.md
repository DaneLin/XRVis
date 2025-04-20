# XRVis 图表属性映射系统使用文档

## 1. 概述

XRVis插件现在支持直接使用命名属性格式的JSON数据，无需事先转换为原始格式。这个新功能通过属性映射系统实现，允许您在图表中指定哪些JSON属性应映射到X轴、Y轴和Z值（高度/数值）。

本文档介绍如何使用属性映射系统加载和显示各种格式的数据。

## 2. 属性映射结构

每个图表类（继承自`AXVChartBase`）都包含一个`PropertyMapping`属性，用于配置JSON属性与图表轴的映射关系：

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

## 3. 支持的数据格式

系统支持两种主要的数据格式：

### 3.1 原始格式

原始格式是一个二维数组，其中每个内部数组表示一个数据点的[Y索引, X索引, Z值]：

```json
[
  [0, 0, 74], 
  [0, 1, 65], 
  [1, 0, 25], 
  [1, 1, 31]
]
```

### 3.2 命名属性格式

命名属性格式使用对象数组，每个对象包含命名字段：

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
  },
  {
    "x": "2022/01/01 01:01",
    "y": "生产2线",
    "z": 8
  }
]
```

系统将自动检测JSON格式并使用适当的处理方法。

## 4. 使用属性映射

### 4.1 在编辑器中设置

1. 在Level编辑器中选择您的图表对象
2. 在Details面板中找到"Chart Property | Data"分类
3. 配置PropertyMapping属性：
   - XProperty：指定映射到X轴的JSON属性名称
   - YProperty：指定映射到Y轴的JSON属性名称
   - ZProperty：指定映射到Z值的JSON属性名称
4. 设置DataFilePath指向您的JSON数据文件
5. 将bAutoLoadData设置为true，以便在游戏开始时自动加载数据

### 4.2 在代码中使用

#### 4.2.1 从文件加载数据

```cpp
// 获取图表引用
AXVBarChart* BarChart = Cast<AXVBarChart>(YourBarChartReference);

if (BarChart)
{
    // 设置属性映射
    BarChart->PropertyMapping.XProperty = "x";
    BarChart->PropertyMapping.YProperty = "y";
    BarChart->PropertyMapping.ZProperty = "z";
    
    // 加载数据文件
    BarChart->LoadDataFromFile("Path/To/YourData.json");
}
```

#### 4.2.2 从字符串加载数据

```cpp
// 假设您有一个JSON字符串
FString JsonString = R"([
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
])";

// 设置属性映射并加载数据
BarChart->PropertyMapping.XProperty = "x";
BarChart->PropertyMapping.YProperty = "y";
BarChart->PropertyMapping.ZProperty = "z";
BarChart->SetValueFromJson(JsonString);
```

#### 4.2.3 蓝图中使用

在蓝图中，您可以使用相同的方法设置属性映射并加载数据：

1. 获取对图表的引用
2. 设置PropertyMapping的XProperty、YProperty和ZProperty
3. 调用LoadDataFromFile或SetValueFromJson方法

## 5. 自动轴标签处理

系统会自动从数据中提取唯一的X和Y值，并按照以下流程处理：

1. 从所有数据项中收集唯一的X和Y值
2. 将这些值排序并创建映射（值->索引）
3. 将唯一值作为轴标签应用到图表上
4. 将命名数据转换为图表内部使用的格式

所有这些处理都是自动完成的，您只需设置属性映射并加载数据。

## 6. 示例场景

### 6.1 业务数据可视化

假设您有一个包含日期、产品类别和销售额的JSON数据：

```json
[
  {"date": "2023-01-01", "product": "电脑", "sales": 120},
  {"date": "2023-01-01", "product": "手机", "sales": 230},
  {"date": "2023-01-02", "product": "电脑", "sales": 145},
  {"date": "2023-01-02", "product": "手机", "sales": 180}
]
```

您可以这样设置属性映射：

```cpp
BarChart->PropertyMapping.XProperty = "date";     // X轴显示日期
BarChart->PropertyMapping.YProperty = "product";  // Y轴显示产品
BarChart->PropertyMapping.ZProperty = "sales";    // Z值（高度）表示销售额
```

### 6.2 生产线监控

对于生产线监控数据：

```json
[
  {"timestamp": "2022/01/01 01:01", "line": "生产1线", "output": 10},
  {"timestamp": "2022/01/02 01:02", "line": "生产1线", "output": 12},
  {"timestamp": "2022/01/01 01:01", "line": "生产2线", "output": 8}
]
```

设置映射：

```cpp
BarChart->PropertyMapping.XProperty = "timestamp";  // X轴显示时间戳
BarChart->PropertyMapping.YProperty = "line";       // Y轴显示生产线
BarChart->PropertyMapping.ZProperty = "output";     // Z值表示产量
```

## 7. 饼图的属性映射

对于饼图，您需要使用不同的属性映射字段：

```cpp
PieChart->PropertyMapping.CategoryProperty = "category";  // 类别名称
PieChart->PropertyMapping.ValueProperty = "value";        // 数值
```

适用于这种格式的数据：

```json
[
  {"category": "分类A", "value": 30},
  {"category": "分类B", "value": 25},
  {"category": "分类C", "value": 45}
]
```

## 8. 故障排除

### 8.1 常见问题

1. **数据不显示**
   - 确保PropertyMapping中的属性名称与JSON数据中的字段名称完全匹配（区分大小写）
   - 检查JSON数据格式是否正确
   - 查看输出日志中是否有错误消息

2. **轴标签不正确**
   - 确认数据中的X和Y值是否符合预期
   - 检查PropertyMapping设置

3. **数值（Z值）不正确**
   - 确保ZProperty指向的字段包含数值类型的数据

### 8.2 调试技巧

- 使用`SetValueFromJson`方法加载较小的测试数据集进行验证
- 检查图表对象的`XAxisLabels`和`YAxisLabels`属性，确认是否包含预期的标签值
- 在代码中添加日志输出，查看处理过程中的中间结果


## 9. 总结

属性映射系统为XRVis图表提供了灵活的数据处理能力，支持直接使用命名属性格式的JSON数据。通过简单设置`PropertyMapping`，您可以轻松将各种数据格式映射到图表的轴和值上，无需复杂的数据预处理。

系统会自动处理数据解析、轴标签提取和格式转换，让您专注于可视化效果而非数据格式转换。 