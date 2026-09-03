# -*- coding: utf-8 -*-
"""
验证训练好的模型
"""
import pickle
import json
import sys
import os

if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

def main():
    print("="*60)
    print("模型验证")
    print("="*60)

    # 1. 加载模型配置
    config_path = "models/model_config.json"
    with open(config_path, 'r', encoding='utf-8') as f:
        config = json.load(f)

    print(f"\n模型版本: {config['model_version']}")
    print(f"模型类型: {config['model_type']}")
    print(f"训练时间: {config['train_date']}")
    print(f"训练样本: {config['train_samples']}")
    print(f"测试样本: {config['test_samples']}")

    # 2. 加载模型
    model_path = "models/load_forecasting_model.pkl"
    with open(model_path, 'rb') as f:
        model = pickle.load(f)

    print(f"\n模型已加载: {model_path}")
    print(f"模型类型: {type(model).__name__}")

    # 3. 显示特征列表
    print(f"\n特征列表 ({len(config['feature_cols'])}个):")
    for i, col in enumerate(config['feature_cols'], 1):
        print(f"  {i:2d}. {col}")

    # 4. 显示模型性能
    print("\n模型性能对比:")
    print("-" * 60)
    metrics = config['metrics']

    print("\n基线模型 Lag-1:")
    m = metrics['baseline_lag1']
    print(f"  MAE:  {m['mae']:.2f} kWh")
    print(f"  RMSE: {m['rmse']:.2f} kWh")
    print(f"  R²:   {m['r2']:.4f}")

    print("\n基线模型 Lag-24:")
    m = metrics['baseline_lag24']
    print(f"  MAE:  {m['mae']:.2f} kWh")
    print(f"  RMSE: {m['rmse']:.2f} kWh")
    print(f"  R²:   {m['r2']:.4f}")

    print("\n机器学习模型:")
    m = metrics['ml_model']
    print(f"  MAE:  {m['mae']:.2f} kWh")
    print(f"  RMSE: {m['rmse']:.2f} kWh")
    print(f"  R²:   {m['r2']:.4f}")

    # 5. 测试预测
    print("\n" + "="*60)
    print("测试预测功能")
    print("="*60)

    # 创建一个测试样本
    test_sample = {
        'hour': 8,
        'weekday': 1,
        'day_of_month': 4,
        'month': 9,
        'is_weekend': 0,
        'hour_sin': 0.866,
        'hour_cos': -0.5,
        'energy_lag_1': 150.0,
        'energy_lag_6': 120.0,
        'energy_lag_24': 140.0,
        'energy_rolling_mean_3': 130.0,
        'order_count': 3,
        'active_chargers': 2
    }

    print("\n输入特征:")
    print(f"  - 时间: 周二早上8点")
    print(f"  - 上一小时充电量: {test_sample['energy_lag_1']} kWh")
    print(f"  - 昨天同时刻充电量: {test_sample['energy_lag_24']} kWh")
    print(f"  - 预期订单数: {test_sample['order_count']}")

    # 预测
    prediction = model.predict([test_sample])[0]

    print(f"\n预测结果:")
    print(f"  - 预测充电负荷: {prediction:.2f} kWh")

    print("\n" + "="*60)
    print("[OK] 模型验证完成！模型可以正常预测。")
    print("="*60)

if __name__ == "__main__":
    main()
