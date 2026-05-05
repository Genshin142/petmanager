#include "rechargedialog.h"
#include <QGraphicsDropShadowEffect>
#include <QRegularExpressionValidator>

RechargeDialog::RechargeDialog(const MemberInfo &info, QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    setupUI(info);
}

void RechargeDialog::setupUI(const MemberInfo &info)
{
    // 主容器 (带阴影和圆角)
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);

    QFrame *mainCard = new QFrame();
    mainCard->setObjectName("MainCard");
    mainCard->setStyleSheet(
        "QFrame#MainCard { background: white; border-radius: 16px; }"
    );
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 4);
    mainCard->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(mainCard);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(25);

    // 1. 标题与会员信息
    QHBoxLayout *header = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("会员余额充值");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1e293b;");
    header->addWidget(titleLabel);
    header->addStretch();
    
    QPushButton *closeBtn = new QPushButton("×");
    closeBtn->setFixedSize(30, 30);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #94a3b8; background: transparent; } "
                           "QPushButton:hover { color: #f43f5e; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    header->addWidget(closeBtn);
    layout->addLayout(header);

    // 会员状态摘要
    QFrame *memberBox = new QFrame();
    memberBox->setStyleSheet(
        "QFrame { background: #f8fafc; border-radius: 10px; border: 1px solid #f1f5f9; } "
        "QLabel { border: none; background: transparent; }"
    );
    QHBoxLayout *memL = new QHBoxLayout(memberBox);
    memL->setContentsMargins(20, 15, 20, 15);

    QVBoxLayout *memText = new QVBoxLayout();
    QLabel *memName = new QLabel(QString("会员: %1").arg(info.name));
    memName->setStyleSheet("font-weight: bold; color: #334155; font-size: 14px;");
    QLabel *memBalance = new QLabel(QString("当前余额: ¥%1").arg(QString::number(info.balance, 'f', 2)));
    memBalance->setStyleSheet("color: #64748b; font-size: 13px;");
    memText->addWidget(memName);
    memText->addWidget(memBalance);
    memL->addLayout(memText);
    memL->addStretch();
    
    QLabel *levelTag = new QLabel(info.level);
    levelTag->setStyleSheet("background: #e0f2fe; color: #0ea5e9; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    memL->addWidget(levelTag);
    layout->addWidget(memberBox);

    // 2. 充值金额选择
    QLabel *amtTitle = new QLabel("选择充值金额");
    amtTitle->setStyleSheet("font-weight: bold; color: #475569; font-size: 14px;");
    layout->addWidget(amtTitle);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(12);
    m_presetGroup = new QButtonGroup(this);
    
    QList<int> amounts = {100, 300, 500, 1000, 2000, 5000};
    for (int i = 0; i < amounts.size(); ++i) {
        QPushButton *btn = new QPushButton(QString("¥%1").arg(amounts[i]));
        btn->setCheckable(true);
        btn->setFixedHeight(50);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1.5px solid #e2e8f0; border-radius: 8px; color: #64748b; font-size: 15px; font-weight: bold; } "
            "QPushButton:hover { border-color: #3b82f6; background: #eff6ff; color: #3b82f6; } "
            "QPushButton:checked { border-color: #3b82f6; background: #3b82f6; color: white; }"
        );
        m_presetGroup->addButton(btn, amounts[i]);
        grid->addWidget(btn, i / 3, i % 3);
    }
    layout->addLayout(grid);

    // 自定义金额
    QHBoxLayout *customL = new QHBoxLayout();
    QLabel *customLabel = new QLabel("其他金额:");
    customLabel->setStyleSheet("color: #64748b; font-size: 13px;");
    m_customInput = new QLineEdit();
    m_customInput->setPlaceholderText("请输入金额");
    m_customInput->setFixedHeight(40);
    m_customInput->setValidator(new QRegularExpressionValidator(QRegularExpression("^\\d+(\\.\\d{0,2})?$"), this));
    m_customInput->setStyleSheet(
        "QLineEdit { border: 1.5px solid #e2e8f0; border-radius: 8px; padding: 0 15px; font-size: 14px; background: white; } "
        "QLineEdit:focus { border-color: #3b82f6; outline: none; }"
    );
    customL->addWidget(customLabel);
    customL->addWidget(m_customInput);
    layout->addLayout(customL);

    // 3. 支付方式
    QLabel *payTitle = new QLabel("支付方式");
    payTitle->setStyleSheet("font-weight: bold; color: #475569; font-size: 14px;");
    layout->addWidget(payTitle);

    QHBoxLayout *payL = new QHBoxLayout();
    m_payMethodGroup = new QButtonGroup(this);
    QStringList payMethods = {"WeChat", "Alipay", "Cash"};
    QStringList payTexts = {"微信支付", "支付宝", "现金支付"};
    for (int i = 0; i < payMethods.size(); ++i) {
        QPushButton *btn = new QPushButton(payTexts[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(40);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 6px; color: #64748b; font-size: 13px; padding: 0 15px; } "
            "QPushButton:checked { background: #eff6ff; border-color: #3b82f6; color: #3b82f6; font-weight: bold; }"
        );
        if (i == 0) btn->setChecked(true);
        m_payMethodGroup->addButton(btn, i);
        payL->addWidget(btn);
    }
    layout->addLayout(payL);

    // 4. 底部按钮
    QHBoxLayout *footer = new QHBoxLayout();
    QPushButton *confirmBtn = new QPushButton("确认充值");
    confirmBtn->setFixedHeight(50);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    confirmBtn->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #2563eb); "
        "color: white; border-radius: 10px; font-size: 16px; font-weight: bold; border: none; } "
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563eb, stop:1 #1d4ed8); } "
        "QPushButton:pressed { background: #1e40af; }"
    );
    connect(confirmBtn, &QPushButton::clicked, this, &RechargeDialog::onConfirm);
    footer->addWidget(confirmBtn);
    layout->addLayout(footer);

    rootLayout->addWidget(mainCard);
    
    // 居中显示
    if (parentWidget()) {
        this->adjustSize();
        QRect parentGeom = parentWidget()->geometry();
        int x = parentGeom.center().x() - this->width() / 2;
        int y = parentGeom.center().y() - this->height() / 2;
        this->move(x, y);
    }

    // 监听逻辑：预设金额点击时清空自定义输入，反之亦然
    connect(m_presetGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int){
        m_customInput->clear();
    });
    connect(m_customInput, &QLineEdit::textChanged, this, [this](const QString &text){
        if (!text.isEmpty()) {
            if (m_presetGroup->checkedButton()) {
                m_presetGroup->setExclusive(false);
                m_presetGroup->checkedButton()->setChecked(false);
                m_presetGroup->setExclusive(true);
            }
        }
    });
}

void RechargeDialog::onConfirm()
{
    if (!m_customInput->text().isEmpty()) {
        m_amount = m_customInput->text().toDouble();
    } else if (m_presetGroup->checkedButton()) {
        m_amount = m_presetGroup->checkedId();
    } else {
        // 提示选择金额
        return;
    }

    if (m_amount <= 0) return;

    int payId = m_payMethodGroup->checkedId();
    if (payId == 0) m_payMethod = "WeChat";
    else if (payId == 1) m_payMethod = "Alipay";
    else m_payMethod = "Cash";

    accept();
}

void RechargeDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void RechargeDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}
