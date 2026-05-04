#include "servicedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>

ServiceDialog::ServiceDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
}

void ServiceDialog::setupUi()
{
    setWindowTitle("服务项目");
    resize(400, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGridLayout *gridLayout = new QGridLayout();

    m_nameEdit = new QLineEdit(this);
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItems({"洗护", "美容", "保健", "寄养", "接送", "其他"});
    
    m_durationEdit = new QLineEdit(this);
    m_priceEdit = new QLineEdit(this);
    m_idEdit = new QLineEdit(this);
    m_idEdit->setPlaceholderText("自动生成可留空");
    m_commAmountEdit = new QLineEdit(this);
    
    gridLayout->addWidget(new QLabel("服务名称:"), 0, 0);
    gridLayout->addWidget(m_nameEdit, 0, 1);
    
    gridLayout->addWidget(new QLabel("所属分类:"), 1, 0);
    gridLayout->addWidget(m_categoryCombo, 1, 1);

    gridLayout->addWidget(new QLabel("服务编码:"), 2, 0);
    gridLayout->addWidget(m_idEdit, 2, 1);

    m_priceTitleLabel = new QLabel("服务价格:");
    gridLayout->addWidget(m_priceTitleLabel, 3, 0);
    gridLayout->addWidget(createInputWithUnit(m_priceEdit, "元"), 3, 1);

    m_durationTitleLabel = new QLabel("标准时长:");
    m_durationUnitLabel = new QLabel("分钟");
    m_durationGroup = createInputWithUnit(m_durationEdit, "分钟");
    gridLayout->addWidget(m_durationTitleLabel, 4, 0);
    gridLayout->addWidget(m_durationGroup, 4, 1);

    m_commTitleLabel = new QLabel("固定提成:");
    gridLayout->addWidget(m_commTitleLabel, 5, 0);
    gridLayout->addWidget(createInputWithUnit(m_commAmountEdit, "元"), 5, 1);

    mainLayout->addLayout(gridLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *saveBtn = new QPushButton("保存", this);
    QPushButton *cancelBtn = new QPushButton("取消", this);
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);

    mainLayout->addLayout(btnLayout);

    connect(saveBtn, &QPushButton::clicked, this, &ServiceDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &ServiceDialog::reject);
}

QWidget* ServiceDialog::createInputWithUnit(QWidget* input, const QString &unit)
{
    QWidget *w = new QWidget(this);
    QHBoxLayout *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(input);
    l->addWidget(new QLabel(unit));
    return w;
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
    info.isActive = true;
    return info;
}

void ServiceDialog::setServiceInfo(const ServiceInfo &info)
{
    m_currentId = info.id;
    m_idEdit->setText(info.id);
    m_nameEdit->setText(info.name);
    m_categoryCombo->setCurrentText(info.category);
    m_priceEdit->setText(QString::number(info.price, 'f', 2));
    m_durationEdit->setText(QString::number(info.durationMinutes));
    m_commAmountEdit->setText(QString::number(info.commissionFixed, 'f', 2));
}
