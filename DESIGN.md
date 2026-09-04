---
name: 东软充电移动端
description: 以城市能源线路图为骨架的浅色 Qt 手机端设计系统
colors:
  route-blue: "#176CFF"
  route-blue-soft: "#EAF1FF"
  live-green: "#149B68"
  live-green-soft: "#E5F6EE"
  warning-amber: "#B66B08"
  warning-amber-soft: "#FFF2D8"
  alert-red: "#D84C4C"
  alert-red-soft: "#FDEAEA"
  mineral-canvas: "#F4F7F6"
  paper-surface: "#FFFFFF"
  ink-blue: "#14243A"
  quiet-ink: "#647487"
  route-line: "#DCE5E8"
typography:
  display:
    fontFamily: "Noto Sans CJK SC, Microsoft YaHei UI, sans-serif"
    fontSize: "28px"
    fontWeight: 700
  title:
    fontFamily: "Noto Sans CJK SC, Microsoft YaHei UI, sans-serif"
    fontSize: "19px"
    fontWeight: 650
  body:
    fontFamily: "Noto Sans CJK SC, Microsoft YaHei UI, sans-serif"
    fontSize: "14px"
    fontWeight: 400
  label:
    fontFamily: "Noto Sans CJK SC, Microsoft YaHei UI, sans-serif"
    fontSize: "13px"
    fontWeight: 400
rounded:
  control: "14px"
  surface: "16px"
  map: "20px"
spacing:
  compact: "8px"
  related: "14px"
  section: "22px"
  screen: "28px"
components:
  button-primary:
    backgroundColor: "{colors.route-blue}"
    textColor: "{colors.paper-surface}"
    rounded: "{rounded.control}"
    height: "50px"
    padding: "0 18px"
  button-secondary:
    backgroundColor: "{colors.route-blue-soft}"
    textColor: "#1558C7"
    rounded: "{rounded.control}"
    height: "50px"
    padding: "0 18px"
  input:
    backgroundColor: "{colors.paper-surface}"
    textColor: "{colors.ink-blue}"
    rounded: "{rounded.control}"
    height: "50px"
    padding: "0 15px"
---

# Design System: 东软充电移动端

## Overview

**Creative North Star: “城市能源线路图”**

界面像一张正在运行的城市交通图：浅色底板负责安静承载，蓝色线路负责引导动作，绿色节点只在服务端确认可用或充电成功时出现。视觉不依靠装饰制造科技感，而是让位置、路线、充电进度和订单状态本身成为识别元素。

整体密度适合 430 × 860 左右的模拟手机窗口。页面避免平均分割成同尺寸卡片，重点内容直接落在画布上，只有余额、费用、站点行等需要组织边界的信息使用浮层。

**Key Characteristics:**

- 冷静的矿物浅灰画布与纸白信息面。
- 一条蓝色动作线路贯穿定位、找站、充电和导航状态。
- 绿色、琥珀色和红色严格表达业务状态。
- 拇指可达的底部导航与大尺寸主操作。

## Colors

蓝色是行动与路线，绿色是已确认的可用状态；背景与文字带轻微冷色，避免普通纯白后台感。

### Primary

- **线路蓝**：主按钮、选中导航、地图线路和焦点描边。
- **线路浅蓝**：次按钮、选中导航托底和信息状态。

### Secondary

- **通电绿**：空闲节点、充电中和成功确认。
- **候车琥珀**：预约、等待和需注意状态。
- **告警红**：服务错误、危险操作和余额不足。

### Neutral

- **矿物画布**：所有页面的连续背景。
- **纸白表面**：输入框、费用面板、站点与充电桩列表行。
- **墨蓝**：标题、正文和关键数字。
- **静默蓝灰**：说明、占位和辅助信息。
- **路线灰**：分隔线、输入描边和充电环底轨。

**The Signal Color Rule.** 绿、琥珀和红只表达业务状态，不用于普通装饰。

## Typography

**Display Font:** Noto Sans CJK SC（Microsoft YaHei UI 后备）  
**Body Font:** Noto Sans CJK SC（Microsoft YaHei UI 后备）

**Character:** 单一无衬线字体保持移动端操作效率，通过重量和尺寸建立层级。金额、里程、电量和功率使用稳定数字排布，不以等宽字体伪造技术感。

### Hierarchy

- **Display**（700，28px）：页面标题；登录主张单独放大到 36px。
- **Title**（650，19px）：区块标题、站点名和关键状态。
- **Body**（400，14px）：表单、操作和主要说明。
- **Label**（400，13px）：位置、单位、帮助和次级状态。

