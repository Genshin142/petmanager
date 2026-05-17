#include "appointmentmodule.h"
#include "appointmentitemdelegate.h"
#include "appointmentdetaildrawer.h"
#include "petdatamanager.h"
#include "addappointmentdialog.h"
#include "fostermodule.h"
#include "logisticsmanager.h"
#include "custommessagedialog.h"
#include "../utils/imageutils.h"
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGraphicsDropShadowEffect>
#include <QStandardItemModel>
#include <QScrollArea>
#include <QStackedWidget>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QDialog>
#include <QMessageBox>

AppointmentModule::AppointmentModule(QWidget *parent)
    : QWidget(parent)
    , m_model(new QStandardItemModel(this))
    , m_delegate(new AppointmentItemDelegate(this))
    , m_currentDate(QDate::currentDate())
{
    setupUI();
    autoExpireStaleAppointments(); // 启动时自动清理过期单
    refreshView();
    updateStats();

    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, [this](){
        updateStats();
        refreshView();
    });
}

void AppointmentModule::setupUI()
{
    // 1. 基础布局
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 左侧主要区域
    QWidget *leftContainer = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(25, 20, 25, 20);
    leftLayout->setSpacing(20);

    // -- 初始化通用搜索框 --
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索宠物、会员、手机号...");
    m_searchEdit->setFixedSize(300, 38);
    m_searchEdit->setStyleSheet("QLineEdit { border: 1px solid #dcdfe6; border-radius: 19px; padding: 0 15px; background: white; font-size: 13px; } QLineEdit:focus { border-color: #409eff; }");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &AppointmentModule::onFilter);

    // -- 初始化通用新增按钮 --
    QPushButton *addBtn = new QPushButton("新增预约");
    addBtn->setFixedSize(130, 42);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #3b82f6; color: #3b82f6; border-radius: 21px; font-weight: bold; font-size: 14px; padding: 0; text-align: center; } "
        "QPushButton:hover { background: #eff6ff; }"
    );
    connect(addBtn, &QPushButton::clicked, this, &AppointmentModule::onAddAppointment);

    // 2. 驾驶舱 (Stats) - 整合标题与统计卡片
    QFrame *dashContainer = new QFrame();
    dashContainer->setObjectName("dashContainer");
    dashContainer->setStyleSheet("#dashContainer { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    
    QVBoxLayout *dashMainLayout = new QVBoxLayout(dashContainer);
    dashMainLayout->setContentsMargins(20, 15, 20, 15);
    dashMainLayout->setSpacing(15);

    QLabel *titleLabel = new QLabel("预约管理中心");
    titleLabel->setStyleSheet("font-size: 20px; color: #1e293b; font-weight: bold; border: none; background: transparent;");
    dashMainLayout->addWidget(titleLabel);

    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(20);
    dashMainLayout->addLayout(statLayout);

    auto createCard = [&](const QString &title, QLabel* &valLabel) {
        QFrame *card = new QFrame();
        card->setFixedHeight(90);
        card->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; } "
                           "QLabel { border: none; background: transparent; }");
        QVBoxLayout *l = new QVBoxLayout(card);
        QLabel *t = new QLabel(title); t->setStyleSheet("color: #94a3b8; font-size: 13px; background: transparent;"); 
        valLabel = new QLabel("0"); valLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #1e293b; background: transparent;");
        l->addWidget(t); l->addWidget(valLabel);
        return card;
    };

    statLayout->addWidget(createCard("今日预约总量", m_statTotal));
    statLayout->addWidget(createCard("洗护/美容队列", m_statGrooming));
    statLayout->addWidget(createCard("物流接送任务", m_statLogistics));
    statLayout->addWidget(createCard("寄养实时负载", m_statBoarding));
    
    leftLayout->addWidget(dashContainer);

    // 3. 操作栏容器化 (Action Card)
    QFrame *actionCard = new QFrame();
    actionCard->setObjectName("ActionCard");
    actionCard->setStyleSheet("#ActionCard { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    
    QHBoxLayout *actionRow = new QHBoxLayout(actionCard);
    actionRow->setContentsMargins(20, 10, 20, 10);
    actionRow->setSpacing(12);
    
    // -- 左侧：搜索与筛选 --
    actionRow->addWidget(m_searchEdit);
    
    m_statusCombo = new QComboBox();
    m_statusCombo->addItems({"全部", "待处理", "服务中", "已完成", "已取消", "已过期"});
    m_statusCombo->setFixedWidth(120);
    m_statusCombo->setFixedHeight(38);
    m_statusCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; color: #606266; font-weight: bold; font-size: 13px; } "
        "QComboBox:hover { border-color: #409eff; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }"
    );
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AppointmentModule::onStatusFilterChanged);
    actionRow->addWidget(m_statusCombo);
    
    actionRow->addStretch();
    
    // -- 中间：日期导航 --
    QPushButton *prevBtn = new QPushButton("< 上一天");
    m_dateBtn = new QPushButton("今天");
    QPushButton *nextBtn = new QPushButton("下一天 >");

    QString navStyle = "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 6px; padding: 6px 12px; color: #606266; font-weight: bold; height: 38px; } QPushButton:hover { color: #409eff; border-color: #409eff; background: #f0f9ff; }";
    prevBtn->setStyleSheet(navStyle); nextBtn->setStyleSheet(navStyle);
    m_dateBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #3b82f6; border-radius: 6px; padding: 6px 15px; color: #3b82f6; font-weight: bold; height: 38px; } QPushButton:hover { background: #eff6ff; }");

    connect(prevBtn, &QPushButton::clicked, this, &AppointmentModule::onPrevDay);
    connect(nextBtn, &QPushButton::clicked, this, &AppointmentModule::onNextDay);
    m_dateBtn->setText(QString("今天 (%1)").arg(m_currentDate.toString("MM/dd")));

    actionRow->addWidget(prevBtn);
    actionRow->addWidget(m_dateBtn);
    actionRow->addWidget(nextBtn);
    
    actionRow->addSpacing(20);
    
    // -- 右侧：新增按钮 (主要操作) --
    actionRow->addWidget(addBtn);
    
    leftLayout->addWidget(actionCard);
    leftLayout->addSpacing(10);
    leftLayout->addSpacing(10);

    // 3. 核心切换区域 (StackedWidget)
    m_stack = new QStackedWidget();

    // --- A. 格栅页面 ---
    m_gridPage = new QWidget();
    QVBoxLayout *gridPageLayout = new QVBoxLayout(m_gridPage);
    gridPageLayout->setContentsMargins(0, 0, 0, 0);
    gridPageLayout->setSpacing(0);

    QFrame *gridCard = new QFrame();
    gridCard->setObjectName("GridCard");
    gridCard->setStyleSheet("#GridCard { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QVBoxLayout *gridCardLayout = new QVBoxLayout(gridCard);
    gridCardLayout->setContentsMargins(20, 20, 20, 20);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");
    
    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;"); // 确保内容层也透明
    QHBoxLayout *kanban = new QHBoxLayout(scrollContent);
    kanban->setContentsMargins(0, 0, 10, 0);
    kanban->setSpacing(25);

    // 今日列
    QWidget *col1 = new QWidget();
    QVBoxLayout *col1L = new QVBoxLayout(col1);
    col1L->setContentsMargins(0, 0, 0, 0);
    m_todayTitle = new QLabel("今日日程安排");
    m_todayTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #2563eb; margin-bottom: 5px; background: transparent;");
    col1L->addWidget(m_todayTitle);
    m_todayGrid = new QVBoxLayout();
    m_todayGrid->setSpacing(12);
    col1L->addLayout(m_todayGrid);
    col1L->addStretch();
    kanban->addWidget(col1);

    // 垂直分割线
    QFrame *line = new QFrame(); line->setFrameShape(QFrame::VLine); line->setStyleSheet("color: #ebeef5; background: transparent;");
    kanban->addWidget(line);

    // 明日列
    QWidget *col2 = new QWidget();
    QVBoxLayout *col2L = new QVBoxLayout(col2);
    col2L->setContentsMargins(0, 0, 0, 0);
    m_tomorrowTitle = new QLabel("明日预约预告");
    m_tomorrowTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #606266; margin-bottom: 5px; background: transparent;");
    col2L->addWidget(m_tomorrowTitle);
    m_tomorrowGrid = new QVBoxLayout();
    m_tomorrowGrid->setSpacing(12);
    col2L->addLayout(m_tomorrowGrid);
    col2L->addStretch();
    kanban->addWidget(col2);

    scroll->setWidget(scrollContent);
    gridCardLayout->addWidget(scroll);
    gridPageLayout->addWidget(gridCard);
    
    m_stack->addWidget(m_gridPage);

    // --- B. 列表页面 ---
    m_listPage = new QWidget();
    QVBoxLayout *listPageLayout = new QVBoxLayout(m_listPage);
    listPageLayout->setContentsMargins(0, 0, 0, 0);

    m_listView = new QListView();
    m_listView->setModel(m_model);
    m_listView->setItemDelegate(m_delegate);
    m_listView->setFrameShape(QFrame::NoFrame);
    m_listView->setSpacing(2);
    m_listView->setMouseTracking(true);
    m_listView->setStyleSheet(
        "QListView { background: #f8fafc; outline: none; padding: 8px; border-radius: 8px; }"
        "QListView::item { border: none; }"
        "QListView::item:selected { background: transparent; }"
        "QListView::item:hover { background: transparent; }"
    );
    connect(m_listView, &QListView::clicked, this, &AppointmentModule::onAppointmentSelected);
    listPageLayout->addWidget(m_listView);

    // 列表模式下的分页栏 (统一标准右对齐)
    QFrame *pageBar = new QFrame();
    pageBar->setFixedHeight(50);
    pageBar->setStyleSheet("QFrame { background: white; border-top: 1px solid #f1f5f9; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    QHBoxLayout *pageL = new QHBoxLayout(pageBar);
    pageL->setContentsMargins(20, 0, 20, 0);

    QPushButton *prevPageBtn = new QPushButton("上一页");
    QPushButton *nextPageBtn = new QPushButton("下一页");
    m_pageLabel = new QLabel("第 1 页 / 共 1 页");
    
    QString pageBtnStyle = "QPushButton { height: 28px; border: 1px solid #e2e8f0; border-radius: 6px; background: white; color: #64748b; font-size: 12px; padding: 0 12px; font-weight: bold; } QPushButton:hover { border-color: #3b82f6; color: #3b82f6; background: #eff6ff; } QPushButton:disabled { background: white; color: #cbd5e1; border-color: #f1f5f9; }";
    prevPageBtn->setStyleSheet(pageBtnStyle);
    nextPageBtn->setStyleSheet(pageBtnStyle);
    m_pageLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold;");

    pageL->addStretch();
    pageL->addWidget(prevPageBtn);
    pageL->addSpacing(20);
    pageL->addWidget(m_pageLabel);
    pageL->addSpacing(20);
    pageL->addWidget(nextPageBtn);

    connect(prevPageBtn, &QPushButton::clicked, this, [this](){ if(m_currentPage > 1) { m_currentPage--; refreshView(); } });
    connect(nextPageBtn, &QPushButton::clicked, this, [this](){ m_currentPage++; refreshView(); }); // 这里逻辑简化，实际 renderList 会处理上限

    m_stack->addWidget(m_listPage);

    leftLayout->addWidget(m_stack, 1);

    // 4. 右侧详情页
    m_drawer = new AppointmentDetailDrawer(this);
    m_drawer->setFixedWidth(450);
    connect(m_drawer, &AppointmentDetailDrawer::confirmRequested, this, &AppointmentModule::handleConfirm);
    connect(m_drawer, &AppointmentDetailDrawer::startServiceRequested, this, &AppointmentModule::handleStartService);
    connect(m_drawer, &AppointmentDetailDrawer::completeRequested, this, &AppointmentModule::handleComplete); // 新增
    connect(m_drawer, &AppointmentDetailDrawer::cancelRequested, this, &AppointmentModule::handleCancel);
    connect(m_drawer, &AppointmentDetailDrawer::modifyRequested, this, &AppointmentModule::handleModify);
    connect(m_drawer, &AppointmentDetailDrawer::mediaUploadRequested, this, &AppointmentModule::handleMediaUpload);
    connect(m_drawer, &AppointmentDetailDrawer::galleryRequested, this, &AppointmentModule::handleGallery);
    connect(m_drawer, &AppointmentDetailDrawer::imageClicked, this, &AppointmentModule::showBigImage);

    rootLayout->addWidget(leftContainer, 1);
    rootLayout->addWidget(m_drawer);
}

void AppointmentModule::refreshView()
{
    autoExpireStaleAppointments(); // 每次刷新时检查过期单
    if (m_filterText.isEmpty()) {
        m_stack->setCurrentWidget(m_gridPage);
        renderGrid();
    } else {
        m_stack->setCurrentWidget(m_listPage);
        renderList();
    }
}

void AppointmentModule::autoExpireStaleAppointments()
{
    // 将所有 "昨天及更早" 的待处理/已确认订单自动标记为 "Expired"
    QDate today = QDate::currentDate();
    auto result = PetDataManager::instance()->getAppointments(1, 9999, "");
    for (auto info : result.first) {
        if (info.date.isEmpty()) continue;
        
        // 只处理还在进行中的状态，已完成/已取消/已过期的不动
        if (info.status != "Pending" && info.status != "Confirmed" && info.status != "In-Service") continue;
        
        // 寄养类型：按离店日期判定过期
        if (info.type == "Boarding") {
            QDate endDate;
            if (!info.boardingEndDate.isEmpty()) {
                endDate = QDate::fromString(info.boardingEndDate, "yyyy-MM-dd");
            } else if (info.duration > 0) {
                endDate = QDate::fromString(info.date, "yyyy-MM-dd").addDays(info.duration);
            }
            if (endDate.isValid() && endDate < today) {
                info.status = "Expired";
                PetDataManager::instance()->updateAppointment(info);
            }
            continue;
        }
        
        // 非寄养：按预约日期判定过期
        QDate apptDate = QDate::fromString(info.date, "yyyy-MM-dd");
        if (apptDate < today) {
            info.status = "Expired";
            PetDataManager::instance()->updateAppointment(info);
        }
    }
}

void AppointmentModule::renderGrid()
{
    // 清理旧视图
    auto clearLayout = [](QVBoxLayout *layout) {
        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
    };
    clearLayout(m_todayGrid);
    clearLayout(m_tomorrowGrid);

    updateDateBtnText();
    m_todayTitle->setText(m_currentDate == QDate::currentDate() ? "今日日程安排" : m_currentDate.toString("MM-dd") + " 日程");
    m_tomorrowTitle->setText(m_currentDate.addDays(1) == QDate::currentDate().addDays(1) ? "明日预约预告" : m_currentDate.addDays(1).toString("MM-dd") + " 预告");

    QString firstApptId;

    auto renderCol = [this, &firstApptId](QVBoxLayout *layout, const QDate &date) {
        auto result = PetDataManager::instance()->getAppointments(1, 100, "", m_statusFilter); // 获取数据并应用状态过滤
        QList<AppointmentInfo> all = result.first;
        
        // 分组时间槽
        QStringList timeSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
        QMap<QString, QList<AppointmentInfo>> groups;
        for (const auto &info : all) {
            if (info.date != date.toString("yyyy-MM-dd")) continue;
            // 寄养类型没有时间槽，放在第一个时间段显示
            if (info.hour.isEmpty()) {
                groups[timeSlots.first()].prepend(info);
                continue;
            }
            // 简单匹配时间槽
            QString s = timeSlots.first(); // 默认第一个
            for (const auto &slotItem : timeSlots) {
                if (info.hour >= slotItem.left(5) && info.hour < slotItem.right(5)) {
                    s = slotItem; break;
                }
            }
            groups[s].append(info);
        }

        for (const auto &slotName : timeSlots) {
            QFrame *slotCard = new QFrame();
            slotCard->setObjectName("slotCard");
            slotCard->setStyleSheet("#slotCard { background: white; border-radius: 8px; border: 1px solid #ebeef5; }");
            QVBoxLayout *sl = new QVBoxLayout(slotCard);
            sl->setContentsMargins(0, 0, 0, 0); 
            sl->setSpacing(0); // 头部与主体紧贴，内部由 body 负责边距
            slotCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

            // Header
            QFrame *header = new QFrame();
            header->setObjectName("slotHeader");
            header->setStyleSheet("#slotHeader { background: #fdfdfd; border-bottom: 1px solid #ebeef5; border-top-left-radius: 8px; border-top-right-radius: 8px; }");
            QHBoxLayout *hl = new QHBoxLayout(header);
            hl->setContentsMargins(15, 10, 15, 10);
            
            QLabel *timeL = new QLabel(slotName); 
            timeL->setStyleSheet("font-weight: 900; color: #1e293b; font-size: 15px; letter-spacing: 0.5px;");
            
            auto slotTasks = groups.value(slotName);
            QLabel *badgeL = new QLabel(QString("%1 预约").arg(slotTasks.size()));
            if (slotTasks.isEmpty()) {
                badgeL->setStyleSheet("background: #f4f4f5; color: #909399; border-radius: 10px; padding: 2px 8px; font-size: 11px; font-weight: bold; border: 1px solid #e9e9eb;");
            } else {
                badgeL->setStyleSheet("background: #ecf5ff; color: #409eff; border-radius: 10px; padding: 2px 8px; font-size: 11px; font-weight: bold; border: 1px solid #d9ecff;");
            }
            hl->addWidget(timeL); hl->addStretch(); hl->addWidget(badgeL);
            sl->addWidget(header);

            // Body
            QWidget *body = new QWidget();
            body->setObjectName("slotBody");
            QVBoxLayout *bl = new QVBoxLayout(body);
            bl->setContentsMargins(10, 8, 10, 8); // 稍微紧凑一点，对标物流中心
            bl->setSpacing(8); 
            body->setMinimumHeight(64); // 深度对标车辆调度中心的卡片高度
            body->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
            
            if (slotTasks.isEmpty()) {
                QLabel *empty = new QLabel("暂无预约");
                empty->setAlignment(Qt::AlignCenter);
                empty->setStyleSheet("color: #dcdfe6; font-size: 13px; background: transparent;");
                bl->addWidget(empty);
            } else {
                for (int i = 0; i < slotTasks.size(); ++i) {
                    const auto &task = slotTasks[i];
                    if (firstApptId.isEmpty()) firstApptId = task.id;

                    QFrame *taskBtn = new QFrame();
                    taskBtn->setObjectName("taskBtn");
                    taskBtn->setMinimumHeight(76); // 改用最小高度，允许内容扩展
                    taskBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); // 宽度跟随，高度固定在 76 或更多
                    taskBtn->setCursor(Qt::PointingHandCursor);
                    
                    // ═══ 超时判定逻辑 (使用统一 helper) ═══
                    bool isOverdue = isAppointmentOverdue(task);
                    bool isSelected = (task.id == m_selectedTaskId);

                    if (isSelected) {
                        taskBtn->setStyleSheet(
                            "QFrame#taskBtn { background: #f0f7ff; border: 2px solid #409eff; border-radius: 6px; } "
                            "QFrame#taskBtn:hover { background: #e1f0ff; }"
                        );
                    } else if (isOverdue) {
                        taskBtn->setStyleSheet(
                            "QFrame#taskBtn { background: #fff5f5; border: 1px solid #fecaca; border-radius: 4px; } "
                            "QFrame#taskBtn:hover { background: #fee2e2; }"
                        );
                    } else {
                        taskBtn->setStyleSheet(
                            "QFrame#taskBtn { background: transparent; border: 1px solid transparent; } "
                            "QFrame#taskBtn:hover { background: #f5f7fa; }"
                        );
                    }

                    QHBoxLayout *tl = new QHBoxLayout(taskBtn);
                    tl->setContentsMargins(12, 8, 12, 8);
                    tl->setSpacing(12);

                    // 宠物头像 (修复: 使用 ImageUtils 支持 Base64 加载)
                    QPixmap pixmap = ImageUtils::loadPixmap(task.petAvatar);
                    if (pixmap.isNull()) pixmap.load(":/images/load_img.jpg");
                    QSize avatarSize(40, 40);
                    QPixmap target(avatarSize);
                    target.fill(Qt::transparent);
                    QPainter p(&target);
                    p.setRenderHint(QPainter::Antialiasing);
                    p.setRenderHint(QPainter::SmoothPixmapTransform);
                    QPainterPath path;
                    path.addEllipse(1, 1, 38, 38);
                    p.setClipPath(path);
                    p.drawPixmap(1, 1, 38, 38, pixmap.scaled(38, 38, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                    p.setClipping(false);
                    QPen pen(QColor("#ebeef5"), 1);
                    p.setPen(pen);
                    p.drawEllipse(1, 1, 38, 38);
                    p.end();

                    QLabel *avatarLabel = new QLabel();
                    avatarLabel->setPixmap(target);
                    avatarLabel->setFixedSize(avatarSize);
                    avatarLabel->setStyleSheet("background: transparent;");
                    avatarLabel->setProperty("avatarPath", task.petAvatar.isEmpty() ? ":/images/load_img.jpg" : task.petAvatar);
                    avatarLabel->setCursor(Qt::PointingHandCursor);
                    avatarLabel->installEventFilter(this);
                    
                    QVBoxLayout *ti = new QVBoxLayout();
                    ti->setSpacing(4);
                    
                    QLabel *topLbl = new QLabel(QString("<span style='color:#303133; font-weight:bold; font-size:14px;'>%1</span> <span style='color:#909399; font-size:12px;'>&nbsp;·&nbsp; %2</span>").arg(task.petName, task.service));
                    topLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
                    
                    QString bottomText;
                    if (task.type == "Boarding" && !task.boardingEndDate.isEmpty()) {
                        QDate startD = QDate::fromString(task.date, "yyyy-MM-dd");
                        QDate endD = QDate::fromString(task.boardingEndDate, "yyyy-MM-dd");
                        bottomText = QString("<span style='color:#606266; font-size:12px;'>%1 &nbsp;|&nbsp; %2/%3 → %4/%5 共%6天</span>")
                            .arg(task.memberName)
                            .arg(startD.month()).arg(startD.day())
                            .arg(endD.month()).arg(endD.day())
                            .arg(task.duration);
                    } else {
                        bottomText = QString("<span style='color:#606266; font-size:12px;'>%1 &nbsp;|&nbsp; %2</span>").arg(task.memberName, task.memberPhone);
                    }
                    QLabel *bottomLbl = new QLabel(bottomText);
                    bottomLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
                    
                    ti->addWidget(topLbl);
                    ti->addWidget(bottomLbl);
                    ti->addStretch();
                    
                    // 状态标签
                    QString displayStatus;
                    if (task.status == "Pending" || task.status == "待处理") displayStatus = "待处理";
                    else if (task.status == "Confirmed" || task.status == "已确认") displayStatus = "已确认";
                    else if (task.status == "In-Service" || task.status == "服务中") displayStatus = "服务中";
                    else if (task.status == "Cancelled" || task.status == "已取消") displayStatus = "已取消";
                    else if (task.status == "Expired" || task.status == "已过期") displayStatus = "已过期";
                    else displayStatus = "已完成";

                    // 如果实时超时，追加醒目的 [已超时] 标记
                    QLabel *statusTag = new QLabel(isOverdue ? "⚠ 已超时" : displayStatus);
                    statusTag->setAttribute(Qt::WA_TransparentForMouseEvents);
                    
                    if (isOverdue) {
                        statusTag->setStyleSheet("background: #fef2f2; color: #dc2626; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; border: 1px solid #fecaca;");
                    } else if (displayStatus == "已确认" || displayStatus == "已完成") {
                        statusTag->setStyleSheet("background: #f0f9eb; color: #67c23a; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; border: 1px solid #e1f3d8;");
                    } else if (displayStatus == "待处理") {
                        statusTag->setStyleSheet("background: #fff7ed; color: #ea580c; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; border: 1px solid #ffedd5;");
                    } else if (displayStatus == "服务中") {
                        statusTag->setStyleSheet("background: #eff6ff; color: #2563eb; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; border: 1px solid #dbeafe;");
                    } else if (displayStatus == "已取消") {
                        statusTag->setStyleSheet("background: #f4f4f5; color: #909399; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; border: 1px solid #e9e9eb; text-decoration: line-through;");
                    } else if (displayStatus == "已过期") {
                        statusTag->setStyleSheet("background: #fafafa; color: #a3a3a3; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; border: 1px solid #e5e5e5;");
                    }

                    tl->addWidget(avatarLabel);
                    tl->addLayout(ti);
                    tl->addStretch();
                    tl->addWidget(statusTag);
                    
                    taskBtn->setProperty("taskId", task.id);
                    taskBtn->installEventFilter(this);
                    
                    bl->addWidget(taskBtn);

                    if (i < slotTasks.size() - 1) {
                        // 移除手动绘制的细线，改用布局 Spacing 自动间隔，效果更透气
                    }
                }
            }
            sl->addWidget(body);
            layout->addWidget(slotCard);
        }
    };

    renderCol(m_todayGrid, m_currentDate);
    renderCol(m_tomorrowGrid, m_currentDate.addDays(1));

    if (m_selectedTaskId.isEmpty() && !firstApptId.isEmpty()) {
        m_selectedTaskId = firstApptId;
    }

    if (!m_selectedTaskId.isEmpty()) {
        // 关键：只有当选中 ID 真正变化时才重载详情页，防止数据同步导致的无谓闪烁
        if (m_drawer->currentInfo().id != m_selectedTaskId) {
            m_drawer->setAppointment(m_selectedTaskId);
        }
        if (!m_drawer->isOpened()) m_drawer->showDrawer();
    } else {
        m_drawer->clearSelection();
        if (!m_drawer->isOpened()) m_drawer->showDrawer();
    }
}

void AppointmentModule::updateStats()
{
    auto stats = PetDataManager::instance()->getAppointmentStats();
    m_statTotal->setText(QString::number(stats.total));
    m_statGrooming->setText(QString::number(stats.grooming));
    m_statLogistics->setText(QString::number(stats.logistics));
    m_statBoarding->setText(QString("%1%").arg(stats.boardingLoad));
}

void AppointmentModule::renderList()
{
    m_model->clear();
    auto result = PetDataManager::instance()->getAppointments(m_currentPage, 20, m_filterText, m_statusFilter);
    for (const auto &info : result.first) {
        QStandardItem *item = new QStandardItem();
        item->setData(QVariant::fromValue(info), Qt::UserRole);
        m_model->appendRow(item);
    }
    m_pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg((result.second + 19) / 20));

    if (!result.first.isEmpty()) {
        QString firstId = result.first.first().id;
        if (m_drawer->currentInfo().id != firstId) {
            m_drawer->setAppointment(firstId);
        }
        if (!m_drawer->isOpened()) m_drawer->showDrawer();
        
        // 视觉上选中第一行
        m_listView->setCurrentIndex(m_model->index(0, 0));
    } else {
        m_drawer->clearSelection();
        if (!m_drawer->isOpened()) m_drawer->showDrawer();
    }
}

void AppointmentModule::onFilter(const QString &text)
{
    m_filterText = text;
    m_currentPage = 1;
    refreshView();
}

void AppointmentModule::onStatusFilterChanged(int index)
{
    Q_UNUSED(index);
    m_statusFilter = m_statusCombo->currentText();
    m_currentPage = 1;
    refreshView();
}

void AppointmentModule::onPrevDay() 
{ 
    m_currentDate = m_currentDate.addDays(-1); 
    updateDateBtnText();
    refreshView(); 
}

void AppointmentModule::onNextDay() 
{ 
    m_currentDate = m_currentDate.addDays(1); 
    updateDateBtnText();
    refreshView(); 
}

void AppointmentModule::onToday() 
{ 
    m_currentDate = QDate::currentDate(); 
    updateDateBtnText();
    refreshView(); 
}

bool AppointmentModule::isAppointmentOverdue(const AppointmentInfo &info) const
{
    if (info.date.isEmpty()) return false;
    
    // 只检测"待处理"和"已确认"状态的单据
    if (info.status != "Pending" && info.status != "Confirmed") return false;
    
    // 寄养类型：根据离店日期判定超时
    if (info.type == "Boarding") {
        if (!info.boardingEndDate.isEmpty()) {
            QDate endDate = QDate::fromString(info.boardingEndDate, "yyyy-MM-dd");
            return endDate < QDate::currentDate();
        }
        // 没有离店日期的寄养，用入住日期+duration估算
        QDate startDate = QDate::fromString(info.date, "yyyy-MM-dd");
        if (info.duration > 0) {
            return startDate.addDays(info.duration) < QDate::currentDate();
        }
        return false; // 无法判定
    }
    
    // 非寄养类型需要时间槽
    if (info.hour.isEmpty()) return false;
    
    QDate apptDate = QDate::fromString(info.date, "yyyy-MM-dd");
    
    // 昨天及更早的单据当然是超时的
    if (apptDate < QDate::currentDate()) return true;
    
    // 今天的单据，检查时间是否已过
    if (apptDate == QDate::currentDate()) {
        // ═══ 统一时间槽逻辑 ═══
        QStringList timeSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
        QTime endTime;
        bool matched = false;
        for (const auto &slot : timeSlots) {
            if (info.hour >= slot.left(5) && info.hour < slot.right(5)) {
                endTime = QTime::fromString(slot.right(5), "HH:mm");
                matched = true;
                break;
            }
        }
        
        if (!matched) {
            // 兜底逻辑：如果不在标准槽内，默认2小时超时
            endTime = QTime::fromString(info.hour, "HH:mm").addSecs(2 * 3600);
        }

        if (QTime::currentTime() > endTime) {
            return true;
        }
    }
    
    return false;
}

void AppointmentModule::updateDateBtnText()
{
    if (m_currentDate == QDate::currentDate()) {
        m_dateBtn->setText(QString("今天 (%1)").arg(m_currentDate.toString("MM/dd")));
    } else if (m_currentDate == QDate::currentDate().addDays(1)) {
        m_dateBtn->setText(QString("明天 (%1)").arg(m_currentDate.toString("MM/dd")));
    } else {
        m_dateBtn->setText(m_currentDate.toString("yyyy/MM/dd"));
    }
}

void AppointmentModule::onAddAppointment()
{
    AddAppointmentDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        auto infos = dlg.getAppointmentInfos();
        for (const auto &info : infos) {
            PetDataManager::instance()->addAppointment(info);
        }
        refreshView();
    }
}

