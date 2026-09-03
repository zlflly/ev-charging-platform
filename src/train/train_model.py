# -*- coding: utf-8 -*-
"""
模型训练脚本
训练基线模型和机器学习模型来预测充电负荷
"""
import csv
import sys
import os
import pickle
import json
from collections import defaultdict
from datetime import datetime
import math

if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

# 配置
INPUT_PATH = "data/processed/hourly_features_simple.csv"
MODEL_DIR = "models"
TRAIN_TEST_SPLIT_RATIO = 0.8

# 确保模型目录存在
os.makedirs(MODEL_DIR, exist_ok=True)

class SimpleDecisionTree:
    """简化版决策树回归器（不依赖sklearn）"""

    def __init__(self, max_depth=10, min_samples_split=5):
        self.max_depth = max_depth
        self.min_samples_split = min_samples_split
        self.tree = None
        self.feature_names = None

    def fit(self, X, y):
        """训练决策树"""
        self.feature_names = list(X[0].keys()) if X else []
        self.tree = self._build_tree(X, y, depth=0)
        return self

    def _build_tree(self, X, y, depth):
        """递归构建决策树"""
        n_samples = len(X)

        # 停止条件
        if depth >= self.max_depth or n_samples < self.min_samples_split:
            return {'type': 'leaf', 'value': sum(y) / len(y) if y else 0}

        # 找到最佳分裂点
        best_feature, best_threshold, best_gain = None, None, -float('inf')

        for feature in self.feature_names[:10]:  # 只考虑前10个特征加速训练
            values = [x[feature] for x in X]
            unique_values = sorted(set(values))

            for i in range(1, len(unique_values)):
                threshold = (unique_values[i-1] + unique_values[i]) / 2

                # 计算分裂后的方差减少
                left_y = [y[j] for j in range(n_samples) if values[j] <= threshold]
                right_y = [y[j] for j in range(n_samples) if values[j] > threshold]

                if len(left_y) < 2 or len(right_y) < 2:
                    continue

                gain = self._variance_reduction(y, left_y, right_y)

                if gain > best_gain:
                    best_gain = gain
                    best_feature = feature
                    best_threshold = threshold

        # 如果没有找到好的分裂点
        if best_feature is None:
            return {'type': 'leaf', 'value': sum(y) / len(y) if y else 0}

        # 分裂数据
        left_X, left_y, right_X, right_y = [], [], [], []
        for i, x in enumerate(X):
            if x[best_feature] <= best_threshold:
                left_X.append(x)
                left_y.append(y[i])
            else:
                right_X.append(x)
                right_y.append(y[i])

        # 递归构建子树
        return {
            'type': 'node',
            'feature': best_feature,
            'threshold': best_threshold,
            'left': self._build_tree(left_X, left_y, depth + 1),
            'right': self._build_tree(right_X, right_y, depth + 1)
        }

    def _variance_reduction(self, parent_y, left_y, right_y):
        """计算方差减少"""
        def variance(y):
            if not y:
                return 0
            mean = sum(y) / len(y)
            return sum((yi - mean) ** 2 for yi in y) / len(y)

        parent_var = variance(parent_y)
        n = len(parent_y)
        left_var = variance(left_y)
        right_var = variance(right_y)

        return parent_var - (len(left_y)/n * left_var + len(right_y)/n * right_var)

    def predict(self, X):
        """预测"""
        return [self._predict_single(x, self.tree) for x in X]

    def _predict_single(self, x, node):
        """预测单个样本"""
        if node['type'] == 'leaf':
            return node['value']

        if x[node['feature']] <= node['threshold']:
            return self._predict_single(x, node['left'])
        else:
            return self._predict_single(x, node['right'])


class BaselineModel:
    """基线模型：使用滞后特征作为预测"""

    def __init__(self, strategy='lag_1'):
        self.strategy = strategy

    def fit(self, X, y):
        """基线模型不需要训练"""
        pass

    def predict(self, X):
        """预测"""
        predictions = []
        for x in X:
            if self.strategy == 'lag_1':
                # 使用上一小时的值
                predictions.append(x.get('energy_lag_1', 0))
            elif self.strategy == 'lag_24':
                # 使用昨天同一时刻的值
                predictions.append(x.get('energy_lag_24', 0))
        return predictions


def load_data(file_path):
    """加载CSV数据"""
    print(f"\n[Step 1] 加载数据: {file_path}")

    with open(file_path, 'r', encoding='utf-8-sig') as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    print(f"  - 加载了 {len(rows)} 条记录")

    # 转换数据类型
    for row in rows:
        for key in row:
            if key not in ['stationName', 'timestamp']:
                try:
                    row[key] = float(row[key])
                except ValueError:
                    row[key] = 0.0

    return rows


