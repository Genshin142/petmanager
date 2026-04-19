#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QFrame>

CustomMessageDialog::CustomMessageDialog(const QString &title, const QString &content, DialogType type, QWidget *parent)
    : QDialog(parent)
{
    setupUI(title, content, type);
}

CustomMessageDialog::~CustomMessageDialog()
{
}

void CustomMessageDialog::showWarning(QWidget *parent, const QString &title, const QString &content)
{
    CustomMessageDialog dialog(title, content, Warning, parent);
    dialog.exec();
}

void CustomMessageDialog::showSuccess(QWidget *parent, const QString &title, const QString &content)
{
    CustomMessageDialog dialog(title, content, Success, parent);
    dialog.exec();
}

bool CustomMessageDialog::confirm(QWidget *parent, const QString &title, const QString &content)
{
    CustomMessageDialog dialog(title, content, Confirm, parent);
    return dialog.exec() == QDialog::Accepted;
}

void CustomMessageDialog::setupUI(const QString &title, const QString &content, DialogType type)
{
    // 隐藏边框，背景透明，确保它作为子弹窗存在
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 自动居中父窗口
    if (parentWidget()) {
        this->setParent(parentWidget());
        QPoint centerPos = parentWidget()->geometry().center();
        this->move(centerPos.x() - 210, centerPos.y() - 100);
    }

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
    titleLabel->setStyleSheet("font-size: 18px; color: #303133; font-weight: bold;");
    containerLayout->addWidget(titleLabel);

    // 内容区（横向：图标 + 文字）
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);

    // 图标和颜色逻辑
    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(40, 40);
    iconLabel->setAlignment(Qt::AlignCenter);
    
    QString iconStyle;
    if (type == Success) {
        iconLabel->setText("✓");
        iconStyle = "background-color: #f0f9eb; color: #67c23a; border-radius: 20px; font-size: 24px;";
    } else if (type == Warning) {
        iconLabel->setText("!");
        iconStyle = "background-color: #fef0f0; color: #f56c6c; border-radius: 20px; font-size: 24px; font-weight: bold;";
    } else { // Confirm
        iconLabel->setText("?");
        iconStyle = "background-color: #edf2fc; color: #409eff; border-radius: 20px; font-size: 24px;";
    }
    iconLabel->setStyleSheet(iconStyle);
    contentLayout->addWidget(iconLabel);

    QLabel *msgLabel = new QLabel(content);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet("font-size: 14px; color: #606266; line-height: 1.6;");
    contentLayout->addWidget(msgLabel, 1);
    
    containerLayout->addLayout(contentLayout);

    // 按钮布局
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 10, 0, 0);
    btnLayout->addStretch();

    if (type == Confirm) {
        QPushButton *cancelBtn = new QPushButton("取消");
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setFixedSize(100, 40);
        cancelBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: white;"
            "   color: #606266;"
            "   border: 1px solid #dcdfe6;"
            "   border-radius: 20px;"
            "   font: 14px 'Microsoft YaHei';"
            "   text-align: center; padding: 0px;"
            "}"
            "QPushButton:hover { background-color: #f5f7fa; color: #409eff; border-color: #c6e2ff; }"
        );
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        btnLayout->addWidget(cancelBtn);
        btnLayout->addSpacing(15);
    }
    
    QPushButton *okBtn = new QPushButton(type == Success ? "知道了" : "确认");
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setFixedSize(110, 40);
    
    QString baseStyle = "border: none; border-radius: 20px; font: bold 14px 'Microsoft YaHei'; text-align: center; padding: 0px;";
    if (type == Success) {
        okBtn->setStyleSheet("QPushButton { background-color: #67c23a; color: #ffffff; " + baseStyle + " }");
    } else if (type == Warning) {
        okBtn->setStyleSheet("QPushButton { background-color: #f56c6c; color: #ffffff; " + baseStyle + " }");
    } else {
        okBtn->setStyleSheet("QPushButton { background-color: #409eff; color: #ffffff; " + baseStyle + " }");
    }
    
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);
    
    containerLayout->addLayout(btnLayout);

    // 固定宽度
    setFixedWidth(420);
}
