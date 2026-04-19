#include "petrecorddrawer.h"
#include "custommessagedialog.h"
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QIcon>

PetRecordDrawer::PetRecordDrawer(QWidget *parent) : QWidget(parent), m_isOpened(false)
{
    m_timelineModel = new PetTimelineModel(this);
    m_timelineDelegate = new TimelineDelegate(this);

    setupUI();
    
    // 连接时间轴委派器的修改/删除信号
    connect(m_timelineDelegate, &TimelineDelegate::editRequested, this, &PetRecordDrawer::onEditLog);
    connect(m_timelineDelegate, &TimelineDelegate::deleteRequested, this, &PetRecordDrawer::onDeleteLog);

    setFixedWidth(0);
    
    m_animation = new QPropertyAnimation(this, "sideWidth");
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
}

void PetRecordDrawer::setupUI()
{
    setObjectName("PetRecordDrawer");
    setStyleSheet("#PetRecordDrawer { background-color: white; border-left: 1px solid #ebeef5; } ");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- 0. 宠物信息头 (Pet Information Header - RESTORED) ---
    QWidget *petHeader = new QWidget();
    petHeader->setFixedHeight(190); // 进一步增加高度 (160->190) 以适应新增的房号 Badge
    petHeader->setStyleSheet("background: white; border-bottom: 1px solid #f0f2f5;");
    QVBoxLayout *petHeaderLayout = new QVBoxLayout(petHeader);
    petHeaderLayout->setContentsMargins(20, 10, 20, 15);
    
    // 关闭按钮行
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->setContentsMargins(0, 0, 0, 0);
    topBar->addStretch();
    QPushButton *closeBtn = new QPushButton("×");
    closeBtn->setFixedSize(30, 30);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #909399; background: transparent; } "
                            "QPushButton:hover { color: #f56c6c; }");
    connect(closeBtn, &QPushButton::clicked, this, &PetRecordDrawer::closeRequested);
    topBar->addWidget(closeBtn);
    petHeaderLayout->addLayout(topBar);

    // 头像与基本信息
    QHBoxLayout *infoLayout = new QHBoxLayout();
    infoLayout->setContentsMargins(0, 5, 0, 5); 
    infoLayout->setAlignment(Qt::AlignTop); // 核心修复：顶对齐，防止空间由于文本增加而互相挤压
    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(90, 90); // 放大头像至 90px
    m_avatarLabel->setStyleSheet("border-radius: 45px; background: #ebeef5; cursor: pointer;");
    m_avatarLabel->setCursor(Qt::PointingHandCursor);
    m_avatarLabel->installEventFilter(this); // 安装过滤器以响应点击
    
    QVBoxLayout *nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(5); // 增加间距
    m_nameLabel = new QLabel("尚未选择");
    m_nameLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #303133; border: none;"); // 增加 border: none
    m_breedLabel = new QLabel("--");
    m_breedLabel->setStyleSheet("font-size: 16px; color: #909399; border: none;"); // 增加 border: none
    m_ownerLabel = new QLabel("主人: --");
    m_ownerLabel->setStyleSheet("font-size: 14px; color: #b2bec3; border: none;"); // 增加 border: none
    
    m_statusBadge = new QLabel();
    m_statusBadge->setFixedWidth(120);
    m_statusBadge->setFixedHeight(22);
    m_statusBadge->setAlignment(Qt::AlignCenter);
    m_statusBadge->setStyleSheet(
        "QLabel { "
        "background-color: #f0f9eb; "
        "color: #67c23a; "
        "border: 1px solid #e1f3d8; "
        "border-radius: 4px; "
        "font-size: 12px; "
        "font-weight: bold; "
        "} "
    );
    m_statusBadge->hide(); // 默认隐藏
    
    nameLayout->addWidget(m_nameLabel);
    nameLayout->addWidget(m_breedLabel);
    nameLayout->addWidget(m_statusBadge);
    nameLayout->addSpacing(5);
    nameLayout->addWidget(m_ownerLabel);

    infoLayout->addWidget(m_avatarLabel);
    infoLayout->addSpacing(20); // 增加头像与文字间距
    infoLayout->addLayout(nameLayout);
    infoLayout->addStretch();
    petHeaderLayout->addLayout(infoLayout);

    mainLayout->addWidget(petHeader);

    // --- 1. 动态状态头 (Dynamic Header) ---
    QWidget *dynamicHeader = new QWidget();
    dynamicHeader->setFixedHeight(50);
    dynamicHeader->setStyleSheet("background: #fcfdfe; border-bottom: 1px solid #f0f2f5;");
    QHBoxLayout *headerL = new QHBoxLayout(dynamicHeader);
    headerL->setContentsMargins(20, 0, 15, 0);

    m_statusDateLabel = new QLabel(QDate::currentDate().toString("yyyy-MM-dd"));
    m_statusDateLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ff9f43;"); // 统一琥珀橙强调
    
    m_toggleCalendarBtn = new QPushButton("点击展开日历");
    m_toggleCalendarBtn->setCursor(Qt::PointingHandCursor);
    m_toggleCalendarBtn->setFixedWidth(100);
    m_toggleCalendarBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dcdfe6; border-radius: 4px; padding: 4px 8px; font-size: 12px; color: #606266; background: white; } "
        "QPushButton:hover { border-color: #409eff; color: #409eff; background: #f0f7ff; }"
    );
    connect(m_toggleCalendarBtn, &QPushButton::clicked, this, &PetRecordDrawer::onToggleCalendar);

    headerL->addWidget(m_statusDateLabel);
    headerL->addStretch();
    headerL->addWidget(m_toggleCalendarBtn);
    mainLayout->addWidget(dynamicHeader);

    // --- 2. 核心内容容器 (Content Wrapper - Use Grid for Overlay) ---
    QWidget *contentWrapper = new QWidget();
    QGridLayout *contentGrid = new QGridLayout(contentWrapper);
    contentGrid->setContentsMargins(0, 0, 0, 0);
    contentGrid->setSpacing(0);

    // [A] 下层：核心展示区 (Content Body)
    m_bodyArea = new QWidget();
    QVBoxLayout *bodyAreaLayout = new QVBoxLayout(m_bodyArea);
    bodyAreaLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *overlayWidget = new QWidget();
    QGridLayout *overlayLayout = new QGridLayout(overlayWidget);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setSpacing(0);

    // 背景图：空状态
    m_emptyPlaceholder = new QWidget();
    QVBoxLayout *emptyL = new QVBoxLayout(m_emptyPlaceholder);
    emptyL->setAlignment(Qt::AlignCenter);
    
    QLabel *emptyIcon = new QLabel("🐾");
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyIcon->setStyleSheet("font-size: 40px; color: #dcdfe6;");
    
    QLabel *emptyText = new QLabel("暂无今日表现记录\n点击下方提交按钮开始记录吧！");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyText->setStyleSheet("color: #909399; font-size: 12px; margin-top: 8px; font-family: 'Microsoft YaHei';");
    
    emptyL->addWidget(emptyIcon);
    emptyL->addWidget(emptyText);

    // 前端列表
    m_timelineView = new QListView();
    m_timelineView->setModel(m_timelineModel);
    m_timelineView->setItemDelegate(m_timelineDelegate);
    m_timelineView->setResizeMode(QListView::Adjust); // 关键修复：当视图宽度变化时自动重新计算项布局
    m_timelineView->setSelectionMode(QAbstractItemView::NoSelection);
    m_timelineView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_timelineView->setStyleSheet("QListView { border: none; background: transparent; } ");
    
    overlayLayout->addWidget(m_emptyPlaceholder, 0, 0);
    overlayLayout->addWidget(m_timelineView, 0, 0);
    
    bodyAreaLayout->addWidget(overlayWidget);
    contentGrid->addWidget(m_bodyArea, 0, 0);

    // [B] 上层：联动日历区 (Calendar View - Overlay)
    m_calendarContainer = new QWidget();
    m_calendarContainer->setMaximumHeight(0); // 默认收起
    m_calendarContainer->setContentsMargins(0, 0, 0, 0);
    // 给日历增加一个白色背景和底边阴影，使其看起来像悬浮层
    m_calendarContainer->setStyleSheet("background-color: white; border-bottom: 1px solid #f0f2f5;");
    
    QVBoxLayout *calCollapsibleL = new QVBoxLayout(m_calendarContainer);
    calCollapsibleL->setContentsMargins(0, 0, 0, 0);
    
    m_calendar = new CompactCalendar();
    m_calendar->setFixedHeight(260); // 增加高度以容纳高级导航栏并防止文字截断
    connect(m_calendar, &QCalendarWidget::clicked, this, &PetRecordDrawer::onCalendarClicked);
    calCollapsibleL->addWidget(m_calendar);
    
    // 将日历添加到 Grid 的同一位置，并顶对齐
    contentGrid->addWidget(m_calendarContainer, 0, 0, Qt::AlignTop);

    mainLayout->addWidget(contentWrapper, 1);
    
    m_calendarAnimation = new QPropertyAnimation(m_calendarContainer, "maximumHeight");
    m_calendarAnimation->setDuration(300);
    m_calendarAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // --- 4. 操作录入区 (Input Bar) ---
    m_bottomStack = new QStackedWidget();
    m_bottomStack->setFixedHeight(220); // 进一步增加总高度以适配大输入框

    m_recordPanel = new QWidget();
    m_recordPanel->setStyleSheet("background-color: white; border-top: 1px solid #f0f2f5;");
    QVBoxLayout *bottomLayout = new QVBoxLayout(m_recordPanel);
    bottomLayout->setContentsMargins(20, 15, 20, 15);
    bottomLayout->setSpacing(15);

    QHBoxLayout *inputRow = new QHBoxLayout();
    inputRow->setSpacing(8); // 设置统一的小间距，使整体更紧凑
    
    QLabel *typeTypeL = new QLabel("类型:");
    typeTypeL->setStyleSheet("color: #909399; font-size: 13px;");
    
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"投喂", "洗护", "检查", "备注", "异常"});
    m_typeCombo->setFixedWidth(75); // 缩减类型选择框宽度
    m_typeCombo->setFixedHeight(32);

    QLabel *opTypeL = new QLabel("经办人:");
    opTypeL->setStyleSheet("color: #909399; font-size: 13px;"); // 取消 margin-left: 10px;

    m_operatorCombo = new QComboBox();
    m_operatorCombo->addItems({"店员小利", "王波", "张师傅", "店长"});
    m_operatorCombo->setFixedWidth(105); // 增加经办人选择框宽度
    m_operatorCombo->setFixedHeight(32);
    
    QString comboStyle = "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 0 5px; background: white; font-size: 13px; color: #606266; } "
                         "QComboBox::drop-down { border: none; width: 20px; } "
                         "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 10px; height: 10px; } ";
    m_typeCombo->setStyleSheet(comboStyle);
    m_operatorCombo->setStyleSheet(comboStyle);

    m_remarkEdit = new QTextEdit();
    m_remarkEdit->setPlaceholderText("请输入今日记录内容...");
    m_remarkEdit->setFixedHeight(130); // 从 85 增加到 130，提供更广阔的输入面积
    m_remarkEdit->setStyleSheet("QTextEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 8px; font-size: 13px; }");

    inputRow->addWidget(typeTypeL);
    inputRow->addWidget(m_typeCombo);
    inputRow->addWidget(opTypeL);
    inputRow->addWidget(m_operatorCombo);
    inputRow->addStretch(); // 让选择框靠左排列

    m_addBtn = new QPushButton("提交记录");
    m_addBtn->setFixedHeight(36);
    m_addBtn->setCursor(Qt::PointingHandCursor);
    m_addBtn->setEnabled(false);
    // 照搬 MemberModule 的新增按钮样式
    m_addBtn->setStyleSheet(
        "QPushButton { background: #67c23a; color: white; padding: 0 15px; border-radius: 4px; font-size: 14px; font-weight: bold; border: none; } "
        "QPushButton:hover { background: #85ce61; } "
        "QPushButton:disabled { background: #f5f7fa; color: #c0c4cc; border: 1px solid #e4e7ed; }"
    );
    connect(m_addBtn, &QPushButton::clicked, this, &PetRecordDrawer::onSubmitQuickAction);
    connect(m_remarkEdit, &QTextEdit::textChanged, this, [this](){
        m_addBtn->setEnabled(!m_remarkEdit->toPlainText().trimmed().isEmpty());
    });

    bottomLayout->addLayout(inputRow);
    bottomLayout->addWidget(m_remarkEdit); // 放到中间
    bottomLayout->addWidget(m_addBtn);

    m_tipPanel = new QWidget();
    m_tipPanel->setStyleSheet("background-color: #f8f9fb; border-top: 1px solid #f0f2f5;");
    QVBoxLayout *tipLayout = new QVBoxLayout(m_tipPanel);
    m_tipLabel = new QLabel("当前状态非寄养中，无法记录今日表现。");
    m_tipLabel->setAlignment(Qt::AlignCenter);
    m_tipLabel->setStyleSheet("color: #909399; font-size: 13px; font-family: 'Microsoft YaHei';");
    tipLayout->addWidget(m_tipLabel);

    m_bottomStack->addWidget(m_recordPanel); // index 0
    m_bottomStack->addWidget(m_tipPanel);    // index 1
    mainLayout->addWidget(m_bottomStack);
}

