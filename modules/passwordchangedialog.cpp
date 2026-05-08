#include "passwordchangedialog.h"
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QTimer>

PasswordChangeDialog::PasswordChangeDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(400, 520);
    setupUI();
    applyStyles();
}

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

void PasswordChangeDialog::updateStrength(const QString &text) {
    int score = 0;
    if (text.length() >= 8) score += 33;
    if (text.contains(QRegularExpression("[A-Za-z]"))) score += 33;
    if (text.contains(QRegularExpression("[0-9]"))) score += 34;
    
    m_strengthBar->setValue(score);
    
    QString chunkColor;
    if (score < 40) {
        m_strengthLabel->setText("强度: 弱");
        chunkColor = "#ef4444";
    } else if (score < 80) {
        m_strengthLabel->setText("强度: 中");
        chunkColor = "#f59e0b";
    } else {
        m_strengthLabel->setText("强度: 强");
        chunkColor = "#10b981";
    }
    
    m_strengthBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; border-radius: 2px; }").arg(chunkColor));
}

void PasswordChangeDialog::validateForm() {
    m_errorLabel->hide();
    
    if (m_currentPwdEdit->text().isEmpty() || m_newPwdEdit->text().isEmpty() || m_confirmPwdEdit->text().isEmpty()) {
        m_errorLabel->setStyleSheet("color: #ef4444; font-size: 12px;");
        m_errorLabel->setText("请填写完整所有密码字段。");
        m_errorLabel->show();
        return;
    }
    
    if (m_newPwdEdit->text().length() < 8 || !m_newPwdEdit->text().contains(QRegularExpression("[A-Za-z]")) || !m_newPwdEdit->text().contains(QRegularExpression("[0-9]"))) {
        m_errorLabel->setStyleSheet("color: #ef4444; font-size: 12px;");
        m_errorLabel->setText("新密码必须至少8位，且包含字母和数字。");
        m_errorLabel->show();
        return;
    }
    
    if (m_newPwdEdit->text() != m_confirmPwdEdit->text()) {
        m_errorLabel->setStyleSheet("color: #ef4444; font-size: 12px;");
        m_errorLabel->setText("两次输入的新密码不一致！");
        m_errorLabel->show();
        return;
    }
    
    // 成功
    m_errorLabel->setStyleSheet("color: #10b981; font-size: 12px; font-weight: bold;");
    m_errorLabel->setText("密码修改成功！正在返回...");
    m_errorLabel->show();
    
    m_saveBtn->setEnabled(false);
    emit passwordUpdated(m_newPwdEdit->text());
    
    // 延迟关闭
    QTimer::singleShot(1500, this, &QDialog::accept);
}
