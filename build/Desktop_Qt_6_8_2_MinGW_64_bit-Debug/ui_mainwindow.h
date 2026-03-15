/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *mainLayout;
    QWidget *sidebar;
    QVBoxLayout *sidebarLayout;
    QLabel *userNameLabel;
    QPushButton *navMember;
    QPushButton *navRole;
    QPushButton *navPet;
    QPushButton *navProduct;
    QPushButton *navFoster;
    QPushButton *navOrder;
    QPushButton *navStats;
    QSpacerItem *sidebarSpacer;
    QStackedWidget *stack;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1200, 800);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        mainLayout = new QHBoxLayout(centralwidget);
        mainLayout->setSpacing(0);
        mainLayout->setObjectName("mainLayout");
        mainLayout->setContentsMargins(0, 0, 0, 0);
        sidebar = new QWidget(centralwidget);
        sidebar->setObjectName("sidebar");
        sidebarLayout = new QVBoxLayout(sidebar);
        sidebarLayout->setSpacing(0);
        sidebarLayout->setObjectName("sidebarLayout");
        userNameLabel = new QLabel(sidebar);
        userNameLabel->setObjectName("userNameLabel");

        sidebarLayout->addWidget(userNameLabel);

        navMember = new QPushButton(sidebar);
        navMember->setObjectName("navMember");

        sidebarLayout->addWidget(navMember);

        navRole = new QPushButton(sidebar);
        navRole->setObjectName("navRole");

        sidebarLayout->addWidget(navRole);

        navPet = new QPushButton(sidebar);
        navPet->setObjectName("navPet");

        sidebarLayout->addWidget(navPet);

        navProduct = new QPushButton(sidebar);
        navProduct->setObjectName("navProduct");

        sidebarLayout->addWidget(navProduct);

        navFoster = new QPushButton(sidebar);
        navFoster->setObjectName("navFoster");

        sidebarLayout->addWidget(navFoster);

        navOrder = new QPushButton(sidebar);
        navOrder->setObjectName("navOrder");

        sidebarLayout->addWidget(navOrder);

        navStats = new QPushButton(sidebar);
        navStats->setObjectName("navStats");

        sidebarLayout->addWidget(navStats);

        sidebarSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        sidebarLayout->addItem(sidebarSpacer);


        mainLayout->addWidget(sidebar);

        stack = new QStackedWidget(centralwidget);
        stack->setObjectName("stack");

        mainLayout->addWidget(stack);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        userNameLabel->setText(QCoreApplication::translate("MainWindow", "\347\256\241\347\220\206\345\221\230\357\274\232Admin", nullptr));
        navMember->setText(QCoreApplication::translate("MainWindow", "\344\274\232\345\221\230\347\256\241\347\220\206", nullptr));
        navRole->setText(QCoreApplication::translate("MainWindow", "\345\221\230\345\267\245\344\270\216\350\247\222\350\211\262", nullptr));
        navPet->setText(QCoreApplication::translate("MainWindow", "\345\256\240\347\211\251\346\241\243\346\241\210\344\270\255\345\277\203", nullptr));
        navProduct->setText(QCoreApplication::translate("MainWindow", "\345\225\206\345\223\201\345\272\223\345\255\230\347\256\241\347\220\206", nullptr));
        navFoster->setText(QCoreApplication::translate("MainWindow", "\345\257\204\345\205\273\345\217\257\350\247\206\345\214\226\346\210\277\346\200\201", nullptr));
        navOrder->setText(QCoreApplication::translate("MainWindow", "\346\224\266\351\223\266\344\270\232\345\212\241\350\277\220\350\220\245", nullptr));
        navStats->setText(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\346\212\245\350\241\250\347\273\237\350\256\241", nullptr));
        (void)MainWindow;
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
