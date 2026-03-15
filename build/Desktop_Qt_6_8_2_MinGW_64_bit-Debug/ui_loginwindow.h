/********************************************************************************
** Form generated from reading UI file 'loginwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGINWINDOW_H
#define UI_LOGINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LoginWindow
{
public:
    QVBoxLayout *verticalLayout;
    QFrame *mainFrame;
    QVBoxLayout *frameLayout;
    QHBoxLayout *topLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *closeBtn;
    QLabel *titleLabel;
    QLabel *subtitleLabel;
    QSpacerItem *vSpacer1;
    QHBoxLayout *logoLayout;
    QSpacerItem *logoSpacerLeft;
    QLabel *logoLabel;
    QSpacerItem *logoSpacerRight;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QSpacerItem *vSpacer2;
    QPushButton *loginBtn;

    void setupUi(QWidget *LoginWindow)
    {
        if (LoginWindow->objectName().isEmpty())
            LoginWindow->setObjectName("LoginWindow");
        LoginWindow->resize(500, 550);
        LoginWindow->setStyleSheet(QString::fromUtf8("\n"
"QWidget#LoginWindow {\n"
"    background-color: #f0f2f5;\n"
"}\n"
"QFrame#mainFrame {\n"
"    background-color: white;\n"
"    border-radius: 12px;\n"
"}\n"
"QLabel#titleLabel {\n"
"    color: #303133;\n"
"    font-size: 28px;\n"
"    font-weight: bold;\n"
"}\n"
"QLabel#subtitleLabel {\n"
"    color: #909399;\n"
"}\n"
"QLineEdit {\n"
"    padding: 12px 15px;\n"
"    border: 1px solid #dcdfe6;\n"
"    border-radius: 6px;\n"
"    font-size: 14px;\n"
"    background-color: #f5f7fa;\n"
"}\n"
"QLineEdit:focus {\n"
"    border: 1px solid #409eff;\n"
"    background-color: white;\n"
"}\n"
"QPushButton#loginBtn {\n"
"    background-color: #409eff;\n"
"    color: white;\n"
"    border: none;\n"
"    border-radius: 6px;\n"
"    padding: 12px;\n"
"    font-size: 16px;\n"
"    font-weight: bold;\n"
"}\n"
"QPushButton#loginBtn:hover {\n"
"    background-color: #66b1ff;\n"
"}\n"
"QPushButton#loginBtn:pressed {\n"
"    background-color: #3a8ee6;\n"
"}\n"
"QPushButton#closeBtn {\n"
"    border: none;\n"
"    background: "
                        "transparent;\n"
"    font-size: 24px;\n"
"    color: #909399;\n"
"}\n"
"QPushButton#closeBtn:hover {\n"
"    color: #f56c6c;\n"
"}\n"
"    "));
        verticalLayout = new QVBoxLayout(LoginWindow);
        verticalLayout->setObjectName("verticalLayout");
        mainFrame = new QFrame(LoginWindow);
        mainFrame->setObjectName("mainFrame");
        frameLayout = new QVBoxLayout(mainFrame);
        frameLayout->setSpacing(15);
        frameLayout->setObjectName("frameLayout");
        frameLayout->setContentsMargins(40, 20, 40, 40);
        topLayout = new QHBoxLayout();
        topLayout->setObjectName("topLayout");
        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        topLayout->addItem(horizontalSpacer);

        closeBtn = new QPushButton(mainFrame);
        closeBtn->setObjectName("closeBtn");

        topLayout->addWidget(closeBtn);


        frameLayout->addLayout(topLayout);

        titleLabel = new QLabel(mainFrame);
        titleLabel->setObjectName("titleLabel");
        QFont font;
        font.setBold(true);
        titleLabel->setFont(font);
        titleLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        frameLayout->addWidget(titleLabel);

        subtitleLabel = new QLabel(mainFrame);
        subtitleLabel->setObjectName("subtitleLabel");
        subtitleLabel->setStyleSheet(QString::fromUtf8("color: #909399;"));
        subtitleLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        frameLayout->addWidget(subtitleLabel);

        vSpacer1 = new QSpacerItem(20, 5, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        frameLayout->addItem(vSpacer1);

        logoLayout = new QHBoxLayout();
        logoLayout->setObjectName("logoLayout");
        logoSpacerLeft = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        logoLayout->addItem(logoSpacerLeft);

        logoLabel = new QLabel(mainFrame);
        logoLabel->setObjectName("logoLabel");
        logoLabel->setMinimumSize(QSize(120, 120));
        logoLabel->setMaximumSize(QSize(120, 120));
        logoLabel->setScaledContents(true);

        logoLayout->addWidget(logoLabel);

        logoSpacerRight = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        logoLayout->addItem(logoSpacerRight);


        frameLayout->addLayout(logoLayout);

        usernameEdit = new QLineEdit(mainFrame);
        usernameEdit->setObjectName("usernameEdit");

        frameLayout->addWidget(usernameEdit);

        passwordEdit = new QLineEdit(mainFrame);
        passwordEdit->setObjectName("passwordEdit");
        passwordEdit->setEchoMode(QLineEdit::EchoMode::Password);

        frameLayout->addWidget(passwordEdit);

        vSpacer2 = new QSpacerItem(20, 30, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        frameLayout->addItem(vSpacer2);

        loginBtn = new QPushButton(mainFrame);
        loginBtn->setObjectName("loginBtn");
        loginBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        frameLayout->addWidget(loginBtn);


        verticalLayout->addWidget(mainFrame);


        retranslateUi(LoginWindow);

        QMetaObject::connectSlotsByName(LoginWindow);
    } // setupUi

    void retranslateUi(QWidget *LoginWindow)
    {
        closeBtn->setText(QCoreApplication::translate("LoginWindow", "\303\227", nullptr));
        titleLabel->setText(QCoreApplication::translate("LoginWindow", "\345\256\240\347\211\251\345\272\227\344\277\241\346\201\257\347\256\241\347\220\206\350\275\257\344\273\266", nullptr));
        subtitleLabel->setText(QString());
        logoLabel->setText(QString());
        usernameEdit->setText(QCoreApplication::translate("LoginWindow", "admin", nullptr));
        usernameEdit->setPlaceholderText(QCoreApplication::translate("LoginWindow", "\350\257\267\350\276\223\345\205\245\347\256\241\347\220\206\345\221\230\350\264\246\345\217\267", nullptr));
        passwordEdit->setText(QCoreApplication::translate("LoginWindow", "123456", nullptr));
        passwordEdit->setPlaceholderText(QCoreApplication::translate("LoginWindow", "\350\257\267\350\276\223\345\205\245\347\231\273\345\275\225\345\257\206\347\240\201", nullptr));
        loginBtn->setText(QCoreApplication::translate("LoginWindow", "\347\253\213\345\215\263\347\231\273\345\275\225", nullptr));
        (void)LoginWindow;
    } // retranslateUi

};

namespace Ui {
    class LoginWindow: public Ui_LoginWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGINWINDOW_H
