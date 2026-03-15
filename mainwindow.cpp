#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "modules/membermodule.h"
#include "modules/rolemodule.h"
#include "modules/petmodule.h"
#include "modules/productmodule.h"
#include "modules/fostermodule.h"
#include "modules/ordermodule.h"
#include "modules/statsmodule.h"
#include <QPushButton>

MainWindow::MainWindow(UserRole role, QString userName, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initSidebar();
    initModules();

    // 根据权限动态隔离
    ui->userNameLabel->setText(QString("当前用户：%1 (%2)").arg(userName).arg(role == ADMIN ? "管理员" : "营业店员"));
    
    if (role == STAFF) {
        // 店员无权访问敏感模块
        ui->navStats->setVisible(false);
        ui->navRole->setVisible(false); 
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initSidebar()
{
    navGroup = new QButtonGroup(this);
    navGroup->addButton(ui->navMember, 0);
    navGroup->addButton(ui->navRole, 1);
    navGroup->addButton(ui->navPet, 2);
    navGroup->addButton(ui->navProduct, 3);
    navGroup->addButton(ui->navFoster, 4);
    navGroup->addButton(ui->navOrder, 5);
    navGroup->addButton(ui->navStats, 6);

    foreach (QAbstractButton *btn, navGroup->buttons()) {
        btn->setCheckable(true);
    }
    ui->navMember->setChecked(true);

    connect(navGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onNavClicked);

    // QSS 美化
    this->setStyleSheet("QMainWindow { background-color: #f5f7fa; }"
                        "QWidget#sidebar { background-color: #304156; min-width: 220px; }"
                        "QPushButton { border: none; color: #bfcbd9; text-align: left; padding: 15px 30px; font-size: 15px; }"
                        "QPushButton:hover { background-color: #263445; color: #409eff; }"
                        "QPushButton:checked { background-color: #1f2d3d; color: #409eff; border-left: 5px solid #409eff; }"
                        "QLabel#userNameLabel { color: white; padding: 20px; font-weight: bold; }");
}

void MainWindow::initModules()
{
    memberMod = new MemberModule(this);
    roleMod = new RoleModule(this);
    petMod = new PetModule(this);
    productMod = new ProductModule(this);
    fosterMod = new FosterModule(this);
    orderMod = new OrderModule(this);
    statsMod = new StatsModule(this);

    ui->stack->addWidget(memberMod);  // index 0
    ui->stack->addWidget(roleMod);    // index 1
    ui->stack->addWidget(petMod);     // index 2
    ui->stack->addWidget(productMod);  // index 3
    ui->stack->addWidget(fosterMod);   // index 4
    ui->stack->addWidget(orderMod);    // index 5
    ui->stack->addWidget(statsMod);    // index 6
}

void MainWindow::onNavClicked(int id)
{
    ui->stack->setCurrentIndex(id);
}