void AppointmentModule::onAppointmentSelected(const QModelIndex &index)
{
    AppointmentInfo info = index.data(Qt::UserRole).value<AppointmentInfo>();
    m_drawer->setAppointment(info.id);
    m_drawer->showDrawer();
}

void AppointmentModule::handleConfirm(const QString &id)
{
    AppointmentInfo info = PetDataManager::instance()->getAppointment(id);
    if (info.id.isEmpty()) return;

    // 超时拦截提示
    if (isAppointmentOverdue(info)) {
        QString msg = QString("该预约已超过预定时间。\n"
                             "宠物: %1\n"
                             "预约时间: %2 %3\n\n"
                             "是否确认这是一笔迟到订单，继续操作？")
                        .arg(info.petName, info.date, info.hour);
        
        if (!CustomMessageDialog::confirm(this, "超时单据提醒", msg)) return;
    }

    // 1. 更新预约状态为已确认
    info.status = "Confirmed";
    PetDataManager::instance()->updateAppointment(info);

    // 2. 根据业务类型执行下发逻辑
    if (info.type == "Transport") {
        // 自动同步到调度中心
        LogisticsTask task;
        task.taskId = "LT" + QString::number(QDateTime::currentMSecsSinceEpoch()).right(6);
        task.petId = info.petId;
        task.type = "接送预约";
        task.appointmentTime = QString("%1 %2").arg(info.date, info.hour);
        task.address = info.address;
        task.status = "待处理";
        task.relatedModule = "预约中心下发";
        task.relatedAppointmentId = info.id;
        
        LogisticsManager::instance()->addLogisticsTask(task);
    } else if (info.type == "Boarding") {
        // 自动锁定房间 (此处预留寄养中心对接接口)
        // FosterManager::instance()->lockRoom(info.station, info.date, info.duration);
    }

    // 3. 刷新详情页和列表
    m_drawer->setAppointment(id);
    refreshView();
}

