# 商品上架与入库登记解耦实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将商品入库登记从商品模块中拆分出来，建立独立的智能入库工作台，并实现条码自动识别与补货逻辑。

**Architecture:** 采用 Master-Detail 架构建立 `InboundModule`。Detail 面板持久化显示记录详情，Master 区域提供条码驱动的录入工作台。通过 `ProductDataManager` 维护入库流水与库存同步。

**Tech Stack:** C++, Qt 6 (Widgets), Custom UI Design System.

---

### Task 1: 准备工作与 MainWindow 集成

**Files:**
- Modify: `e:/QT/work/PetManager/mainwindow.h`
- Modify: `e:/QT/work/PetManager/mainwindow.cpp`

- [ ] **Step 1: 在 MainWindow 头文件中声明新模块与导航按钮**
```cpp
// mainwindow.h
class InboundModule; // 前向声明
// ...
private:
    QPushButton *navInbound;
    InboundModule *inboundMod;
```

- [ ] **Step 2: 在 MainWindow 中初始化新模块并添加到侧边栏**
```cpp
// mainwindow.cpp
#include "modules/inboundmodule.h"
// ...
void MainWindow::initSidebar() {
    // ... 在 navProduct 之后插入
    navInbound = new QPushButton("📥 商品入库登记");
    navGroup->addButton(navInbound, 11);
    ui->sidebarLayout->insertWidget(5, navInbound); // 紧跟在商品库存管理后面
}

void MainWindow::initModules(UserRole role) {
    // ...
    inboundMod = new InboundModule(this);
    ui->stack->addWidget(inboundMod); // index 11
}
```

- [ ] **Step 3: 提交更改**
```bash
git add mainwindow.h mainwindow.cpp
git commit -m "chore: Integrate InboundModule into MainWindow navigation"
```

---

### Task 2: 创建 InboundModule 基础架构

**Files:**
- Create: `e:/QT/work/PetManager/modules/inboundmodule.h`
- Create: `e:/QT/work/PetManager/modules/inboundmodule.cpp`

- [ ] **Step 1: 定义 InboundModule 类结构**
```cpp
// inboundmodule.h
#ifndef INBOUNDMODULE_H
#define INBOUNDMODULE_H
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include "../common_types.h"

class InboundModule : public QWidget {
    Q_OBJECT
public:
    explicit InboundModule(QWidget *parent = nullptr);
private:
    void setupUI();
    void setupDetailDrawer();
    void updatePreviewCard(const QString &barcode);
    // ... UI 成员变量声明
};
#endif
```

- [ ] **Step 2: 实现基础 UI 框架（Master-Detail）**
```cpp
// inboundmodule.cpp
void InboundModule::setupUI() {
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    // 左侧工作台 (Master)
    QWidget *masterPanel = new QWidget();
    QVBoxLayout *masterLayout = new QVBoxLayout(masterPanel);
    
    // 顶部条码录入区
    QLineEdit *barcodeEdit = new QLineEdit();
    barcodeEdit->setPlaceholderText("扫描或输入条形码...");
    
    // 中间预览卡片
    QFrame *previewCard = new QFrame();
    
    // 底部入库表单
    // ...
    
    // 右侧详情抽屉 (Detail)
    setupDetailDrawer();
    
    rootLayout->addWidget(masterPanel, 1);
    rootLayout->addWidget(m_detailDrawer, 0);
}
```

- [ ] **Step 3: 提交更改**
```bash
git add modules/inboundmodule.h modules/inboundmodule.cpp
git commit -m "feat: Scaffold InboundModule with Master-Detail layout"
```

---

### Task 3: 实现智能识别与录入逻辑

**Files:**
- Modify: `e:/QT/work/PetManager/modules/inboundmodule.cpp`

- [ ] **Step 1: 实现条码回车监听逻辑**
```cpp
connect(barcodeEdit, &QLineEdit::returnPressed, this, [=](){
    QString barcode = barcodeEdit->text().trimmed();
    updatePreviewCard(barcode);
});
```

- [ ] **Step 2: 实现预览卡片更新逻辑（区分已上架/未上架）**
```cpp
void InboundModule::updatePreviewCard(const QString &barcode) {
    ProductInfo info = ProductDataManager::instance()->getProduct(barcode);
    if (!info.barcode.isEmpty()) {
        // 已上架：显示信息，锁定静态字段
        lblStatusTip->setText("✅ 商品已上架，系统已自动填充资料");
        nameEdit->setText(info.name);
        nameEdit->setReadOnly(true);
    } else {
        // 未上架：提示手动输入
        lblStatusTip->setText("⚠️ 该商品尚未上架，入库后请手动去库存看板上架");
        nameEdit->clear();
        nameEdit->setReadOnly(false);
    }
}
```

- [ ] **Step 3: 实现“确认入库”功能，同步库存**
```cpp
void InboundModule::onConfirmInbound() {
    StockInRecord rec;
    rec.barcode = barcodeEdit->text();
    // ... 填充字段
    ProductDataManager::instance()->addRecord(rec);
    
    // 如果已上架，同步更新库存
    ProductInfo info = ProductDataManager::instance()->getProduct(rec.barcode);
    if (!info.barcode.isEmpty()) {
        info.stock += rec.quantity;
        ProductDataManager::instance()->updateProduct(info);
    }
}
```

- [ ] **Step 4: 提交更改**
```bash
git commit -m "feat: Implement barcode auto-fill and stock sync logic"
```

---

### Task 4: 重构商品模块 (ProductModule)

**Files:**
- Modify: `e:/QT/work/PetManager/modules/productmodule.cpp`

- [ ] **Step 1: 移除“入库记录回溯”标签页**
- [ ] **Step 2: 将“商品入库登记”按钮更名为“商品上架”**
- [ ] **Step 3: 调整按钮逻辑，使其专注于建立新档案**
- [ ] **Step 4: 提交更改**
```bash
git commit -m "refactor: Clean up ProductModule and rename inbound buttons to listing"
```

---

### Task 5: 最终验证与 UI 抛光

- [ ] **Step 1: 检查所有模块的样式一致性**
- [ ] **Step 2: 验证店员权限下的可见性**
- [ ] **Step 3: 运行完整入库流测试**
