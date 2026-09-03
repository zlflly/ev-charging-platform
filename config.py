# 数据库连接配置
DATABASE_PATH = "data/raw/charging_platform.db"

# 数据处理配置
TRAIN_TEST_SPLIT_RATIO = 0.8  # 训练集占比
MIN_HISTORICAL_HOURS = 168  # 至少需要168小时（7天）的历史数据

# 时间特征配置
LAG_FEATURES = [1, 6, 24, 168]  # 滞后特征：1h, 6h, 24h, 168h
ROLLING_WINDOWS = [3, 6, 24]  # 滚动窗口：3h, 6h, 24h

# 预测窗口配置
HORIZONS = {
    "1h": 1,
    "6h": 6,
    "24h": 24
}

# 模型配置
MODEL_VERSION = "v1.0"
MODEL_SAVE_PATH = "models/"

# 推理服务配置
INFERENCE_HOST = "127.0.0.1"
INFERENCE_PORT = 5000

# Web 大屏配置
DASHBOARD_PORT = 8080
DASHBOARD_REFRESH_INTERVAL = 60  # 秒

# 数据质量阈值
MAX_DURATION_MINUTES = 480  # 最大充电时长（分钟）
MIN_ENERGY_KWH = 0.1  # 最小充电量（千瓦时）
MAX_ENERGY_KWH = 200  # 最大充电量（千瓦时）
