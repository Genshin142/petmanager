#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "modules/membermodule.h"
#include "modules/rolemodule.h"
#include "modules/petmodule.h"
#include "modules/productmodule.h"
#include "modules/fostermodule.h"
#include "modules/appointmentmodule.h"
#include "modules/checkoutmodule.h"
#include "modules/statsmodule.h"
#include "modules/performancemodule.h"
#include "modules/salarymodule.h"
#include <QPushButton>

MainWindow::MainWindow(UserRole role, QString userName, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_role(role)
{
    ui->setupUi(this);
    this->setWindowState(Qt::WindowMaximized); // 默认启动最大化铺满全屏
    
    initSidebar();
    initModules(role);

    // 根据权限动态隔离
    ui->userNameLabel->setText(QString("当前用户：%1 (%2)").arg(userName).arg(role == ADMIN ? "管理员" : "营业店员"));
    
    if (role == STAFF) {
        // 店员无权访问敏感模块
        ui->navStats->setVisible(false);
        ui->navRole->setVisible(false); 
        ui->navPerformance->setVisible(false);
        navSalary->setVisible(false);
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
    navGroup->addButton(ui->navCheckout, 6);
    navGroup->addButton(ui->navPerformance, 7);
    navGroup->addButton(ui->navStats, 8);

    // 为管理员专属模块添加标识
    ui->navRole->setText("员工与角色 (管理员)");
    ui->navPerformance->setText("业绩核销 (管理员)");
    ui->navStats->setText("数据报表 (管理员)");

    // 动态添加薪资管理按钮
    navSalary = new QPushButton("薪资管理中心 (管理员)");
    navGroup->addButton(navSalary, 9);
    ui->sidebarLayout->insertWidget(9, navSalary); // 插在业绩核算后

    foreach (QAbstractButton *btn, navGroup->buttons()) {
        btn->setCheckable(true);
    }
    ui->navMember->setChecked(true);

    connect(navGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onNavClicked);

    // QSS 美化
    this->setStyleSheet("QMainWindow { background-color: #f5f7fa; }"
                        "QWidget#sidebar { background-color: #304156; min-width: 260px; }"
                        "QPushButton { border: none; color: #bfcbd9; text-align: left; padding: 15px 20px; font-size: 14px; }"
                        "QPushButton:hover { background-color: #263445; color: #409eff; }"
                        "QPushButton:checked { background-color: #1f2d3d; color: #409eff; border-left: 5px solid #409eff; }"
                        "QLabel#userNameLabel { color: white; padding: 20px; font-weight: bold; }");
}

void MainWindow::initModules(UserRole role)
{
    memberMod = new MemberModule(role, this);
    roleMod = new RoleModule(this);
    petMod = new PetModule(this);
    productMod = new ProductModule(role, this);
    fosterMod = new FosterModule(this);
    apptMod = new AppointmentModule(this);
    checkoutMod = new CheckoutModule(this);
    perfMod = new PerformanceModule(this);
    statsMod = new StatsModule(this);
    salaryMod = new SalaryModule(this);

    ui->stack->addWidget(memberMod);  // index 0
    ui->stack->addWidget(roleMod);    // index 1
    ui->stack->addWidget(petMod);     // index 2
    ui->stack->addWidget(productMod);  // index 3
    ui->stack->addWidget(fosterMod);   // index 4
    ui->stack->addWidget(apptMod);     // index 5
    ui->stack->addWidget(checkoutMod); // index 6
    ui->stack->addWidget(perfMod);     // index 7
    ui->stack->addWidget(statsMod);    // index 8
    ui->stack->addWidget(salaryMod);   // index 9

    // 跨模块同步逻辑
    connect(memberMod, &MemberModule::sig_petAdded, petMod, &PetModule::addPet);
    connect(memberMod, &MemberModule::sig_requestPetJump, this, &MainWindow::onJumpToPetRequested);
}

void MainWindow::onNavClicked(int id)
{
    ui->stack->setCurrentIndex(id);
}

void MainWindow::onJumpToPetRequested(const QString &memberName, const QString &petName)
{
    // 1. 切换左侧导航按钮状态 (PetModule ID 为 2)
    ui->navPet->setChecked(true);
    
    // 2. 切换右侧堆栈页面
    ui->stack->setCurrentIndex(2);
    
    // 3. 执行 PetModule 内部针对会员的筛选以及宠物的高亮定位
    if (petMod) {
        petMod->filterByMemberAndHighlightPet(memberName, petName);
    }
}