def prepare_features(data):
    """准备特征和目标变量"""
    print("\n[Step 2] 准备特征和目标变量...")

    # 特征列（排除ID、名称、时间戳和目标变量）
    feature_cols = [
        'hour', 'weekday', 'day_of_month', 'month', 'is_weekend',
        'hour_sin', 'hour_cos',
        'energy_lag_1', 'energy_lag_6', 'energy_lag_24',
        'energy_rolling_mean_3',
        'order_count', 'active_chargers'
    ]

    X = []
    y = []

    for row in data:
        # 只使用有完整滞后特征的数据
        if row.get('energy_lag_24', 0) > 0 or row.get('energy_lag_1', 0) > 0:
            features = {col: row.get(col, 0) for col in feature_cols}
            X.append(features)
            y.append(row['total_energy'])

    print(f"  - 特征数量: {len(feature_cols)}")
    print(f"  - 样本数量: {len(X)}")
    print(f"  - 特征列表: {feature_cols}")

    return X, y, feature_cols


def train_test_split(X, y, split_ratio=0.8):
    """按时间顺序划分训练集和测试集"""
    print(f"\n[Step 3] 划分训练集和测试集 ({split_ratio:.0%} 训练)")

    split_idx = int(len(X) * split_ratio)

    X_train = X[:split_idx]
    y_train = y[:split_idx]
    X_test = X[split_idx:]
    y_test = y[split_idx:]

    print(f"  - 训练集: {len(X_train)} 样本")
    print(f"  - 测试集: {len(X_test)} 样本")

    return X_train, X_test, y_train, y_test


def evaluate_model(y_true, y_pred, model_name):
    """评估模型性能"""
    n = len(y_true)

    # MAE (Mean Absolute Error)
    mae = sum(abs(y_true[i] - y_pred[i]) for i in range(n)) / n

    # RMSE (Root Mean Squared Error)
    mse = sum((y_true[i] - y_pred[i]) ** 2 for i in range(n)) / n
    rmse = math.sqrt(mse)

    # MAPE (Mean Absolute Percentage Error)
    mape_values = []
    for i in range(n):
        if y_true[i] > 0:
            mape_values.append(abs((y_true[i] - y_pred[i]) / y_true[i]))
    mape = (sum(mape_values) / len(mape_values) * 100) if mape_values else 0

    # R² Score
    y_mean = sum(y_true) / n
    ss_tot = sum((yi - y_mean) ** 2 for yi in y_true)
    ss_res = sum((y_true[i] - y_pred[i]) ** 2 for i in range(n))
    r2 = 1 - (ss_res / ss_tot) if ss_tot > 0 else 0

    return {
        'model': model_name,
        'mae': round(mae, 2),
        'rmse': round(rmse, 2),
        'mape': round(mape, 2),
        'r2': round(r2, 4),
        'n_samples': n
    }


