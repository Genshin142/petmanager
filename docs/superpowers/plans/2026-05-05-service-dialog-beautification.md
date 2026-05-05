# 服务项目对话框美化实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将服务项目对话框升级为现代化的、分组的卡片式风格，并支持服务描述字段。

**Architecture:** 采用无边框窗口 + 阴影背景框 + 自定义标题栏。使用 QSS 进行全局样式注入，模仿“新增会员档案”的视觉语言。字段采用分组网格布局。

**Tech Stack:** Qt (C++), QSS, QGraphicsDropShadowEffect.

---

### Task 1: 更新 ServiceDialog 头文件

**Files:**
- Modify: `e:\QT\work\PetManager\modules\servicedialog.h`

- [ ] **Step 1: 添加必要的头文件和成员变量**
    - 添加 `QTextEdit`, `QFrame`, `QVBoxLayout` 等头文件（如果缺失）。
    - 在 `private` 区域添加 UI 组件指针：`m_descriptionEdit`, `m_bgFrame`, `m_titleLabel` 等。

```cpp
// e:\QT\work\PetManager\modules\servicedialog.h

#include <QTextEdit>
#include <QFrame>

// ... inside class ServiceDialog ...
private:
    void setupUi();
    QWidget* createInputWithUnit(QWidget* input, const QString &unit, const QString &label);

    QFrame *m_bgFrame;
    QLabel *m_titleLabel;
    QLineEdit *m_nameEdit;
    QComboBox *m_categoryCombo;
    QLineEdit *m_idEdit;
    QLineEdit *m_priceEdit;
    QLineEdit *m_durationEdit;
    QLineEdit *m_commAmountEdit;
    QTextEdit *m_descriptionEdit;
```

---

### Task 2: 实现无边框窗口与阴影背景

**Files:**
- Modify: `e:\QT\work\PetManager\modules\servicedialog.cpp`

- [ ] **Step 1: 设置窗口属性与创建阴影**
    - 在构造函数中设置 `Qt::FramelessWindowHint` 和 `Qt::WA_TranslucentBackground`。
    - 创建 `m_bgFrame` 并添加 `QGraphicsDropShadowEffect`。

```cpp
// e:\QT\work\PetManager\modules\servicedialog.cpp

ServiceDialog::ServiceDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setupUi();
}
```

---

### Task 3: 重构 setupUi 实现分组布局

**Files:**
- Modify: `e:\QT\work\PetManager\modules\servicedialog.cpp`

- [ ] **Step 1: 实现分组网格布局**
    - 移除旧的 `setupUi` 逻辑。
    - 创建自定义标题栏。
    - 分别创建“基础信息”和“财务与时间”两个分组。
    - 添加多行文本框 `m_descriptionEdit` 用于服务描述。

```cpp
// e:\QT\work\PetManager\modules\servicedialog.cpp

void ServiceDialog::setupUi()
{
    resize(500, 600);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    m_bgFrame = new QFrame(this);
    m_bgFrame->setObjectName("bgFrame");
    QVBoxLayout *bgLayout = new QVBoxLayout(m_bgFrame);
    bgLayout->setContentsMargins(25, 20, 25, 20);
    mainLayout->addWidget(m_bgFrame);

    // 标题
    m_titleLabel = new QLabel("新增服务项目", m_bgFrame);
    m_titleLabel->setObjectName("titleLabel");
    bgLayout->addWidget(m_titleLabel);

    // 基础信息分组
    bgLayout->addWidget(createGroupTitle("基础信息"));
    QGridLayout *baseGrid = new QGridLayout();
    m_nameEdit = new QLineEdit(m_bgFrame);
    m_categoryCombo = new QComboBox(m_bgFrame);
    m_categoryCombo->addItems({"洗护", "美容", "保健", "寄养", "接送", "其他"});
    m_idEdit = new QLineEdit(m_bgFrame);
    m_idEdit->setPlaceholderText("自动生成可留空");

    baseGrid->addWidget(createFormItem("服务名称", m_nameEdit), 0, 0);
    baseGrid->addWidget(createFormItem("所属分类", m_categoryCombo), 0, 1);
    baseGrid->addWidget(createFormItem("服务编码", m_idEdit), 1, 0, 1, 2);
    bgLayout->addLayout(baseGrid);

    // 财务与时间分组
    bgLayout->addWidget(createGroupTitle("财务与时间"));
    QGridLayout *financeGrid = new QGridLayout();
    m_priceEdit = new QLineEdit(m_bgFrame);
    m_durationEdit = new QLineEdit(m_bgFrame);
    m_commAmountEdit = new QLineEdit(m_bgFrame);

    financeGrid->addWidget(createFormItem("服务价格", createInputWithUnit(m_priceEdit, "元")), 0, 0);
    financeGrid->addWidget(createFormItem("标准时长", createInputWithUnit(m_durationEdit, "分钟")), 0, 1);
    financeGrid->addWidget(createFormItem("固定提成", createInputWithUnit(m_commAmountEdit, "元")), 1, 0, 1, 2);
    bgLayout->addLayout(financeGrid);

    // 描述
    bgLayout->addWidget(createGroupTitle("详细说明"));
    m_descriptionEdit = new QTextEdit(m_bgFrame);
    m_descriptionEdit->setPlaceholderText("输入服务项目的详细介绍...");
    bgLayout->addWidget(m_descriptionEdit);

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *cancelBtn = new QPushButton("取消", m_bgFrame);
    QPushButton *saveBtn = new QPushButton("保存", m_bgFrame);
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    bgLayout->addLayout(btnLayout);

    // 信号连接 (略)
}
```

---

### Task 4: 注入 QSS 样式

**Files:**
- Modify: `e:\QT\work\PetManager\modules\servicedialog.cpp`

- [ ] **Step 1: 编写并设置 QSS**
    - 在 `setupUi` 中设置 QSS 字符串，同步会员模块风格。

```cpp
// e:\QT\work\PetManager\modules\servicedialog.cpp

this->setStyleSheet(
    "QFrame#bgFrame { background-color: #ffffff; border-radius: 12px; }"
    "QLabel#titleLabel { font-size: 18px; color: #303133; font-weight: bold; margin-bottom: 10px; }"
    "QLineEdit, QComboBox, QTextEdit { border: 1px solid #e2e8f0; border-radius: 6px; padding: 6px 10px; background-color: #f8fafc; }"
    "QLineEdit:focus, QComboBox:focus, QTextEdit:focus { border: 2px solid #3b82f6; background-color: #ffffff; }"
    "QPushButton { background-color: #3b82f6; color: white; border-radius: 6px; padding: 10px 24px; font-weight: bold; }"
    "QPushButton#cancelBtn { background-color: #f1f5f9; color: #64748b; }"
);
```

---

### Task 5: 更新数据存取逻辑

**Files:**
- Modify: `e:\QT\work\PetManager\modules\servicedialog.cpp`

- [ ] **Step 1: 更新 getServiceInfo**
    - 读取 `m_descriptionEdit` 的内容并存入 `ServiceInfo::description`。

- [ ] **Step 2: 更新 setServiceInfo**
    - 将 `ServiceInfo::description` 的内容设置到 `m_descriptionEdit`。
    - 动态更新标题为“修改服务信息”。

---

### Task 6: 验证与清理

- [ ] **Step 1: 运行并截图检查**
    - 确认布局、阴影、圆角是否符合设计。
- [ ] **Step 2: 测试保存功能**
    - 确认服务描述字段能正常存取。
- [ ] **Step 3: 提交代码**
