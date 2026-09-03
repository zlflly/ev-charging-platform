# EV Charging Platform - 数据分析与大屏模块

**负责人**：韩嘉冉
**分支**：`ran-dashboard-ml`  
**模块**：数据可视化大屏（Web + ECharts）+ 机器学习负荷预测

---

## 📋 项目概述

本模块负责将充电桩平台的业务数据转化为"可分析、可预测、可展示"的数据产品，包括：

1. **数据可视化大屏**：展示营收、订单、设备状态、站点负荷等运营指标
2. **数据预处理管线**：从订单数据构建时间序列特征
3. **充电负荷智能预测**：预测未来 1h/6h/24h 各站点充电负荷、空闲桩数量和高峰时段
4. **模型推理服务**：提供可调用的预测接口

---

## 🗂️ 项目结构

```
ev-charging-platform/
├── data/                      # 数据目录
│   ├── raw/                   # 原始数据（数据库、CSV等）
│   └── processed/             # 预处理后的数据
├── src/                       # 源代码
│   ├── preprocess/            # 数据预处理脚本
│   ├── train/                 # 模型训练脚本
│   └── inference/             # 推理服务
├── models/                    # 训练好的模型文件
├── web/                       # Web大屏前端
│   └── dashboard/             # ECharts可视化页面
├── config.py                  # 配置文件
├── DATA_DICTIONARY.md         # 数据字典
└── README.md                  # 本文件
```

---

## 🎯 核心功能

### 1. 数据预处理管线
- 从 SQLite 数据库提取订单、站点、充电桩数据
- 按站点和小时聚合充电量、订单数、在用桩数
- 构造时间特征（小时、星期、是否周末等）
- 构造滞后特征（lag_1, lag_6, lag_24, lag_168）
- 构造滚动统计特征（rolling_mean）

### 2. 充电负荷预测模型
- **预测目标**：每站每小时充电负荷（kWh）、在用桩数
- **预测窗口**：1小时、6小时、24小时
- **模型类型**：基线模型 + 机器学习模型（RandomForest/GradientBoosting）
- **评估指标**：MAE、RMSE
- **衍生输出**：预测空闲桩数、高峰时段、峰值负荷

### 3. 模型推理服务
- 提供 HTTP API 接口（Flask/FastAPI）
- 输入：stationId、当前时间
- 输出：1h/6h/24h 预测结果（JSON格式）
- 支持 fallback（数据不足时回退到基线预测）

### 4. Web 可视化大屏
- 基于 ECharts 的全屏数据展示页面
- 核心 KPI：总营收、订单数、空闲桩数、设备状态
- 趋势图表：营收趋势、订单趋势
- 站点分析：各站点负荷排名、空闲率
- 预测展示：历史值 vs 预测值、未来高峰时段

---

## 📊 数据契约

详见 [DATA_DICTIONARY.md](DATA_DICTIONARY.md)

### 核心数据表
- **orders**：订单表（orderId, userId, stationId, chargerId, startTime, endTime, energyKwh, status）
- **stations**：充电站表（stationId, stationName, totalChargers）
- **chargers**：充电桩表（chargerId, stationId, chargerType, power, status）

### 预测输出格式
```json
{
  "stationId": 1,
  "stationName": "北理工充电站A",
  "generatedAt": "2026-09-03T16:00:00",
  "forecasts": [
    {
      "horizon": "1h",
      "predictedLoad": 45.2,
      "predictedActiveChargers": 3,
      "predictedFreeChargers": 5,
      "confidence": "high"
    }
  ],
  "modelVersion": "v1.0",
  "source": "model"
}
```

---

## 🚀 快速开始

### 环境要求
- Python 3.8+
- 依赖库：pandas, numpy, scikit-learn, flask, matplotlib

### 安装依赖
```bash
pip install -r requirements.txt
```

### 生成样例数据（开发阶段）
```bash
python src/generate_sample_data.py
```

### 数据预处理
```bash
python src/preprocess/preprocess_pipeline.py
```

### 训练模型
```bash
python src/train/train_model.py
```

### 启动推理服务
```bash
python src/inference/inference_service.py
```

### 启动Web大屏
```bash
cd web/dashboard
python -m http.server 8080
```

---

## 📅 开发计划

| Commit | 功能 | 状态 |
|--------|------|------|
| Commit 0 | 数据契约与工程骨架 | ✅ 进行中 |
| Commit 1 | 数据预处理管线 | ⏳ 待开始 |
| Commit 2 | 基线模型与ML模型 | ⏳ 待开始 |
| Commit 3 | 多时间窗口预测 | ⏳ 待开始 |
| Commit 4 | 模型推理服务 | ⏳ 待开始 |
| Commit 5 | Web大屏骨架 | ⏳ 待开始 |
| Commit 6 | 业务数据图表 | ⏳ 待开始 |
| Commit 7 | 预测结果可视化 | ⏳ 待开始 |
| Commit 8 | 系统集成 | ⏳ 待开始 |
| Commit 9 | 文档与交付 | ⏳ 待开始 |

---

## 🤝 协作接口

### 与成员1（服务端与数据库）
- **需要确认**：订单表字段、站点表字段、数据抽取方式
- **提供**：预测结果输出格式、推理服务接口

### 与成员2（用户Qt客户端）
- **需要确认**：是否需要"低拥堵推荐"功能
- **提供**：预测空闲桩数、站点推荐评分

### 与成员3（管理员Qt客户端）
- **需要确认**：KPI统计口径一致性
- **提供**：相同口径的营收、订单数据

---

## ⚠️ 注意事项

1. **数据口径一致**：营收、订单数等指标必须与成员1的统计保持一致
2. **避免未来信息泄漏**：所有特征必须在预测时刻可获得
3. **模拟数据标记**：开发阶段使用的模拟数据已明确标注
4. **模型可复现**：所有脚本可重复运行，结果可复现

---

## 📝 待办事项

- [ ] 与成员1确认数据库字段和抽取方式
- [ ] 完成数据预处理管线
- [ ] 训练并评估基线模型和ML模型
- [ ] 实现 1h/6h/24h 多窗口预测
- [ ] 部署推理服务
- [ ] 开发 Web 可视化大屏
- [ ] 集成预测结果到系统

---

**最后更新**：2026-09-03  
**当前状态**：Commit 0 进行中
