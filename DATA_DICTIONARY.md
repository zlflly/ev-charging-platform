# 数据字典

## 一、业务数据表结构

### 1.1 订单表（orders）
| 字段名 | 类型 | 说明 | 来源 |
|--------|------|------|------|
| orderId | INT | 订单ID（主键） | 成员1 |
| userId | INT | 用户ID | 成员1 |
| stationId | INT | 充电站ID | 成员1 |
| chargerId | INT | 充电桩ID | 成员1 |
| startTime | DATETIME | 充电开始时间 | 成员1 |
| endTime | DATETIME | 充电结束时间 | 成员1 |
| duration | INT | 充电时长（分钟） | 成员1 |
| energyKwh | FLOAT | 充电量（千瓦时） | 成员1 |
| amount | DECIMAL | 订单金额（元） | 成员1 |
| status | VARCHAR | 订单状态（completed/cancelled/pending） | 成员1 |

**关键判断标准**：
- 只有 `status='completed'` 的订单才计入统计和模型训练
- `startTime` 和 `endTime` 用于确定订单所属时间段
- `energyKwh` 是预测的核心目标之一

---

### 1.2 充电站表（stations）
| 字段名 | 类型 | 说明 | 来源 |
|--------|------|------|------|
| stationId | INT | 充电站ID（主键） | 成员1 |
| stationName | VARCHAR | 充电站名称 | 成员1 |
| address | VARCHAR | 地址 | 成员1 |
| latitude | FLOAT | 纬度 | 成员1 |
| longitude | FLOAT | 经度 | 成员1 |
| totalChargers | INT | 充电桩总数 | 成员1 |
| status | VARCHAR | 站点状态（active/inactive） | 成员1 |

---

### 1.3 充电桩表（chargers）
| 字段名 | 类型 | 说明 | 来源 |
|--------|------|------|------|
| chargerId | INT | 充电桩ID（主键） | 成员1 |
| stationId | INT | 所属充电站ID | 成员1 |
| chargerType | VARCHAR | 充电桩类型（fast/slow） | 成员1 |
| power | FLOAT | 额定功率（千瓦） | 成员1 |
| status | VARCHAR | 充电桩状态（available/occupied/fault） | 成员1 |

---

## 二、预测目标定义

### 2.1 核心预测指标

我们选择**站点级小时粒度**作为预测单元：

| 预测目标 | 定义 | 单位 | 说明 |
|---------|------|------|------|
| **hourlyLoad** | 每站每小时总充电量 | kWh | 该站点在该小时内所有完成订单的 `energyKwh` 总和 |
| **activeChargers** | 每站每小时在用桩数 | 个 | 该小时内有订单正在充电的桩数量（去重） |
| **orderCount** | 每站每小时订单数 | 个 | 该小时内完成的订单总数 |

### 2.2 预测时间窗口（Horizon）

- **1h**：预测未来1小时的充电负荷
- **6h**：预测未来6小时的充电负荷
- **24h**：预测未来24小时的充电负荷

### 2.3 衍生输出

| 输出字段 | 定义 | 计算方式 |
|---------|------|----------|
| **predictedFreeChargers** | 预测空闲桩数 | `totalChargers - predictedActiveChargers` |
| **peakTime** | 高峰时段 | 在预测窗口内负荷最大的时刻 |
| **peakLoad** | 峰值负荷 | 高峰时段的预测负荷值 |

---

## 三、时间序列特征设计

### 3.1 时间特征
- `hour`：小时（0-23）
- `weekday`：星期几（0=周一, 6=周日）
- `is_weekend`：是否周末（0/1）
- `month`：月份（1-12）
- `day_of_month`：每月第几天（1-31）

### 3.2 滞后特征（Lag Features）
- `lag_1`：上一小时的充电量
- `lag_6`：6小时前的充电量
- `lag_24`：24小时前的充电量（昨天同一时刻）
- `lag_168`：168小时前的充电量（上周同一时刻）

### 3.3 滚动统计特征
- `rolling_mean_3h`：过去3小时平均充电量
- `rolling_mean_6h`：过去6小时平均充电量
- `rolling_mean_24h`：过去24小时平均充电量

---

## 四、数据质量约定

### 4.1 缺失值处理
- 无订单的时间段：充电量填充为 0
- 异常超长订单（duration > 480分钟）：记录日志但保留
- 缺失 `energyKwh` 字段：跳过该订单并记录

### 4.2 时间对齐
- 所有时间戳按小时取整（向下取整到整点）
- 时间序列必须连续，不允许跳跃

### 4.3 训练/验证集划分
- 按时间顺序划分，前80%训练，后20%验证
- **禁止随机打乱**，避免未来信息泄漏

---

## 五、协作接口约定

### 5.1 与成员1的数据契约
- 成员1提供 SQLite 数据库或 CSV 导出
- 订单表必须包含 `startTime`, `endTime`, `energyKwh`, `status` 字段
- 站点表必须包含 `totalChargers` 字段

### 5.2 预测结果输出格式（JSON）
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
    },
    {
      "horizon": "6h",
      "predictedLoad": 38.6,
      "predictedActiveChargers": 2,
      "predictedFreeChargers": 6,
      "peakTime": "2026-09-03T18:00:00",
      "peakLoad": 52.1,
      "confidence": "medium"
    },
    {
      "horizon": "24h",
      "predictedLoad": 42.8,
      "predictedActiveChargers": 3,
      "predictedFreeChargers": 5,
      "peakTime": "2026-09-04T09:00:00",
      "peakLoad": 68.3,
      "confidence": "low"
    }
  ],
  "modelVersion": "v1.0",
  "source": "model"
}
```

---

## 六、可选扩展特征（暂不实现）

以下特征在说明书中提及，但由于数据来源未明确，暂标记为"可选扩展"：

- **天气数据**：温度、降水、天气类型
- **节假日标记**：是否法定节假日
- **实时路况**：周边交通拥堵情况

这些特征可在基础模型完成后，通过外部 API 或 CSV 补充。

---

## 七、注意事项

1. **不能有未来信息泄漏**：任何特征都必须在预测时刻可获得
2. **保持口径一致**：营收、订单数等指标必须与成员1的统计口径一致
3. **模拟数据标记**：如使用模拟数据，必须明确标注，后续可无缝替换真实数据
