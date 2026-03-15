#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QFrame>

CustomMessageDialog::CustomMessageDialog(const QString &title, const QString &content, bool isConfirm, QWidget *parent)
    : QDialog(parent)
{
    setupUI(title, content, isConfirm);
}

CustomMessageDialog::~CustomMessageDialog()
{
}

void CustomMessageDialog::showWarning(QWidget *parent, const QString &title, const QString &content)
{
    CustomMessageDialog dialog(title, content, false, parent); // Pass false for isConfirm
    dialog.exec();
}

bool CustomMessageDialog::confirm(QWidget *parent, const QString &title, const QString &content)
{
    CustomMessageDialog dialog(title, content, true, parent); // Pass true for isConfirm
    return dialog.exec() == QDialog::Accepted;
}

void CustomMessageDialog::setupUI(const QString &title, const QString &content, bool isConfirm)
{
    // 隐藏边框，背景透明
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 主布局，留出阴影边距
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    // 背景容器
    QFrame *bgFrame = new QFrame(this);
    bgFrame->setObjectName("bgFrame");
    bgFrame->setStyleSheet(
        "QFrame#bgFrame {"
        "   background-color: #ffffff;"
        "   border-radius: 12px;"
        "}"
    );
    layout->addWidget(bgFrame);

    // 阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 5);
    bgFrame->setGraphicsEffect(shadow);

    // 容器内部布局
    QVBoxLayout *containerLayout = new QVBoxLayout(bgFrame);
    containerLayout->setContentsMargins(30, 25, 30, 25);
    containerLayout->setSpacing(20);

    // 标题
    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #303133;");
    containerLayout->addWidget(titleLabel);

    // 内容区（横向：图标 + 文字）
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);

    // 这里由于没有现成的图标，我们用一个彩色的 QLabel 模拟
    QLabel *iconLabel = new QLabel("!");
    iconLabel->setFixedSize(36, 36);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet(
        "background-color: #fef0f0; "
        "color: #f56c6c; "
        "font-size: 20px; "
        "font-weight: bold; "
        "border-radius: 18px;"
    );
    contentLayout->addWidget(iconLabel);

    QLabel *msgLabel = new QLabel(content);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet("font-size: 14px; color: #606266; line-height: 1.5;");
    contentLayout->addWidget(msgLabel, 1);
    
    containerLayout->addLayout(contentLayout);

    // 按钮布局
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 10, 0, 0);
    btnLayout->addStretch();

    if (isConfirm) {
        QPushButton *cancelBtn = new QPushButton("取消");
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setFixedSize(80, 32);
        cancelBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #f4f4f5;"
            "   color: #606266;"
            "   border: none;"
            "   border-radius: 4px;"
            "   font: 14px 'Microsoft YaHei';"
            "   text-align: center;"
            "   padding: 0px;"
            "}"
            "QPushButton:hover { background-color: #e9e9eb; }"
        );
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        btnLayout->addWidget(cancelBtn);
        btnLayout->addSpacing(10);
    }
    
    QPushButton *okBtn = new QPushButton("确认");
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setFixedSize(90, 34);
    okBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #409eff;"
        "   color: #ffffff;"
        "   border: none;"
        "   border-radius: 4px;"
        "   font: 14px 'Microsoft YaHei', 'Segoe UI', sans-serif;"
        "   text-align: center;"  /* 👈 添加这一行强制文字水平居中 */
        "   padding: 0px;"        /* 建议：如果不需要严格为0，可以删掉这行或改为合适的内边距 */
        "   margin: 0px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #66b1ff;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3a8ee6;"
        "}"
        );
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);
    
    containerLayout->addLayout(btnLayout);

    // 固定宽度
    setFixedWidth(450);
}
