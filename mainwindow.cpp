#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "modules/membermodule.h"
#include "modules/rolemodule.h"
#include "modules/petmodule.h"
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
#include "modules/schedulemodule.h"
#include <QPushButton>
#include <QTimer>
#include <QDateTime>
#include <QPaintEvent>

MainWindow::MainWindow(UserRole role, QString userName, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_role(role)
    , m_userName(userName)
{
    m_startupTimer.start();
    qDebug() << "[PERF] ===== MainWindow Diagnostic Timer Started =====";

    ui->setupUi(this);
    this->setWindowState(Qt::WindowMaximized); // 默认启动最大化铺满全屏
    
    // Initialize module pointers to prevent garbage values
    memberMod = nullptr;
    roleMod = nullptr;
    petMod = nullptr;
    fosterMod = nullptr;
    apptMod = nullptr;
    checkoutMod = nullptr;
    statsMod = nullptr;
    financeMod = nullptr;
    logisticsMod = nullptr;
    inboundMod = nullptr;
    serviceMod = nullptr;
    personalMod = nullptr;
    scheduleMod = nullptr;

    initSidebar();

    // 根据权限动态隔离
    ui->userNameLabel->setText(QString("当前用户：%1 (%2)").arg(userName).arg(role == ADMIN ? "管理员" : "营业店员"));
    
    if (role == STAFF) {
        // 店员无权访问敏感模块
        ui->navStats->setVisible(false);
        ui->navRole->setVisible(false); 
        navFinance->setVisible(false);
        navSchedule->setVisible(false); // 店员不可见
    }

    // Defer modules initialization to ensure a stable startup sequence
    qDebug() << "[PERF]" << m_startupTimer.elapsed() << "ms: Scheduling deferred modules initialization (100ms)...";
    QTimer::singleShot(100, this, [this, role]() {
        qDebug() << "[PERF]" << m_startupTimer.elapsed() << "ms: Defer timer fired. Starting initModules()...";
        
        QElapsedTimer moduleTimer;
        moduleTimer.start();
        initModules(role);
        qDebug() << "[PERF]" << m_startupTimer.elapsed() << "ms: initModules() finished. Took" << moduleTimer.elapsed() << "ms.";
        
        updateBadges();
        
        // Setup a recurring timer for badge updates after initial load
        QTimer *badgeTimer = new QTimer(this);
        connect(badgeTimer, &QTimer::timeout, this, &MainWindow::updateBadges);
        badgeTimer->start(5000); // 5s interval is safer than 2s

        // Record responsive/interactive time by queueing a 0ms singleShot
        QTimer::singleShot(0, this, [this]() {
            qDebug() << "[PERF]" << m_startupTimer.elapsed() << "ms: Qt event loop returned to idle. Application is now FULLY INTERACTIVE / OPERABLE!";
            qDebug() << "[PERF] ========================================================";
        });
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
    ui->navProduct->setVisible(false); // 隐藏商品档案入口
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

    // 动态添加商品库存管理按钮 (Index 10) - 统一管理入口
    navInbound = new QPushButton("商品库存管理");
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

    // 动态添加排班管理按钮 (管理员) (Index 13)
    navSchedule = new QPushButton("排班管理 (管理员)");
    navGroup->addButton(navSchedule, 13);
    ui->sidebarLayout->insertWidget(13, navSchedule);

    // 动态添加个人中心按钮 (Index 12) - 置于最底部
    navPersonal = new QPushButton("个人中心");
    navGroup->addButton(navPersonal, 12);
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
    qDebug() << "Initializing modules (Lazy Loading & Background Preloading enabled)...";
    
    // 1. 同步实例化并展示首页面 (MemberModule - index 0)
    memberMod = new MemberModule(role, this);
    ui->stack->addWidget(memberMod); // index 0
    
    // 2. 为所有其他 index 插入空白占位 QWidget，以确保 stacked 索引与原逻辑保持一致 (1 到 13)
    for (int i = 1; i < 14; ++i) {
        ui->stack->addWidget(new QWidget());
    }
    
    // 3. 跨模块跳转与同步机制绑定 (仅限主窗口直接关联的信号槽)
    connect(memberMod, &MemberModule::sig_requestPetJump, this, &MainWindow::onJumpToPetRequested);
    connect(memberMod, &MemberModule::sig_jumpToPetModule, this, &MainWindow::onJumpToPetById);
    
    // 4. 将默认首页面设为 index 0 展现给用户
    ui->stack->setCurrentIndex(0);
    
    // 5. 开启后台异步闲时渐进式预加载链条 (延迟 3.5 秒启动以确保首屏极致渲染，此后每隔 1.5 秒加载一个模块)
    QTimer::singleShot(3500, this, [this, role]() {
        preloadNextModule(1, role);
    });
}

QWidget* MainWindow::getModulePointer(int index) const
{
    switch (index) {
        case 0:  return memberMod;
        case 1:  return roleMod;
        case 2:  return petMod;
        case 4:  return fosterMod;
        case 5:  return apptMod;
        case 6:  return checkoutMod;
        case 7:  return financeMod;
        case 8:  return statsMod;
        case 9:  return logisticsMod;
        case 10: return inboundMod;
        case 11: return serviceMod;
        case 12: return personalMod;
        case 13: return scheduleMod;
        default: break;
    }
    return nullptr;
}

bool MainWindow::isModuleLoaded(int index) const
{
    return getModulePointer(index) != nullptr;
}

void MainWindow::replacePlaceholder(int index, QWidget *newWidget)
{
    if (index < 0 || index >= ui->stack->count()) return;
    
    QWidget *oldWidget = ui->stack->widget(index);
    ui->stack->removeWidget(oldWidget);
    ui->stack->insertWidget(index, newWidget);
    
    if (oldWidget) {
        oldWidget->deleteLater();
    }
}

QWidget* MainWindow::loadModule(int index, UserRole role)
{
    QWidget *mod = getModulePointer(index);
    if (mod) return mod; // 已加载则直接返回
    
    QElapsedTimer loadTimer;
    loadTimer.start();
    qDebug() << "[PERF] On-demand lazy-loading module at index" << index << "started...";
    
    switch (index) {
        case 0:
            memberMod = new MemberModule(role, this);
            replacePlaceholder(0, memberMod);
            connect(memberMod, &MemberModule::sig_requestPetJump, this, &MainWindow::onJumpToPetRequested);
            connect(memberMod, &MemberModule::sig_jumpToPetModule, this, &MainWindow::onJumpToPetById);
            if (petMod) {
                connect(memberMod, &MemberModule::sig_petAdded, petMod, &PetModule::addPet);
            }
            mod = memberMod;
            break;
        case 1:
            roleMod = new RoleModule(this);
            replacePlaceholder(1, roleMod);
            mod = roleMod;
            break;
        case 2:
            petMod = new PetModule(role, this);
            replacePlaceholder(2, petMod);
            if (memberMod) {
                connect(memberMod, &MemberModule::sig_petAdded, petMod, &PetModule::addPet);
            }
            mod = petMod;
            break;
        case 4:
            fosterMod = new FosterModule(this);
            replacePlaceholder(4, fosterMod);
            mod = fosterMod;
            break;
        case 5:
            apptMod = new AppointmentModule(this);
            replacePlaceholder(5, apptMod);
            mod = apptMod;
            break;
        case 6:
            checkoutMod = new CheckoutModule(this);
            replacePlaceholder(6, checkoutMod);
            mod = checkoutMod;
            break;
        case 7:
            financeMod = new FinanceModule(this);
            replacePlaceholder(7, financeMod);
            mod = financeMod;
            break;
        case 8:
            statsMod = new StatsModule(this);
            replacePlaceholder(8, statsMod);
            mod = statsMod;
            break;
        case 9:
            logisticsMod = new LogisticsModule(this);
            replacePlaceholder(9, logisticsMod);
            mod = logisticsMod;
            break;
        case 10:
            inboundMod = new InboundModule(role, this);
            replacePlaceholder(10, inboundMod);
            mod = inboundMod;
            break;
        case 11:
            serviceMod = new ServiceManagementModule(role, this);
            replacePlaceholder(11, serviceMod);
            mod = serviceMod;
            break;
        case 12:
            personalMod = new PersonalModule(role, m_userName, this);
            replacePlaceholder(12, personalMod);
            mod = personalMod;
            break;
        case 13:
            scheduleMod = new ScheduleModule(this);
            replacePlaceholder(13, scheduleMod);
            mod = scheduleMod;
            break;
        default:
            break;
    }
    
    qDebug() << "[PERF] On-demand lazy-loading module at index" << index << "completed in" << loadTimer.elapsed() << "ms.";
    return mod;
}

void MainWindow::preloadNextModule(int index, UserRole role)
{
    if (index >= 14) {
        qDebug() << "[PERF] Background incremental preloading completely finished!";
        return;
    }
    if (index == 3) { // 商品档案为原置空占位
        preloadNextModule(4, role);
        return;
    }
    if (isModuleLoaded(index)) { // 已经由用户手动点击或触发加载，跳过
        preloadNextModule(index + 1, role);
        return;
    }
    
    // 延迟 1500ms 在下一次事件循环 idle 间隙创建下一个模块，确保给主线程留出充足交互渲染余地
    QTimer::singleShot(1500, this, [this, index, role]() {
        loadModule(index, role);
        preloadNextModule(index + 1, role);
    });
}

void MainWindow::onNavClicked(int id)
{
    loadModule(id, m_role);
    ui->stack->setCurrentIndex(id);
}

void MainWindow::onJumpToPetRequested(const QString &memberName, const QString &petName)
{
    // 1. 确保 PetModule (index 2) 已经实例化完毕
    loadModule(2, m_role);
    
    // 2. 切换左侧导航按钮状态
    ui->navPet->setChecked(true);
    
    // 3. 切换右侧堆栈页面
    ui->stack->setCurrentIndex(2);
    
    // 4. 执行 PetModule 内部定位
    if (petMod) {
        petMod->filterByMemberAndHighlightPet(memberName, petName);
    }
}

void MainWindow::onJumpToPetById(const QString &petId)
{
    // 1. 确保 PetModule (index 2) 已经实例化完毕
    loadModule(2, m_role);
    
    // 2. 切换侧边栏状态与页面
    ui->navPet->setChecked(true);
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

    qDebug() << "Product badge skip.";
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    if (m_firstPaint) {
        m_firstPaint = false;
        qDebug() << "[PERF]" << m_startupTimer.elapsed() << "ms: First paintEvent executed. Main window is now VISIBLE on screen!";
    }
}
