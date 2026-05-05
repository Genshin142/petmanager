#include "servicedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>

ServiceDialog::ServiceDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setupUi();
}

void ServiceDialog::setupUi()
{
    resize(520, 620);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    m_bgFrame = new QFrame(this);
    m_bgFrame->setObjectName("bgFrame");
    
    // 添加阴影
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 4);
    m_bgFrame->setGraphicsEffect(shadow);

    QVBoxLayout *bgLayout = new QVBoxLayout(m_bgFrame);
    bgLayout->setContentsMargins(25, 0, 25, 25);
    mainLayout->addWidget(m_bgFrame);

    // 标题栏
    QWidget *header = new QWidget(m_bgFrame);
    header->setFixedHeight(50);
    header->setStyleSheet("background: transparent;"); // 显式设置透明
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    
    m_titleLabel = new QLabel("新增服务项目", header);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1e293b;");
    
    QPushButton *closeBtn = new QPushButton("×", header);
    closeBtn->setFixedSize(30, 30);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #94a3b8; background: transparent; } "
                           "QPushButton:hover { color: #f43f5e; }");
    
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    bgLayout->addWidget(header);

    // 基础信息分组
    bgLayout->addWidget(createGroupTitle("基础信息"));
    QGridLayout *baseGrid = new QGridLayout();
    baseGrid->setSpacing(15);
    
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
    bgLayout->addSpacing(10);
    bgLayout->addWidget(createGroupTitle("财务与时间"));
    QGridLayout *financeGrid = new QGridLayout();
    financeGrid->setSpacing(15);

    m_priceEdit = new QLineEdit(m_bgFrame);
    m_durationEdit = new QLineEdit(m_bgFrame);
    m_commAmountEdit = new QLineEdit(m_bgFrame);

    financeGrid->addWidget(createFormItem("服务价格", createInputWithUnit(m_priceEdit, "元")), 0, 0);
    financeGrid->addWidget(createFormItem("标准时长", createInputWithUnit(m_durationEdit, "分钟")), 0, 1);
    financeGrid->addWidget(createFormItem("固定提成", createInputWithUnit(m_commAmountEdit, "元")), 1, 0, 1, 2);
    bgLayout->addLayout(financeGrid);

    // 详细说明
    bgLayout->addSpacing(10);
    bgLayout->addWidget(createGroupTitle("详细说明"));
    m_descriptionEdit = new QTextEdit(m_bgFrame);
    m_descriptionEdit->setPlaceholderText("在此输入服务项目的详细介绍...");
    m_descriptionEdit->setFixedHeight(80);
    bgLayout->addWidget(m_descriptionEdit);

    bgLayout->addStretch();

    // 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    QPushButton *cancelBtn = new QPushButton("取消", m_bgFrame);
    QPushButton *saveBtn = new QPushButton("保存", m_bgFrame);
    cancelBtn->setObjectName("cancelBtn");
    saveBtn->setObjectName("saveBtn");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedHeight(40); // 增加高度
    saveBtn->setFixedHeight(40);
    cancelBtn->setMinimumWidth(100);
    saveBtn->setMinimumWidth(100);

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    bgLayout->addLayout(btnLayout);

    // 样式设置
    this->setStyleSheet(
        "QFrame#bgFrame { background-color: #ffffff; border-radius: 12px; }"
        "QLabel, QCheckBox, QRadioButton { background: transparent; }" // 基础控件透明
        
        // 输入框与下拉框基础样式
        "QLineEdit, QComboBox, QTextEdit { "
        "   border: 1px solid #e2e8f0; border-radius: 6px; padding: 8px 12px; "
        "   background-color: #f8fafc; color: #1e293b; font-size: 14px; "
        "}"
        "QLineEdit:focus, QComboBox:focus, QTextEdit:focus { "
        "   border: 2px solid #3b82f6; background-color: #ffffff; "
        "}"
        
        // 下拉框特化样式
        "QComboBox::drop-down { border: none; width: 30px; }"
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; }"
        "QComboBox QAbstractItemView { "
        "   border: 1px solid #e2e8f0; background-color: white; "
        "   selection-background-color: #eff6ff; selection-color: #1e40af; "
        "   outline: none; "
        "}"
        
        // 按钮样式
        "QPushButton#saveBtn { "
        "   background-color: #3b82f6; color: white; border: none; "
        "   border-radius: 6px; font-weight: bold; padding: 0px 20px; "
        "}"
        "QPushButton#saveBtn:hover { background-color: #2563eb; }"
        "QPushButton#cancelBtn { "
        "   background-color: #f1f5f9; color: #64748b; border: none; "
        "   border-radius: 6px; font-weight: bold; padding: 0px 20px; "
        "}"
        "QPushButton#cancelBtn:hover { background-color: #e2e8f0; }"
        "QLabel { color: #64748b; font-size: 14px; }"
    );



    connect(saveBtn, &QPushButton::clicked, this, &ServiceDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &ServiceDialog::reject);
    connect(closeBtn, &QPushButton::clicked, this, &ServiceDialog::reject);
}

QWidget* ServiceDialog::createInputWithUnit(QWidget* input, const QString &unit)
{
    QWidget *w = new QWidget(m_bgFrame);
    w->setStyleSheet("background: transparent;");
    QHBoxLayout *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(8);
    l->addWidget(input);
    QLabel *unitLabel = new QLabel(unit, w);
    unitLabel->setStyleSheet("color: #94a3b8; font-size: 13px; width: 30px; background: transparent;");
    l->addWidget(unitLabel);
    return w;
}

QWidget* ServiceDialog::createFormItem(const QString &label, QWidget *widget)
{
    QWidget *w = new QWidget(m_bgFrame);
    w->setStyleSheet("background: transparent;");
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);
    QLabel *lbl = new QLabel(label, w);
    lbl->setStyleSheet("font-weight: 500; color: #64748b; font-size: 13px; background: transparent;");
    l->addWidget(lbl);
    l->addWidget(widget);
    return w;
}


QWidget* ServiceDialog::createGroupTitle(const QString &title)
{
    QLabel *lbl = new QLabel(title, m_bgFrame);
    lbl->setStyleSheet("color: #3b82f6; font-weight: bold; font-size: 14px; "
                      "border-left: 4px solid #3b82f6; padding-left: 8px; margin-bottom: 5px;");
    return lbl;
}

ServiceInfo ServiceDialog::getServiceInfo() const
{
    ServiceInfo info;
    info.id = m_idEdit->text().isEmpty() ? m_currentId : m_idEdit->text();
    info.name = m_nameEdit->text();
    info.category = m_categoryCombo->currentText();
    info.price = m_priceEdit->text().toDouble();
    info.durationMinutes = m_durationEdit->text().toInt();
    info.commissionFixed = m_commAmountEdit->text().toDouble();
    info.description = m_descriptionEdit->toPlainText();
    info.isActive = true;
    return info;
}

void ServiceDialog::setServiceInfo(const ServiceInfo &info)
{
    m_currentId = info.id;
    m_titleLabel->setText("修改服务项目");
    
    m_idEdit->setText(info.id);
    m_nameEdit->setText(info.name);
    m_categoryCombo->setCurrentText(info.category);
    m_priceEdit->setText(QString::number(info.price, 'f', 2));
    m_durationEdit->setText(QString::number(info.durationMinutes));
    m_commAmountEdit->setText(QString::number(info.commissionFixed, 'f', 2));
    m_descriptionEdit->setPlainText(info.description);
    
    m_idEdit->setReadOnly(true);
    m_idEdit->setStyleSheet("QLineEdit { background-color: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0; }");
}