void PetRecordDrawer::setPet(const PetInfo &info, const QList<PetActivityLog> &logs, const QList<FosterBatch> &batches)
{
    Q_UNUSED(batches);
    m_currentPet = info;
    m_allLogs = logs;
    
    // 1. Restore Pet Info UI
    m_nameLabel->setText(info.name);
    m_breedLabel->setText(QString("%1 · %2").arg(info.breed, info.age));
    m_ownerLabel->setText(QString("主人: %1 (%2)").arg(info.ownerName, info.ownerId));
    
    // 即时状态：寄养房号 Badge
    if (info.status == "寄养中") {
        QString room = info.roomNo.isEmpty() ? "未分配" : info.roomNo;
        m_statusBadge->setText(QString("寄养中：%1 房").arg(room));
        m_statusBadge->show();
    } else {
        m_statusBadge->hide();
    }
    
    // Avatar (放大到 90px)
    QPixmap pixmap(info.avatarPath);
    if (pixmap.isNull()) pixmap.load(":/images/load_img.jpg");
    QSize avatarSize(90, 90);
    QPixmap target(avatarSize);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, avatarSize.width(), avatarSize.height());
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, avatarSize.width(), avatarSize.height(), 
                       pixmap.scaled(avatarSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    m_avatarLabel->setPixmap(target);

    // 2. Update Header Date
    m_statusDateLabel->setText(QDate::currentDate().toString("yyyy-MM-dd"));

    // 3. Timeline Data
    m_timelineModel->setLogs(logs);
    
    // 4. Calendar logic
    QDate fosterStart = QDate::fromString(info.joinTime, "yyyy-MM-dd");
    m_calendar->setFosterRange(fosterStart, QDate()); 
    m_calendar->setSelectedDate(QDate::currentDate());

    // 5. 更新日历状态点 (三态映射)
    QSet<QDate> done, doing, danger;
    for (const auto &log : logs) {
        QDate d = QDateTime::fromString(log.time, "yyyy-MM-dd HH:mm").date();
        if (log.isAlert) danger.insert(d);
        else done.insert(d);
    }
    // 注：目前尚未接入“预约记录”，doing 暂时留空
    m_calendar->setStatusData(done, doing, danger);

    updateEmptyState();
    updateBatchStatus(info.status);

    // 关键修复：确保切换宠物后，时间轴视图强制回到顶部，避免停留在上一个宠物的旧偏移位置
    if (m_timelineModel->rowCount() > 0) {
        m_timelineView->scrollTo(m_timelineModel->index(0), QAbstractItemView::PositionAtTop);
    }
}

