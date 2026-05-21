#include "petrecorddrawer.h"
#include "../utils/imageutils.h"
#include "petmodule.h"
#include "fostermodule.h"
#include "petdatamanager.h"
#include "custommessagedialog.h"
#include <QPainter>
#include <QPainterPath>
#include <QIcon>
#include <QScrollArea>
#include <QComboBox>
#include <QScrollBar>
#include <QDateTime>
#include <QMenu>
#include <QAction>
#include <QButtonGroup>
#include <QStackedWidget>

PetRecordDrawer::PetRecordDrawer(QWidget *parent) : QWidget(parent), m_isOpened(true)
{
    m_timelineWidget = new PetTimelineWidget(this);
    setupUI();
    
    connect(m_timelineWidget->delegate(), &TimelineDelegate::editRequested, this, &PetRecordDrawer::onEditLog);
    connect(m_timelineWidget->delegate(), &TimelineDelegate::deleteRequested, this, &PetRecordDrawer::onDeleteLog);

    setFixedWidth(450);
    m_animation = new QPropertyAnimation(this, "sideWidth");
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
}

void PetRecordDrawer::setupUI()
{
    setObjectName("PetRecordDrawer");
    setStyleSheet("#PetRecordDrawer { background-color: white; border-left: 1px solid #ebeef5; } "
                  "QLabel { border: none; background: transparent; padding: 0; margin: 0; }");
    
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20); 
    outerLayout->setSpacing(0);

    QFrame *container = new QFrame();
    container->setObjectName("DrawerContainer");
    container->setStyleSheet("#DrawerContainer { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    container->setAttribute(Qt::WA_StyledBackground);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    outerLayout->addWidget(container);

    // --- 占位部分 (m_emptyWidget) ---
    m_emptyWidget = new QWidget();
    QVBoxLayout *emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    QLabel *emptyIcon = new QLabel("🐾");
    emptyIcon->setStyleSheet("font-size: 48px; color: #dcdfe6;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    QLabel *emptyText = new QLabel("暂无宠物信息\n请在左侧列表选择或录入新宠物");
    emptyText->setStyleSheet("color: #909399; font-size: 14px; line-height: 1.5;");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyLayout->addStretch();
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addSpacing(20);
    emptyLayout->addWidget(emptyText);
    emptyLayout->addStretch();
    mainLayout->addWidget(m_emptyWidget);

    // --- 内容部分 (m_contentWidget) ---
    m_contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    mainLayout->addWidget(m_contentWidget);

    // --- 1. 顶部：宠物名片 (常驻) ---
    QWidget *header = new QWidget();
    header->setFixedHeight(200); // 增加高度
    header->setStyleSheet("background: white; border-top-left-radius: 12px; border-top-right-radius: 12px; border: none;");
    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(20, 15, 20, 0);

    // 关闭按钮
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addStretch();
    QPushButton *closeBtn = new QPushButton("×");
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #909399; background: transparent; } QPushButton:hover { color: #f56c6c; }");
    connect(closeBtn, &QPushButton::clicked, this, &PetRecordDrawer::closeRequested);
    topBar->addWidget(closeBtn);
    headerLayout->addLayout(topBar);

    QHBoxLayout *infoLayout = new QHBoxLayout();
    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(100, 100);
    m_avatarLabel->setStyleSheet("border-radius: 50px; background: #f0f2f5; border: none;");
    m_avatarLabel->installEventFilter(this);
    m_avatarLabel->setCursor(Qt::PointingHandCursor);

    QVBoxLayout *nameCol = new QVBoxLayout();
    nameCol->setSpacing(4);
    m_nameLabel = new QLabel("尚未选择");
    m_nameLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #303133;");
    
    m_breedLabel = new QLabel("--");
    m_breedLabel->setStyleSheet("font-size: 14px; color: #606266;");
    
    nameCol->addWidget(m_nameLabel);
    nameCol->addWidget(m_breedLabel);

    m_roomBadge = new QLabel();
    m_roomBadge->hide();
    m_roomBadge->setStyleSheet(
        "background: #dcfce7; color: #166534; "
        "border-radius: 12px; padding: 4px 12px; font-weight: bold; font-size: 11px;"
    );

    infoLayout->addWidget(m_avatarLabel);
    infoLayout->addSpacing(15);
    infoLayout->addLayout(nameCol);
    infoLayout->addStretch();

    // 恢复编辑按钮
    m_editBtn = new QPushButton("修改资料");
    m_editBtn->setFixedSize(90, 32);
    m_editBtn->setCursor(Qt::PointingHandCursor);
    m_editBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #409eff; border-radius: 6px; } "
        "QPushButton:hover { background: #ecf5ff; }"
    );
    
    QHBoxLayout *m_editBtnLayout = new QHBoxLayout(m_editBtn);
    m_editBtnLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *m_editBtnText = new QLabel("修改资料");
    m_editBtnText->setAlignment(Qt::AlignCenter);
    m_editBtnText->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_editBtnText->setStyleSheet("color: #409eff; font-size: 12px; font-weight: bold; background: transparent; border: none;");
    m_editBtnLayout->addWidget(m_editBtnText);
    
    m_editBtn->setParent(container);
    m_editBtn->hide();

    connect(m_editBtn, &QPushButton::clicked, this, [this](){ emit editRequested(m_currentPet); });
    // infoLayout->addWidget(m_editBtn, 0, Qt::AlignTop | Qt::AlignRight);

    headerLayout->addLayout(infoLayout);
    headerLayout->addSpacing(15); // 增加间距
    headerLayout->addStretch();

    contentLayout->addWidget(header);

       // --- 2. 导航栏 (Tab Bar - Segmented Control Style) ---
    QWidget *tabWidget = new QWidget();
    tabWidget->setFixedHeight(40);
    tabWidget->setObjectName("SegmentedControl");
    tabWidget->setStyleSheet("#SegmentedControl { background: #f1f5f9; border-radius: 20px; border: none; }");
    QHBoxLayout *tabLayout = new QHBoxLayout(tabWidget);
    tabLayout->setContentsMargins(4, 4, 4, 4);
    tabLayout->setSpacing(4);

    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->setExclusive(true);

    QStringList tabs = {"档案", "寄养情况", "服务轨迹"};
    for (int i = 0; i < tabs.size(); ++i) {
        QPushButton *btn = new QPushButton(tabs[i]);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(32);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setStyleSheet(
            "QPushButton { border: none; font-size: 13px; color: #64748b; background: transparent; border-radius: 16px; padding: 0; text-align: center; } "
            "QPushButton:hover { color: #1e293b; } "
            "QPushButton:checked { color: #3b82f6; font-weight: bold; background: white; }"
        );
        m_tabGroup->addButton(btn, i);
        tabLayout->addWidget(btn);
    }

    // --- 居中并限制宽度 ---
    QWidget *tabContainer = new QWidget();
    tabContainer->setFixedHeight(60);
    QHBoxLayout *tabContainerLayout = new QHBoxLayout(tabContainer);
    tabContainerLayout->setContentsMargins(0, 10, 0, 10);
    tabContainerLayout->addStretch();
    tabWidget->setFixedWidth(280);
    tabContainerLayout->addWidget(tabWidget);
    tabContainerLayout->addStretch();

    contentLayout->addWidget(tabContainer);

    // --- 3. 内容区 (Stacked Widget) ---
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->setStyleSheet("QStackedWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    m_stackedWidget->addWidget(createArchivePage());        // Index 0
    m_stackedWidget->addWidget(createBoardingPage());       // Index 1
    m_stackedWidget->addWidget(createServiceHistoryPage());  // Index 2
    contentLayout->addWidget(m_stackedWidget);

    connect(m_tabGroup, &QButtonGroup::idClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    m_tabGroup->button(0)->setChecked(true);
    
    // 采用绝对定位固定位置 (297, 21) 确保全系统对齐
    m_editBtn->setParent(container);
    m_editBtn->move(297, 21);
    m_editBtn->raise();

    showEmptyState(true); // 默认显示占位界面
}

void PetRecordDrawer::setPet(const PetInfo &info, const QList<PetActivityLog> &logs, const QList<PetMedia> &media, const QList<FosterBatch> &batches)
{
    if (info.id.isEmpty()) {
        showEmptyState(true);
        return;
    }
    showEmptyState(false);

    m_currentPet = info;
    m_allLogs = logs;
    m_currentMedia = media;
    m_currentBatches = batches;
    
    m_editBtn->show(); // 确保绝对定位按钮显示
    m_editBtn->raise();
    
    // --- 顶部 Header 更新 ---
    m_nameLabel->setText(info.name);
    m_breedLabel->setText(info.breed);
    if (info.status == "在店 (寄养中)") {
        m_roomBadge->setText("房间：" + (info.roomNo.isEmpty() ? "B-102" : info.roomNo));
        m_roomBadge->show();
    } else {
        m_roomBadge->hide();
    }
    
    // 头像处理
    QPixmap srcPix = ImageUtils::loadPixmap(info.avatarPath);
    if (srcPix.isNull()) srcPix.load(":/images/load_img.jpg");
    
    QSize size(100, 100);
    QPixmap target(size);
    target.fill(Qt::transparent);
    QPainter p(&target);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath path;
    path.addEllipse(1, 1, size.width() - 2, size.height() - 2);
    p.setClipPath(path);
    
    QPixmap scaled = srcPix.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = (size.width() - scaled.width()) / 2;
    int y = (size.height() - scaled.height()) / 2;
    p.drawPixmap(x, y, scaled);
    m_avatarLabel->setPixmap(target);
    m_avatarLabel->setProperty("avatarPath", info.avatarPath); // 确保点击放大有效

    // --- Tab 1: 档案页更新 ---
    m_valGender->setText(info.gender);
    m_valAge->setText(info.age.isEmpty() ? "--" : info.age);
    m_valOwnerName->setText(info.ownerName);
    m_valOwnerPhone->setText(info.ownerPhone.isEmpty() ? "--" : info.ownerPhone);
    m_valOwnerId->setText(info.ownerId.isEmpty() ? "--" : info.ownerId);
    
    m_valHealthStatus->setText(info.health.isEmpty() ? QString::fromUtf8("健康良好") : info.health);
    QString vacStr = info.vaccine.isEmpty() ? QString::fromUtf8("未接种") : info.vaccine;
    m_vaccineBtn->setText(vacStr);
    
    // 动态更新疫苗按钮样式
    if (vacStr == QString::fromUtf8("未接种")) {
        m_vaccineBtn->setStyleSheet("QPushButton#VaccineBtn { background: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 4px; font-size: 11px; font-weight: bold; padding: 0 10px; } "
                                   "QPushButton#VaccineBtn:hover { background: #f56c6c; color: white; }");
    } else {
        m_vaccineBtn->setStyleSheet("QPushButton#VaccineBtn { background: #ecf5ff; color: #409eff; border: 1px solid #b3d8ff; border-radius: 4px; font-size: 11px; font-weight: bold; padding: 0 10px; } "
                                   "QPushButton#VaccineBtn:hover { background: #409eff; color: white; }");
    }
    
    m_valWeight->setText(QString("%1 kg").arg(info.weight, 0, 'f', 1));
    
    m_valMedicalHistory->setText(info.medicalHistory.isEmpty() ? "无重大病史记录" : info.medicalHistory);
    m_valDietary->setText(info.dietary.isEmpty() ? "常规饮食，无过敏" : info.dietary);

    // --- Tab 2: 寄养页更新 ---
    auto updateSummary = [=](const FosterBatch &b) {
        m_dateInVal->setText(b.startTime);
        QDate s = QDate::fromString(b.startTime, "yyyy-MM-dd");
        QDate e = b.isActive ? QDate::currentDate() : QDate::fromString(b.endTime, "yyyy-MM-dd");
        int d = s.isValid() && e.isValid() ? s.daysTo(e) + 1 : 1;
        m_durationVal->setText(QString("%1 天").arg(d));
        
        if (b.isActive) {
            m_dateOutTitle->setText("预计离店时间");
            m_dateOutVal->setText(info.fosterEndTime.isEmpty() ? "尚未设定" : info.fosterEndTime);
            m_weightInVal->setText(QString("%1 kg").arg(info.weight, 0, 'f', 1));
            m_weightOutVal->setText("尚未结算");
            m_weightOutVal->setStyleSheet("color: #909399; font-size: 13px; font-weight: bold;");
        } else {
            m_dateOutTitle->setText("离店时间");
            m_dateOutVal->setText(b.endTime);
            m_weightInVal->setText("20.5 kg"); 
            m_weightOutVal->setText("21.2 kg");
            m_weightOutVal->setStyleSheet("color: #67c23a; font-size: 13px; font-weight: bold;");
        }
        m_periodBtn->setText(QString("%1 ~ %2").arg(b.startTime, b.isActive ? "至今" : b.endTime));
    };

    if (!m_currentBatches.isEmpty()) {
        m_boardingStack->setCurrentIndex(0);
        updateSummary(m_currentBatches.first());
    } else {
        m_boardingStack->setCurrentIndex(1);
    }
    m_timelineWidget->setLogs(logs);

    m_tabGroup->button(0)->setChecked(true);
    m_stackedWidget->setCurrentIndex(0);
    
    // --- Tab 3: 服务轨迹更新 ---
    while (m_serviceHistoryLayout->count() > 0) {
        QLayoutItem *item = m_serviceHistoryLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->hide();
            item->widget()->deleteLater();
        }
        delete item;
    }

    auto appointments = PetDataManager::instance()->getAppointmentsForPet(info.id);
    if (appointments.isEmpty() && batches.isEmpty()) {
        QLabel *emptyL = new QLabel(QString::fromUtf8("暂无服务历史记录"));
        emptyL->setAlignment(Qt::AlignCenter);
        emptyL->setStyleSheet("color: #909399; font-size: 13px; margin-top: 50px;");
        m_serviceHistoryLayout->addWidget(emptyL);
    } else {
        // 1. 渲染寄养服务卡片 (首位展示，超轻量级紧凑，沉稳黑色字)
        for (const auto &b : batches) {
            QFrame *card = new QFrame();
            card->setStyleSheet("QFrame { background: #fcfcfd; border-radius: 6px; border: 1px solid #ebeef5; padding: 8px 12px; } QLabel { border: none; }");
            QVBoxLayout *cardL = new QVBoxLayout(card);
            cardL->setContentsMargins(0, 0, 0, 0);
            cardL->setSpacing(4);

            QHBoxLayout *top = new QHBoxLayout();
            top->setContentsMargins(0, 0, 0, 0);

            QLabel *typeL = new QLabel(QString::fromUtf8("宠物寄养服务"));
            typeL->setStyleSheet("color: #303133; font-weight: bold; font-size: 13px;"); // 墨黑色高端质感
            
            QLabel *statusL = new QLabel(b.isActive ? QString::fromUtf8("进行中") : QString::fromUtf8("已完成"));
            QString statusStyle = "font-size: 10px; padding: 2px 8px; border-radius: 10px; font-weight: bold; ";
            if (!b.isActive) statusStyle += "background: #dcfce7; color: #166534;";
            else statusStyle += "background: #e0f2fe; color: #0369a1;";
            statusL->setStyleSheet(statusStyle);

            top->addWidget(typeL);
            top->addStretch();
            top->addWidget(statusL);
            cardL->addLayout(top);

            QLabel *infoL = new QLabel(QString::fromUtf8("时间: %1 ~ %2   |   房间: %3")
                .arg(b.startTime, b.isActive ? QString::fromUtf8("至今") : b.endTime, info.roomNo.isEmpty() ? QString::fromUtf8("B-102") : info.roomNo));
            infoL->setStyleSheet("color: #909399; font-size: 12px;");
            
            cardL->addWidget(infoL);
            m_serviceHistoryLayout->addWidget(card);
        }

        // 2. 渲染普通服务卡片
        for (const auto &appt : appointments) {
            QFrame *card = new QFrame();
            card->setStyleSheet("QFrame { background: #fcfcfd; border-radius: 8px; border: 1px solid #ebeef5; padding: 12px; } QLabel { border: none; }");
            QVBoxLayout *cardL = new QVBoxLayout(card);
            cardL->setSpacing(8);

            QHBoxLayout *top = new QHBoxLayout();
            QString color = "#409eff";
            if (appt.type == "Boarding") { color = "#f56c6c"; }
            else if (appt.type == "Grooming" || appt.type == "Beauty") { color = "#67c23a"; }
            else if (appt.type == "Transport") { color = "#e6a23c"; }

            QLabel *typeL = new QLabel(appt.service);
            typeL->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px;").arg(color));
            
            QLabel *statusL = new QLabel(appt.status == "Pending" ? "待处理" : (appt.status == "Completed" ? "已完成" : "进行中"));
            QString statusStyle = "font-size: 11px; padding: 4px 12px; border-radius: 12px; font-weight: bold; ";
            if (appt.status == "Completed") statusStyle += "background: #dcfce7; color: #166534;";
            else if (appt.status == "Pending") statusStyle += "background: #ffedd5; color: #9a3412;";
            else statusStyle += "background: #e0f2fe; color: #0369a1;";
            statusL->setStyleSheet(statusStyle);

            top->addWidget(typeL);
            top->addStretch();
            top->addWidget(statusL);
            cardL->addLayout(top);

            QHBoxLayout *bottom = new QHBoxLayout();
            QVBoxLayout *infoCol = new QVBoxLayout();
            infoCol->setSpacing(4);

            QLabel *timeL = new QLabel(QString("%1 %2").arg(appt.date, appt.hour));
            timeL->setStyleSheet("color: #606266; font-size: 13px;");
            infoCol->addWidget(timeL);

            // 操作人员和服务金额
            QHBoxLayout *detailRow = new QHBoxLayout();
            detailRow->setSpacing(15);
            
            QLabel *staffL = new QLabel("操作人: " + (appt.staff.isEmpty() ? "未指定" : appt.staff));
            staffL->setStyleSheet("color: #909399; font-size: 12px;");
            
            QLabel *amountL = new QLabel(QString("金额: ¥%1").arg(appt.amount, 0, 'f', 2));
            amountL->setStyleSheet("color: #f56c6c; font-size: 12px; font-weight: bold;");
            
            detailRow->addWidget(staffL);
            detailRow->addWidget(amountL);
            detailRow->addStretch();
            infoCol->addLayout(detailRow);

            if (!appt.notes.isEmpty()) {
                QLabel *noteL = new QLabel("备注: " + appt.notes);
                noteL->setStyleSheet("color: #909399; font-size: 12px; font-style: italic;");
                noteL->setWordWrap(true);
                infoCol->addWidget(noteL);
            }
            bottom->addLayout(infoCol, 1);

            // 右侧影像查看按钮
            if (!appt.photos.isEmpty()) {
                QPushButton *mediaBtn = new QPushButton("影像留档");
                mediaBtn->setFixedWidth(96);
                mediaBtn->setCursor(Qt::PointingHandCursor);
                // 移除固定高度，使用内边距确保文字居中不被裁剪
                mediaBtn->setStyleSheet(
                    "QPushButton { background-color: #fa8c16; color: white; border: none; border-radius: 15px; font-size: 13px; font-weight: bold; padding: 6px 0; } "
                    "QPushButton:hover { background-color: #ff9c12; }"
                );
                connect(mediaBtn, &QPushButton::clicked, this, [this, appt]() {
                    QList<PetMedia> medias;
                    PetMedia m; m.urls = appt.photos; m.type = "image"; m.title = appt.service + "服务记录";
                    // 为该预约下的所有照片分配相同的时间点
                    QString ts = appt.date + " " + appt.hour.split(" ").first();
                    for (int i = 0; i < appt.photos.size(); ++i) m.timestamps << ts;
                    medias << m;
                    // 传入 flatten = true，表示服务记录直接平铺展示
                    PetMediaArchiveDialog dlg(m_currentPet.id, m_currentPet.name, medias, this->window(), "", "", true);
                    dlg.exec();
                });
                bottom->addWidget(mediaBtn, 0, Qt::AlignVCenter);
            }

            cardL->addLayout(bottom);
            m_serviceHistoryLayout->addWidget(card);
        }
    }
    m_serviceHistoryLayout->addStretch();

    updateEmptyState();
}

void PetRecordDrawer::updateEmptyState() {}
void PetRecordDrawer::showEmptyState(bool empty)
{
    if (m_emptyWidget) m_emptyWidget->setVisible(empty);
    if (m_contentWidget) m_contentWidget->setVisible(!empty);
    if (m_editBtn) m_editBtn->setVisible(!empty);
}
void PetRecordDrawer::updateBatchStatus(const QString &status) { Q_UNUSED(status); }

void PetRecordDrawer::addLogItem(const PetActivityLog &log)
{
    m_allLogs.append(log);
    m_timelineWidget->setLogs(m_allLogs);
    m_timelineWidget->view()->scrollTo(m_timelineWidget->model()->index(m_timelineWidget->model()->rowCount() - 1), QAbstractItemView::PositionAtBottom);
}

void PetRecordDrawer::onSubmitQuickAction() {}
void PetRecordDrawer::onCalendarClicked(const QDate &date) { Q_UNUSED(date); }
void PetRecordDrawer::onToggleCalendar() {}
void PetRecordDrawer::onEditLog(const QModelIndex &index) { Q_UNUSED(index); }
void PetRecordDrawer::onDeleteLog(const QModelIndex &index) { Q_UNUSED(index); }
void PetRecordDrawer::onBatchChanged(int index) { Q_UNUSED(index); }

void PetRecordDrawer::showDrawer()
{
    m_isOpened = true;
    m_timelineWidget->view()->doItemsLayout(); 
    m_animation->stop();
    m_animation->setStartValue(width());
    m_animation->setEndValue(450); 
    m_animation->start();
}

void PetRecordDrawer::hideDrawer()
{
    m_isOpened = false;
    m_animation->stop();
    m_animation->setStartValue(width());
    m_animation->setEndValue(0);
    m_animation->start();
}

#include <QDebug>
bool PetRecordDrawer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avatarLabel && event->type() == QEvent::MouseButtonPress) {
        qDebug() << "[PetRecordDrawer] Avatar Clicked! Emitting signal with path:" << m_currentPet.avatarPath;
        emit avatarClicked(m_currentPet.avatarPath);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

QWidget* PetRecordDrawer::createServiceHistoryPage()
{
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
                          "QScrollArea > QWidget > QWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    
    QWidget *content = new QWidget();
    content->setStyleSheet("background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    m_serviceHistoryLayout = new QVBoxLayout(content);
    m_serviceHistoryLayout->setContentsMargins(16, 20, 16, 20);
    m_serviceHistoryLayout->setSpacing(15);
    
    scroll->setWidget(content);
    return scroll;
}

QWidget* PetRecordDrawer::createArchivePage()
{
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
                          "QScrollArea > QWidget > QWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    
    QWidget *content = new QWidget();
    content->setStyleSheet("background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(16);

    auto createGroupCard = [&](const QString &title, QVBoxLayout* &cardLayout) {
        QWidget *group = new QWidget();
        QVBoxLayout *groupL = new QVBoxLayout(group);
        groupL->setContentsMargins(0, 0, 0, 0);
        groupL->setSpacing(10);

        QLabel *tLabel = new QLabel(title);
        tLabel->setStyleSheet("color: #334155; font-size: 15px; font-weight: bold; margin-left: 4px;");
        groupL->addWidget(tLabel);

        QFrame *card = new QFrame();
        card->setObjectName("ArchiveGroupCard");
        card->setStyleSheet("QFrame#ArchiveGroupCard { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; } QLabel { border: none; background: transparent; }");
        cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(12);
        
        groupL->addWidget(card);
        return group;
    };

    auto addDetailRow = [&](QVBoxLayout *layout, const QString &label, QLabel* &valLabel) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *titleL = new QLabel(label);
        titleL->setStyleSheet("color: #909399; font-size: 13px; background: transparent;");
        titleL->setFixedWidth(75);
        valLabel = new QLabel("--");
        valLabel->setStyleSheet("color: #303133; font-size: 13px; font-weight: 500; background: transparent;");
        valLabel->setWordWrap(true);
        row->addWidget(titleL);
        row->addWidget(valLabel, 1);
        layout->addLayout(row);
    };

    // 1. 身份与联系
    QVBoxLayout *baseL;
    contentLayout->addWidget(createGroupCard(QString::fromUtf8("身份与联系"), baseL));
    addDetailRow(baseL, QString::fromUtf8("性别"), m_valGender);
    addDetailRow(baseL, QString::fromUtf8("年龄/生日"), m_valAge);
    addDetailRow(baseL, QString::fromUtf8("主人 ID"), m_valOwnerId);
    addDetailRow(baseL, QString::fromUtf8("主人姓名"), m_valOwnerName);
    addDetailRow(baseL, QString::fromUtf8("联系电话"), m_valOwnerPhone);

    // 2. 健康与接种
    QVBoxLayout *healthL;
    contentLayout->addWidget(createGroupCard(QString::fromUtf8("健康与接种"), healthL));
    addDetailRow(healthL, QString::fromUtf8("健康状态"), m_valHealthStatus);
    
    QHBoxLayout *vacRow = new QHBoxLayout();
    QLabel *vacTitle = new QLabel(QString::fromUtf8("疫苗接种"));
    vacTitle->setStyleSheet("color: #909399; font-size: 13px;");
    vacTitle->setFixedWidth(75);
    m_vaccineBtn = new QPushButton("--");
    m_vaccineBtn->setObjectName("VaccineBtn");
    m_vaccineBtn->setCursor(Qt::PointingHandCursor);
    m_vaccineBtn->setFixedHeight(26); // 取消定宽，让文字自由伸展
    // 默认样式会在 setPet 中被覆盖
    m_vaccineBtn->setStyleSheet("QPushButton#VaccineBtn { background: #f0f9eb; color: #67c23a; border: 1px solid #e1f3d8; border-radius: 4px; font-size: 11px; font-weight: bold; padding: 0 10px; }");
    connect(m_vaccineBtn, &QPushButton::clicked, this, [this]() {
        // 调用疫苗接种详情弹窗
        VaccineDetailDialog dlg(m_currentPet.name, PetDataManager::instance()->getVaccines(m_currentPet.id), this->window());
        dlg.exec();
    });
    vacRow->addWidget(vacTitle);
    vacRow->addWidget(m_vaccineBtn);
    vacRow->addStretch();
    healthL->addLayout(vacRow);
    
    addDetailRow(healthL, QString::fromUtf8("当前体重"), m_valWeight);

    // 3. 医疗与禁忌
    QVBoxLayout *medL;
    contentLayout->addWidget(createGroupCard(QString::fromUtf8("医疗与禁忌"), medL));
    addDetailRow(medL, QString::fromUtf8("以往病例"), m_valMedicalHistory);
    addDetailRow(medL, QString::fromUtf8("饮食禁忌"), m_valDietary);

    contentLayout->addStretch();
    scroll->setWidget(content);
    return scroll;
}

QWidget* PetRecordDrawer::createBoardingPage()
{
    m_boardingStack = new QStackedWidget();

    // 1. 正常内容视图 (Index 0)
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
                          "QScrollArea > QWidget > QWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    
    QWidget *content = new QWidget();
    content->setStyleSheet("background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(20);

    // 1. 摘要卡片
    m_detailCard = new QFrame();
    m_detailCard->setObjectName("BoardingSummaryCard");
    m_detailCard->setStyleSheet("QFrame#BoardingSummaryCard { background: #fcfcfd; border: 1px solid #ebeef5; border-radius: 12px; } QLabel { border: none; background: transparent; }");
    QVBoxLayout *cardLayout = new QVBoxLayout(m_detailCard);
    cardLayout->setContentsMargins(15, 15, 15, 15);
    cardLayout->setSpacing(12);

    m_weightInVal = new QLabel("--");
    m_weightOutVal = new QLabel("--");
    m_dateInVal = new QLabel("--");
    m_dateOutVal = new QLabel("--");
    m_durationVal = new QLabel("--");
    m_dateOutTitle = new QLabel("离店时间");

    auto styleVal = [](QLabel *l, const QString &c) { l->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold; background: transparent;").arg(c)); };
    styleVal(m_weightInVal, "#e6a23c");
    styleVal(m_weightOutVal, "#909399");
    styleVal(m_dateInVal, "#303133");
    styleVal(m_dateOutVal, "#303133");
    styleVal(m_durationVal, "#f56c6c");
    m_dateOutTitle->setStyleSheet("color: #909399; font-size: 13px; background: transparent;");

    auto addRow = [&](const QString &l1, QLabel *v1, const QString &l2, QLabel *v2, QLabel *customTitle2 = nullptr) {
        QHBoxLayout *r = new QHBoxLayout();
        auto makePart = [&](const QString &l, QLabel *v, QLabel *ct = nullptr) {
            QVBoxLayout *c = new QVBoxLayout(); c->setSpacing(2);
            QLabel *title = ct ? ct : new QLabel(l);
            title->setStyleSheet("color: #909399; font-size: 13px; background: transparent;");
            c->addWidget(title); c->addWidget(v);
            return c;
        };
        r->addLayout(makePart(l1, v1), 1);
        r->addLayout(makePart(l2, v2, customTitle2), 1);
        cardLayout->addLayout(r);
    };

    addRow(QString::fromUtf8("入住体重"), m_weightInVal, QString::fromUtf8("离店体重"), m_weightOutVal);
    addRow(QString::fromUtf8("入住时间"), m_dateInVal, "", m_dateOutVal, m_dateOutTitle);
    cardLayout->setSpacing(16);
    QLabel *emptyPlaceholder = new QLabel();
    addRow(QString::fromUtf8("入住天数"), m_durationVal, "", emptyPlaceholder, nullptr);

    contentLayout->addWidget(m_detailCard);

    // 2. 动态时间轴
    QHBoxLayout *timelineHeader = new QHBoxLayout();
    QLabel *title = new QLabel("寄养动态");
    title->setStyleSheet("font-weight: bold; color: #303133; font-size: 16px;");
    
    m_periodBtn = new QPushButton(QString::fromUtf8("当前入住批次"));
    m_periodBtn->setCursor(Qt::PointingHandCursor);
    m_periodBtn->setStyleSheet("QPushButton { background: #f0f7ff; color: #409eff; border: none; border-radius: 12px; padding: 4px 15px; font-size: 13px; font-weight: bold; } QPushButton:hover { background: #e1f0ff; }");
    
    timelineHeader->addWidget(title);
    timelineHeader->addStretch();
    timelineHeader->addWidget(m_periodBtn);
    contentLayout->addLayout(timelineHeader);

    // 恢复批次切换菜单逻辑
    connect(m_periodBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentBatches.isEmpty()) return;
        
        QMenu *menu = new QMenu(this);
        menu->setStyleSheet("QMenu { background: white; border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px; } QMenu::item { padding: 5px 20px; border-radius: 2px; } QMenu::item:selected { background: #f0f7ff; color: #409eff; }");
        
        for (const auto &b : m_currentBatches) {
            QString labelText = QString("%1 ~ %2 (%3)").arg(b.startTime, b.isActive ? "至今" : b.endTime, b.isActive ? "当前" : "历史");
            QAction *act = menu->addAction(labelText);
            if (b.isActive) { QFont f = act->font(); f.setBold(true); act->setFont(f); }
            
            connect(act, &QAction::triggered, this, [this, b]() {
                // 更新摘要看板
                m_dateInVal->setText(b.startTime);
                QDate s = QDate::fromString(b.startTime, "yyyy-MM-dd");
                QDate e = b.isActive ? QDate::currentDate() : QDate::fromString(b.endTime, "yyyy-MM-dd");
                int d = s.isValid() && e.isValid() ? s.daysTo(e) + 1 : 1;
                m_durationVal->setText(QString("%1 天").arg(d));
                
                if (b.isActive) {
                    m_dateOutTitle->setText("预计离店时间");
                    m_dateOutVal->setText(m_currentPet.fosterEndTime.isEmpty() ? "尚未设定" : m_currentPet.fosterEndTime);
                    m_weightInVal->setText(QString("%1 kg").arg(m_currentPet.weight, 0, 'f', 1));
                    m_weightOutVal->setText("尚未结算");
                    m_weightOutVal->setStyleSheet("color: #909399; font-size: 13px; font-weight: bold;");
                } else {
                    m_dateOutTitle->setText(QString::fromUtf8("离店时间"));
                    m_dateOutVal->setText(b.endTime);
                    m_weightInVal->setText(QString::fromUtf8("20.5 kg")); 
                    m_weightOutVal->setText(QString::fromUtf8("21.2 kg"));
                    m_weightOutVal->setStyleSheet("color: #67c23a; font-size: 13px; font-weight: bold;");
                }
                m_periodBtn->setText(QString("%1 ~ %2").arg(b.startTime, b.isActive ? QString::fromUtf8("至今") : b.endTime));
                
                // 切换批次时同步更新选中时间
                m_selectedStartDate = b.startTime;
                m_selectedEndDate = b.isActive ? QString::fromUtf8("至今") : b.endTime;

                // 切换时间轴
                if (b.isActive) {
                    m_timelineWidget->setLogs(PetDataManager::instance()->getLogs(m_currentPet.id));
                } else {
                    QList<PetActivityLog> hisLogs;
                    hisLogs << PetActivityLog{b.startTime + " 08:30", QString::fromUtf8("入住"), QString::fromUtf8("系统记录：办理入住成功"), "🏠", false, QString::fromUtf8("系统"), m_currentPet.roomNo};
                    hisLogs << PetActivityLog{b.endTime + " 17:00", QString::fromUtf8("离店"), QString::fromUtf8("系统记录：结算离店完成"), "🏁", false, QString::fromUtf8("系统"), m_currentPet.roomNo};
                    m_timelineWidget->setLogs(hisLogs);
                }
            });
        }
        menu->exec(m_periodBtn->mapToGlobal(QPoint(0, m_periodBtn->height())));
        menu->deleteLater();
    });

    contentLayout->addWidget(m_timelineWidget, 1);

    // 在寄养页也增加一个影像入口，方便切换批次后直接查看
    QPushButton *archiveBtn2 = new QPushButton(QString::fromUtf8("查看该批次影像档案"));
    archiveBtn2->setFixedHeight(45);
    archiveBtn2->setCursor(Qt::PointingHandCursor);
    archiveBtn2->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #e6a23c; font-weight: bold; font-size: 14px; } QPushButton:hover { border-color: #e6a23c; background: #fcf6ec; }");
    connect(archiveBtn2, &QPushButton::clicked, this, [this]() {
        PetMediaArchiveDialog dlg(m_currentPet.id, m_currentPet.name, m_currentMedia, this->window(), m_selectedStartDate, m_selectedEndDate);
        dlg.exec();
    });
    contentLayout->addWidget(archiveBtn2);

    scroll->setWidget(content);
    m_boardingStack->addWidget(scroll);

    // 2. 空状态视图 (Index 1)
    QWidget *emptyWidget = new QWidget();
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyWidget);
    QLabel *emptyLabel = new QLabel(QString::fromUtf8("暂无相关寄养记录"));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #909399; font-size: 14px;");
    emptyLayout->addWidget(emptyLabel);
    m_boardingStack->addWidget(emptyWidget);

    return m_boardingStack;
}
