#include "selectpetdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QFrame>

SelectPetDialog::SelectPetDialog(const QStringList &allPets, const QStringList &selectedPets, QWidget *parent)
    : QDialog(parent)
{
    setupUI(allPets, selectedPets);
}

void SelectPetDialog::setupUI(const QStringList &allPets, const QStringList &selectedPets)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    QFrame *bgFrame = new QFrame(this);
    bgFrame->setObjectName("bgFrame");
    bgFrame->setStyleSheet(
        "QFrame#bgFrame {"
        "   background-color: #ffffff;"
        "   border-radius: 12px;"
        "}"
    );
    layout->addWidget(bgFrame);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 5);
    bgFrame->setGraphicsEffect(shadow);

    QVBoxLayout *containerLayout = new QVBoxLayout(bgFrame);
    containerLayout->setContentsMargins(25, 20, 25, 20);
    containerLayout->setSpacing(15);

    QLabel *titleLabel = new QLabel("关联宠物档案");
    titleLabel->setStyleSheet("font-size: 18px; color: #303133;");
    containerLayout->addWidget(titleLabel);

    petListWidget = new QListWidget();
    petListWidget->setStyleSheet(
        "QListWidget {"
        "   border: 1px solid #dcdfe6;"
        "   border-radius: 4px;"
        "   padding: 5px;"
        "   outline: none;"
        "}"
        "QListWidget::item {"
        "   height: 36px;"
        "   padding-left: 10px;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #f5f7fa;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #409eff;"
        "   color: white;"
        "}"
    );

    for (const QString &pet : allPets) {
        QListWidgetItem *item = new QListWidgetItem(pet, petListWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        if (selectedPets.contains(pet)) {
            item->setCheckState(Qt::Checked);
        } else {
            item->setCheckState(Qt::Unchecked);
        }
    }
    containerLayout->addWidget(petListWidget);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedSize(100, 32);
    cancelBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #f4f4f5;"
        "   color: #606266;"
        "   border: none;"
        "   border-radius: 4px;"
        "   font: 14px 'Microsoft YaHei', 'Segoe UI', sans-serif;"
        "   text-align: center;"
        "   padding: 0px;"
        "}"
        "QPushButton:hover { background-color: #e9e9eb; }"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    QPushButton *okBtn = new QPushButton("关联确认");
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setFixedSize(110, 34);
    okBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #409eff;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 4px;"
        "   font: 14px 'Microsoft YaHei', 'Segoe UI', sans-serif;"
        "   text-align: center;"
        "   padding: 0px;"
        "}"
        "QPushButton:hover { background-color: #66b1ff; }"
    );
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);

    containerLayout->addLayout(btnLayout);

    setFixedWidth(400);
    setFixedHeight(500);
}

QStringList SelectPetDialog::getSelectedPets() const
{
    QStringList selected;
    for (int i = 0; i < petListWidget->count(); ++i) {
        QListWidgetItem *item = petListWidget->item(i);
        if (item->checkState() == Qt::Checked) {
            selected << item->text();
        }
    }
    return selected;
}