void PetRecordDrawer::updateEmptyState()
{
    bool hasRows = m_timelineModel->rowCount() > 0;
    m_timelineView->setVisible(hasRows);
    m_emptyPlaceholder->setVisible(!hasRows);
}

void PetRecordDrawer::updateBatchStatus(const QString &status)
{
    m_bottomStack->setVisible(true);

    if (status == "寄养中") {
        m_bottomStack->setCurrentIndex(0);
        m_addBtn->setText("提交今日记录");
    } else {
        m_bottomStack->setCurrentIndex(1);
        if (status == "离店") {
            m_tipLabel->setText("宠物已离店，无法记录表现记录。");
        } else {
            m_tipLabel->setText(QString("当前处于%1状态，暂无法记录表现。").arg(status));
        }
    }
}

void PetRecordDrawer::addLogItem(const PetActivityLog &log)
{
    m_allLogs.append(log);
    m_timelineModel->setLogs(m_allLogs);
    updateEmptyState();
    
    // 自动滚动到底部以查看最新添加的记录
    m_timelineView->scrollTo(m_timelineModel->index(m_timelineModel->rowCount() - 1), QAbstractItemView::PositionAtBottom);
    
    // Update Calendar (三态映射)
    QSet<QDate> done, doing, danger;
    for (const auto &l : m_allLogs) {
        QDate dl = QDateTime::fromString(l.time, "yyyy-MM-dd HH:mm").date();
        if (l.isAlert) danger.insert(dl);
        else done.insert(dl);
    }
    m_calendar->setStatusData(done, doing, danger);
}

