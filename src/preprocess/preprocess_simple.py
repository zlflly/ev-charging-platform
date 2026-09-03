# -*- coding: utf-8 -*-
"""
简化版数据预处理管线
不需要pandas，使用标准库实现核心功能
"""
import sqlite3
import csv
import sys
import os
from datetime import datetime, timedelta
from collections import defaultdict
import math

if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

# 配置
DATABASE_PATH = "data/raw/charging_platform.db"
OUTPUT_PATH = "data/processed/hourly_features_simple.csv"

def load_stations(conn):
    """加载充电站信息"""
    cursor = conn.cursor()
    cursor.execute("""
        SELECT stationId, stationName, totalChargers
        FROM stations
        WHERE status = 'active'
    """)
    stations = {}
    for row in cursor.fetchall():
        stations[row[0]] = {
            'stationId': row[0],
            'stationName': row[1],
            'totalChargers': row[2]
        }
    return stations

def load_orders(conn):
    """加载订单数据"""
    cursor = conn.cursor()
    cursor.execute("""
        SELECT
            stationId,
            chargerId,
            startTime,
            energyKwh,
            amount
        FROM orders
        WHERE status = 'completed'
          AND energyKwh IS NOT NULL
    """)

    orders = []
    for row in cursor.fetchall():
        orders.append({
            'stationId': row[0],
            'chargerId': row[1],
            'startTime': datetime.strptime(row[2], '%Y-%m-%d %H:%M:%S'),
            'energyKwh': row[3],
            'amount': row[4]
        })

    return orders

def aggregate_hourly(orders):
    """按站点和小时聚合"""
    hourly_data = defaultdict(lambda: {
        'order_count': 0,
        'total_energy': 0.0,
        'active_chargers': set(),
        'total_amount': 0.0
    })

    for order in orders:
        # 向下取整到小时
        hour = order['startTime'].replace(minute=0, second=0, microsecond=0)
        key = (order['stationId'], hour)

        hourly_data[key]['order_count'] += 1
        hourly_data[key]['total_energy'] += order['energyKwh']
        hourly_data[key]['active_chargers'].add(order['chargerId'])
        hourly_data[key]['total_amount'] += order['amount']

    # 转换为列表
    result = []
    for (station_id, timestamp), data in sorted(hourly_data.items()):
        result.append({
            'stationId': station_id,
            'timestamp': timestamp,
            'order_count': data['order_count'],
            'total_energy': round(data['total_energy'], 2),
            'active_chargers': len(data['active_chargers']),
            'total_amount': round(data['total_amount'], 2)
        })

    return result

def add_time_features(hourly_data):
    """添加时间特征"""
    for row in hourly_data:
        ts = row['timestamp']
        row['hour'] = ts.hour
        row['weekday'] = ts.weekday()
        row['day_of_month'] = ts.day
        row['month'] = ts.month
        row['is_weekend'] = 1 if ts.weekday() >= 5 else 0

        # 周期性编码
        row['hour_sin'] = round(math.sin(2 * math.pi * ts.hour / 24), 4)
        row['hour_cos'] = round(math.cos(2 * math.pi * ts.hour / 24), 4)

    return hourly_data

def add_lag_features(hourly_data):
    """添加滞后特征"""
    # 按站点分组
    by_station = defaultdict(list)
    for row in hourly_data:
        by_station[row['stationId']].append(row)

    # 为每个站点添加滞后特征
    for station_id, rows in by_station.items():
        # 按时间排序
        rows.sort(key=lambda x: x['timestamp'])

        for i, row in enumerate(rows):
            # lag_1: 上一小时
            if i >= 1:
                row['energy_lag_1'] = rows[i-1]['total_energy']
            else:
                row['energy_lag_1'] = 0

            # lag_6: 6小时前
            if i >= 6:
                row['energy_lag_6'] = rows[i-6]['total_energy']
            else:
                row['energy_lag_6'] = 0

            # lag_24: 24小时前
            if i >= 24:
                row['energy_lag_24'] = rows[i-24]['total_energy']
            else:
                row['energy_lag_24'] = 0

            # rolling_mean_3: 过去3小时平均
            if i >= 3:
                row['energy_rolling_mean_3'] = round(
                    sum(rows[j]['total_energy'] for j in range(i-3, i)) / 3, 2
                )
            else:
                row['energy_rolling_mean_3'] = 0

    return hourly_data

def save_to_csv(hourly_data, stations, output_path):
    """保存为CSV"""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    fieldnames = [
        'stationId', 'stationName', 'timestamp',
        'order_count', 'total_energy', 'active_chargers', 'total_amount',
        'hour', 'weekday', 'day_of_month', 'month', 'is_weekend',
        'hour_sin', 'hour_cos',
        'energy_lag_1', 'energy_lag_6', 'energy_lag_24',
        'energy_rolling_mean_3'
    ]

    with open(output_path, 'w', newline='', encoding='utf-8-sig') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()

        for row in hourly_data:
            station = stations.get(row['stationId'], {})
            row['stationName'] = station.get('stationName', 'Unknown')
            row['timestamp'] = row['timestamp'].strftime('%Y-%m-%d %H:%M:%S')
            writer.writerow({k: row.get(k, '') for k in fieldnames})

def main():
    print("="*60)
    print("简化版数据预处理管线")
    print("="*60)

    # 连接数据库
    print("\n[Step 1] 连接数据库...")
    conn = sqlite3.connect(DATABASE_PATH)

    # 加载数据
    print("[Step 2] 加载充电站信息...")
    stations = load_stations(conn)
    print(f"  - 加载了 {len(stations)} 个充电站")

    print("[Step 3] 加载订单数据...")
    orders = load_orders(conn)
    print(f"  - 加载了 {len(orders)} 条订单")

    if len(orders) == 0:
        print("[ERROR] 没有订单数据！")
        return

    print(f"  - 时间范围: {min(o['startTime'] for o in orders)} 至 {max(o['startTime'] for o in orders)}")

    # 聚合
    print("[Step 4] 按小时聚合...")
    hourly_data = aggregate_hourly(orders)
    print(f"  - 聚合后得到 {len(hourly_data)} 条小时级记录")

    # 添加特征
    print("[Step 5] 添加时间特征...")
    hourly_data = add_time_features(hourly_data)

    print("[Step 6] 添加滞后特征...")
    hourly_data = add_lag_features(hourly_data)

    # 保存
    print("[Step 7] 保存到CSV...")
    save_to_csv(hourly_data, stations, OUTPUT_PATH)
    print(f"  - 已保存到: {OUTPUT_PATH}")

    print("\n" + "="*60)
    print("数据预处理完成！")
    print("="*60)
    print(f"总记录数: {len(hourly_data)}")
    print(f"充电站数: {len(stations)}")
    print(f"输出文件: {OUTPUT_PATH}")
    print("="*60)

    conn.close()

if __name__ == "__main__":
    main()
