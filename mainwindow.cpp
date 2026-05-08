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
#include "modules/financemodule.h"
#include "modules/logisticsmodule.h"
#include "modules/inboundmodule.h"
#include "modules/servicemanagementmodule.h"
#include "modules/logisticsmanager.h"
#include "modules/petdatamanager.h"
#include "modules/personalmodule.h"
#include <QPushButton>
#include <QTimer>
#include <QDateTime>

MainWindow::MainWindow(UserRole role, QString userName, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_role(role)
    , m_userName(userName)
{
    ui->setupUi(this);
    this->setWindowState(Qt::WindowMaximized); // 默认启动最大化铺满全屏
    
    // Initialize module pointers to prevent garbage values
    memberMod = nullptr;
    roleMod = nullptr;
    petMod = nullptr;
    productMod = nullptr;
    fosterMod = nullptr;
    apptMod = nullptr;
    checkoutMod = nullptr;
    statsMod = nullptr;
    financeMod = nullptr;
    logisticsMod = nullptr;
    inboundMod = nullptr;
    serviceMod = nullptr;
    personalMod = nullptr;

    initSidebar();

    // 根据权限动态隔离
    ui->userNameLabel->setText(QString("当前用户：%1 (%2)").arg(userName).arg(role == ADMIN ? "管理员" : "营业店员"));
    
    if (role == STAFF) {
        // 店员无权访问敏感模块
        ui->navStats->setVisible(false);
        ui->navRole->setVisible(false); 
        navFinance->setVisible(false);
    }

    // Defer modules initialization to ensure a stable startup sequence
    QTimer::singleShot(100, this, [this, role]() {
        initModules(role);
        updateBadges();
        
        // Setup a recurring timer for badge updates after initial load
        QTimer *badgeTimer = new QTimer(this);
        connect(badgeTimer, &QTimer::timeout, this, &MainWindow::updateBadges);
        badgeTimer->start(5000); // 5s interval is safer than 2s
    });
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
    ui->navProduct->setText("商品档案管理");
    navGroup->addButton(ui->navProduct, 3);
    navGroup->addButton(ui->navFoster, 4);
    navGroup->addButton(ui->navOrder, 5);
    navGroup->addButton(ui->navCheckout, 6);
    ui->navPerformance->setVisible(false); // 隐藏旧按钮
    navGroup->addButton(ui->navStats, 8);

    // 为管理员专属模块添加标识
    ui->navRole->setText("员工与角色 (管理员)");
    ui->navPet->setText("宠物档案中心");
    ui->navFoster->setText("寄养管理中心");
    // ui->navPerformance->setText("业绩核销 (管理员)"); // 已隐藏
    ui->navStats->setText("数据报表 (管理员)");

    // 动态添加调度中心按钮 (Index 9)
    navLogistics = new QPushButton("车辆调度中心");
    navGroup->addButton(navLogistics, 9);
    ui->sidebarLayout->insertWidget(6, navLogistics);

    // 动态添加入库登记按钮 (Index 10)
    navInbound = new QPushButton("商品入库登记");
    navGroup->addButton(navInbound, 10);
    ui->sidebarLayout->insertWidget(4, navInbound); 

    // 动态添加服务管理按钮 (Index 11)
    navService = new QPushButton("服务项目管理");
    navGroup->addButton(navService, 11);
    ui->sidebarLayout->insertWidget(5, navService); 

    // 动态添加财务结算中心按钮 (管理员) (Index 7)
    navFinance = new QPushButton("财务结算中心 (管理员)");
    navGroup->addButton(navFinance, 7); 
    ui->sidebarLayout->insertWidget(11, navFinance);

    // 动态添加个人中心按钮 (Index 12) - 置于最底部
    navPersonal = new QPushButton("个人中心");
    navGroup->addButton(navPersonal, 12);
    // 插入到 spacer 之后或之前？Spacer 在 UI 文件中是最后一个 item。
    // 我们直接 addWidget 到底部
    ui->sidebarLayout->addWidget(navPersonal);

    foreach (QAbstractButton *btn, navGroup->buttons()) {
        btn->setCheckable(true);
    }
    ui->navMember->setChecked(true);

    connect(navGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onNavClicked);

    // QSS 美化
    this->setStyleSheet("QMainWindow { background-color: #f5f7fa; }"
                        "QWidget#sidebar { background-color: #304156; min-width: 260px; max-width: 260px; }"
                        "QPushButton { border: none; color: #bfcbd9; text-align: left; padding: 15px 20px; font-size: 14px; }"
                        "QPushButton:hover { background-color: #263445; color: #409eff; }"
                        "QPushButton:checked { background-color: #1f2d3d; color: #409eff; border-left: 5px solid #409eff; }"
                        "QLabel#userNameLabel { color: white; padding: 20px; font-weight: bold; }");
}

void MainWindow::initModules(UserRole role)
{
    qDebug() << "Initializing modules...";
    memberMod = new MemberModule(role, this);
    qDebug() << "RoleModule...";
    roleMod = new RoleModule(this);
    qDebug() << "PetModule...";
    petMod = new PetModule(role, this);
    qDebug() << "ProductModule...";
    productMod = new ProductModule(role, this);
    qDebug() << "FosterModule...";
    fosterMod = new FosterModule(this);
    qDebug() << "AppointmentModule...";
    apptMod = new AppointmentModule(this);
    qDebug() << "CheckoutModule...";
    checkoutMod = new CheckoutModule(this);
    qDebug() << "FinanceModule...";
    financeMod = new FinanceModule(this);
    qDebug() << "StatsModule...";
    statsMod = new StatsModule(this);
    qDebug() << "LogisticsModule...";
    logisticsMod = new LogisticsModule(this);
    qDebug() << "InboundModule...";
    inboundMod = new InboundModule(role, this);
    qDebug() << "ServiceManagementModule...";
    serviceMod = new ServiceManagementModule(role, this);
    qDebug() << "Modules initialized.";
    
    ui->stack->addWidget(memberMod);  // index 0
    ui->stack->addWidget(roleMod);    // index 1
    ui->stack->addWidget(petMod);     // index 2
    ui->stack->addWidget(productMod);  // index 3
    ui->stack->addWidget(fosterMod);   // index 4
    ui->stack->addWidget(apptMod);     // index 5
    ui->stack->addWidget(checkoutMod); // index 6
    ui->stack->addWidget(financeMod);  // index 7
    ui->stack->addWidget(statsMod);    // index 8
    ui->stack->addWidget(logisticsMod); // index 9
    ui->stack->addWidget(inboundMod);   // index 10
    ui->stack->addWidget(serviceMod);   // index 11
    
    personalMod = new PersonalModule(role, m_userName, this);
    ui->stack->addWidget(personalMod);  // index 12

    // 跨模块同步逻辑
    connect(memberMod, &MemberModule::sig_petAdded, petMod, &PetModule::addPet);
    connect(memberMod, &MemberModule::sig_requestPetJump, this, &MainWindow::onJumpToPetRequested);
    connect(memberMod, &MemberModule::sig_jumpToPetModule, this, &MainWindow::onJumpToPetById);
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

void MainWindow::onJumpToPetById(const QString &petId)
{
    // 1. 切换侧边栏状态
    ui->navPet->setChecked(true);
    // 2. 切换页面索引 (PetModule 在 index 2)
    ui->stack->setCurrentIndex(2);
    // 3. 执行精准定位
    if (petMod) {
        petMod->selectPetById(petId);
    }
}

void MainWindow::updateBadges()
{
    qDebug() << "Updating badges...";
    // 1. 车辆调度中心提醒 (基于临期任务)
    if (LogisticsManager::instance()) {
        QList<LogisticsTask> tasks = LogisticsManager::instance()->getAllTasks();
        QDateTime now = QDateTime::currentDateTime();
        int urgentLogisticsCount = 0;
        
        for (const auto &task : tasks) {
            if (task.status != "待处理") continue;
            if (task.appointmentTime.contains(now.date().toString("yyyy-MM-dd"))) {
                QString startTimeStr = task.appointmentTime.mid(11, 5);
                QTime startTime = QTime::fromString(startTimeStr, "HH:mm");
                if (startTime.isValid() && now.time() >= startTime.addSecs(-1800)) { // 30分钟内
                    urgentLogisticsCount++;
                }
            }
        }
        
        if (navLogistics) {
            if (urgentLogisticsCount > 0) {
                navLogistics->setText(QString("车辆调度中心  (%1)").arg(urgentLogisticsCount));
                navLogistics->setStyleSheet("QPushButton { color: #ff4d4f; font-weight: bold; } "
                                            "QPushButton:checked { color: #409eff; border-left: 5px solid #409eff; }");
            } else {
                navLogistics->setText("车辆调度中心");
                navLogistics->setStyleSheet(""); // 恢复默认侧边栏样式
            }
        }
    }
    qDebug() << "Logistics badge updated.";

    if (ui->navProduct) {
        int lowStockCount = 0;
        // 3. 商品库存报警 (确保模块已初始化)
        if (productMod) {
            lowStockCount = productMod->getLowStockCount();
        }

        if (lowStockCount > 0) {
            ui->navProduct->setText(QString("商品档案管理  (%1)").arg(lowStockCount));
            ui->navProduct->setStyleSheet("QPushButton { color: #faad14; font-weight: bold; } "
                                        "QPushButton:checked { color: #409eff; border-left: 5px solid #409eff; }");
        } else {
            ui->navProduct->setText("商品档案管理");
            ui->navProduct->setStyleSheet("");
        }
    }
    qDebug() << "Product badge updated.";
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    return QMainWindow::eventFilter(obj, event);
}