void PetRecordDrawer::onCalendarClicked(const QDate &date)
{
    m_statusDateLabel->setText(date.toString("yyyy-MM-dd"));
    
    QString dateStr = date.toString("yyyy-MM-dd");
    int row = m_timelineModel->findFirstIndexForDate(dateStr);
    if (row != -1) {
        m_timelineView->scrollTo(m_timelineModel->index(row), QAbstractItemView::PositionAtTop);
        m_timelineModel->setHighlight(row + 1); 
    }
    
    // 选定日期后自动折叠日历
    onToggleCalendar();
}

void PetRecordDrawer::onSubmitQuickAction()
{
    QString type = m_typeCombo->currentText();
    QString remark = m_remarkEdit->toPlainText().trimmed();
    
    if (remark.isEmpty()) {
        remark = QString("执行了%1操作").arg(type);
    }
    
    QString icon = "📝";
    if (type == "投喂") icon = "🍖";
    else if (type == "洗护") icon = "🛁";
    else if (type == "检查") icon = "🩺";
    else if (type == "异常") icon = "⚠️";

    PetActivityLog log;
    log.type = type;
    log.icon = icon;
    log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
    log.remark = remark;
    log.isAlert = (type == "异常");
    log.operatorName = m_operatorCombo->currentText(); // 从下拉框获取经办人
    log.roomNo = m_currentPet.roomNo; // 固化当时房号
    
    emit logAdded(m_currentPet.id, log);
    addLogItem(log);
    
    m_remarkEdit->clear(); // 清空内容
}

