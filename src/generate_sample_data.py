# -*- coding: utf-8 -*-
"""
样例数据生成脚本

用途：
在成员1提供真实数据之前，生成结构一致的模拟数据用于开发测试。
生成的数据结构与最终数据库保持一致，后续可无缝替换。

注意：
- 生成的数据已标记为模拟数据
- 仅用于开发阶段验证代码逻辑
- 不作为最终演示数据
"""

import sqlite3
import random
from datetime import datetime, timedelta
import os
import sys

# 设置输出编码为UTF-8
if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

# 确保数据目录存在
os.makedirs("data/raw", exist_ok=True)

# 连接数据库
conn = sqlite3.connect("data/raw/charging_platform.db")
cursor = conn.cursor()

# 创建表结构
cursor.execute("""
CREATE TABLE IF NOT EXISTS stations (
    stationId INTEGER PRIMARY KEY,
    stationName TEXT NOT NULL,
    address TEXT,
    latitude REAL,
    longitude REAL,
    totalChargers INTEGER NOT NULL,
    status TEXT DEFAULT 'active'
)
""")

cursor.execute("""
CREATE TABLE IF NOT EXISTS chargers (
    chargerId INTEGER PRIMARY KEY,
    stationId INTEGER NOT NULL,
    chargerType TEXT NOT NULL,
    power REAL NOT NULL,
    status TEXT DEFAULT 'available',
    FOREIGN KEY (stationId) REFERENCES stations(stationId)
)
""")

cursor.execute("""
CREATE TABLE IF NOT EXISTS users (
    userId INTEGER PRIMARY KEY,
    username TEXT NOT NULL,
    phone TEXT,
    balance REAL DEFAULT 0.0
)
""")

cursor.execute("""
CREATE TABLE IF NOT EXISTS orders (
    orderId INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    stationId INTEGER NOT NULL,
    chargerId INTEGER NOT NULL,
    startTime TEXT NOT NULL,
    endTime TEXT,
    duration INTEGER,
    energyKwh REAL,
    amount REAL,
    status TEXT DEFAULT 'pending',
    FOREIGN KEY (userId) REFERENCES users(userId),
    FOREIGN KEY (stationId) REFERENCES stations(stationId),
    FOREIGN KEY (chargerId) REFERENCES chargers(chargerId)
)
""")

print("[OK] 数据表创建成功")

# 插入模拟充电站数据
stations_data = [
    (1, "北理工充电站A", "北京市海淀区中关村南大街5号", 39.9611, 116.3678, 8, "active"),
    (2, "北理工充电站B", "北京市海淀区中关村南大街7号", 39.9622, 116.3689, 6, "active"),
    (3, "中关村充电站", "北京市海淀区中关村大街1号", 39.9789, 116.3456, 10, "active"),
    (4, "五道口充电站", "北京市海淀区成府路28号", 39.9934, 116.3401, 5, "active"),
]

cursor.executemany("""
INSERT OR REPLACE INTO stations
(stationId, stationName, address, latitude, longitude, totalChargers, status)
VALUES (?, ?, ?, ?, ?, ?, ?)
""", stations_data)

print(f"[OK] 插入 {len(stations_data)} 个充电站")

# 插入模拟充电桩数据
chargers_data = []
charger_id = 1
for station in stations_data:
    station_id = station[0]
    total_chargers = station[5]
    for i in range(total_chargers):
        charger_type = "fast" if i % 3 == 0 else "slow"
        power = 120.0 if charger_type == "fast" else 60.0
        chargers_data.append((charger_id, station_id, charger_type, power, "available"))
        charger_id += 1

cursor.executemany("""
INSERT OR REPLACE INTO chargers
(chargerId, stationId, chargerType, power, status)
VALUES (?, ?, ?, ?, ?)
""", chargers_data)

print(f"[OK] 插入 {len(chargers_data)} 个充电桩")

# 插入模拟用户数据
users_data = [(i, f"user_{i}", f"138{i:08d}", random.uniform(50, 500)) for i in range(1, 51)]
cursor.executemany("""
INSERT OR REPLACE INTO users (userId, username, phone, balance)
VALUES (?, ?, ?, ?)
""", users_data)

print(f"[OK] 插入 {len(users_data)} 个用户")

# 生成模拟订单数据（最近30天）
print("[INFO] 正在生成模拟订单数据（最近30天）...")

start_date = datetime.now() - timedelta(days=30)
orders_generated = 0

for day in range(30):
    current_date = start_date + timedelta(days=day)

    # 每天生成不同数量的订单（模拟高峰和低谷）
    is_weekend = current_date.weekday() >= 5
    base_orders = random.randint(30, 50) if is_weekend else random.randint(40, 70)

    for _ in range(base_orders):
        # 随机选择充电站和桩
        station = random.choice(stations_data)
        station_id = station[0]
        available_chargers = [c for c in chargers_data if c[1] == station_id]
        charger = random.choice(available_chargers)
        charger_id = charger[0]

        # 随机选择用户
        user_id = random.randint(1, 50)

        # 生成充电时间（早高峰7-9点、晚高峰17-19点概率更高）
        hour_weights = [2]*7 + [10]*2 + [3]*8 + [10]*2 + [3]*5  # 24小时权重
        hour = random.choices(range(24), weights=hour_weights)[0]
        minute = random.randint(0, 59)

        start_time = current_date.replace(hour=hour, minute=minute, second=0)

        # 充电时长（30-180分钟）
        duration = random.randint(30, 180)
        end_time = start_time + timedelta(minutes=duration)

        # 充电量（基于时长和功率）
        charger_power = charger[3]
        energy_kwh = round((duration / 60) * charger_power * random.uniform(0.7, 0.9), 2)

        # 计算金额（假设1.5元/kWh）
        amount = round(energy_kwh * 1.5, 2)

        # 90%的订单完成，10%取消或进行中
        status_choices = ["completed"] * 9 + ["cancelled"]
        status = random.choice(status_choices)

        cursor.execute("""
        INSERT INTO orders
        (userId, stationId, chargerId, startTime, endTime, duration, energyKwh, amount, status)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            user_id, station_id, charger_id,
            start_time.strftime("%Y-%m-%d %H:%M:%S"),
            end_time.strftime("%Y-%m-%d %H:%M:%S") if status == "completed" else None,
            duration if status == "completed" else None,
            energy_kwh if status == "completed" else None,
            amount if status == "completed" else None,
            status
        ))

        orders_generated += 1

conn.commit()
print(f"[OK] 插入 {orders_generated} 条模拟订单")

# 验证数据
cursor.execute("SELECT COUNT(*) FROM orders WHERE status='completed'")
completed_orders = cursor.fetchone()[0]

cursor.execute("SELECT MIN(startTime), MAX(startTime) FROM orders")
time_range = cursor.fetchone()

print("\n" + "="*50)
print("数据生成摘要")
print("="*50)
print(f"充电站数量: {len(stations_data)}")
print(f"充电桩数量: {len(chargers_data)}")
print(f"用户数量: {len(users_data)}")
print(f"订单总数: {orders_generated}")
print(f"已完成订单: {completed_orders}")
print(f"时间范围: {time_range[0]} 至 {time_range[1]}")
print("="*50)
print("\n[WARNING] 注意：这是模拟数据，仅用于开发测试！")
print("[OK] 数据库文件保存在: data/raw/charging_platform.db")

conn.close()
