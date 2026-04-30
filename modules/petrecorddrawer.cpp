#include "petrecorddrawer.h"
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
#include <QGraphicsDropShadowEffect>

PetRecordDrawer::PetRecordDrawer(QWidget *parent) : QWidget(parent), m_isOpened(false)
{
    m_timelineWidget = new PetTimelineWidget(this);
    setupUI();
    
    connect(m_timelineWidget->delegate(), &TimelineDelegate::editRequested, this, &PetRecordDrawer::onEditLog);
    connect(m_timelineWidget->delegate(), &TimelineDelegate::deleteRequested, this, &PetRecordDrawer::onDeleteLog);

    setFixedWidth(0);
    m_animation = new QPropertyAnimation(this, "sideWidth");
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
}

void PetRecordDrawer::setupUI()
{
    setObjectName("PetRecordDrawer");
    setStyleSheet("#PetRecordDrawer { background-color: white; border-left: 1px solid #ebeef5; } "
                  "QLabel { border: none; background: transparent; }");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- 1. 顶部：宠物名片 ---
    QWidget *petHeader = new QWidget();
    petHeader->setFixedHeight(180);
    petHeader->setStyleSheet("background: white; border: none;");
    QVBoxLayout *headerTop = new QVBoxLayout(petHeader);
    headerTop->setContentsMargins(20, 10, 20, 10);

    // 关闭按钮
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addStretch();
    QPushButton *closeBtn = new QPushButton("×");
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #909399; background: transparent; } QPushButton:hover { color: #f56c6c; }");
    connect(closeBtn, &QPushButton::clicked, this, &PetRecordDrawer::closeRequested);
    topBar->addWidget(closeBtn);
    headerTop->addLayout(topBar);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(100, 100);
    m_avatarLabel->setStyleSheet("border-radius: 50px; background: #f0f2f5; border: none;");
    m_avatarLabel->installEventFilter(this);
    m_avatarLabel->setCursor(Qt::PointingHandCursor);

    QVBoxLayout *nameCol = new QVBoxLayout();
    nameCol->setSpacing(2);
    m_nameLabel = new QLabel("尚未选择");
    m_nameLabel->setStyleSheet("font-size: 20px; font-weight: 800; color: #303133; border: none; background: transparent;");
    
    m_breedLabel = new QLabel("--");
    m_breedLabel->setStyleSheet("font-size: 15px; color: #333333; font-weight: 600; border: none; background: transparent;");
    
    m_ownerLabel = new QLabel();
    m_ownerLabel->setStyleSheet("color: #606266; font-size: 13px;");
    
    nameCol->addWidget(m_nameLabel);
    nameCol->addWidget(m_breedLabel);
    nameCol->addWidget(m_ownerLabel);

    m_roomBadge = new QLabel();
    m_roomBadge->hide(); // 默认隐藏
    m_roomBadge->setStyleSheet(
        "background: #ecf5ff; color: #409eff; border: 1px solid #d9ecff; "
        "border-radius: 4px; padding: 4px 10px; font-weight: bold; font-size: 13px;"
    );
    
    headerLayout->addWidget(m_avatarLabel);
    headerLayout->addSpacing(15);
    headerLayout->addLayout(nameCol);
    headerLayout->addStretch();
    headerLayout->addWidget(m_roomBadge, 0, Qt::AlignTop | Qt::AlignRight);
    
    headerTop->addLayout(headerLayout);
    headerTop->addStretch(); // 向上顶，防止切边

    mainLayout->addWidget(petHeader);

    // --- 2. 摘要卡片 (detailCard) ---
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; } ");
    
    QWidget *content = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(20);

    m_detailCard = new QFrame();
    m_detailCard->setStyleSheet("background: #fcfcfd; border: none; border-radius: 12px;");
    QVBoxLayout *cardLayout = new QVBoxLayout(m_detailCard);
    cardLayout->setContentsMargins(15, 15, 15, 15);
    cardLayout->setSpacing(12);

    m_weightInVal = new QLabel("--");
    m_weightOutVal = new QLabel("--");
    m_dateInVal = new QLabel("--");
    m_dateOutVal = new QLabel("--");
    m_durationVal = new QLabel("--");
    m_dateOutTitle = new QLabel("离店时间");

    auto styleVal = [](QLabel *l, const QString &c) { l->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold; border: none; background: transparent;").arg(c)); };
    styleVal(m_weightInVal, "#e6a23c");
    styleVal(m_weightOutVal, "#909399");
    styleVal(m_dateInVal, "#303133");
    styleVal(m_dateOutVal, "#303133");
    styleVal(m_durationVal, "#f56c6c");

    auto addRow = [&](const QString &l1, QLabel *v1, const QString &l2, QLabel *v2, QLabel *customTitle2 = nullptr) {
        QHBoxLayout *r = new QHBoxLayout();
        auto makePart = [&](const QString &l, QLabel *v, QLabel *ct = nullptr) {
            QVBoxLayout *c = new QVBoxLayout(); c->setSpacing(2);
            QLabel *title = ct ? ct : new QLabel(l);
            title->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
            c->addWidget(title); c->addWidget(v);
            return c;
        };
        r->addLayout(makePart(l1, v1), 1);
        r->addLayout(makePart(l2, v2, customTitle2), 1);
        cardLayout->addLayout(r);
    };

    addRow("入住体重", m_weightInVal, "离店体重", m_weightOutVal);
    QFrame *line = new QFrame(); line->setFixedHeight(1); line->setStyleSheet("background: #f0f2f5;"); cardLayout->addWidget(line);
    addRow("入住时间", m_dateInVal, "", m_dateOutVal, m_dateOutTitle);
    QLabel *emptyPlaceholder = new QLabel();
    emptyPlaceholder->setStyleSheet("border: none; background: transparent;");
    addRow("入住天数", m_durationVal, "", emptyPlaceholder, nullptr);

    contentLayout->addWidget(m_detailCard);

    // --- 3. 动态时间轴 ---
    QHBoxLayout *timelineHeader = new QHBoxLayout();
    QLabel *title = new QLabel("寄养动态");
    title->setStyleSheet("font-weight: bold; color: #303133; font-size: 16px;");
    
    m_periodBtn = new QPushButton("当前入住批次");
    m_periodBtn->setCursor(Qt::PointingHandCursor);
    m_periodBtn->setStyleSheet("QPushButton { background: #f0f7ff; color: #409eff; border: none; border-radius: 12px; padding: 4px 15px; font-size: 13px; font-weight: bold; } QPushButton:hover { background: #e1f0ff; }");
    
    timelineHeader->addWidget(title);
    timelineHeader->addStretch();
    timelineHeader->addWidget(m_periodBtn);
    timelineHeader->addSpacing(20); // 向左偏移，避免紧贴右侧
    contentLayout->addLayout(timelineHeader);

    // 在 setupUI 中持久化连接，动态构建菜单
    connect(m_periodBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentBatches.isEmpty()) return;
        
        QMenu *menu = new QMenu(this);
        menu->setStyleSheet("QMenu { background: white; border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px; } QMenu::item { padding: 5px 20px; border-radius: 2px; } QMenu::item:selected { background: #f0f7ff; color: #409eff; }");
        
        for (const auto &b : m_currentBatches) {
            QString labelText = QString("%1 ~ %2 (%3)").arg(b.startTime, b.isActive ? "至今" : b.endTime, b.isActive ? "当前" : "历史");
            QAction *act = menu->addAction(labelText);
            if (b.isActive) { QFont f = act->font(); f.setBold(true); act->setFont(f); }
            
            connect(act, &QAction::triggered, this, [this, b]() {
                // 更新摘要卡片
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
                    m_weightOutVal->setStyleSheet("color: #909399; font-size: 13px; font-weight: bold; border: none; background: transparent;");
                } else {
                    m_dateOutTitle->setText("离店时间");
                    m_dateOutVal->setText(b.endTime);
                    m_weightInVal->setText("20.5 kg"); 
                    m_weightOutVal->setText("21.2 kg");
                    m_weightOutVal->setStyleSheet("color: #67c23a; font-size: 13px; font-weight: bold; border: none; background: transparent;");
                }
                m_periodBtn->setText(QString("%1 ~ %2").arg(b.startTime, b.isActive ? "至今" : b.endTime));
                
                // 切换时间轴
                if (b.isActive) {
                    m_timelineWidget->setLogs(PetDataManager::instance()->getLogs(m_currentPet.id));
                } else {
                    QList<PetActivityLog> hisLogs;
                    hisLogs << PetActivityLog{b.startTime + " 08:30", "入住", "系统记录：办理入住成功", "", false, "系统", m_currentPet.roomNo};
                    hisLogs << PetActivityLog{b.endTime + " 17:00", "离店", "系统记录：结算离店完成", "", false, "系统", m_currentPet.roomNo};
                    m_timelineWidget->setLogs(hisLogs);
                }
            });
        }
        menu->exec(m_periodBtn->mapToGlobal(QPoint(0, m_periodBtn->height())));
        menu->deleteLater();
    });

    contentLayout->addWidget(m_timelineWidget, 1);

    // --- 4. 底部操作栏 ---
    m_archiveBtn = new QPushButton("影像分类档案");
    m_archiveBtn->setFixedHeight(45);
    m_archiveBtn->setCursor(Qt::PointingHandCursor);
    m_archiveBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #e6a23c; font-weight: bold; font-size: 14px; } QPushButton:hover { border-color: #e6a23c; background: #fcf6ec; }");
    connect(m_archiveBtn, &QPushButton::clicked, this, [this]() {
        PetMediaArchiveDialog dlg(m_currentPet.name, m_currentMedia, this->window());
        dlg.exec();
    });
    contentLayout->addWidget(m_archiveBtn);

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);
    
    m_bottomStack = new QStackedWidget(); 
    m_bottomStack->hide();
}