void AppointmentModule::handleStartService(const QString &id, const QString &staff)
{
    AppointmentInfo info = PetDataManager::instance()->getAppointment(id);
    if (info.id.isEmpty()) return;

    // 超时拦截提示
    if (isAppointmentOverdue(info)) {
        QString msg = QString("该预约已超过预定时间。\n"
                             "宠物: %1\n"
                             "预约时间: %2 %3\n\n"
                             "是否确认客户迟到，继续开始服务？")
                        .arg(info.petName, info.date, info.hour);
        
        if (!CustomMessageDialog::confirm(this, "超时单据提醒", msg)) return;
    }

    info.status = "In-Service";
    info.staff = staff;
    PetDataManager::instance()->updateAppointment(info);
    
    m_drawer->setAppointment(id);
    refreshView();
}

void AppointmentModule::handleComplete(const QString &id)
{
    AppointmentInfo info = PetDataManager::instance()->getAppointment(id);
    if (info.id.isEmpty()) return;

    if (CustomMessageDialog::confirm(this, "服务完成确认", 
                                     QString("确定 [%1] 的所有服务项目已执行完毕并可以关单了吗？\n确认后将生成待支付订单。").arg(info.petName))) {
        // 1. 更新状态为已完成
        info.status = "Completed";
        PetDataManager::instance()->updateAppointment(info);
        
        // 2. 生成清结算订单 (核心闭环)
        OrderInfo order;
        order.id = "ORD" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
        order.sourceModule = "Appointment";
        order.relatedId = info.id;
        order.petId = info.petId;
        order.petName = info.petName;
        order.memberName = info.memberName;
        order.itemDetails = info.service;
        order.totalAmount = info.amount;
        order.finalAmount = info.amount; // 初始不打折
        order.status = "Unpaid";
        order.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        
        PetDataManager::instance()->addOrder(order);

        // 3. 如果是寄养业务，自动触发离店影像录入
        if (info.type == "Boarding") {
            handleMediaUpload(id);
        }
        
        m_drawer->setAppointment(id);
        refreshView();
        
        CustomMessageDialog::showSuccess(this, "预约完成", "服务已完成，清结算订单已下发至订单中心。");
    }
}

