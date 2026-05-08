# 密码修改对话框优化 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将现有基础的密码修改弹窗替换为美观、现代、功能完善的“专业简约卡片”样式对话框，具备实时密码强度校验功能。

**Architecture:** 创建继承自 `QDialog` 的 `PasswordChangeDialog`，使用 `QVBoxLayout` 进行垂直布局。利用 `QLineEdit` 和 `QProgressBar` 实现输入和强度反馈。样式通过全局或局部的 `setStyleSheet` 注入。与父模块 `PersonalModule` 通过信号/槽解耦通信。

**Tech Stack:** C++17, Qt6 (QtWidgets), QSS

---

### Task 1: 创建 PasswordChangeDialog 基础框架

**Files:**
- Create: `e:\QT\work\PetManager\modules\passwordchangedialog.h`
- Create: `e:\QT\work\PetManager\modules\passwordchangedialog.cpp`

- [ ] **Step 1: 编写头文件定义**

```cpp
#ifndef PASSWORDCHANGEDIALOG_H
#define PASSWORDCHANGEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

class PasswordChangeDialog : public QDialog {
    Q_OBJECT
public:
    explicit PasswordChangeDialog(QWidget *parent = nullptr);

signals:
    void passwordUpdated(const QString &newPassword);

private slots:
    void validateForm();
    void updateStrength(const QString &text);
    void toggleVisibility(QLineEdit *lineEdit);

private:
    void setupUI();
    void applyStyles();

    QLineEdit *m_currentPwdEdit;
    QLineEdit *m_newPwdEdit;
    QLineEdit *m_confirmPwdEdit;
    QProgressBar *m_strengthBar;
    QLabel *m_strengthLabel;
    QLabel *m_errorLabel;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // PASSWORDCHANGEDIALOG_H
```

- [ ] **Step 2: 编写基础实现（空方法）**

```cpp
#include "passwordchangedialog.h"

PasswordChangeDialog::PasswordChangeDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(400, 520);
    setupUI();
    applyStyles();
}

void PasswordChangeDialog::setupUI() {}
void PasswordChangeDialog::applyStyles() {}
void PasswordChangeDialog::validateForm() {}
void PasswordChangeDialog::updateStrength(const QString &text) {}
void PasswordChangeDialog::toggleVisibility(QLineEdit *lineEdit) {}
```

- [ ] **Step 3: 编译检查**

Run: 构建项目以确保没有编译错误。
Expected: 编译通过，无未定义符号。

### Task 2: 搭建 UI 布局与组件

**Files:**
- Modify: `e:\QT\work\PetManager\modules\passwordchangedialog.cpp`

- [ ] **Step 1: 实现 setupUI 方法**

```cpp
#include <QHBoxLayout>
#include <QIcon>

// 在 setupUI() 内补充：
void PasswordChangeDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // 1. Header
    QLabel *titleLabel = new QLabel("修改登录密码");
    titleLabel->setObjectName("TitleLabel");
    QLabel *subTitleLabel = new QLabel("请确保密码至少 8 位并包含字母和数字。");
    subTitleLabel->setObjectName("SubTitleLabel");
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subTitleLabel);

    // 2. Input Fields Helper
    auto createInputField = [this](const QString &label, QLineEdit* &edit) {
        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(8);
        QLabel *l = new QLabel(label);
        l->setObjectName("FieldLabel");
        edit = new QLineEdit();
        edit->setEchoMode(QLineEdit::Password);
        edit->setFixedHeight(40);
        edit->setPlaceholderText("请输入" + label);
        vl->addWidget(l);
        vl->addWidget(edit);
        return vl;
    };

    mainLayout->addLayout(createInputField("当前密码", m_currentPwdEdit));
    mainLayout->addLayout(createInputField("设置新密码", m_newPwdEdit));
    
    // 3. 强度条
    QHBoxLayout *strengthLayout = new QHBoxLayout();
    m_strengthLabel = new QLabel("强度: 弱");
    m_strengthLabel->setObjectName("StrengthLabel");
    m_strengthBar = new QProgressBar();
    m_strengthBar->setFixedHeight(4);
    m_strengthBar->setTextVisible(false);
    m_strengthBar->setValue(0);
    strengthLayout->addWidget(m_strengthLabel);
    strengthLayout->addWidget(m_strengthBar);
    mainLayout->addLayout(strengthLayout);

    mainLayout->addLayout(createInputField("确认新密码", m_confirmPwdEdit));

    // 4. 错误提示
    m_errorLabel = new QLabel("");
    m_errorLabel->setObjectName("ErrorLabel");
    m_errorLabel->hide();
    mainLayout->addWidget(m_errorLabel);

    mainLayout->addStretch();

    // 5. Buttons
    m_saveBtn = new QPushButton("保存修改");
    m_saveBtn->setFixedHeight(44);
    m_saveBtn->setObjectName("PrimaryBtn");
    
    m_cancelBtn = new QPushButton("取消");
    m_cancelBtn->setFixedHeight(44);
    m_cancelBtn->setObjectName("SecondaryBtn");

    mainLayout->addWidget(m_saveBtn);
    mainLayout->addWidget(m_cancelBtn);

    // Connections
    connect(m_newPwdEdit, &QLineEdit::textChanged, this, &PasswordChangeDialog::updateStrength);
    connect(m_saveBtn, &QPushButton::clicked, this, &PasswordChangeDialog::validateForm);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}
```

