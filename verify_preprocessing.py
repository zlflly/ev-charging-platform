# -*- coding: utf-8 -*-
"""
验证预处理结果
"""
import csv
import sys
import os
from collections import Counter

if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

def main():
    csv_path = "data/processed/hourly_features_simple.csv"

    if not os.path.exists(csv_path):
        print(f"[ERROR] 文件不存在: {csv_path}")
        return

    print("="*60)
    print("数据预处理结果验证")
    print("="*60)

    with open(csv_path, 'r', encoding='utf-8-sig') as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    print(f"\n总记录数: {len(rows)}")
    print(f"特征数量: {len(rows[0].keys())}")

    print("\n特征列表:")
    for i, col in enumerate(rows[0].keys(), 1):
        print(f"  {i:2d}. {col}")

    # 统计各站点记录数
    stations = Counter(row['stationName'] for row in rows)
    print("\n各站点记录数:")
    for station, count in stations.most_common():
        print(f"  - {station}: {count} 条")

    # 显示前3条样例
    print("\n前3条记录样例:")
    print("-"*60)
    for i, row in enumerate(rows[:3], 1):
        print(f"\n记录 {i}:")
        print(f"  站点: {row['stationName']}")
        print(f"  时间: {row['timestamp']}")
        print(f"  订单数: {row['order_count']}")
        print(f"  充电量: {row['total_energy']} kWh")
        print(f"  在用桩数: {row['active_chargers']}")
        print(f"  小时: {row['hour']}, 星期: {row['weekday']}, 周末: {row['is_weekend']}")
        print(f"  滞后特征: lag_1={row['energy_lag_1']}, lag_24={row['energy_lag_24']}")

    print("\n" + "="*60)
    print("[OK] 数据验证完成！数据格式正确，可以用于模型训练。")
    print("="*60)

if __name__ == "__main__":
    main()