**The One Reading Line Rule.** 一个视图只允许一个最高层级标题；状态标签不得与标题争夺注意力。

## Layout

应用以 430 × 860 为基准竖屏，最小宽度 390px、最大宽度 520px。首页左右边距为 14px，详情页为 18px，登录页为 28px；相关内容间距主要使用 8px 与 14px，区块之间使用 22px 以上。底部导航只出现在四个主页面，任务详情与路线导航使用沉浸式全高布局。

首页让搜索区紧贴标题，随后立即进入大面积线路地图，真实站点列表顺着地图向下延续。站点选择、站点详情、充电桩详情与路线导航形成单向深入、逐级返回的任务链。充电页把状态、进度和主操作放在一条垂直轴上；主操作贴近底部但不遮挡导航。

## Elevation & Depth

系统不使用阴影。层级由背景明度、空间留白和状态色建立：画布承载页面，纸白承载可操作信息，浅蓝承载财务或选中状态。

**The Flat Route Rule.** 静态表面保持平整；不要同时给容器添加边框和阴影。

## Shapes

输入和按钮使用 13px 圆角，信息表面使用 16px，地图使用 20px。状态标签可以使用小型胶囊，但主按钮和卡片不能变成全圆胶囊。地图线路、充电环和导航图标统一使用圆端描边。

## Components

### Buttons

- **Primary:** 线路蓝底、纸白文字、14px 圆角、高 50px；只用于当前视图最重要的动作。
- **Navigation Primary:** 与主按钮完全相同的线路蓝底、纸白文字；用于站点页进入路线导航的最终动作。
- **Secondary:** 浅蓝底与深蓝文字，用于查看、保存和路线规划。
- **Quiet:** 无底色的蓝色文字，用于返回、刷新和同步。
- **Danger:** 浅红底与深红文字，用于结束充电和退出登录。
- **States:** hover 加深一个色阶，pressed 再加深，disabled 转为低对比蓝灰。

**The Bottom Action Rule.** 任务详情页的底部操作区统一使用 18px 左右边距与 18px 底部边距，按钮高度统一为 50px；每页最下面的最终操作始终占满同一宽度和位置。

### Cards / Containers

- **Corner Style:** 信息表面 16px，地图 20px。
- **Background:** 纸白或浅蓝，不使用透明玻璃。
- **Shadow Strategy:** 无阴影。
- **Internal Padding:** 14–20px，取决于信息密度。

### Inputs / Fields

- **Style:** 纸白底、1px 路线灰描边、14px 圆角、高 50px。
- **Focus:** 2px 线路蓝描边，内边距同步减少 1px，避免布局跳动。
- **Error / Disabled:** 错误文案直接出现在字段附近，控件不以震动或装饰动画表达错误。

### Navigation

底部四项导航为“首页 / 充电 / 订单 / 我的”。图标由 Qt Painter 统一绘制为 2px 圆端线条；选中项在图标后使用浅蓝托底，并同时将图标和文字切换为线路蓝。

### Energy Map

配置高德 Key 时由 QWebEngineView 承载真实地图、站点与路线；未配置时使用矿物灰城市底板、白色道路和线路蓝主路线作为可运行降级态。站点由带白色外环的业务状态节点表达；没有真实坐标时不绘制假节点。

### Station and Charger Flow

站点列表突出名称、距离、电价和真实空闲数；站点详情把站名、地址、状态、电价与大号空闲数合并在一张浅蓝身份卡中。充电桩详情同样把桩编号、状态、功率和站点位置合并为完整设备卡，预约与导航固定在底部。路线页让地图占据大部分视口，底部只保留终点、时间、距离和开始导航操作。

### Charge Gauge

270° 开口环只表达真实订单进度；绿色前景与路线灰底轨配合，中心显示百分比或订单状态。没有订单时显示破折号与“等待订单”，不制造默认进度。

## Do's and Don'ts

### Do:

- **Do** 让每个写操作在服务端确认后才切换状态色和按钮文案。
- **Do** 保持 44px 以上的触控目标和底部拇指区主操作。
- **Do** 同时使用文字、形状和颜色表达空闲、等待、故障与离线。
- **Do** 用位置、路线、进度和订单状态形成页面视觉主角。

### Don't:

- **Don't** 用假站点、假订单或假进度填满空状态。
- **Don't** 使用渐变文字、玻璃拟态、彩色光晕或无意义阴影。
- **Don't** 用 emoji 或 Unicode 字符替代正式图标。
- **Don't** 将所有内容平均切成相同尺寸的卡片。
