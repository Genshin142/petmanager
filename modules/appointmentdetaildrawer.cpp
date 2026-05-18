#include "appointmentdetaildrawer.h"
#include "petdatamanager.h"
#include "staffselectiondialog.h"
#include "../utils/imageutils.h"
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QFrame>
#include <QEvent>
#include <QGridLayout>
#include <QDateTime>
#include <QWheelEvent>
#include <QCoreApplication>
#include "custommessagedialog.h"

AppointmentDetailDrawer::AppointmentDetailDrawer(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    m_animation = new QPropertyAnimation(this, "sideWidth");
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    // 关键：监听数据变动信号，实现多界面同步
    connect(PetDataManager::instance(), &PetDataManager::petDataChanged, this, [this](const QString &petId) {
        if (m_isOpened && m_currentInfo.petId == petId && !m_isDeletingPhoto) {
            // 重新从数据源获取最新状态并刷新 UI
            setAppointment(m_currentInfo.id);
        }
    });
}

void AppointmentDetailDrawer::setupUI()
{
    setObjectName("AppointmentDetailDrawer");
    setStyleSheet("#AppointmentDetailDrawer { background: transparent; border: none; }");

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(10, 20, 10, 20);
    outerLayout->setSpacing(0);

    QFrame *mainContainer = new QFrame();
    mainContainer->setObjectName("mainContainer");
    mainContainer->setStyleSheet("#mainContainer { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    outerLayout->addWidget(mainContainer);

    QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_mainStack = new QStackedWidget(this);
    mainLayout->addWidget(m_mainStack);

    // --- Index 0: 正常内容区 ---
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // 1. Header (紧凑型设计)
    QWidget *header = new QWidget();
    header->setFixedHeight(160); // 从 220 减小到 160
    header->setStyleSheet("background: white; border: none; border-top-left-radius: 12px; border-top-right-radius: 12px;");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 25, 20, 15); // 减小上边距
    headerLayout->setSpacing(15);
    headerLayout->setAlignment(Qt::AlignVCenter);

    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(80, 80); // 头像从 100 减到 80
    m_avatarLabel->setStyleSheet("background: transparent; border: none;");
    m_avatarLabel->installEventFilter(this);
    headerLayout->addWidget(m_avatarLabel);

    QVBoxLayout *nameCol = new QVBoxLayout();
    nameCol->setSpacing(6); // 间距从 10 减到 6
    nameCol->setAlignment(Qt::AlignVCenter);

    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->setSpacing(8);
    nameRow->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_petNameLabel = new QLabel("宠物名称");
    m_petNameLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #303133; background: transparent; border: none;");
    
    m_genderLabel = new QLabel();
    m_genderLabel->setFixedSize(18, 18);
    m_genderLabel->setAlignment(Qt::AlignCenter);
    
    nameRow->addWidget(m_petNameLabel);
    nameRow->addWidget(m_genderLabel);
    nameRow->addStretch();
    
    m_idLabel = new QLabel("ID: --");
    m_idLabel->setStyleSheet("font-size: 13px; color: #606266; background: transparent; border: none;");
    
    m_statusTag = new QLabel("待处理");
    m_statusTag->setStyleSheet("background: #ffedd5; color: #9a3412; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    m_statusTag->setFixedWidth(80);
    m_statusTag->setAlignment(Qt::AlignCenter);

    nameCol->addLayout(nameRow);
    nameCol->addWidget(m_idLabel);
    nameCol->addWidget(m_statusTag);
    headerLayout->addLayout(nameCol);
    headerLayout->addStretch();
    contentLayout->addWidget(header);

    // 2. 内容区 (滚动)
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
                          "QWidget#scrollContent { background: white; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    
    QWidget *content = new QWidget();
    content->setObjectName("scrollContent");
    content->setStyleSheet("background: white;");
    QVBoxLayout *scrollContentLayout = new QVBoxLayout(content);
    scrollContentLayout->setContentsMargins(20, 15, 20, 15);
    scrollContentLayout->setSpacing(12);
    scrollContentLayout->setAlignment(Qt::AlignTop);

    // 2.1 变动信息区域 (会被 setAppointment 清空)
    m_variableWidget = new QWidget();
    m_specLayout = new QVBoxLayout(m_variableWidget);
    m_specLayout->setContentsMargins(0, 0, 0, 0);
    m_specLayout->setSpacing(12);
    scrollContentLayout->addWidget(m_variableWidget);

    // 2.2 影像轮播区域 (固定组件，不会被清空)
    m_photoCarousel = new QWidget();
    m_photoCarousel->setFixedHeight(300); // 增加高度到 300
    m_photoCarousel->setObjectName("PhotoCarousel");
    m_photoCarousel->setStyleSheet("QWidget#PhotoCarousel { background: #f8fafc; border-radius: 12px; border: 1px dashed #dcdfe6; }");
    QVBoxLayout *carouselLayout = new QVBoxLayout(m_photoCarousel);
    carouselLayout->setContentsMargins(0, 0, 0, 0);
    
    m_photoLabel = new QLabel();
    m_photoLabel->setObjectName("CarouselPhoto"); // 明确身份
    m_photoLabel->setAlignment(Qt::AlignCenter);
    m_photoLabel->setCursor(Qt::PointingHandCursor);
    m_photoLabel->installEventFilter(this);
    
    // 删除按钮 overlay (红色 X)
    m_delPhotoBtn = new QPushButton("X", m_photoCarousel); // 使用标准大写 X 保证兼容性
    m_delPhotoBtn->setFixedSize(28, 28);
    m_delPhotoBtn->setCursor(Qt::PointingHandCursor);
    m_delPhotoBtn->setToolTip("从档案中删除此照片");
    m_delPhotoBtn->setStyleSheet(
        "QPushButton { background: #ff4d4f; color: white; border-radius: 14px; font-weight: bold; border: 2px solid white; font-size: 16px; text-align: center; padding: 0; } "
        "QPushButton:hover { background: #ff7875; }"
    );
    m_delPhotoBtn->hide(); 

    connect(m_delPhotoBtn, &QPushButton::clicked, this, [this]() {
        if (m_photos.isEmpty() || m_photoIndex < 0 || m_photoIndex >= m_photos.size()) return;
        
        QString currentUrl = m_photos[m_photoIndex];
        // 关键：与 PetMediaArchiveDialog 逻辑一致，服务照片标题为 "业务名服务记录"
        QString currentTitle = m_currentInfo.service + "服务记录";
        
        // #include "custommessagedialog.h"  // 已移至顶部
        if (CustomMessageDialog::confirm(this, "删除确认", "确定要永久删除这张服务照片吗？\n删除后影像档案也将同步更新。")) {
            m_isDeletingPhoto = true; // 开启拦截模式
            // 1. 同步后端
            PetDataManager::instance()->deleteMediaPhoto(m_currentInfo.petId, currentTitle, currentUrl);
            
            // 2. 更新本地缓存
            m_photos.removeAt(m_photoIndex);
            
            // 3. 刷新 UI (仅局部刷新轮播图，不重载整个界面)
            if (m_photos.isEmpty()) {
                m_photoCarousel->hide();
            } else {
                if (m_photoIndex >= m_photos.size()) m_photoIndex = m_photos.size() - 1;
                updateCarousel();
            }
            
            // 4. 发出信号，通知列表页刷新 (如果需要，但照片变动通常不需要刷新左侧列表)
            // PetDataManager::instance()->notifyGlobalDataChanged(); // 移除此项，避免触发整个 Module 的重绘闪烁
            m_isDeletingPhoto = false; // 关闭拦截模式
        }
    });

    carouselLayout->addWidget(m_photoLabel);
    
    m_photoCarousel->hide();
    scrollContentLayout->addWidget(m_photoCarousel);

    scrollContentLayout->addStretch();

    scroll->setWidget(content);
    contentLayout->addWidget(scroll);

    // 3. 底部操作栏
    QFrame *bottomBar = new QFrame();
    bottomBar->setFixedHeight(80);
    bottomBar->setStyleSheet("background: white; border-top: 1px solid #f0f2f5; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QHBoxLayout *btnLayout = new QHBoxLayout(bottomBar);
    btnLayout->setContentsMargins(20, 0, 20, 0);
    btnLayout->setSpacing(10);

    m_confirmBtn = new QPushButton("确认预约");
    m_confirmBtn->setFixedHeight(44);
    m_confirmBtn->setCursor(Qt::PointingHandCursor);
    m_confirmBtn->setStyleSheet("QPushButton { background: #67c23a; color: white; border-radius: 8px; font-weight: bold; border: none; font-size: 14px; } QPushButton:hover { background: #85ce61; }");
    connect(m_confirmBtn, &QPushButton::clicked, this, [this](){ emit confirmRequested(m_currentInfo.id); });

    m_startBtn = new QPushButton("开始服务");
    m_startBtn->setFixedHeight(44);
    m_startBtn->setCursor(Qt::PointingHandCursor);
    m_startBtn->setStyleSheet("QPushButton { background: #409eff; color: white; border-radius: 8px; font-weight: bold; border: none; font-size: 14px; } QPushButton:hover { background: #66b1ff; }");
    connect(m_startBtn, &QPushButton::clicked, this, &AppointmentDetailDrawer::onStartBtnClicked);

    m_mediaBtn = new QPushButton("上传记录照片");
    m_mediaBtn->setFixedHeight(44);
    m_mediaBtn->setCursor(Qt::PointingHandCursor);
    m_mediaBtn->setStyleSheet("QPushButton { background: #fdf6ec; color: #e6a23c; border: 1px solid #faecd8; border-radius: 8px; font-weight: bold; font-size: 14px; } QPushButton:hover { background: #e6a23c; color: white; }");
    connect(m_mediaBtn, &QPushButton::clicked, this, [this](){ emit mediaUploadRequested(m_currentInfo.id); });
    
    m_completeBtn = new QPushButton("完成服务");
    m_completeBtn->setFixedHeight(44);
    m_completeBtn->setCursor(Qt::PointingHandCursor);
    m_completeBtn->setStyleSheet("QPushButton { background: #67c23a; color: white; border-radius: 8px; font-weight: bold; border: none; font-size: 14px; } QPushButton:hover { background: #85ce61; }");
    connect(m_completeBtn, &QPushButton::clicked, this, [this](){ emit completeRequested(m_currentInfo.id); });

    m_modifyBtn = new QPushButton("修改信息");
    m_modifyBtn->setFixedHeight(44);
    m_modifyBtn->setCursor(Qt::PointingHandCursor);
    m_modifyBtn->setStyleSheet("QPushButton { background: white; color: #409eff; border: 1px solid #c6e2ff; border-radius: 8px; font-weight: bold; font-size: 14px; } QPushButton:hover { background: #ecf5ff; }");
    connect(m_modifyBtn, &QPushButton::clicked, this, [this](){ emit modifyRequested(m_currentInfo); });

    m_cancelBtn = new QPushButton("取消预约");
    m_cancelBtn->setFixedHeight(44);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setStyleSheet("QPushButton { background: white; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 8px; } QPushButton:hover { background: #fef0f0; }");
    connect(m_cancelBtn, &QPushButton::clicked, this, [this](){ emit cancelRequested(m_currentInfo.id); });

    m_logisticsHint = new QLabel("ℹ 当前状态由车辆调度中心实时同步");
    m_logisticsHint->setStyleSheet("color: #909399; font-size: 13px; font-style: italic;");
    m_logisticsHint->setAlignment(Qt::AlignCenter);
    m_logisticsHint->hide();

    btnLayout->addWidget(m_confirmBtn, 2);
    btnLayout->addWidget(m_startBtn, 2);
    btnLayout->addWidget(m_mediaBtn, 2);
    btnLayout->addWidget(m_completeBtn, 2); // 新增
    btnLayout->addWidget(m_modifyBtn, 1);
    btnLayout->addWidget(m_cancelBtn, 1);
    btnLayout->addWidget(m_logisticsHint, 1);
    contentLayout->addWidget(bottomBar);

    m_mainStack->addWidget(contentWidget);

    // --- Index 1: 空状态 ---
    QWidget *emptyWidget = new QWidget();
    emptyWidget->setStyleSheet("background: white;");
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *emptyIcon = new QLabel("📅");
    emptyIcon->setStyleSheet("font-size: 48px; color: #dcdfe6;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    
    QLabel *emptyLabel = new QLabel("暂无预约订单\n请在左侧列表选择或录入新订单");
    emptyLabel->setStyleSheet("color: #909399; font-size: 14px; line-height: 1.5;");
    emptyLabel->setAlignment(Qt::AlignCenter);
    
    emptyLayout->addStretch();
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addSpacing(20);
    emptyLayout->addWidget(emptyLabel);
    emptyLayout->addStretch();
    m_mainStack->addWidget(emptyWidget);
}

QWidget* AppointmentDetailDrawer::createSectionTitle(const QString &text)
{
    QLabel *title = new QLabel(text);
    title->setStyleSheet("font-size: 15px; color: #334155; font-weight: bold; margin-left: 4px; margin-bottom: 5px; border: none; background: transparent;");
    return title;
}

// 移除分割线生成器

void AppointmentDetailDrawer::addInfoRow(QGridLayout *grid, int row, const QString &label, const QString &value)
{
    QLabel *lLbl = new QLabel(label + "：");
    lLbl->setStyleSheet("color: #94a3b8; font-size: 13px; background: transparent; border: none;");
    lLbl->setFixedWidth(80);
    QLabel *vLbl = new QLabel(value.isEmpty() ? "--" : value);
    vLbl->setStyleSheet("color: #1e293b; font-size: 13px; font-weight: bold; background: transparent; border: none;");
    vLbl->setWordWrap(true);
    grid->addWidget(lLbl, row, 0, Qt::AlignTop);
    grid->addWidget(vLbl, row, 1, Qt::AlignTop);
}

void AppointmentDetailDrawer::setAppointment(const QString &id)
{
    m_photoCarousel->hide(); // 关键：更新开始前先隐藏，防止布局抖动闪现
    m_mainStack->setCurrentIndex(0);
    m_currentInfo = PetDataManager::instance()->getAppointment(id);
    PetInfo pet = PetDataManager::instance()->getPet(m_currentInfo.petId);
    
    // 1. 更新 Header
    m_petNameLabel->setText(m_currentInfo.petName);
    
    if (pet.gender == QString::fromUtf8("公")) {
        m_genderLabel->setText(QString::fromUtf8("♂"));
        m_genderLabel->setStyleSheet("background: #ecf5ff; color: #409eff; border-radius: 10px; font-weight: bold; font-size: 12px;");
    } else if (pet.gender == QString::fromUtf8("母")) {
        m_genderLabel->setText(QString::fromUtf8("♀"));
        m_genderLabel->setStyleSheet("background: #fef0f0; color: #f56c6c; border-radius: 10px; font-weight: bold; font-size: 12px;");
    } else {
        m_genderLabel->setText("");
        m_genderLabel->setStyleSheet("background: transparent;");
    }

    m_idLabel->setText(QString("ID: %1").arg(m_currentInfo.petId));
    
    // 头像处理 (同步为 80x80 圆形，修复: 使用 ImageUtils 支持 Base64 加载)
    QPixmap pixmap = ImageUtils::loadPixmap(m_currentInfo.petAvatar);
    if (pixmap.isNull()) pixmap.load(":/images/load_img.jpg");
    QSize avatarSize(80, 80); // 同步 setupUI 中的尺寸
    QPixmap target(avatarSize);
    target.fill(Qt::transparent);
    QPainter p(&target);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, avatarSize.width(), avatarSize.height());
    p.setClipPath(path);
    p.drawPixmap(0, 0, pixmap.scaled(avatarSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    m_avatarLabel->setPixmap(target);
    m_avatarLabel->setProperty("path", m_currentInfo.petAvatar.isEmpty() ? ":/images/load_img.jpg" : m_currentInfo.petAvatar);

    QString translatedStatus = m_currentInfo.status;
    if (m_currentInfo.status == "Pending") translatedStatus = "待处理";
    else if (m_currentInfo.status == "Confirmed") translatedStatus = "已确认";
    else if (m_currentInfo.status == "In-Service") translatedStatus = "服务中";
    else if (m_currentInfo.status == "Completed") translatedStatus = "已完成";
    else if (m_currentInfo.status == "Cancelled") translatedStatus = "已取消";
    else if (m_currentInfo.status == "Expired") translatedStatus = "已过期";
    
    // 实时超时判定 (根据标准时间槽判定)
    bool isOverdue = false;
    if (!m_currentInfo.date.isEmpty() && !m_currentInfo.hour.isEmpty()) {
        QDate apptDate = QDate::fromString(m_currentInfo.date, "yyyy-MM-dd");
        if (apptDate == QDate::currentDate()) {
            // 匹配时间槽结束时间
            QStringList timeSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
            QTime endTime;
            bool matched = false;
            for (const auto &slot : timeSlots) {
                if (m_currentInfo.hour >= slot.left(5) && m_currentInfo.hour < slot.right(5)) {
                    endTime = QTime::fromString(slot.right(5), "HH:mm");
                    matched = true;
                    break;
                }
            }
            if (!matched) endTime = QTime::fromString(m_currentInfo.hour, "HH:mm").addSecs(2 * 3600);

            if (QTime::currentTime() > endTime && 
                (m_currentInfo.status == "Pending" || m_currentInfo.status == "Confirmed")) {
                isOverdue = true;
                translatedStatus = "⚠ 已超时";
            }
        } else if (apptDate < QDate::currentDate()) {
            // 历史单据如果不处于终态，在详情页也视为超时/异常
            if (m_currentInfo.status == "Pending" || m_currentInfo.status == "Confirmed" || m_currentInfo.status == "In-Service") {
                isOverdue = true;
                translatedStatus = "⚠ 已超时";
            }
        }
    }
    
    m_statusTag->setText(translatedStatus);
    if (isOverdue) {
        m_statusTag->setStyleSheet("background: #fee2e2; color: #991b1b; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    } else if (m_currentInfo.status == "Pending" || m_currentInfo.status == "待处理") {
        m_statusTag->setStyleSheet("background: #ffedd5; color: #9a3412; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    } else if (m_currentInfo.status == "Confirmed" || m_currentInfo.status == "已确认" || m_currentInfo.status == "Completed" || m_currentInfo.status == "已完成") {
        m_statusTag->setStyleSheet("background: #dcfce7; color: #166534; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    } else if (m_currentInfo.status == "In-Service" || m_currentInfo.status == "服务中") {
        m_statusTag->setStyleSheet("background: #e0f2fe; color: #0369a1; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    } else if (m_currentInfo.status == "Cancelled" || m_currentInfo.status == "已取消") {
        m_statusTag->setStyleSheet("background: #f1f5f9; color: #64748b; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; text-decoration: line-through;");
    } else if (m_currentInfo.status == "Expired" || m_currentInfo.status == "已过期") {
        m_statusTag->setStyleSheet("background: #f1f5f9; color: #64748b; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    } else {
        m_statusTag->setStyleSheet("background: #f1f5f9; color: #64748b; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    }

    // 2. 清理并重构内容区
    QLayoutItem *item;
    while ((item = m_specLayout->takeAt(0)) != nullptr) {
        if (item->widget()) { item->widget()->hide(); item->widget()->deleteLater(); }
        if (item->layout()) {
            QLayoutItem *subItem;
            while ((subItem = item->layout()->takeAt(0)) != nullptr) {
                if (subItem->widget()) { subItem->widget()->hide(); subItem->widget()->deleteLater(); }
                delete subItem;
            }
            item->layout()->deleteLater();
        }
        delete item;
    }
    
    m_variableWidget->update();
    QCoreApplication::processEvents();

    auto addSection = [&](const QString &title, std::function<void(QGridLayout*)> filler) {
        m_specLayout->addWidget(createSectionTitle(title));
        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; } QLabel { background: transparent; border: none; }");
        QGridLayout *grid = new QGridLayout(card);
        grid->setContentsMargins(16, 16, 16, 16);
        grid->setVerticalSpacing(12);
        grid->setHorizontalSpacing(10);
        filler(grid);
        m_specLayout->addWidget(card);
        m_specLayout->addSpacing(10);
    };

    // --- A. 宠物档案 ---
    addSection("宠物档案", [&](QGridLayout *g) {
        addInfoRow(g, 0, "品种", pet.breed);
        addInfoRow(g, 1, "年龄", pet.age);
    });

    // --- B. 主客关系 ---
    addSection("主客关系", [&](QGridLayout *g) {
        addInfoRow(g, 0, "姓名", pet.ownerName + " (" + pet.ownerId + ")");
        addInfoRow(g, 1, "电话", pet.ownerPhone);
    });

    // --- C. 预约详情 ---
    addSection("预约详情", [&](QGridLayout *g) {
        QString fullService = m_currentInfo.service;
        QString category = fullService;
        QString details = "常规项目";
        
        if (fullService.contains("(") && fullService.endsWith(")")) {
            int start = fullService.indexOf("(");
            category = fullService.left(start).trimmed();
            details = fullService.mid(start + 1, fullService.length() - start - 2);
        }
        
        addInfoRow(g, 0, "业务类型", category);
        addInfoRow(g, 1, "预约时间", QString("%1 %2").arg(m_currentInfo.date, m_currentInfo.hour));
        
        int currentRow = 2;
        if (m_currentInfo.status == "In-Service" || m_currentInfo.status == "服务中") {
            addInfoRow(g, currentRow++, "服务人员", m_currentInfo.staff.isEmpty() ? "待分配" : m_currentInfo.staff);
        }
        
        addInfoRow(g, currentRow++, "项目明细", details);
        if (m_currentInfo.type == "Transport") addInfoRow(g, currentRow++, "接送地址", m_currentInfo.address);
        else if (m_currentInfo.type == "Boarding") addInfoRow(g, currentRow++, "房间号", m_currentInfo.roomNo.isEmpty() ? "待分配" : m_currentInfo.roomNo);
    });

    // --- D. 备注需求 ---
    if (!m_currentInfo.notes.isEmpty()) {
        addSection("备注需求", [&](QGridLayout *g) {
            QLabel *notesVal = new QLabel(m_currentInfo.notes);
            notesVal->setStyleSheet("color: #1e293b; font-size: 13px; font-weight: bold; background: transparent; border: none;");
            notesVal->setWordWrap(true);
            g->addWidget(notesVal, 0, 0);
        });
    }
    
    bool isBoarding = (m_currentInfo.type == "Boarding");
    
    if (!m_photos.isEmpty() && !isBoarding) {
        m_photoCarousel->show();
        updateCarousel();
    } else {
        m_photoCarousel->hide();
    }

    // --- 按钮显示逻辑 ---
    bool isPending = (m_currentInfo.status == "Pending" || m_currentInfo.status == "待处理");
    bool isConfirmed = (m_currentInfo.status == "Confirmed" || m_currentInfo.status == "已确认");
    bool isInService = (m_currentInfo.status == "In-Service" || m_currentInfo.status == "服务中");

    bool isTransport = (m_currentInfo.type == "Transport");
    
    m_confirmBtn->setVisible(isPending);
    m_startBtn->setVisible(isConfirmed && !isTransport); // 接送类由调度中心驱动状态
    m_mediaBtn->setVisible(isInService && !isBoarding); 
    m_completeBtn->setVisible(isInService && !isTransport); 
    m_modifyBtn->setVisible(isPending);
    m_cancelBtn->setVisible(isPending || isConfirmed);
    
    // 物流提示
    m_logisticsHint->setVisible(isTransport && (isConfirmed || isInService));

    if (isBoarding) {
        m_startBtn->setText("办理入住");
        m_completeBtn->setText("办理离店");
        m_confirmBtn->setText("确认并锁房");
    } else if (m_currentInfo.type == "Transport") {
        m_startBtn->setText("开始服务");
        m_completeBtn->setText("完成服务");
        m_confirmBtn->setText("确认并派送单");
    } else {
        m_startBtn->setText("开始服务");
        m_completeBtn->setText("完成服务");
        m_confirmBtn->setText("确认预约");
    }
}


void AppointmentDetailDrawer::clearSelection()
{
    m_mainStack->setCurrentIndex(1);
}

void AppointmentDetailDrawer::showDrawer() { m_isOpened = true; setFixedWidth(450); }
void AppointmentDetailDrawer::hideDrawer() { m_isOpened = false; setFixedWidth(0); }

void AppointmentDetailDrawer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void AppointmentDetailDrawer::updateCarousel() {
    if (m_photos.isEmpty()) return;
    if (m_photoIndex < 0) m_photoIndex = m_photos.size() - 1;
    if (m_photoIndex >= m_photos.size()) m_photoIndex = 0;

    QString path = m_photos[m_photoIndex];
    QPixmap pix = ImageUtils::loadPixmap(path);
    if (pix.isNull()) pix.load(":/images/load_img.jpg");

    // 绘制主图 + 底部指示点
    QSize targetSize(350, 280); // 总高度 280
    QPixmap result(targetSize);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // 1. 绘制缩放后的图片 (留出底部 30px 给圆点)
    QRect imgRect(0, 0, targetSize.width(), targetSize.height() - 30);
    QPixmap scaledPix = pix.scaled(imgRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = (imgRect.width() - scaledPix.width()) / 2;
    int y = (imgRect.height() - scaledPix.height()) / 2;
    p.drawPixmap(x, y, scaledPix);

    // 2. 绘制指示点 (Dots) - 放在图片正下方的空白区域
    if (m_photos.size() > 1) {
        int dotSize = 7;
        int spacing = 10;
        int totalWidth = m_photos.size() * dotSize + (m_photos.size() - 1) * spacing;
        int startX = (targetSize.width() - totalWidth) / 2;
        int dotY = targetSize.height() - 20; // 距离底部 20px

        for (int i = 0; i < m_photos.size(); ++i) {
            p.setPen(Qt::NoPen);
            if (i == m_photoIndex) {
                p.setBrush(QColor("#409eff"));
            } else {
                p.setBrush(QColor(0, 0, 0, 40));
            }
            p.drawEllipse(startX + i * (dotSize + spacing), dotY, dotSize, dotSize);
        }
    }
    p.end();

    m_photoLabel->setPixmap(result);
    m_photoLabel->setProperty("path", path); // 属性名完全对齐头像的 "path"

    // 3. 定位并显示删除按钮 (相对于 m_photoCarousel)
    m_delPhotoBtn->show();
    m_delPhotoBtn->raise();
    // 固定在容器右上角，留出一点边距
    m_delPhotoBtn->move(m_photoCarousel->width() - 35, 10);
}

bool AppointmentDetailDrawer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_photoLabel) {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *wheel = static_cast<QWheelEvent*>(event);
            if (wheel->angleDelta().y() > 0) m_photoIndex--;
            else m_photoIndex++;
            updateCarousel();
            return true;
        }
    }

    // 统一处理所有图片的点击 (包含头像和轮播图)
    if (event->type() == QEvent::MouseButtonPress) { // 改用 Press，响应更快且不受滚动干扰
        if (obj->property("path").isValid()) {
            if (obj == m_photoLabel || obj->objectName() == "CarouselPhoto") {
                // 如果是轮播图，发射图集信号
                emit galleryRequested(m_photos, m_photoIndex);
            } else {
                // 如果是头像，发射单图信号
                emit imageClicked(obj->property("path").toString());
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AppointmentDetailDrawer::onStartBtnClicked()
{
    if (m_currentInfo.type == "Boarding") {
        // 1. 寄养直接办理入住，不选人员
        emit startServiceRequested(m_currentInfo.id, "寄养中心");
        
        // 2. 立即触发“上传入店图片”逻辑
        emit mediaUploadRequested(m_currentInfo.id);
    } else {
        StaffSelectionDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            QString item = dlg.selectedStaff();
            if (!item.isEmpty()) {
                emit startServiceRequested(m_currentInfo.id, item);
            }
        }
    }
}
