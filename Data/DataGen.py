import json
import random
import math

def generate_chart_data(rows, cols, min_value, max_value):
    """
    Generate chart data with explicit property mapping
    """
    data = []
    
    # Generate data with named properties
    for i in range(rows):
        for j in range(cols):
            item = {
                "row": i,          # X property - row
                "column": j,       # Y property - column
                "value": random.randint(min_value, max_value)  # Z property - value
            }
            data.append(item)
            
    return data

def generate_alternative_data(rows, cols, min_value, max_value):
    """
    Generate alternative data format with more meaningful property names
    """
    data = []
    
    # Sales data by year and product category
    years = [2015 + i for i in range(rows)]
    categories = [f"Product{chr(65+i)}" for i in range(cols)]
    
    for year_idx, year in enumerate(years):
        for cat_idx, category in enumerate(categories):
            item = {
                "year": year,             # X property - year
                "category": category,     # Y property - product category
                "sales": random.randint(min_value, max_value)  # Z property - sales value
            }
            data.append(item)
            
    return data

def generate_random_data_with_trend(rows, cols, min_value, max_value):
    """
    Generate random data with trend (without NumPy, English only)
    """
    data = []
    regions = ["North", "South", "East", "West", "Central"]
    products = ["Phone", "Computer", "Tablet", "Headphone", "Watch", "Camera", "Console", "Speaker", "Router", "TV"]
    
    # Extend product list if needed
    if cols > len(products):
        for i in range(cols - len(products)):
            products.append(f"Product{i+1}")
    
    # Extend regions list if needed
    if rows > len(regions):
        for i in range(rows - len(regions)):
            regions.append(f"Region{i+1}")
    
    # Create trend using sine and cosine
    for i in range(rows):
        for j in range(cols):
            # Create base value using math functions
            x_val = 3 * i / max(1, rows - 1)
            y_val = 3 * j / max(1, cols - 1)
            
            # Create trend value based on sine and cosine
            base_value = 30 * math.sin(x_val) + 30 * math.cos(y_val) + 40
            
            # Add random variation
            value = int(base_value + random.uniform(-10, 10))
            
            # Ensure within min and max limits
            value = max(min_value, min(value, max_value))
            
            item = {
                "region": regions[i % len(regions)],    # X property - region
                "product": products[j % len(products)], # Y property - product
                "sales": value                          # Z property - sales value
            }
            data.append(item)
    
    return data

def generate_original_format_data(rows, cols, min_value, max_value):
    """
    Generate data in original format like [[0, 0, 74], [0, 1, 65], ...]
    """
    data = []
    
    for i in range(rows):
        for j in range(cols):
            # Generate data like [0, 0, 74]
            item = [i, j, random.randint(min_value, max_value)]
            data.append(item)
            
    return data

def get_int_input(prompt, default_value, min_allowed=1):
    """Get integer input with default value and minimum value validation"""
    while True:
        try:
            user_input = input(f"{prompt} [default {default_value}]: ")
            if user_input.strip() == "":
                return default_value
            value = int(user_input)
            if value < min_allowed:
                print(f"Please enter a value not less than {min_allowed}")
                continue
            return value
        except ValueError:
            print("Please enter a valid integer")

def main():
    print("=" * 50)
    print("Chart Data Generator - Manual Parameter Input")
    print("=" * 50)
    
    # Get user input parameters
    rows = get_int_input("Enter number of rows", 10)
    cols = get_int_input("Enter number of columns", 10)
    min_value = get_int_input("Enter minimum value", 10, 0)
    max_value = get_int_input("Enter maximum value", 100)
    
    # Ensure max value is greater than min value
    while max_value <= min_value:
        print(f"Maximum value must be greater than minimum value {min_value}")
        max_value = get_int_input("Re-enter maximum value", 100)
    
    print("\nGenerating data...")
    
    # Generate four different data formats
    simple_data = generate_chart_data(rows, cols, min_value, max_value)
    business_data = generate_alternative_data(rows, cols, min_value, max_value)
    trend_data = generate_random_data_with_trend(rows, cols, min_value, max_value)
    original_format_data = generate_original_format_data(rows, cols, min_value, max_value)
    
    # Save JSON files
    with open('chart_data_simple.json', 'w', encoding='utf-8') as f:
        json.dump(simple_data, f, indent=2)
    
    with open('chart_data_business.json', 'w', encoding='utf-8') as f:
        json.dump(business_data, f, indent=2)
    
    with open('chart_data_trend.json', 'w', encoding='utf-8') as f:
        json.dump(trend_data, f, indent=2)
    
    with open('original_format_data.json', 'w', encoding='utf-8') as f:
        json.dump(original_format_data, f, indent=2)
    
    # Print usage instructions
    print("\nSuccessfully generated the following JSON files:")
    print(f"1. chart_data_simple.json - contains {rows}×{cols} = {rows*cols} data points")
    print("   Property mapping: X=row, Y=column, Z=value")
    print()
    print(f"2. chart_data_business.json - contains {rows}×{cols} = {rows*cols} data points")
    print("   Property mapping: X=year, Y=category, Z=sales")
    print()
    print(f"3. chart_data_trend.json - contains {rows}×{cols} = {rows*cols} data points")
    print("   Property mapping: X=region, Y=product, Z=sales")
    print()
    print(f"4. original_format_data.json - contains {rows}×{cols} = {rows*cols} data points")
    print("   Format: [[row, col, value], [row, col, value], ...] (original format for direct use)")
    print()
    print("When using in Unreal Engine, make sure to set the correct property mapping in PropertyMapping\n")

if __name__ == "__main__":
    main()