void PetRecordDrawer::onEditLog(const QModelIndex &index)
{
    // 修改功能：将内容回填至录入框
    QString type = index.data(PetTimelineModel::TypeRole).toString();
    QString content = index.data(PetTimelineModel::ContentRole).toString();
    QString op = index.data(PetTimelineModel::OperatorRole).toString();

    m_typeCombo->setCurrentText(type);
    m_operatorCombo->setCurrentText(op);
    m_remarkEdit->setPlainText(content);
    m_remarkEdit->setFocus();
}

void PetRecordDrawer::onDeleteLog(const QModelIndex &index)
{
    // 删除功能：弹出确认框
    if (CustomMessageDialog::confirm(this, "确认删除", "确定要永久移除这条表现记录吗？")) {
        int row = index.row();
        // 简单处理：如果 model 只有 log（没有 header），直接按 row 删除
        // 复杂处理：根据数据 ID 或时间戳匹配 m_allLogs 移除
        m_allLogs.removeAt(row); 
        m_timelineModel->setLogs(m_allLogs);
        updateEmptyState();
        
        // 同步更新日历状态点
        QSet<QDate> done, doing, danger;
        for (const auto &l : m_allLogs) {
            QDate dl = QDateTime::fromString(l.time, "yyyy-MM-dd HH:mm").date();
            if (l.isAlert) danger.insert(dl);
            else done.insert(dl);
        }
        m_calendar->setStatusData(done, doing, danger);
    }
}

void PetRecordDrawer::onBatchChanged(int index)
{
    Q_UNUSED(index);
}

void PetRecordDrawer::showDrawer()
{
    m_isOpened = true;
    m_timelineView->doItemsLayout(); // 展开前强制重新计算内饰布局，防止气泡宽度缓存错误
    m_animation->stop();
    m_animation->setStartValue(width());
    m_animation->setEndValue(330); // 适度增加宽度至 330px
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

void PetRecordDrawer::onToggleCalendar()
{
    bool isVisible = m_calendarContainer->maximumHeight() > 0;
    
    m_calendarAnimation->stop();
    if (isVisible) {
        m_calendarAnimation->setStartValue(m_calendarContainer->height());
        m_calendarAnimation->setEndValue(0);
        m_toggleCalendarBtn->setText("点击展开日历");
    } else {
        m_calendarAnimation->setStartValue(m_calendarContainer->height());
        m_calendarAnimation->setEndValue(260); // 同步增加展开高度
        m_toggleCalendarBtn->setText("点击收起日历");
    }
    m_calendarAnimation->start();
}
bool PetRecordDrawer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avatarLabel && event->type() == QEvent::MouseButtonRelease) {
        emit avatarClicked(m_currentPet.avatarPath);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}
