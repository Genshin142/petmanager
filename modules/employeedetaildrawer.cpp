#include "employeedetaildrawer.h"
#include "../utils/imageutils.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QEvent>
#include <QTimer>
#include <QSizePolicy>
#include <QComboBox>
#include <QGridLayout>
#include "scheduledatamanager.h"
#include "editattendancedialog.h"
#include <QTime>

EmployeeDetailDrawer::EmployeeDetailDrawer(QWidget *parent) : QWidget(parent), m_isOpened(true), m_selectedAttendDate(QDate::currentDate())
{
    setupUI();
    setFixedWidth(450);
    m_animation = new QPropertyAnimation(this, "sideWidth");
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    // 自动刷新：当排班数据更新时，自动刷新日历及考勤表
    connect(ScheduleDataManager::instance(), &ScheduleDataManager::scheduleChanged, this, &EmployeeDetailDrawer::refreshCalendar);
    connect(ScheduleDataManager::instance(), &ScheduleDataManager::scheduleChanged, this, &EmployeeDetailDrawer::refreshAttendCalendar);
}

void EmployeeDetailDrawer::setupUI()
{
    setObjectName("EmployeeDetailDrawer");
    setStyleSheet("#EmployeeDetailDrawer { background-color: white; border-left: 1px solid #ebeef5; } "
                  "QLabel { border: none; background: transparent; padding: 0; margin: 0; }");
    
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20); // 关键：与左侧卡片边距对齐
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
    QLabel *emptyIcon = new QLabel("👤");
    emptyIcon->setStyleSheet("font-size: 48px; color: #dcdfe6;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    QLabel *emptyText = new QLabel("暂无员工信息\n请在左侧列表选择或录入新员工");
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

    // --- 1. 顶部：员工名片 ---
    QWidget *header = new QWidget();
    header->setFixedHeight(200); 
    header->setStyleSheet("background: white; border-top-left-radius: 12px; border-top-right-radius: 12px; border: none;");
    QVBoxLayout *headerTop = new QVBoxLayout(header);
    headerTop->setContentsMargins(20, 15, 20, 0);

    // 关闭按钮
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addStretch();
    QPushButton *closeBtn = new QPushButton("×");
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #909399; background: transparent; } QPushButton:hover { color: #f56c6c; }");
    connect(closeBtn, &QPushButton::clicked, this, &EmployeeDetailDrawer::closeRequested);
    topBar->addWidget(closeBtn);
    headerTop->addLayout(topBar);

    QHBoxLayout *infoLayout = new QHBoxLayout();
    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(100, 100);
    m_avatarLabel->setStyleSheet("border-radius: 50px; background: #f0f2f5; border: none;");
    m_avatarLabel->installEventFilter(this);
    m_avatarLabel->setCursor(Qt::PointingHandCursor);

    QVBoxLayout *nameCol = new QVBoxLayout();
    nameCol->setSpacing(2);
    m_nameLabel = new QLabel("尚未选择");
    m_nameLabel->setStyleSheet("font-size: 20px; font-weight: 800; color: #303133;");
    
    m_roleLabel = new QLabel("--");
    m_roleLabel->setStyleSheet("font-size: 11px; color: #0369a1; font-weight: bold; background: #e0f2fe; border-radius: 12px; padding: 4px 12px;");
    m_roleLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    m_idLabel = new QLabel("工号: --");
    m_idLabel->setStyleSheet("font-size: 14px; color: #606266; font-weight: 500;");
    
    nameCol->addWidget(m_nameLabel);
    nameCol->addWidget(m_roleLabel);
    nameCol->addWidget(m_idLabel);
    
    infoLayout->addWidget(m_avatarLabel);
    infoLayout->addSpacing(15);
    infoLayout->addLayout(nameCol);
    infoLayout->addStretch();
    
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
    
    // 采用绝对定位固定位置 (297, 21)
    m_editBtn->setParent(container);
    m_editBtn->move(297, 21);
    m_editBtn->raise();
    
    // 监听按钮的 hover 状态来改变 Label 颜色（可选，但为了更好的体验）
    // 这里简单处理，如果不追求 hover 变色可以不写复杂逻辑
    
    connect(m_editBtn, &QPushButton::clicked, this, [this](){ emit editRequested(m_currentEmployee); });
    // infoLayout->addWidget(m_editBtn, 0, Qt::AlignTop | Qt::AlignRight);

    headerTop->addLayout(infoLayout);
    headerTop->addSpacing(15); 
    
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

    QStringList tabs = {"档案", "排班", "考勤"};
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

    contentLayout->addWidget(header);
    contentLayout->addWidget(tabContainer);

    // --- 3. 堆叠内容区 ---
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->setStyleSheet("QStackedWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    m_stackedWidget->addWidget(createProfilePage());    // Index 0
    m_stackedWidget->addWidget(createSchedulePage());   // Index 1
    m_stackedWidget->addWidget(createAttendancePage()); // Index 2 (考勤页)

    contentLayout->addWidget(m_stackedWidget);

    connect(m_tabGroup, &QButtonGroup::idClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    m_tabGroup->button(0)->setChecked(true); // 默认选中档案
    
    // 采用绝对定位固定位置
    m_editBtn->setParent(container);
    m_editBtn->move(297, 21);
    m_editBtn->raise();

    showEmptyState(true); // 默认显示占位界面
}

QWidget* EmployeeDetailDrawer::createProfilePage()
{
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
                          "QScrollArea > QWidget > QWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    
    QWidget *content = new QWidget();
    content->setStyleSheet("background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(16);

    auto createGroupCard = [&](const QString &title) {
        QVBoxLayout *groupLayout = new QVBoxLayout();
        groupLayout->setContentsMargins(0, 0, 0, 0);
        groupLayout->setSpacing(10);
        
        QLabel *groupTitle = new QLabel(title);
        groupTitle->setStyleSheet("color: #334155; font-size: 15px; font-weight: bold; margin-left: 4px;");
        groupLayout->addWidget(groupTitle);
        
        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; }");
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(12);
        groupLayout->addWidget(card);
        contentLayout->addLayout(groupLayout);
        return cardLayout;
    };

    auto addDetailRow = [&](QVBoxLayout *layout, const QString &label, QLabel* &valLabel) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *titleL = new QLabel(label);
        titleL->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        titleL->setFixedWidth(70);
        valLabel = new QLabel("--");
        valLabel->setStyleSheet("color: #303133; font-size: 13px; font-weight: 500; border: none; background: transparent;");
        valLabel->setWordWrap(true);
        row->addWidget(titleL);
        row->addWidget(valLabel, 1);
        layout->addLayout(row);
    };

    // 分组 1: 身份背景
    auto identityLayout = createGroupCard("身份与背景");
    addDetailRow(identityLayout, "性别年龄", m_genderAgeVal);
    addDetailRow(identityLayout, "最高学历", m_eduVal);
    addDetailRow(identityLayout, "身份证号", m_idCardVal);

    // 分组 2: 入职岗位
    auto jobLayout = createGroupCard("入职与岗位");
    addDetailRow(jobLayout, "入职日期", m_joinDateVal);
    addDetailRow(jobLayout, "所属部门", m_deptVal);
    addDetailRow(jobLayout, "基本薪资", m_salaryVal);
    addDetailRow(jobLayout, "银行卡号", m_bankCardVal);
    addDetailRow(jobLayout, "当前状态", m_statusVal);

    // 分组 3: 联系与安全
    auto contactLayout = createGroupCard("联系与安全");
    addDetailRow(contactLayout, "联系电话", m_phoneVal);
    addDetailRow(contactLayout, "电子邮箱", m_emailVal);
    addDetailRow(contactLayout, "详细住址", m_addressVal);
    addDetailRow(contactLayout, "紧急联系", m_emergencyVal);

    contentLayout->addStretch();
    scroll->setWidget(content);
    return scroll;
}

QWidget* EmployeeDetailDrawer::createSchedulePage()
{
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
                          "QScrollArea > QWidget > QWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    
    QWidget *content = new QWidget();
    content->setStyleSheet("background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(16);

    // 卡片 1: 今日班次
    QFrame *todayCard = new QFrame();
    todayCard->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #409eff, stop:1 #73b9ff); border-radius: 8px;");
    QVBoxLayout *tLayout = new QVBoxLayout(todayCard);
    tLayout->setContentsMargins(20, 20, 20, 20);
    QLabel *tTitle = new QLabel("今日排班 (实时)");
    tTitle->setStyleSheet("color: rgba(255,255,255,0.8); font-size: 13px; border: none; background: transparent;");
    tLayout->addWidget(tTitle);
    m_todayShiftLabel = new QLabel("加载中...");
    m_todayShiftLabel->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-top: 5px; border: none; background: transparent;");
    tLayout->addWidget(m_todayShiftLabel);

    // 卡片 2: 本月考勤全景日历
    QFrame *monthCalendarCard = new QFrame();
    monthCalendarCard->setStyleSheet("background: #fcfcfd; border-radius: 8px; border: 1px solid #ebeef5;");
    QVBoxLayout *calLayout = new QVBoxLayout(monthCalendarCard);
    calLayout->setContentsMargins(15, 15, 15, 15);
    calLayout->setSpacing(10);

    QHBoxLayout *calHeader = new QHBoxLayout();
    QLabel *calTitle = new QLabel("月度考勤全景");
    calTitle->setStyleSheet("color: #303133; font-size: 14px; font-weight: bold; border: none; background: transparent;");
    calHeader->addWidget(calTitle);
    calHeader->addStretch();

    // 年月选择器容器
    QHBoxLayout *selectorLayout = new QHBoxLayout();
    selectorLayout->setSpacing(2);

    auto createSimpleCombo = [&](const QStringList &items, int width) {
        QComboBox *cb = new QComboBox();
        cb->addItems(items);
        cb->setFixedWidth(width);
        cb->setStyleSheet(
            "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 1px 5px; font-size: 12px; color: #606266; background: #ffffff; } "
            "QComboBox:hover { border-color: #409eff; } "
            "QComboBox::drop-down { border: none; width: 16px; } "
            "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 10px; height: 10px; }"
        );
        return cb;
    };

    QStringList years;
    for(int i=2023; i<=2026; ++i) years << QString::number(i) + "年";
    m_yearCombo = createSimpleCombo(years, 75);
    m_yearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + "年");

    QStringList months;
    for(int i=1; i<=12; ++i) months << QString::number(i) + "月";
    m_monthCombo = createSimpleCombo(months, 65);
    m_monthCombo->setCurrentText(QString::number(QDate::currentDate().month()) + "月");

    selectorLayout->addWidget(m_yearCombo);
    selectorLayout->addWidget(m_monthCombo);
    calHeader->addLayout(selectorLayout);
    calLayout->addLayout(calHeader);

    // 日历网格
    m_calendarGrid = new QGridLayout();
    m_calendarGrid->setSpacing(4);
    calLayout->addLayout(m_calendarGrid);
    
    // 绑定信号
    connect(m_yearCombo, &QComboBox::currentIndexChanged, this, &EmployeeDetailDrawer::refreshCalendar);
    connect(m_monthCombo, &QComboBox::currentIndexChanged, this, &EmployeeDetailDrawer::refreshCalendar);

    // 图例
    QHBoxLayout *legend = new QHBoxLayout();
    legend->addStretch();
    auto addLegend = [&](const QString &bgColor, const QString &txtColor, const QString &text) {
        QLabel *dot = new QLabel();
        dot->setFixedSize(12, 12);
        dot->setStyleSheet(QString("background: %1; border-radius: 6px; border: 1px solid %2;").arg(bgColor, txtColor));
        QLabel *txt = new QLabel(text);
        txt->setStyleSheet(QString("color: %1; font-size: 11px; border: none; background: transparent; font-weight: bold;").arg(txtColor));
        legend->addWidget(dot);
        legend->addWidget(txt);
        legend->addSpacing(15);
    };
    addLegend("#f0f9eb", "#67c23a", "上班");
    addLegend("#fdf6ec", "#e6a23c", "请假");
    addLegend("#f5f7fa", "#909399", "休息");
    addLegend("#409eff", "#409eff", "今日");
    calLayout->addLayout(legend);

    // 卡片 3: 月度考勤摘要
    QFrame *monthCard = new QFrame();
    monthCard->setStyleSheet("background: #fcfcfd; border-radius: 8px; border: 1px solid #ebeef5;");
    QVBoxLayout *mLayout = new QVBoxLayout(monthCard);
    mLayout->setContentsMargins(15, 15, 15, 15);
    QLabel *mTitle = new QLabel("本月考勤");
    mTitle->setStyleSheet("color: #303133; font-size: 14px; font-weight: bold; margin-bottom: 8px; border: none; background: transparent;");
    mLayout->addWidget(mTitle);
    
    auto addStat = [&](const QString &label, const QString &val, const QString &color, QLabel* &valPtr) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *l = new QLabel(label);
        l->setStyleSheet("color: #606266; font-size: 13px; border: none; background: transparent;");
        valPtr = new QLabel(val);
        valPtr->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; border: none; background: transparent;").arg(color));
        row->addWidget(l);
        row->addStretch();
        row->addWidget(valPtr);
        mLayout->addLayout(row);
    };
    addStat("正常出勤", "0 天", "#67c23a", m_statWorkLabel);
    addStat("迟到/早退", "0 次", "#e6a23c", m_statLeaveLabel); // 这里复用为请假天数展示，或者保留业务逻辑
    addStat("休息天数", "0 天", "#909399", m_statRestLabel);

    contentLayout->addWidget(todayCard);
    contentLayout->addWidget(monthCalendarCard);
    contentLayout->addWidget(monthCard);
    contentLayout->addStretch();
    scroll->setWidget(content);
    return scroll;
}
void EmployeeDetailDrawer::setEmployee(const EmployeeInfo &info)
{
    if (info.id.isEmpty()) {
        showEmptyState(true);
        return;
    }
    showEmptyState(false);

    m_currentEmployee = info;
    m_editBtn->show();
    m_editBtn->raise();
    
    m_nameLabel->setText(info.name);
    m_roleLabel->setText(info.role);
    m_idLabel->setText(QString::fromUtf8("工号: %1").arg(info.id));
    // 内部 Label 方式不需要在这里重复 setText
    
    m_genderAgeVal->setText(QString("%1 / %2岁").arg(info.gender).arg(info.age));
    m_phoneVal->setText(info.phone);
    m_emailVal->setText(info.email.isEmpty() ? "未填写" : info.email);
    m_idCardVal->setText(info.idCard);
    m_salaryVal->setText(QString("￥%1").arg(info.baseSalary));
    m_bankCardVal->setText(info.bankCard.isEmpty() ? "未绑定" : info.bankCard);
    m_statusVal->setText(info.status);
    
    // 填充新增字段
    m_joinDateVal->setText(info.joinDate.isEmpty() ? "2023-01-01" : info.joinDate);
    m_deptVal->setText(info.department.isEmpty() ? "未分配" : info.department);
    m_eduVal->setText(info.education.isEmpty() ? "未填写" : info.education);
    m_addressVal->setText(info.address.isEmpty() ? "未录入" : info.address);
    
    QString eContact = info.emergencyContact;
    if (!info.emergencyPhone.isEmpty()) eContact += " (" + info.emergencyPhone + ")";
    m_emergencyVal->setText(eContact.isEmpty() ? "未设置" : eContact);

    // Avatar 渲染
    QString effectivePath = info.imgPath;
    if (effectivePath.isEmpty()) {
        effectivePath = (info.gender == "男" ? ":/images/male.png" : ":/images/female.png");
    }
    m_currentEmployee.imgPath = effectivePath; // 确保 eventFilter 能拿到有效的路径进行放大
    
    QPixmap pixmap = ImageUtils::loadPixmap(effectivePath);
    if (pixmap.isNull()) {
        pixmap.load(":/images/load_img.jpg");
    }
    
    QSize avatarSize(100, 100);
    QPixmap target(avatarSize);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    // 留出 1px 边距防止裁切边缘被截断
    path.addEllipse(1, 1, avatarSize.width() - 2, avatarSize.height() - 2);
    painter.setClipPath(path);
    
    QPixmap scaled = pixmap.scaled(avatarSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = (avatarSize.width() - scaled.width()) / 2;
    int y = (avatarSize.height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    m_avatarLabel->setPixmap(target);

    // 刷新排班与考勤日历
    refreshCalendar();
    refreshAttendCalendar();
}

void EmployeeDetailDrawer::showEmptyState(bool empty)
{
    if (m_emptyWidget) m_emptyWidget->setVisible(empty);
    if (m_contentWidget) m_contentWidget->setVisible(!empty);
    if (m_editBtn) m_editBtn->setVisible(!empty);
}

void EmployeeDetailDrawer::showDrawer()
{
    if (m_isOpened) return;
    m_isOpened = true;
    m_animation->stop();
    m_animation->setStartValue(width());
    m_animation->setEndValue(450); // 统一宽度为 450px，与会员模块一致
    m_animation->start();
}

void EmployeeDetailDrawer::hideDrawer()
{
    m_isOpened = false;
    m_animation->stop();
    m_animation->setStartValue(width());
    m_animation->setEndValue(0);
    m_animation->start();
}

void EmployeeDetailDrawer::refreshCalendar()
{
    if (!m_calendarGrid || !m_yearCombo || !m_monthCombo || m_currentEmployee.id.isEmpty()) return;

    // 清空现有网格内容
    QLayoutItem *child;
    while ((child = m_calendarGrid->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    // 1. 计算日期基础
    int year = m_yearCombo->currentText().left(4).toInt();
    int month = m_monthCombo->currentText().left(m_monthCombo->currentText().size() - 1).toInt();
    QDate startDate(year, month, 1);
    int daysInMonth = startDate.daysInMonth();
    int startCol = startDate.dayOfWeek() % 7; // 0 是周日

    // 2. 添加表头
    QStringList weekDays = {"日", "一", "二", "三", "四", "五", "六"};
    for (int i = 0; i < 7; ++i) {
        QLabel *w = new QLabel(weekDays[i]);
        w->setAlignment(Qt::AlignCenter);
        w->setStyleSheet("color: #C0C4CC; font-size: 11px; margin-bottom: 5px; border: none; background: transparent;");
        m_calendarGrid->addWidget(w, 0, i);
    }

    // 3. 填充日期
    QDate today = QDate::currentDate();
    int workCount = 0, leaveCount = 0, restCount = 0;

    for (int day = 1; day <= daysInMonth; ++day) {
        QDate curr(year, month, day);
        int row = (day + startCol - 1) / 7 + 1;
        int col = (day + startCol - 1) % 7;

        QLabel *dayLabel = new QLabel(QString::number(day));
        dayLabel->setFixedSize(32, 32);
        dayLabel->setAlignment(Qt::AlignCenter);

        // 获取真实排班
        ScheduleInfo info = ScheduleDataManager::instance()->getSchedule(m_currentEmployee.id, curr);
        
        bool isWork = (info.type == SHIFT_MORNING || info.type == SHIFT_EVENING || info.type == SHIFT_CUSTOM);
        bool isLeave = (info.type == SHIFT_OFF && info.note.contains("假"));

        if (isWork) workCount++;
        else if (isLeave) leaveCount++;
        else restCount++;

        QString style = "font-size: 12px; border-radius: 16px; font-weight: 500; ";
        if (isLeave) {
            style += "background: #ffedd5; color: #9a3412; border: 1px solid #fed7aa;"; 
        } else if (isWork) {
            style += "background: #dcfce7; color: #166534; border: 1px solid #bbf7d0;";
        } else {
            style += "background: #f1f5f9; color: #64748b; border: 1px solid #e2e8f0;";
        }

        // 如果是今天，添加显眼的蓝色边框，而不是覆盖背景
        if (curr == today) {
            style += "border: 2px solid #3b82f6; font-weight: bold;";
            
            // 同步更新顶部“今日排班”卡片
            if (m_todayShiftLabel) {
                if (isWork) {
                    QString start = info.startTime.length() > 5 ? info.startTime.left(5) : info.startTime;
                    QString end = info.endTime.length() > 5 ? info.endTime.left(5) : info.endTime;
                    m_todayShiftLabel->setText(QString("%1 - %2").arg(start).arg(end));
                } else {
                    m_todayShiftLabel->setText(info.note.contains("假") ? "今日请假" : "今日休息");
                }
            }
        }

        dayLabel->setStyleSheet(style);
        m_calendarGrid->addWidget(dayLabel, row, col);
    }

    // 4. 更新底部统计
    m_statWorkLabel->setText(QString("%1 天").arg(workCount));
    m_statLeaveLabel->setText(QString("%1 天").arg(leaveCount));
    m_statRestLabel->setText(QString("%1 天").arg(restCount));
}

bool EmployeeDetailDrawer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avatarLabel && event->type() == QEvent::MouseButtonRelease) {
        emit avatarClicked(m_currentEmployee.imgPath);
        return true;
    }
    if (event->type() == QEvent::Wheel) {
        if (obj && (obj->objectName() == "attendanceScroll" || (obj->parent() && obj->parent()->objectName() == "attendanceScroll"))) {
            return true; // 拦截并吞掉鼠标滚轮事件，彻底阻止上下滚动
        }
    }
    return QWidget::eventFilter(obj, event);
}

QWidget* EmployeeDetailDrawer::createAttendancePage()
{
    QScrollArea *scroll = new QScrollArea();
    scroll->setObjectName("attendanceScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 彻底隐藏垂直滚动条
    scroll->installEventFilter(this);
    scroll->viewport()->installEventFilter(this);
    scroll->setStyleSheet(
        "QScrollArea { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
        "QScrollArea > QWidget > QWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
    );
    
    QWidget *content = new QWidget();
    content->setObjectName("attendanceScrollContent");
    content->setStyleSheet("background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(12, 10, 12, 20); // 底部外边距微调到 20px，完美把卡片托起，露出精致的圆角！
    contentLayout->setSpacing(4);

    // 卡片 1: 今日打卡状态 (白底卡片风格)
    QFrame *todayCard = new QFrame();
    todayCard->setObjectName("todayCard");
    todayCard->setStyleSheet("#todayCard { background: #ffffff; border-radius: 8px; border: 1px solid #ebeef5; }");
    QVBoxLayout *tLayout = new QVBoxLayout(todayCard);
    tLayout->setContentsMargins(15, 10, 15, 10);
    tLayout->setSpacing(10);

    QHBoxLayout *tHeaderLayout = new QHBoxLayout();
    m_attendTodayTitleLabel = new QLabel("当日考勤明细");
    m_attendTodayTitleLabel->setStyleSheet("color: #303133; font-size: 14px; font-weight: bold; border: none; background: transparent;");
    tHeaderLayout->addWidget(m_attendTodayTitleLabel);
    tHeaderLayout->addStretch();
    
    QPushButton *editAttendBtn = new QPushButton("补签/微调");
    editAttendBtn->setCursor(Qt::PointingHandCursor);
    editAttendBtn->setStyleSheet("QPushButton { background: #3b82f6; color: white; border: none; border-radius: 6px; padding: 4px 10px; font-size: 11px; font-weight: bold; } "
                                 "QPushButton:hover { background: #2563eb; } "
                                 "QPushButton:pressed { background: #1d4ed8; }");
    tHeaderLayout->addWidget(editAttendBtn);
    tLayout->addLayout(tHeaderLayout);

    m_attendTodayShiftWidget = new QWidget();
    m_attendTodayShiftWidget->setStyleSheet("background: transparent; border: none;");
    QVBoxLayout *detailLayout = new QVBoxLayout(m_attendTodayShiftWidget);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(6);
    tLayout->addWidget(m_attendTodayShiftWidget);

    connect(editAttendBtn, &QPushButton::clicked, this, [this]() {
        if (!m_selectedAttendDate.isValid()) return;
        ScheduleInfo info = ScheduleDataManager::instance()->getSchedule(m_currentEmployee.id, m_selectedAttendDate);
        EditAttendanceDialog dlg(info, this);
        if (dlg.exec() == QDialog::Accepted) {
            ScheduleInfo newInfo = dlg.getUpdatedInfo();
            ScheduleDataManager::instance()->saveSchedule(newInfo);
        }
    });

    // 卡片 2: 本月考勤日历网格
    QFrame *monthCalendarCard = new QFrame();
    monthCalendarCard->setObjectName("monthCalendarCard");
    monthCalendarCard->setStyleSheet("#monthCalendarCard { background: #fcfcfd; border-radius: 8px; border: 1px solid #ebeef5; }");
    QVBoxLayout *calLayout = new QVBoxLayout(monthCalendarCard);
    calLayout->setContentsMargins(12, 6, 12, 6);
    calLayout->setSpacing(4);

    QHBoxLayout *calHeader = new QHBoxLayout();
    QLabel *calTitle = new QLabel("月度考勤全景");
    calTitle->setStyleSheet("color: #303133; font-size: 13px; font-weight: bold; border: none; background: transparent;");
    calHeader->addWidget(calTitle);
    calHeader->addStretch();

    // 年月选择器
    QHBoxLayout *selectorLayout = new QHBoxLayout();
    selectorLayout->setSpacing(2);

    auto createSimpleCombo = [&](const QStringList &items, int width) {
        QComboBox *cb = new QComboBox();
        cb->addItems(items);
        cb->setFixedWidth(width);
        cb->setStyleSheet(
            "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 1px 5px; font-size: 11px; color: #606266; background: #ffffff; } "
            "QComboBox:hover { border-color: #409eff; } "
            "QComboBox::drop-down { border: none; width: 16px; } "
            "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 10px; height: 10px; }"
        );
        return cb;
    };

    QStringList years;
    for(int i=2023; i<=2026; ++i) years << QString::number(i) + "年";
    m_attendYearCombo = createSimpleCombo(years, 70);
    m_attendYearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + "年");

    QStringList months;
    for(int i=1; i<=12; ++i) months << QString::number(i) + "月";
    m_attendMonthCombo = createSimpleCombo(months, 60);
    m_attendMonthCombo->setCurrentText(QString::number(QDate::currentDate().month()) + "月");

    selectorLayout->addWidget(m_attendYearCombo);
    selectorLayout->addWidget(m_attendMonthCombo);
    calHeader->addLayout(selectorLayout);
    calLayout->addLayout(calHeader);

    // 考勤网格
    m_attendCalendarGrid = new QGridLayout();
    m_attendCalendarGrid->setSpacing(2);
    calLayout->addLayout(m_attendCalendarGrid);
    
    // 绑定信号
    connect(m_attendYearCombo, &QComboBox::currentIndexChanged, this, &EmployeeDetailDrawer::refreshAttendCalendar);
    connect(m_attendMonthCombo, &QComboBox::currentIndexChanged, this, &EmployeeDetailDrawer::refreshAttendCalendar);

    // 图例 (考勤)
    QHBoxLayout *legend = new QHBoxLayout();
    legend->addStretch();
    auto addLegend = [&](const QString &bgColor, const QString &txtColor, const QString &text) {
        QLabel *dot = new QLabel();
        dot->setFixedSize(8, 8);
        dot->setStyleSheet(QString("background: %1; border-radius: 4px; border: 1px solid %2;").arg(bgColor, txtColor));
        QLabel *txt = new QLabel(text);
        txt->setStyleSheet(QString("color: %1; font-size: 10px; border: none; background: transparent; font-weight: bold;").arg(txtColor));
        legend->addWidget(dot);
        legend->addWidget(txt);
        legend->addSpacing(8);
    };
    addLegend("#dcfce7", "#166534", "正常");
    addLegend("#fef3c7", "#b45309", "迟到/早退");
    addLegend("#fee2e2", "#991b1b", "缺卡");
    addLegend("#f1f5f9", "#64748b", "休息");
    calLayout->addLayout(legend);

    // 卡片 3: 本月考勤汇总看板 (横向紧凑扁平设计)
    QFrame *monthCard = new QFrame();
    monthCard->setObjectName("monthCard");
    monthCard->setStyleSheet("#monthCard { background: #fcfcfd; border-radius: 8px; border: 1px solid #ebeef5; }");
    monthCard->setFixedHeight(76); // Lock to compact, elegant height
    QVBoxLayout *mLayout = new QVBoxLayout(monthCard);
    mLayout->setContentsMargins(15, 8, 15, 8);
    mLayout->setSpacing(4);
    
    QLabel *mTitle = new QLabel("本月考勤统计");
    mTitle->setStyleSheet("color: #303133; font-size: 12px; font-weight: bold; border: none; background: transparent;");
    mLayout->addWidget(mTitle);

    QHBoxLayout *statsHLayout = new QHBoxLayout();
    statsHLayout->setSpacing(12);
    statsHLayout->setContentsMargins(0, 0, 0, 0);

    auto addStatH = [&](const QString &label, const QString &val, const QString &color, QLabel* &valPtr) {
        QFrame *itemFrame = new QFrame();
        itemFrame->setStyleSheet("background: #fcfcfd; border: none;");
        QVBoxLayout *itemLayout = new QVBoxLayout(itemFrame);
        itemLayout->setContentsMargins(0, 0, 0, 0);
        itemLayout->setSpacing(2);

        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("color: #909399; font-size: 11px; border: none; background: transparent;");
        lbl->setAlignment(Qt::AlignCenter);

        valPtr = new QLabel(val);
        valPtr->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 800; border: none; background: transparent;").arg(color));
        valPtr->setAlignment(Qt::AlignCenter);

        itemLayout->addWidget(lbl);
        itemLayout->addWidget(valPtr);
        statsHLayout->addWidget(itemFrame);
    };

    addStatH("正常出勤", "0 天", "#166534", m_statNormalLabel);
    addStatH("迟到/早退", "0 次", "#b45309", m_statLateLabel);
    addStatH("未打卡/缺卡", "0 天", "#991b1b", m_statAbsentLabel);

    mLayout->addLayout(statsHLayout);

    contentLayout->addWidget(monthCalendarCard);
    contentLayout->addWidget(todayCard);
    contentLayout->addWidget(monthCard);
    contentLayout->addStretch();
    
    scroll->setWidget(content);
    return scroll;
}

void EmployeeDetailDrawer::refreshAttendCalendar()
{
    if (!m_attendCalendarGrid || !m_attendYearCombo || !m_attendMonthCombo || m_currentEmployee.id.isEmpty()) return;

    this->setUpdatesEnabled(false);

    // 清空现有网格内容
    QLayoutItem *child;
    while ((child = m_attendCalendarGrid->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->hide();
            child->widget()->setParent(nullptr);
            child->widget()->deleteLater();
        }
        delete child;
    }

    // 1. 计算日期基础
    int year = m_attendYearCombo->currentText().left(4).toInt();
    int month = m_attendMonthCombo->currentText().left(m_attendMonthCombo->currentText().size() - 1).toInt();
    QDate startDate(year, month, 1);
    int daysInMonth = startDate.daysInMonth();
    int startCol = startDate.dayOfWeek() % 7; // 0 是周日

    // 2. 添加表头
    QStringList weekDays = {"日", "一", "二", "三", "四", "五", "六"};
    for (int i = 0; i < 7; ++i) {
        QLabel *w = new QLabel(weekDays[i]);
        w->setAlignment(Qt::AlignCenter);
        w->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold; margin-bottom: 5px; border: none; background: transparent;");
        m_attendCalendarGrid->addWidget(w, 0, i);
    }

    // 3. 填充日期
    QDate today = QDate::currentDate();
    int normalCount = 0, lateCount = 0, absentCount = 0;

    for (int day = 1; day <= daysInMonth; ++day) {
        QDate curr(year, month, day);
        int row = (day + startCol - 1) / 7 + 1;
        int col = (day + startCol - 1) % 7;

        // 获取排班考勤数据
        ScheduleInfo info = ScheduleDataManager::instance()->getSchedule(m_currentEmployee.id, curr);
        
        bool isWork = (info.type == SHIFT_MORNING || info.type == SHIFT_EVENING || info.type == SHIFT_CUSTOM);
        bool isLeave = (info.type == SHIFT_OFF && info.note.contains("假"));

        // 确保选中的日期在当前月份内，如果不在，智能更新为该月1号（或今天，如果今天在当前月份内）
        if (!m_selectedAttendDate.isValid() || m_selectedAttendDate.year() != year || m_selectedAttendDate.month() != month) {
            QDate realToday = QDate::currentDate();
            if (realToday.year() == year && realToday.month() == month) {
                m_selectedAttendDate = realToday;
            } else {
                m_selectedAttendDate = QDate(year, month, 1);
            }
        }

        QWidget *cellWidget = new QWidget();
        cellWidget->setFixedSize(40, 39);
        cellWidget->setStyleSheet("QWidget { background: transparent; border: none; }");
        QVBoxLayout *cellLayout = new QVBoxLayout(cellWidget);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        cellLayout->setSpacing(0);
        cellLayout->setAlignment(Qt::AlignCenter);

        QPushButton *dayBtn = new QPushButton();
        dayBtn->setFixedSize(30, 30);
        dayBtn->setCursor(Qt::PointingHandCursor);

        QLabel *numLabel = new QLabel(QString::number(day), dayBtn);
        numLabel->setFixedSize(30, 30);
        numLabel->setAlignment(Qt::AlignCenter);
        numLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

        QLabel *statusLabel = new QLabel();
        statusLabel->setAlignment(Qt::AlignCenter);

        QString statusText = "";
        QString dotColor = "";

        if (isLeave) {
            statusText = "请假";
        } else if (info.type == SHIFT_OFF) {
            statusText = "休息";
        } else if (isWork) {
            if (info.clockIn.isEmpty() && info.clockOut.isEmpty()) {
                if (curr < today) {
                    dotColor = "#f56c6c"; // 缺卡
                    absentCount++;
                } else {
                    dotColor = ""; // 今天及未来的未打卡日期不显示状态点，保持日历极其干净清爽
                }
            } else {
                bool isLate = false;
                bool isEarly = false;
                
                if (!info.clockIn.isEmpty() && !info.startTime.isEmpty()) {
                    QTime actualIn = QTime::fromString(info.clockIn, "HH:mm");
                    QTime planStart = QTime::fromString(info.startTime, "HH:mm");
                    if (actualIn > planStart) isLate = true;
                }
                if (!info.clockOut.isEmpty() && !info.endTime.isEmpty()) {
                    QTime actualOut = QTime::fromString(info.clockOut, "HH:mm");
                    QTime planEnd = QTime::fromString(info.endTime, "HH:mm");
                    if (actualOut < planEnd) isEarly = true;
                }
                
                if (isLate || isEarly) {
                    dotColor = "#e6a23c"; // 迟到/早退
                    lateCount++;
                } else {
                    dotColor = "#67c23a"; // 正常
                    normalCount++;
                }
            }
        }

        // 选中和今日样式处理
        bool isSelected = (curr == m_selectedAttendDate);
        bool isToday = (curr == today);
        
        QString btnStyle = "";
        QString statusColor = "#909399";
        QString numColor = "#303133";
        
        if (isSelected) {
            // 被选中的日期：蓝色背景
            btnStyle = "QPushButton { border-radius: 15px; border: none; background: #3b82f6; padding: 0px; margin: 0px; }";
            statusColor = "#3b82f6";
            numColor = "#ffffff";
        } else if (isToday) {
            // 今天日期（未选中）：透明蓝色圆形背景，蓝色文字
            btnStyle = "QPushButton { border-radius: 15px; border: 1px solid rgba(59, 130, 246, 0.4); background: rgba(59, 130, 246, 0.12); padding: 0px; margin: 0px; } QPushButton:hover { background: rgba(59, 130, 246, 0.2); }";
            statusColor = "#3b82f6";
            numColor = "#3b82f6";
        } else {
            btnStyle = "QPushButton { border-radius: 15px; border: none; background: transparent; padding: 0px; margin: 0px; } QPushButton:hover { background: #f2f6fc; }";
            numColor = "#303133";
        }
        
        dayBtn->setStyleSheet(btnStyle);
        numLabel->setStyleSheet(QString("QLabel { color: %1; font-family: 'Segoe UI', 'Microsoft YaHei'; font-size: 15px; font-weight: bold; border: none; background: transparent; padding: 0px; margin: 0px; }").arg(numColor));

        // 设置 statusLabel 文本与颜色
        if (!statusText.isEmpty()) {
            statusLabel->setText(statusText);
            statusLabel->setStyleSheet(QString("QLabel { font-family: 'Segoe UI', 'Microsoft YaHei'; font-size: 9px; font-weight: bold; color: %1; border: none; background: transparent; padding-top: 1px; padding-bottom: 0px; margin: 0px; }").arg(statusColor));
        } else if (!dotColor.isEmpty()) {
            statusLabel->setText("●");
            statusLabel->setStyleSheet(QString("QLabel { font-family: 'Segoe UI', 'Microsoft YaHei'; font-size: 11px; font-weight: bold; color: %1; border: none; background: transparent; padding-top: 2px; padding-bottom: 0px; margin: 0px; }").arg(dotColor));
        } else {
            statusLabel->setText("");
        }

        cellLayout->addWidget(dayBtn);
        cellLayout->addWidget(statusLabel);

        // 选中状态与顶卡详细内容更新
        if (isSelected) {
            if (m_attendTodayTitleLabel) {
                QString dateStr = curr.toString("yyyy-MM-dd");
                m_attendTodayTitleLabel->setText(QString("%1%2").arg(dateStr).arg(isToday ? " (今天)" : ""));
            }

            if (m_attendTodayShiftWidget) {
                // 彻底清除所有旧的直属子控件，防止子布局/控件残留造成重叠和多重竖线Bug
                QList<QWidget*> children = m_attendTodayShiftWidget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
                for (QWidget *child : children) {
                    child->hide();
                    child->deleteLater();
                }
                
                QLayout *lay = m_attendTodayShiftWidget->layout();
                if (lay) {
                    delete lay;
                }
                
                QVBoxLayout *vbox = new QVBoxLayout(m_attendTodayShiftWidget);
                vbox->setContentsMargins(0, 0, 0, 0);
                vbox->setSpacing(12);

                int clockCount = 0;
                if (!info.clockIn.isEmpty()) clockCount++;
                if (!info.clockOut.isEmpty()) clockCount++;

                QString shiftStr = "-";
                if (isWork) {
                    shiftStr = QString("固定上下班 白班 %1-%2").arg(info.startTime).arg(info.endTime);
                } else if (isLeave) {
                    shiftStr = "请假";
                } else {
                    shiftStr = "休息";
                }

                QString statStr = QString("打卡 %1 次").arg(clockCount);
                if (isWork && !info.clockIn.isEmpty() && !info.clockOut.isEmpty()) {
                    QTime inTime = QTime::fromString(info.clockIn, "HH:mm");
                    QTime outTime = QTime::fromString(info.clockOut, "HH:mm");
                    if (inTime.isValid() && outTime.isValid()) {
                        int secs = inTime.secsTo(outTime);
                        double workHours = qMax(0.0, secs / 3600.0);
                        statStr = QString("打卡 %1 次，工时 %2 小时").arg(clockCount).arg(QString::number(workHours, 'f', 2));
                    }
                }

                // Row 1: 当日班次
                QLabel *lblShift = new QLabel(QString("当日班次: %1").arg(shiftStr));
                lblShift->setStyleSheet("color: #606266; font-size: 13px; font-family: 'Segoe UI', 'Microsoft YaHei'; border: none; background: transparent;");
                vbox->addWidget(lblShift);

                // Row 2: 出勤统计
                QLabel *lblStat = new QLabel(QString("出勤统计: %1").arg(statStr));
                lblStat->setStyleSheet("color: #606266; font-size: 13px; font-family: 'Segoe UI', 'Microsoft YaHei'; border: none; background: transparent;");
                vbox->addWidget(lblStat);

                // Space
                vbox->addSpacing(4);

                // Grid layout for timeline
                QGridLayout *grid = new QGridLayout();
                grid->setContentsMargins(0, 4, 0, 4);
                grid->setHorizontalSpacing(8);
                grid->setVerticalSpacing(10); // Compact premium spacing
                vbox->addLayout(grid);

                // 上班打卡详情
                QString inDotColor = "#909399";
                QString inTimeDesc = "--";
                QString inBadgeStyle = "";
                QString inBadgeText = "-";
                bool inHasRecord = false;

                if (isWork) {
                    if (info.clockIn.isEmpty()) {
                        inDotColor = "#f56c6c";
                        inTimeDesc = "--";
                        inBadgeStyle = "color: #c62828; font-size: 11px; font-weight: bold; border: none; background: transparent; padding: 0;";
                        inBadgeText = "缺卡";
                    } else {
                        inTimeDesc = info.clockIn;
                        inHasRecord = true;
                        bool late = false;
                        if (!info.startTime.isEmpty()) {
                            QTime plan = QTime::fromString(info.startTime, "HH:mm");
                            QTime act = QTime::fromString(info.clockIn, "HH:mm");
                            if (act > plan) late = true;
                        }
                        if (late) {
                            inDotColor = "#e6a23c";
                            inBadgeStyle = "color: #ef6c00; font-size: 11px; font-weight: bold; border: none; background: transparent; padding: 0;";
                            inBadgeText = "迟到";
                        } else {
                            inDotColor = "#67c23a";
                            inBadgeStyle = "color: #2e7d32; font-size: 11px; font-weight: bold; border: none; background: transparent; padding: 0;";
                            inBadgeText = "正常";
                        }
                    }
                } else {
                    inTimeDesc = isLeave ? "请假" : "休息";
                    inBadgeStyle = "color: #606266; font-size: 11px; font-weight: bold; border: none; background: transparent; padding: 0;";
                    inBadgeText = isLeave ? "请假" : "休息";
                }

                // 下班打卡详情
                QString outDotColor = "#909399";
                QString outTimeDesc = "--";
                QString outBadgeStyle = "";
                QString outBadgeText = "-";
                bool outHasRecord = false;

                if (isWork) {
                    if (info.clockOut.isEmpty()) {
                        outDotColor = "#f56c6c";
                        outTimeDesc = "--";
                        outBadgeStyle = "color: #c62828; font-size: 11px; font-weight: bold; border: none; background: transparent; padding: 0;";
                        outBadgeText = "缺卡";
                    } else {
                        outTimeDesc = info.clockOut;
                        outHasRecord = true;
                        bool early = false;
                        if (!info.endTime.isEmpty()) {
                            QTime plan = QTime::fromString(info.endTime, "HH:mm");
                            QTime act = QTime::fromString(info.clockOut, "HH:mm");
                            if (act < plan) early = true;
                        }
                        if (early) {
                            outDotColor = "#e6a23c";
                            outBadgeStyle = "color: #ef6c00; font-size: 11px; font-weight: bold; border: none; background: transparent; padding: 0;";
                            outBadgeText = "早退";
                        } else {
                            outDotColor = "#67c23a";
                            outBadgeStyle = "color: #2e7d32; font-size: 11px; font-weight: bold; border: none; background: transparent; padding: 0;";
                            outBadgeText = "正常";
                        }
                    }
                } else {
                    outTimeDesc = isLeave ? "请假" : "休息";
                    outBadgeStyle = "color: #606266; font-size: 11px; font-weight: bold; border: none; background: transparent; padding: 0;";
                    outBadgeText = isLeave ? "请假" : "休息";
                }

                // 1. 上班 ●
                QLabel *lblInDot = new QLabel("●");
                lblInDot->setFixedWidth(16);
                lblInDot->setAlignment(Qt::AlignCenter);
                lblInDot->setStyleSheet(QString("color: %1; font-size: 16px; border: none; background: transparent;").arg(inDotColor));
                grid->addWidget(lblInDot, 0, 0);

                // 上班文字与徽章
                QWidget *wInHeader = new QWidget();
                QHBoxLayout *hInHeader = new QHBoxLayout(wInHeader);
                hInHeader->setContentsMargins(0, 0, 0, 0);
                hInHeader->setSpacing(8);
                
                QLabel *lblInTitle = new QLabel(QString("上班 %1").arg(inTimeDesc));
                lblInTitle->setStyleSheet("color: #1e293b; font-size: 18px; font-weight: 800; border: none; background: transparent;");
                hInHeader->addWidget(lblInTitle);

                if (inBadgeText != "-") {
                    QLabel *lblInBadge = new QLabel(inBadgeText);
                    lblInBadge->setStyleSheet(inBadgeStyle);
                    hInHeader->addWidget(lblInBadge);
                }
                hInHeader->addStretch();
                grid->addWidget(wInHeader, 0, 1);

                // 2. 上班连线 │ (使用 QFrame 绘制高精度实线)
                QFrame *lblInLine = new QFrame();
                lblInLine->setFixedWidth(16);
                lblInLine->setFixedHeight(22); // 稍微紧凑的高度，保持完美间隙！
                lblInLine->setStyleSheet("background-color: #dcdfe6; margin-left: 7px; margin-right: 7px; border: none;");
                grid->addWidget(lblInLine, 1, 0);

                // 3. 下班 ●
                QLabel *lblOutDot = new QLabel("●");
                lblOutDot->setFixedWidth(16);
                lblOutDot->setAlignment(Qt::AlignCenter);
                lblOutDot->setStyleSheet(QString("color: %1; font-size: 16px; border: none; background: transparent;").arg(outDotColor));
                grid->addWidget(lblOutDot, 2, 0);

                // 下班文字与徽章
                QWidget *wOutHeader = new QWidget();
                QHBoxLayout *hOutHeader = new QHBoxLayout(wOutHeader);
                hOutHeader->setContentsMargins(0, 0, 0, 0);
                hOutHeader->setSpacing(8);

                QLabel *lblOutTitle = new QLabel(QString("下班 %1").arg(outTimeDesc));
                lblOutTitle->setStyleSheet("color: #1e293b; font-size: 18px; font-weight: 800; border: none; background: transparent;");
                hOutHeader->addWidget(lblOutTitle);

                if (outBadgeText != "-") {
                    QLabel *lblOutBadge = new QLabel(outBadgeText);
                    lblOutBadge->setStyleSheet(outBadgeStyle);
                    hOutHeader->addWidget(lblOutBadge);
                }
                hOutHeader->addStretch();
                grid->addWidget(wOutHeader, 2, 1);

                // Divider line
                QFrame *line = new QFrame();
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                line->setStyleSheet("color: #ebeef5; background: #ebeef5; max-height: 1px; border: none;");
                vbox->addWidget(line);
                vbox->addSpacing(4);

                // Row: 考勤备注
                QWidget *wNote = new QWidget();
                QHBoxLayout *hNote = new QHBoxLayout(wNote);
                hNote->setContentsMargins(0, 0, 0, 0);
                hNote->setSpacing(8);

                QLabel *lblNoteBadge = new QLabel("考勤备注:");
                lblNoteBadge->setStyleSheet("color: #475569; font-size: 13px; font-weight: bold; border: none; background: transparent;");
                hNote->addWidget(lblNoteBadge);

                QLabel *lblNoteValue = new QLabel(info.note.isEmpty() ? "无备注信息" : info.note);
                lblNoteValue->setStyleSheet("color: #64748b; font-size: 13px; border: none; background: transparent;");
                hNote->addWidget(lblNoteValue);
                hNote->addStretch();

                vbox->addWidget(wNote);

                // 如果选中的是今天，增加“上班打卡”和“下班打卡”的操作按钮
                if (isToday) {
                    vbox->addSpacing(8);
                    
                    QFrame *lineSep2 = new QFrame();
                    lineSep2->setFixedHeight(1);
                    lineSep2->setStyleSheet("background-color: #ebeef5; border: none;");
                    vbox->addWidget(lineSep2);
                    
                    vbox->addSpacing(8);

                    QHBoxLayout *hBtnLayout = new QHBoxLayout();
                    hBtnLayout->setContentsMargins(0, 0, 0, 0);
                    hBtnLayout->setSpacing(12);
                    bool hasClockIn = !info.clockIn.isEmpty();
                    bool hasClockOut = !info.clockOut.isEmpty();

                    QPushButton *btnClockIn = new QPushButton(hasClockIn ? "已打卡上班" : "上班打卡");
                    btnClockIn->setEnabled(!hasClockIn);
                    btnClockIn->setCursor(hasClockIn ? Qt::ArrowCursor : Qt::PointingHandCursor);
                    btnClockIn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                    btnClockIn->setStyleSheet(
                        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #2563eb); color: white; border: none; border-radius: 6px; padding: 8px 16px; font-size: 13px; font-weight: bold; font-family: 'Segoe UI', 'Microsoft YaHei'; } "
                        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563eb, stop:1 #1d4ed8); } "
                        "QPushButton:pressed { background: #1e3a8a; } "
                        "QPushButton:disabled { background: #e4e7ed; color: #a8abb2; border: none; }"
                    );

                    QPushButton *btnClockOut = new QPushButton(hasClockOut ? "已打卡下班" : "下班打卡");
                    btnClockOut->setEnabled(!hasClockOut);
                    btnClockOut->setCursor(hasClockOut ? Qt::ArrowCursor : Qt::PointingHandCursor);
                    btnClockOut->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                    btnClockOut->setStyleSheet(
                        "QPushButton { background: #ffffff; color: #4b5563; border: 1px solid #d1d5db; border-radius: 6px; padding: 8px 16px; font-size: 13px; font-weight: bold; font-family: 'Segoe UI', 'Microsoft YaHei'; } "
                        "QPushButton:hover { background: #f9fafb; color: #2563eb; border-color: #3b82f6; } "
                        "QPushButton:pressed { background: #f3f4f6; } "
                        "QPushButton:disabled { background: #f5f7fa; color: #c0c4cc; border: 1px solid #e4e7ed; }"
                    );

                    hBtnLayout->addWidget(btnClockIn);
                    hBtnLayout->addWidget(btnClockOut);
                    vbox->addLayout(hBtnLayout);

                    // 绑定打卡点击事件
                    connect(btnClockIn, &QPushButton::clicked, this, [this, info]() {
                        ScheduleInfo newInfo = info;
                        newInfo.clockIn = QTime::currentTime().toString("HH:mm");
                        ScheduleDataManager::instance()->saveSchedule(newInfo);
                        refreshAttendCalendar();
                    });

                    connect(btnClockOut, &QPushButton::clicked, this, [this, info]() {
                        ScheduleInfo newInfo = info;
                        newInfo.clockOut = QTime::currentTime().toString("HH:mm");
                        ScheduleDataManager::instance()->saveSchedule(newInfo);
                        refreshAttendCalendar();
                    });
                }

                // 强制刷新滚动内容的高度，更新滚动条区间，彻底消除底部遮挡和切边Bug
                QWidget *p = m_attendTodayShiftWidget;
                while (p && p->objectName() != "attendanceScrollContent") {
                    p = p->parentWidget();
                }
                if (p) {
                    p->adjustSize();
                }
            }
        }

        m_attendCalendarGrid->addWidget(cellWidget, row, col);

        // 点击事件：单击选中并更新卡片详情，再次点击选中项或点击“补签/微调”按钮唤起弹窗
        connect(dayBtn, &QPushButton::clicked, this, [this, curr, info]() {
            if (m_selectedAttendDate == curr) {
                EditAttendanceDialog dlg(info, this);
                if (dlg.exec() == QDialog::Accepted) {
                    ScheduleInfo newInfo = dlg.getUpdatedInfo();
                    ScheduleDataManager::instance()->saveSchedule(newInfo);
                }
            } else {
                m_selectedAttendDate = curr;
                refreshAttendCalendar();
            }
        });
    }

    // 4. 更新本月考勤看板
    m_statNormalLabel->setText(QString("%1 天").arg(normalCount));
    m_statLateLabel->setText(QString("%1 次").arg(lateCount));
    m_statAbsentLabel->setText(QString("%1 天").arg(absentCount));

    this->setUpdatesEnabled(true);
}