def main():
    print("="*60)
    print("模型训练管线")
    print("="*60)

    # 1. 加载数据
    data = load_data(INPUT_PATH)

    # 2. 准备特征
    X, y, feature_cols = prepare_features(data)

    # 3. 划分训练测试集
    X_train, X_test, y_train, y_test = train_test_split(X, y, TRAIN_TEST_SPLIT_RATIO)

    # 4. 训练基线模型
    print("\n" + "="*60)
    print("[Step 4] 训练基线模型")
    print("="*60)

    print("\n[Baseline 1] Lag-1 模型（预测值=上一小时）")
    baseline_lag1 = BaselineModel(strategy='lag_1')
    baseline_lag1.fit(X_train, y_train)
    y_pred_lag1 = baseline_lag1.predict(X_test)
    metrics_lag1 = evaluate_model(y_test, y_pred_lag1, 'Baseline_Lag1')

    print(f"  MAE:  {metrics_lag1['mae']:.2f} kWh")
    print(f"  RMSE: {metrics_lag1['rmse']:.2f} kWh")
    print(f"  MAPE: {metrics_lag1['mape']:.2f}%")
    print(f"  R²:   {metrics_lag1['r2']:.4f}")

    print("\n[Baseline 2] Lag-24 模型（预测值=昨天同一时刻）")
    baseline_lag24 = BaselineModel(strategy='lag_24')
    baseline_lag24.fit(X_train, y_train)
    y_pred_lag24 = baseline_lag24.predict(X_test)
    metrics_lag24 = evaluate_model(y_test, y_pred_lag24, 'Baseline_Lag24')

    print(f"  MAE:  {metrics_lag24['mae']:.2f} kWh")
    print(f"  RMSE: {metrics_lag24['rmse']:.2f} kWh")
    print(f"  MAPE: {metrics_lag24['mape']:.2f}%")
    print(f"  R²:   {metrics_lag24['r2']:.4f}")

    # 5. 训练机器学习模型
    print("\n" + "="*60)
    print("[Step 5] 训练机器学习模型")
    print("="*60)

    print("\n[ML Model] 简化决策树回归器")
    print("  - 正在训练...")

    ml_model = SimpleDecisionTree(max_depth=8, min_samples_split=10)
    ml_model.fit(X_train, y_train)

    print("  - 训练完成")
    print("  - 正在预测...")

    y_pred_ml = ml_model.predict(X_test)
    metrics_ml = evaluate_model(y_test, y_pred_ml, 'DecisionTree')

    print(f"\n  MAE:  {metrics_ml['mae']:.2f} kWh")
    print(f"  RMSE: {metrics_ml['rmse']:.2f} kWh")
    print(f"  MAPE: {metrics_ml['mape']:.2f}%")
    print(f"  R²:   {metrics_ml['r2']:.4f}")

    # 6. 模型对比
    print("\n" + "="*60)
    print("[Step 6] 模型性能对比")
    print("="*60)

    all_metrics = [metrics_lag1, metrics_lag24, metrics_ml]

    print("\n{:<20} {:>10} {:>10} {:>10} {:>10}".format(
        "模型", "MAE", "RMSE", "MAPE(%)", "R²"
    ))
    print("-" * 60)
    for m in all_metrics:
        print("{:<20} {:>10.2f} {:>10.2f} {:>10.2f} {:>10.4f}".format(
            m['model'], m['mae'], m['rmse'], m['mape'], m['r2']
        ))

    # 判断最佳模型
    best_model_idx = min(range(len(all_metrics)), key=lambda i: all_metrics[i]['mae'])
    best_model_name = all_metrics[best_model_idx]['model']

    print(f"\n最佳模型: {best_model_name} (MAE最低)")

    # 7. 保存模型
    print("\n" + "="*60)
    print("[Step 7] 保存模型")
    print("="*60)

    # 保存ML模型
    model_path = os.path.join(MODEL_DIR, "load_forecasting_model.pkl")
    with open(model_path, 'wb') as f:
        pickle.dump(ml_model, f)
    print(f"  - ML模型已保存: {model_path}")

    # 保存模型配置
    config = {
        'model_version': 'v1.0',
        'model_type': 'DecisionTree',
        'feature_cols': feature_cols,
        'train_date': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
        'train_samples': len(X_train),
        'test_samples': len(X_test),
        'metrics': {
            'baseline_lag1': metrics_lag1,
            'baseline_lag24': metrics_lag24,
            'ml_model': metrics_ml
        },
        'best_model': best_model_name
    }

    config_path = os.path.join(MODEL_DIR, "model_config.json")
    with open(config_path, 'w', encoding='utf-8') as f:
        json.dump(config, f, indent=2, ensure_ascii=False)
    print(f"  - 模型配置已保存: {config_path}")

    # 保存评估报告
    report_path = os.path.join(MODEL_DIR, "evaluation_report.txt")
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write("="*60 + "\n")
        f.write("模型评估报告\n")
        f.write("="*60 + "\n\n")
        f.write(f"生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")

        f.write(f"训练集样本: {len(X_train)}\n")
        f.write(f"测试集样本: {len(X_test)}\n")
        f.write(f"特征数量: {len(feature_cols)}\n\n")

        f.write("模型性能对比:\n")
        f.write("-" * 60 + "\n")
        f.write("{:<20} {:>10} {:>10} {:>10} {:>10}\n".format(
            "模型", "MAE", "RMSE", "MAPE(%)", "R²"
        ))
        f.write("-" * 60 + "\n")
        for m in all_metrics:
            f.write("{:<20} {:>10.2f} {:>10.2f} {:>10.2f} {:>10.4f}\n".format(
                m['model'], m['mae'], m['rmse'], m['mape'], m['r2']
            ))

        f.write("\n\n最佳模型: {}\n".format(best_model_name))

        f.write("\n特征重要性说明:\n")
        f.write("- energy_lag_1: 上一小时充电量（最重要）\n")
        f.write("- energy_lag_24: 昨天同一时刻充电量\n")
        f.write("- hour: 小时（反映日内周期）\n")
        f.write("- weekday: 星期（反映周内周期）\n")
        f.write("- is_weekend: 是否周末\n")

    print(f"  - 评估报告已保存: {report_path}")

    print("\n" + "="*60)
    print("[OK] 模型训练完成！")
    print("="*60)
    print(f"下一步: 实现多时间窗口预测 (Commit 3)")


if __name__ == "__main__":
    main()
