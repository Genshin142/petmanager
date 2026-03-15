/********************************************************************************
** Form generated from reading UI file 'addmemberdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADDMEMBERDIALOG_H
#define UI_ADDMEMBERDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_AddMemberDialog
{
public:
    QVBoxLayout *mainLayout;
    QFrame *bgFrame;
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *nameEdit;
    QLabel *label_2;
    QLineEdit *phoneEdit;
    QLabel *label_3;
    QComboBox *levelCombo;
    QLabel *label_4;
    QLineEdit *pointsEdit;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AddMemberDialog)
    {
        if (AddMemberDialog->objectName().isEmpty())
            AddMemberDialog->setObjectName("AddMemberDialog");
        AddMemberDialog->resize(400, 280);
        mainLayout = new QVBoxLayout(AddMemberDialog);
        mainLayout->setSpacing(0);
        mainLayout->setObjectName("mainLayout");
        mainLayout->setContentsMargins(20, 20, 20, 20);
        bgFrame = new QFrame(AddMemberDialog);
        bgFrame->setObjectName("bgFrame");
        verticalLayout = new QVBoxLayout(bgFrame);
        verticalLayout->setSpacing(15);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(20, 20, 20, 20);
        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        formLayout->setHorizontalSpacing(15);
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

        phoneEdit = new QLineEdit(bgFrame);
        phoneEdit->setObjectName("phoneEdit");

        formLayout->setWidget(1, QFormLayout::FieldRole, phoneEdit);

        label_3 = new QLabel(bgFrame);
        label_3->setObjectName("label_3");

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        levelCombo = new QComboBox(bgFrame);
        levelCombo->setObjectName("levelCombo");

        formLayout->setWidget(2, QFormLayout::FieldRole, levelCombo);

        label_4 = new QLabel(bgFrame);
        label_4->setObjectName("label_4");

        formLayout->setWidget(3, QFormLayout::LabelRole, label_4);

        pointsEdit = new QLineEdit(bgFrame);
        pointsEdit->setObjectName("pointsEdit");

        formLayout->setWidget(3, QFormLayout::FieldRole, pointsEdit);


        verticalLayout->addLayout(formLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(bgFrame);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Orientation::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Save);

        verticalLayout->addWidget(buttonBox);


        mainLayout->addWidget(bgFrame);


        retranslateUi(AddMemberDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, AddMemberDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, AddMemberDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(AddMemberDialog);
    } // setupUi

    void retranslateUi(QDialog *AddMemberDialog)
    {
        AddMemberDialog->setWindowTitle(QCoreApplication::translate("AddMemberDialog", "\346\226\260\345\242\236\344\274\232\345\221\230", nullptr));
        label->setText(QCoreApplication::translate("AddMemberDialog", "\344\274\232\345\221\230\345\247\223\345\220\215\357\274\232", nullptr));
        nameEdit->setPlaceholderText(QCoreApplication::translate("AddMemberDialog", "\350\257\267\350\276\223\345\205\245\345\247\223\345\220\215", nullptr));
        label_2->setText(QCoreApplication::translate("AddMemberDialog", "\346\211\213\346\234\272\345\217\267\347\240\201\357\274\232", nullptr));
        phoneEdit->setPlaceholderText(QCoreApplication::translate("AddMemberDialog", "\350\257\267\350\276\223\345\205\245\346\211\213\346\234\272\345\217\267", nullptr));
        label_3->setText(QCoreApplication::translate("AddMemberDialog", "\344\274\232\345\221\230\347\255\211\347\272\247\357\274\232", nullptr));
        label_4->setText(QCoreApplication::translate("AddMemberDialog", "\345\210\235\345\247\213\347\247\257\345\210\206\357\274\232", nullptr));
        pointsEdit->setPlaceholderText(QCoreApplication::translate("AddMemberDialog", "\350\257\267\350\276\223\345\205\245\345\210\235\345\247\213\347\247\257\345\210\206, \344\276\213\345\246\202: 0", nullptr));
        pointsEdit->setText(QCoreApplication::translate("AddMemberDialog", "0", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AddMemberDialog: public Ui_AddMemberDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADDMEMBERDIALOG_H
