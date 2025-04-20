import json
import random
import math

def generate_chart_data(rows, cols, min_value, max_value):
    """
    生成类似于示例的柱状图数据，但带有明确的属性映射
    """
    data = []
    
    # 生成与示例相似的数据，但使用命名属性
    for i in range(rows):
        for j in range(cols):
            # 使用命名属性代替索引位置
            item = {
                "row": i,          # X属性 - 行
                "column": j,       # Y属性 - 列
                "value": random.randint(min_value, max_value)  # Z属性 - 数值
            }
            data.append(item)
            
    return data

def generate_alternative_data(rows, cols, min_value, max_value):
    """
    生成另一种格式的数据，使用更有意义的属性名
    """
    data = []
    
    # 假设是销售数据，按年份和产品类别
    years = [2015 + i for i in range(rows)]
    categories = [f"产品{chr(65+i)}" for i in range(cols)]
    
    for year_idx, year in enumerate(years):
        for cat_idx, category in enumerate(categories):
            item = {
                "year": year,             # X属性 - 年份
                "category": category,     # Y属性 - 产品类别
                "sales": random.randint(min_value, max_value)  # Z属性 - 销售额
            }
            data.append(item)
            
    return data

def generate_random_data_with_trend(rows, cols, min_value, max_value):
    """
    生成具有趋势的随机数据（不使用NumPy）
    """
    data = []
    regions = ["北部", "南部", "东部", "西部", "中部"]
    products = ["手机", "电脑", "平板", "耳机", "手表", "相机", "游戏机", "音箱", "路由器", "电视"]
    
    # 如果提供的列比products长，则扩展products
    if cols > len(products):
        for i in range(cols - len(products)):
            products.append(f"产品{i+1}")
    
    # 如果提供的行比regions长，则扩展regions
    if rows > len(regions):
        for i in range(rows - len(regions)):
            regions.append(f"区域{i+1}")
    
    # 使用正弦和余弦创建一些趋势
    for i in range(rows):
        for j in range(cols):
            # 使用数学函数创建基础值
            x_val = 3 * i / max(1, rows - 1)
            y_val = 3 * j / max(1, cols - 1)
            
            # 基于正弦和余弦的有趋势值
            base_value = 30 * math.sin(x_val) + 30 * math.cos(y_val) + 40
            
            # 添加一些随机变化
            value = int(base_value + random.uniform(-10, 10))
            
            # 确保在最小和最大值之间
            value = max(min_value, min(value, max_value))
            
            item = {
                "region": regions[i % len(regions)],    # X属性 - 区域
                "product": products[j % len(products)], # Y属性 - 产品
                "sales": value                          # Z属性 - 销售值
            }
            data.append(item)
    
    return data

def get_int_input(prompt, default_value, min_allowed=1):
    """获取整数输入，带有默认值和最小值验证"""
    while True:
        try:
            user_input = input(f"{prompt} [默认 {default_value}]: ")
            if user_input.strip() == "":
                return default_value
            value = int(user_input)
            if value < min_allowed:
                print(f"请输入不小于 {min_allowed} 的值")
                continue
            return value
        except ValueError:
            print("请输入有效的整数")

def main():
    print("=" * 50)
    print("柱状图数据生成器 - 手动参数输入")
    print("=" * 50)
    
    # 获取用户输入的参数
    rows = get_int_input("请输入行数", 10)
    cols = get_int_input("请输入列数", 10)
    min_value = get_int_input("请输入最小值", 10, 0)
    max_value = get_int_input("请输入最大值", 100)
    
    # 确保最大值大于最小值
    while max_value <= min_value:
        print(f"最大值必须大于最小值 {min_value}")
        max_value = get_int_input("请重新输入最大值", 100)
    
    print("\n正在生成数据...")
    
    # 生成三种不同格式的数据
    simple_data = generate_chart_data(rows, cols, min_value, max_value)
    business_data = generate_alternative_data(rows, cols, min_value, max_value)
    trend_data = generate_random_data_with_trend(rows, cols, min_value, max_value)
    
    # 保存到JSON文件
    with open('chart_data_simple.json', 'w', encoding='utf-8') as f:
        json.dump(simple_data, f, indent=2)
    
    with open('chart_data_business.json', 'w', encoding='utf-8') as f:
        json.dump(business_data, f, indent=2)
    
    with open('chart_data_trend.json', 'w', encoding='utf-8') as f:
        json.dump(trend_data, f, indent=2)
    
    # 打印使用说明
    print("\n成功生成以下JSON文件:")
    print(f"1. chart_data_simple.json - 包含 {rows}×{cols} = {rows*cols} 个数据点")
    print("   属性映射: X=row, Y=column, Z=value")
    print()
    print(f"2. chart_data_business.json - 包含 {rows}×{cols} = {rows*cols} 个数据点")
    print("   属性映射: X=year, Y=category, Z=sales")
    print()
    print(f"3. chart_data_trend.json - 包含 {rows}×{cols} = {rows*cols} 个数据点")
    print("   属性映射: X=region, Y=product, Z=sales")
    print()
    print("在Unreal中使用时，请确保在PropertyMapping中设置正确的属性名称\n")

if __name__ == "__main__":
    main()