void PetRecordDrawer::setPet(const PetInfo &info, const QList<PetActivityLog> &logs, const QList<PetMedia> &media, const QList<FosterBatch> &batches)
{
    m_currentPet = info;
    m_allLogs = logs;
    m_currentMedia = media;
    m_currentBatches = batches; // 核心：持久化当前宠物的批次数据
    
    m_nameLabel->setText(info.name);
    
    // 品种与性别富文本展示 (性别符号放大到 18px)
    QString genderSymbol = (info.gender == "公" || info.gender == "雄" || info.gender == "M") ? "♂" : "♀";
    QString genderColor = (genderSymbol == "♂") ? "#409EFF" : "#F56C6C";
    m_breedLabel->setText(QString("%1  <span style='color:%2; font-size:18px; font-weight:bold;'>%3</span>")
                         .arg(info.breed).arg(genderColor).arg(genderSymbol));
    
    m_ownerLabel->setText(QString("主人：%1 | ID：%2").arg(info.ownerName).arg(info.ownerId));

    // 动态显示房间号
    if (info.status == "在店 (寄养中)") {
        // 模拟分配一个房间号，后续可从数据管理器获取真实关联
        m_roomBadge->setText("房间：B-102");
        m_roomBadge->show();
    } else {
        m_roomBadge->hide();
    }
    
    // Avatar 高清渲染
    QPixmap pixmap(info.avatarPath);
    if (pixmap.isNull()) pixmap.load(":/images/load_img.jpg");
    QSize size(100, 100);
    QPixmap target(size);
    target.fill(Qt::transparent);
    QPainter p(&target);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, size.width(), size.height());
    p.setClipPath(path);
    p.drawPixmap(0, 0, pixmap.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    m_avatarLabel->setPixmap(target);

    // 辅助函数：更新摘要看板
    auto updateSummary = [=](const FosterBatch &b) {
        m_dateInVal->setText(b.startTime);
        QDate s = QDate::fromString(b.startTime, "yyyy-MM-dd");
        QDate e = b.isActive ? QDate::currentDate() : QDate::fromString(b.endTime, "yyyy-MM-dd");
        int d = s.isValid() && e.isValid() ? s.daysTo(e) + 1 : 1;
        m_durationVal->setText(QString("%1 天").arg(d));
        
        if (b.isActive) {
            m_dateOutTitle->setText("🚪 预计离店时间");
            m_dateOutVal->setText(info.fosterEndTime.isEmpty() ? "尚未设定" : info.fosterEndTime);
            m_weightInVal->setText(QString("%1 kg").arg(info.weight, 0, 'f', 1));
            m_weightOutVal->setText("尚未结算");
            m_weightOutVal->setStyleSheet("color: #909399; font-size: 13px; font-weight: bold; border: none; background: transparent;");
        } else {
            m_dateOutTitle->setText("🚪 离店时间");
            m_dateOutVal->setText(b.endTime);
            m_weightInVal->setText("20.5 kg"); 
            m_weightOutVal->setText("21.2 kg");
            m_weightOutVal->setStyleSheet("color: #67c23a; font-size: 13px; font-weight: bold; border: none; background: transparent;");
        }
        m_periodBtn->setText(QString("%1 ~ %2").arg(b.startTime, b.isActive ? "至今" : b.endTime));
    };

    // 默认显示最新批次
    if (!m_currentBatches.isEmpty()) {
        updateSummary(m_currentBatches.first());
        m_timelineWidget->setLogs(logs);
    } else {
        updateSummary(FosterBatch{"B-CUR", info.fosterStartTime, "至今", true});
        m_timelineWidget->setLogs(logs);
    }

    updateEmptyState();
}

void PetRecordDrawer::updateEmptyState() {}
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

bool PetRecordDrawer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avatarLabel && event->type() == QEvent::MouseButtonRelease) {
        emit avatarClicked(m_currentPet.avatarPath);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}