### Task 3: 注入视觉样式 (QSS)

**Files:**
- Modify: `e:\QT\work\PetManager\modules\passwordchangedialog.cpp`

- [ ] **Step 1: 实现 applyStyles 方法**

```cpp
void PasswordChangeDialog::applyStyles() {
    this->setStyleSheet(R"(
        QDialog {
            background-color: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
        }
        QLabel#TitleLabel {
            font-size: 18px;
            font-weight: bold;
            color: #1e293b;
        }
        QLabel#SubTitleLabel {
            font-size: 12px;
            color: #64748b;
        }
        QLabel#FieldLabel {
            font-size: 13px;
            font-weight: 600;
            color: #334155;
        }
        QLineEdit {
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            padding: 0 10px;
            color: #1e293b;
            background-color: #f8fafc;
        }
        QLineEdit:focus {
            border: 1px solid #3b82f6;
            background-color: #ffffff;
        }
        QProgressBar {
            background-color: #f1f5f9;
            border: none;
            border-radius: 2px;
        }
        QProgressBar::chunk {
            background-color: #3b82f6;
            border-radius: 2px;
        }
        QLabel#StrengthLabel {
            font-size: 11px;
            color: #94a3b8;
        }
        QLabel#ErrorLabel {
            font-size: 12px;
            color: #ef4444;
        }
        QPushButton#PrimaryBtn {
            background-color: #0f172a;
            color: #ffffff;
            font-weight: bold;
            font-size: 14px;
            border-radius: 6px;
            border: none;
        }
        QPushButton#PrimaryBtn:hover {
            background-color: #1e293b;
        }
        QPushButton#SecondaryBtn {
            background-color: transparent;
            color: #64748b;
            font-weight: bold;
            font-size: 14px;
            border-radius: 6px;
            border: 1px solid transparent;
        }
        QPushButton#SecondaryBtn:hover {
            background-color: #f1f5f9;
            color: #334155;
        }
    )");
}
```

### Task 4: 实现业务逻辑 (校验与反馈)

**Files:**
- Modify: `e:\QT\work\PetManager\modules\passwordchangedialog.cpp`

- [ ] **Step 1: 实现密码强度与表单校验逻辑**

```cpp
#include <QRegularExpression>

void PasswordChangeDialog::updateStrength(const QString &text) {
    int score = 0;
    if (text.length() >= 8) score += 33;
    if (text.contains(QRegularExpression("[A-Za-z]"))) score += 33;
    if (text.contains(QRegularExpression("[0-9]"))) score += 34;
    
    m_strengthBar->setValue(score);
    
    QString chunkColor;
    if (score < 40) {
        m_strengthLabel->setText("强度: 弱");
        chunkColor = "#ef4444"; // Red
    } else if (score < 80) {
        m_strengthLabel->setText("强度: 中");
        chunkColor = "#f59e0b"; // Orange
    } else {
        m_strengthLabel->setText("强度: 强");
        chunkColor = "#10b981"; // Green
    }
    
    m_strengthBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; border-radius: 2px; }").arg(chunkColor));
}

void PasswordChangeDialog::validateForm() {
    m_errorLabel->hide();
    
    if (m_currentPwdEdit->text().isEmpty() || m_newPwdEdit->text().isEmpty() || m_confirmPwdEdit->text().isEmpty()) {
        m_errorLabel->setText("请填写完整所有密码字段。");
        m_errorLabel->show();
        return;
    }
    
    if (m_newPwdEdit->text().length() < 8 || !m_newPwdEdit->text().contains(QRegularExpression("[A-Za-z]")) || !m_newPwdEdit->text().contains(QRegularExpression("[0-9]"))) {
        m_errorLabel->setText("新密码必须至少8位，且包含字母和数字。");
        m_errorLabel->show();
        return;
    }
    
    if (m_newPwdEdit->text() != m_confirmPwdEdit->text()) {
        m_errorLabel->setText("两次输入的新密码不一致！");
        m_errorLabel->show();
        return;
    }
    
    // 成功
    m_errorLabel->setStyleSheet("color: #10b981;");
    m_errorLabel->setText("密码修改成功！");
    m_errorLabel->show();
    
    m_saveBtn->setEnabled(false);
    emit passwordUpdated(m_newPwdEdit->text());
    
    // 延迟关闭
    QTimer::singleShot(1500, this, &QDialog::accept);
}

// 暂省略可视化切换(眼睛图标)，可后续作为附加功能添加。
```

### Task 5: 接入 PersonalModule

**Files:**
- Modify: `e:\QT\work\PetManager\modules\personalmodule.cpp`

- [ ] **Step 1: 引入头文件**

```cpp
#include "passwordchangedialog.h"
// 请确保添加到文件顶部
```

- [ ] **Step 2: 替换 onActionClicked 中的原有弹窗**

```cpp
// 在 PersonalModule::onActionClicked 中替换 "安全设置" 和 "修改密码" 的分支逻辑
    if (actionName == "安全设置" || actionName == "修改密码") {
        PasswordChangeDialog dialog(this);
        dialog.exec();
        return; // 直接返回，不再执行后续原本的 QMessageBox 逻辑
    }
```

- [ ] **Step 3: 整体编译与运行测试**

Run: 构建项目并启动应用，进入个人中心，点击“修改密码”。
Expected: 弹出全新设计的白色简约对话框，支持输入校验和强度显示。