void AppointmentModule::handleCancel(const QString &id)
{
    AppointmentInfo info = PetDataManager::instance()->getAppointment(id);
    if (info.id.isEmpty() || info.status == "Cancelled") return;

    // 检查是否有同组级联记录 (如关联的接送返程单)
    QList<AppointmentInfo> relatedInfos;
    if (!info.groupId.isEmpty()) {
        auto allInGroup = PetDataManager::instance()->getAppointmentsByGroupId(info.groupId);
        for (const auto &grpInfo : allInGroup) {
            if (grpInfo.id != id && grpInfo.status != "Cancelled") {
                relatedInfos.append(grpInfo);
            }
        }
    }

    if (!relatedInfos.isEmpty()) {
        QString msg = QString("您正在取消该预约。\n系统检测到该宠物还有 %1 项同时创建的关联服务（如接送返程）。\n请问是否需要一并取消？").arg(relatedInfos.size());
        
        QMessageBox box(this);
        box.setWindowTitle("智能级联提示");
        box.setText(msg);
        box.setIcon(QMessageBox::Question);
        QPushButton *btnCancelAll = box.addButton("一并取消所有关联", QMessageBox::AcceptRole);
        QPushButton *btnCancelSingle = box.addButton("仅取消当前项", QMessageBox::RejectRole);
        box.addButton("暂不取消", QMessageBox::DestructiveRole);
        
        box.exec();
        
        if (box.clickedButton() == btnCancelAll) {
            // 取消所有关联记录
            for (auto &grpInfo : relatedInfos) {
                grpInfo.status = "Cancelled";
                PetDataManager::instance()->updateAppointment(grpInfo);
            }
        } else if (box.clickedButton() == btnCancelSingle) {
            // 继续执行仅取消当前项，无额外操作
        } else {
            // 用户放弃操作
            return;
        }
    }

    info.status = "Cancelled";
    PetDataManager::instance()->updateAppointment(info);
    
    // 同步取消关联的物流任务
    LogisticsManager::instance()->cancelTaskByAppointmentId(info.id);
    
    // 如果想要彻底删除，可以使用 removeAppointment
    // PetDataManager::instance()->removeAppointment(id);
    
    m_drawer->clearSelection();
    refreshView();
}

