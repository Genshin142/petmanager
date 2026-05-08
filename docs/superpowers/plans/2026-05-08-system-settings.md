# 系统设置模块 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 开发一个带左侧导航与右侧多页面切换的全局系统设置模态对话框，包含门店信息、打印设置和积分规则三个核心子页面。

**Tech Stack:** C++17, Qt6 (QDialog, QListWidget, QStackedWidget, QFormLayout)

---

### Task 1: 创建对话框主体框架与导航

**Files:**
- Create: `e:\QT\work\PetManager\modules\systemsettingsdialog.h`
- Create: `e:\QT\work\PetManager\modules\systemsettingsdialog.cpp`

- [ ] **Step 1: 编写 systemsettingsdialog.h**
声明主类 `SystemSettingsDialog`，包含左侧 `QListWidget` 和右侧 `QStackedWidget`。声明用于构建不同页面的私有辅助方法：`createStoreInfoPage()`, `createPrintConfigPage()`, `createPointsRulePage()`。

- [ ] **Step 2: 编写 systemsettingsdialog.cpp (基础骨架与 QSS)**
实现左右分栏布局 (`QHBoxLayout`)，设置白灰配色的 QSS。将左侧 ListWidget 的 `currentRowChanged` 信号绑定到右侧 StackedWidget 的 `setCurrentIndex` 槽上。

### Task 2: 实现【门店信息】与【积分规则】页面

- [ ] **Step 1: 实现 createStoreInfoPage()**
在 `systemsettingsdialog.cpp` 中：
使用 `QFormLayout` 或 `QVBoxLayout`。
字段：门店名称、联系电话、详细地址、营业时间。
Logo区域：一个 `QLabel` 用于显示图片（可使用灰色背景加边框模拟占位），加一个“上传 Logo” 的 `QPushButton`。

- [ ] **Step 2: 实现 createPointsRulePage()**
在 `systemsettingsdialog.cpp` 中：
字段：启用积分系统（QCheckBox），消费获分比例（水平布局 + QLineEdit），积分抵扣比例（水平布局 + QLineEdit）。
风控设置：单次最高抵扣订单金额的 X%（带后缀符的输入框）。

### Task 3: 实现【打印设置】页面并接通底层

- [ ] **Step 1: 实现 createPrintConfigPage()**
引入 `<QtPrintSupport/QPrinterInfo>` 以获取系统打印机列表（如果环境不支持，可捕捉或暂时硬编码，这里尝试使用 `QPrinterInfo::availablePrinterNames()` 加载给 QComboBox）。
选项：默认打印机（QComboBox）、纸张规格（80/58）、打印 Logo（CheckBox）、多联打印（CheckBox）、页眉与页脚文本框。

- [ ] **Step 2: 连接保存与取消按钮**
在主面板底部添加“保存配置”与“取消”按钮，点击取消时关闭对话框，点击保存时可弹出成功提示然后关闭。

### Task 4: 项目注册与模块集成

- [ ] **Step 1: 修改 PetManager.pro**
在 `HEADERS` 中添加 `modules/systemsettingsdialog.h`。
在 `SOURCES` 中添加 `modules/systemsettingsdialog.cpp`。
（注意：为了使用 `QPrinterInfo`，需要在 `.pro` 的 `QT +=` 行中添加 `printsupport` 模块）。

- [ ] **Step 2: 修改 personalmodule.cpp**
包含头文件 `#include "systemsettingsdialog.h"`。
在 `onActionClicked` 中的 `actionName == "系统设置"` 分支里，移除原来的 QMessageBox，替换为实例化并 `.exec()` 调用 `SystemSettingsDialog`。
