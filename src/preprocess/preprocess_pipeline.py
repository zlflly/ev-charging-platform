# -*- coding: utf-8 -*-
"""
数据预处理管线
从原始订单数据生成机器学习可用的时间序列特征数据集
"""
import sqlite3
import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import sys
import os

if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

# 导入配置
import sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from config import (
    DATABASE_PATH,
    LAG_FEATURES,
    ROLLING_WINDOWS,
    TRAIN_TEST_SPLIT_RATIO,
    MIN_HISTORICAL_HOURS
)

class DataPreprocessor:
    """数据预处理器"""

    def __init__(self, db_path=DATABASE_PATH):
        self.db_path = db_path
        self.stations_df = None
        self.orders_df = None
        self.hourly_df = None
        self.feature_df = None

    def load_data(self):
        """从数据库加载原始数据"""
        print("\n[Step 1] 从数据库加载原始数据...")
        conn = sqlite3.connect(self.db_path)

        # 加载充电站信息
        self.stations_df = pd.read_sql_query("""
            SELECT stationId, stationName, totalChargers, status
            FROM stations
            WHERE status = 'active'
        """, conn)
        print(f"  - 加载了 {len(self.stations_df)} 个充电站")

        # 加载已完成的订单
        self.orders_df = pd.read_sql_query("""
            SELECT
                orderId,
                stationId,
                chargerId,
                startTime,
                endTime,
                duration,
                energyKwh,
                amount,
                status
            FROM orders
            WHERE status = 'completed'
              AND energyKwh IS NOT NULL
              AND duration IS NOT NULL
        """, conn)

        conn.close()

        # 转换时间字段
        self.orders_df['startTime'] = pd.to_datetime(self.orders_df['startTime'])
        self.orders_df['endTime'] = pd.to_datetime(self.orders_df['endTime'])

        print(f"  - 加载了 {len(self.orders_df)} 条完成订单")
        print(f"  - 时间范围: {self.orders_df['startTime'].min()} 至 {self.orders_df['startTime'].max()}")

        # 数据质量检查
        self._check_data_quality()

        return self

    def _check_data_quality(self):
        """数据质量检查"""
        print("\n[Data Quality Check]")

        # 检查缺失值
        missing = self.orders_df.isnull().sum()
        if missing.sum() > 0:
            print(f"  ! 发现缺失值:")
            print(missing[missing > 0])

        # 检查异常值
        abnormal_duration = self.orders_df[self.orders_df['duration'] > 480]
        if len(abnormal_duration) > 0:
            print(f"  ! 发现 {len(abnormal_duration)} 条异常超长订单 (>480分钟)")

        abnormal_energy = self.orders_df[
            (self.orders_df['energyKwh'] < 0.1) |
            (self.orders_df['energyKwh'] > 200)
        ]
        if len(abnormal_energy) > 0:
            print(f"  ! 发现 {len(abnormal_energy)} 条异常充电量订单 (<0.1 或 >200 kWh)")

        if missing.sum() == 0 and len(abnormal_duration) == 0 and len(abnormal_energy) == 0:
            print("  ✓ 数据质量良好，无明显异常")

    def aggregate_hourly(self):
        """按站点和小时聚合数据"""
        print("\n[Step 2] 按站点和小时聚合数据...")

        # 将开始时间向下取整到小时
        self.orders_df['hour'] = self.orders_df['startTime'].dt.floor('H')

        # 按站点和小时聚合
        hourly_agg = self.orders_df.groupby(['stationId', 'hour']).agg({
            'orderId': 'count',          # 订单数
            'energyKwh': 'sum',          # 总充电量
            'chargerId': 'nunique',      # 在用桩数（去重）
            'duration': 'sum',           # 总充电时长
            'amount': 'sum'              # 总金额
        }).reset_index()

        hourly_agg.columns = [
            'stationId', 'timestamp',
            'order_count', 'total_energy',
            'active_chargers', 'total_duration',
            'total_amount'
        ]

        print(f"  - 聚合后得到 {len(hourly_agg)} 条小时级记录")

        # 创建完整的时间序列索引（填充缺失的小时）
        self.hourly_df = self._fill_missing_hours(hourly_agg)

        print(f"  - 填充后得到 {len(self.hourly_df)} 条记录（包含空时段）")

        return self

    def _fill_missing_hours(self, hourly_agg):
        """填充缺失的小时，确保时间序列连续"""
        # 获取时间范围
        min_time = hourly_agg['timestamp'].min()
        max_time = hourly_agg['timestamp'].max()

        # 创建完整的小时序列
        all_hours = pd.date_range(start=min_time, end=max_time, freq='H')

        # 为每个站点创建完整时间序列
        station_ids = self.stations_df['stationId'].unique()

        complete_index = pd.MultiIndex.from_product(
            [station_ids, all_hours],
            names=['stationId', 'timestamp']
        )

        # 创建完整的DataFrame
        complete_df = pd.DataFrame(index=complete_index).reset_index()

        # 合并实际数据
        merged_df = complete_df.merge(
            hourly_agg,
            on=['stationId', 'timestamp'],
            how='left'
        )

        # 填充缺失值为0（没有订单的时段）
        merged_df = merged_df.fillna({
            'order_count': 0,
            'total_energy': 0,
            'active_chargers': 0,
            'total_duration': 0,
            'total_amount': 0
        })

        # 添加站点信息
        merged_df = merged_df.merge(
            self.stations_df[['stationId', 'stationName', 'totalChargers']],
            on='stationId',
            how='left'
        )

        return merged_df.sort_values(['stationId', 'timestamp']).reset_index(drop=True)

    def create_time_features(self):
        """创建时间特征"""
        print("\n[Step 3] 创建时间特征...")

        df = self.hourly_df.copy()

        # 基础时间特征
        df['hour'] = df['timestamp'].dt.hour
        df['weekday'] = df['timestamp'].dt.weekday  # 0=周一, 6=周日
        df['day_of_month'] = df['timestamp'].dt.day
        df['month'] = df['timestamp'].dt.month
        df['is_weekend'] = (df['weekday'] >= 5).astype(int)

        # 周期性编码（sin/cos）- 让模型理解时间的周期性
        df['hour_sin'] = np.sin(2 * np.pi * df['hour'] / 24)
        df['hour_cos'] = np.cos(2 * np.pi * df['hour'] / 24)
        df['weekday_sin'] = np.sin(2 * np.pi * df['weekday'] / 7)
        df['weekday_cos'] = np.cos(2 * np.pi * df['weekday'] / 7)

        print(f"  - 创建了 9 个时间特征")

        self.feature_df = df
        return self

    def create_lag_features(self):
        """创建滞后特征"""
        print("\n[Step 4] 创建滞后特征...")

        df = self.feature_df.copy()
        lag_count = 0

        # 对每个站点分别创建滞后特征
        for lag in LAG_FEATURES:
            df[f'energy_lag_{lag}'] = df.groupby('stationId')['total_energy'].shift(lag)
            df[f'orders_lag_{lag}'] = df.groupby('stationId')['order_count'].shift(lag)
            df[f'chargers_lag_{lag}'] = df.groupby('stationId')['active_chargers'].shift(lag)
            lag_count += 3

        print(f"  - 创建了 {lag_count} 个滞后特征 (lag: {LAG_FEATURES})")

        self.feature_df = df
        return self

    def create_rolling_features(self):
        """创建滚动统计特征"""
        print("\n[Step 5] 创建滚动统计特征...")

        df = self.feature_df.copy()
        rolling_count = 0

        # 对每个站点分别创建滚动特征
        for window in ROLLING_WINDOWS:
            # 滚动平均
            df[f'energy_rolling_mean_{window}'] = df.groupby('stationId')['total_energy'].transform(
                lambda x: x.shift(1).rolling(window=window, min_periods=1).mean()
            )
            df[f'orders_rolling_mean_{window}'] = df.groupby('stationId')['order_count'].transform(
                lambda x: x.shift(1).rolling(window=window, min_periods=1).mean()
            )

            # 滚动标准差（反映波动性）
            df[f'energy_rolling_std_{window}'] = df.groupby('stationId')['total_energy'].transform(
                lambda x: x.shift(1).rolling(window=window, min_periods=2).std()
            )

            rolling_count += 3

        print(f"  - 创建了 {rolling_count} 个滚动统计特征 (window: {ROLLING_WINDOWS})")

        # 填充 NaN（用0填充标准差的NaN）
        std_cols = [col for col in df.columns if 'rolling_std' in col]
        df[std_cols] = df[std_cols].fillna(0)

        self.feature_df = df
        return self

    def remove_insufficient_data(self):
        """移除历史数据不足的行"""
        print("\n[Step 6] 移除历史数据不足的行...")

        original_len = len(self.feature_df)

        # 移除包含NaN的行（主要是滞后特征导致的前几行）
        self.feature_df = self.feature_df.dropna()

        removed = original_len - len(self.feature_df)
        print(f"  - 移除了 {removed} 行历史数据不足的记录")
        print(f"  - 剩余 {len(self.feature_df)} 条可用于训练的记录")

        return self

    def save_processed_data(self):
        """保存处理后的数据"""
        print("\n[Step 7] 保存处理后的数据...")

        # 确保目录存在
        os.makedirs("data/processed", exist_ok=True)

        # 保存完整特征数据
        output_path = "data/processed/hourly_features.csv"
        self.feature_df.to_csv(output_path, index=False, encoding='utf-8-sig')
        print(f"  - 保存完整数据到: {output_path}")

        # 保存数据摘要
        self._save_data_summary()

        return self

    def _save_data_summary(self):
        """保存数据摘要"""
        summary_path = "data/processed/data_summary.txt"

        with open(summary_path, 'w', encoding='utf-8') as f:
            f.write("="*60 + "\n")
            f.write("数据预处理摘要\n")
            f.write("="*60 + "\n\n")

            f.write(f"生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")

            f.write(f"总记录数: {len(self.feature_df)}\n")
            f.write(f"充电站数: {self.feature_df['stationId'].nunique()}\n")
            f.write(f"时间范围: {self.feature_df['timestamp'].min()} 至 {self.feature_df['timestamp'].max()}\n")
            f.write(f"时间跨度: {(self.feature_df['timestamp'].max() - self.feature_df['timestamp'].min()).days} 天\n\n")

            f.write(f"特征总数: {len(self.feature_df.columns)}\n")
            f.write(f"  - 时间特征: 9\n")
            f.write(f"  - 滞后特征: {len(LAG_FEATURES) * 3}\n")
            f.write(f"  - 滚动特征: {len(ROLLING_WINDOWS) * 3}\n\n")

            f.write("各站点记录数:\n")
            station_counts = self.feature_df.groupby('stationName').size()
            for station, count in station_counts.items():
                f.write(f"  - {station}: {count}\n")

            f.write("\n特征列表:\n")
            for col in self.feature_df.columns:
                f.write(f"  - {col}\n")

        print(f"  - 保存数据摘要到: {summary_path}")

    def get_summary(self):
        """打印处理摘要"""
        print("\n" + "="*60)
        print("数据预处理完成摘要")
        print("="*60)
        print(f"总记录数: {len(self.feature_df)}")
        print(f"充电站数: {self.feature_df['stationId'].nunique()}")
        print(f"时间范围: {self.feature_df['timestamp'].min()} 至 {self.feature_df['timestamp'].max()}")
        print(f"特征总数: {len(self.feature_df.columns)}")
        print("="*60)


def main():
    """主函数"""
    print("="*60)
    print("数据预处理管线")
    print("="*60)

    # 创建预处理器
    preprocessor = DataPreprocessor()

    # 执行预处理流程
    (preprocessor
     .load_data()
     .aggregate_hourly()
     .create_time_features()
     .create_lag_features()
     .create_rolling_features()
     .remove_insufficient_data()
     .save_processed_data()
     .get_summary())

    print("\n[OK] 数据预处理完成！")
    print("下一步: 运行 src/train/train_model.py 训练模型")


if __name__ == "__main__":
    main()