void AppointmentModule::handleModify(const AppointmentInfo &info)
{
    AddAppointmentDialog dlg(this);
    dlg.setWindowTitle("修改预约服务");
    dlg.setInitialData(info);
    
    if (dlg.exec() == QDialog::Accepted) {
        auto infos = dlg.getAppointmentInfos();
        if (!infos.isEmpty()) {
            AppointmentInfo updatedInfo = infos.first();
            // 保持原有的 ID 和状态
            updatedInfo.id = info.id;
            updatedInfo.status = info.status;
            
            PetDataManager::instance()->updateAppointment(updatedInfo);
            
            // 如果用户在修改时勾选了生成返程单，则将多出来的作为新增
            for (int i = 1; i < infos.size(); ++i) {
                PetDataManager::instance()->addAppointment(infos[i]);
            }
            
            refreshView();
            m_drawer->setAppointment(info.id); // 刷新右侧详情界面
        }
    }
}

void AppointmentModule::handleMediaUpload(const QString &id) {
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this, "选择记录照片", "", "Images (*.png *.jpg *.jpeg *.webp)"
    );
    
    if (filePaths.isEmpty()) return;

    // 实际项目中这里应该有文件拷贝逻辑到应用目录，此处模拟保存路径
    PetDataManager::instance()->updateAppointmentPhotos(id, filePaths);
    
    // 刷新侧滑栏显示
    m_drawer->setAppointment(id);
    
}

void AppointmentModule::handleGallery(const QStringList &paths, int index) {
    (new FullImagePreviewDialog(paths, index, this->window()))->show();
}

void AppointmentModule::showBigImage(const QString &path)
{
    if (path.isEmpty()) return;
    (new FullImagePreviewDialog(QStringList{path}, 0, this->window()))->show();
}

bool AppointmentModule::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        if (obj->property("avatarPath").isValid()) {
            showBigImage(obj->property("avatarPath").toString());
            return true;
        }
        
        QWidget *w = qobject_cast<QWidget*>(obj);
        while (w) {
            if (w->property("taskId").isValid()) {
                QString tid = w->property("taskId").toString();
                m_selectedTaskId = tid; // 保存选中ID
                m_drawer->setAppointment(tid);
                m_drawer->showDrawer();
                refreshView(); // 刷新以应用边框
                return true;
            }
            w = w->parentWidget();
        }

        // Click on preview overlay to close
        QDialog *previewDlg = qobject_cast<QDialog*>(obj);
        if (!previewDlg) {
             previewDlg = qobject_cast<QDialog*>(obj->parent());
        }
        if (previewDlg && previewDlg->windowFlags() & Qt::FramelessWindowHint) {
            previewDlg->close();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
