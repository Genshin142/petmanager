/********************************************************************************
** Form generated from reading UI file 'addpetdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADDPETDIALOG_H
#define UI_ADDPETDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_AddPetDialog
{
public:
    QVBoxLayout *mainVLayout;
    QFrame *bgFrame;
    QVBoxLayout *containerLayout;
    QLabel *titleLabel;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *nameEdit;
    QLabel *label_2;
    QHBoxLayout *breedLayout;
    QComboBox *speciesCombo;
    QComboBox *breedCombo;
    QLabel *label_3;
    QLineEdit *historyEdit;
    QLabel *label_4;
    QHBoxLayout *dateLayout;
    QComboBox *yearCombo;
    QLabel *labelYear;
    QComboBox *monthCombo;
    QLabel *labelMonth;
    QComboBox *dayCombo;
    QLabel *labelDay;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *btnLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *saveBtn;
    QPushButton *cancelBtn;

    void setupUi(QDialog *AddPetDialog)
    {
        if (AddPetDialog->objectName().isEmpty())
            AddPetDialog->setObjectName("AddPetDialog");
        AddPetDialog->resize(450, 550);
        mainVLayout = new QVBoxLayout(AddPetDialog);
        mainVLayout->setSpacing(0);
        mainVLayout->setObjectName("mainVLayout");
        mainVLayout->setContentsMargins(20, 20, 20, 20);
        bgFrame = new QFrame(AddPetDialog);
        bgFrame->setObjectName("bgFrame");
        bgFrame->setStyleSheet(QString::fromUtf8("QFrame#bgFrame { background-color: #ffffff; border-radius: 12px; }"));
        containerLayout = new QVBoxLayout(bgFrame);
        containerLayout->setSpacing(15);
        containerLayout->setObjectName("containerLayout");
        containerLayout->setContentsMargins(30, 25, 30, 25);
        titleLabel = new QLabel(bgFrame);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setStyleSheet(QString::fromUtf8("font-size: 18px; font-weight: bold; color: #303133;"));

        containerLayout->addWidget(titleLabel);

        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        formLayout->setVerticalSpacing(15);
        label = new QLabel(bgFrame);
        label->setObjectName("label");

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        nameEdit = new QLineEdit(bgFrame);
        nameEdit->setObjectName("nameEdit");

        formLayout->setWidget(0, QFormLayout::FieldRole, nameEdit);

        label_2 = new QLabel(bgFrame);
        label_2->setObjectName("label_2");

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        breedLayout = new QHBoxLayout();
        breedLayout->setObjectName("breedLayout");
        speciesCombo = new QComboBox(bgFrame);
        speciesCombo->setObjectName("speciesCombo");

        breedLayout->addWidget(speciesCombo);

        breedCombo = new QComboBox(bgFrame);
        breedCombo->setObjectName("breedCombo");

        breedLayout->addWidget(breedCombo);


        formLayout->setLayout(1, QFormLayout::FieldRole, breedLayout);

        label_3 = new QLabel(bgFrame);
        label_3->setObjectName("label_3");

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        historyEdit = new QLineEdit(bgFrame);
        historyEdit->setObjectName("historyEdit");

        formLayout->setWidget(2, QFormLayout::FieldRole, historyEdit);

        label_4 = new QLabel(bgFrame);
        label_4->setObjectName("label_4");

        formLayout->setWidget(3, QFormLayout::LabelRole, label_4);

        dateLayout = new QHBoxLayout();
        dateLayout->setObjectName("dateLayout");
        yearCombo = new QComboBox(bgFrame);
        yearCombo->setObjectName("yearCombo");

        dateLayout->addWidget(yearCombo);

        labelYear = new QLabel(bgFrame);
        labelYear->setObjectName("labelYear");

        dateLayout->addWidget(labelYear);

        monthCombo = new QComboBox(bgFrame);
        monthCombo->setObjectName("monthCombo");

        dateLayout->addWidget(monthCombo);

        labelMonth = new QLabel(bgFrame);
        labelMonth->setObjectName("labelMonth");

        dateLayout->addWidget(labelMonth);

        dayCombo = new QComboBox(bgFrame);
        dayCombo->setObjectName("dayCombo");

        dateLayout->addWidget(dayCombo);

        labelDay = new QLabel(bgFrame);
        labelDay->setObjectName("labelDay");

        dateLayout->addWidget(labelDay);


        formLayout->setLayout(3, QFormLayout::FieldRole, dateLayout);


        containerLayout->addLayout(formLayout);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        containerLayout->addItem(verticalSpacer);

        btnLayout = new QHBoxLayout();
        btnLayout->setObjectName("btnLayout");
        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        btnLayout->addItem(horizontalSpacer);

        saveBtn = new QPushButton(bgFrame);
        saveBtn->setObjectName("saveBtn");

        btnLayout->addWidget(saveBtn);

        cancelBtn = new QPushButton(bgFrame);
        cancelBtn->setObjectName("cancelBtn");

        btnLayout->addWidget(cancelBtn);


        containerLayout->addLayout(btnLayout);


        mainVLayout->addWidget(bgFrame);


        retranslateUi(AddPetDialog);

        QMetaObject::connectSlotsByName(AddPetDialog);
    } // setupUi

    void retranslateUi(QDialog *AddPetDialog)
    {
        AddPetDialog->setWindowTitle(QCoreApplication::translate("AddPetDialog", "\346\267\273\345\212\240\345\256\240\347\211\251\346\241\243\346\241\210", nullptr));
        titleLabel->setText(QCoreApplication::translate("AddPetDialog", "\346\267\273\345\212\240\345\256\240\347\211\251\344\277\241\346\201\257", nullptr));
        label->setText(QCoreApplication::translate("AddPetDialog", "\345\256\240\347\211\251\345\247\223\345\220\215:", nullptr));
        nameEdit->setPlaceholderText(QCoreApplication::translate("AddPetDialog", "\350\257\267\350\276\223\345\205\245\345\256\240\347\211\251\345\247\223\345\220\215", nullptr));
        label_2->setText(QCoreApplication::translate("AddPetDialog", "\345\256\240\347\211\251\347\247\215\347\261\273:", nullptr));
        label_3->setText(QCoreApplication::translate("AddPetDialog", "\345\256\240\347\211\251\347\227\205\345\217\262:", nullptr));
        historyEdit->setPlaceholderText(QCoreApplication::translate("AddPetDialog", "\346\227\240\347\227\205\345\217\262\350\257\267\345\241\253\346\227\240", nullptr));
        label_4->setText(QCoreApplication::translate("AddPetDialog", "\346\234\200\350\277\221\347\226\253\350\213\227:", nullptr));
        labelYear->setText(QCoreApplication::translate("AddPetDialog", "\345\271\264", nullptr));
        labelMonth->setText(QCoreApplication::translate("AddPetDialog", "\346\234\210", nullptr));
        labelDay->setText(QCoreApplication::translate("AddPetDialog", "\346\227\245", nullptr));
        saveBtn->setText(QCoreApplication::translate("AddPetDialog", "\344\277\235\345\255\230\350\256\260\345\275\225", nullptr));
        cancelBtn->setText(QCoreApplication::translate("AddPetDialog", "\345\217\226\346\266\210", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AddPetDialog: public Ui_AddPetDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADDPETDIALOG_H
