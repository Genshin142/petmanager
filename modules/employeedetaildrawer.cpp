#include "employeedetaildrawer.h"
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

EmployeeDetailDrawer::EmployeeDetailDrawer(QWidget *parent) : QWidget(parent), m_isOpened(false)
{
    setupUI();
    setFixedWidth(0);
    m_animation = new QPropertyAnimation(this, "sideWidth");
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
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

    QStringList tabs = {"档案", "排班", "动态"};
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

    mainLayout->addWidget(header);
    mainLayout->addWidget(tabContainer);

    // --- 3. 堆叠内容区 ---
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->setStyleSheet("QStackedWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    m_stackedWidget->addWidget(createProfilePage());    // Index 0
    m_stackedWidget->addWidget(createSchedulePage());   // Index 1
    
    // 动态占位
    QWidget *logPage = new QWidget();
    logPage->setStyleSheet("background: white; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QVBoxLayout *logLayout = new QVBoxLayout(logPage);
    QLabel *logEmpty = new QLabel("暂无最新操作动态");
    logEmpty->setAlignment(Qt::AlignCenter);
    logEmpty->setStyleSheet("color: #909399; font-size: 13px;");
    logLayout->addWidget(logEmpty);
    m_stackedWidget->addWidget(logPage); // Index 2

    mainLayout->addWidget(m_stackedWidget);

    connect(m_tabGroup, &QButtonGroup::idClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    m_tabGroup->button(0)->setChecked(true); // 默认选中档案
    
    // 采用绝对定位固定位置
    m_editBtn->setParent(container);
    m_editBtn->move(297, 21);
    m_editBtn->raise();
}

QWidget* EmployeeDetailDrawer::createProfilePage()
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
    QLabel *tTime = new QLabel("09:00 - 18:00");
    tTime->setStyleSheet("color: white; font-size: 24px; font-weight: bold; margin-top: 5px; border: none; background: transparent;");
    
    tLayout->addWidget(tTitle);
    tLayout->addWidget(tTime);

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
    QComboBox *yearCombo = createSimpleCombo(years, 75);
    yearCombo->setCurrentText("2026年");

    QStringList months;
    for(int i=1; i<=12; ++i) months << QString::number(i) + "月";
    QComboBox *monthCombo = createSimpleCombo(months, 65);
    monthCombo->setCurrentText("4月");

    selectorLayout->addWidget(yearCombo);
    selectorLayout->addWidget(monthCombo);
    calHeader->addLayout(selectorLayout);
    calLayout->addLayout(calHeader);

    // 日历网格
    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(4);
    QStringList weekDays = {"日", "一", "二", "三", "四", "五", "六"};
    for (int i = 0; i < 7; ++i) {
        QLabel *w = new QLabel(weekDays[i]);
        w->setAlignment(Qt::AlignCenter);
        w->setStyleSheet("color: #C0C4CC; font-size: 11px; margin-bottom: 5px; border: none; background: transparent;");
        grid->addWidget(w, 0, i);
    }

    // 填充日期 (以 4 月为例，1号是周三)
    int startCol = 3; 
    int daysInMonth = 30;
    int today = 29;

    for (int day = 1; day <= daysInMonth; ++day) {
        int row = (day + startCol - 1) / 7 + 1;
        int col = (day + startCol - 1) % 7;

        QLabel *dayLabel = new QLabel(QString::number(day));
        dayLabel->setFixedSize(32, 32);
        dayLabel->setAlignment(Qt::AlignCenter);
        
        // 模拟排班数据
        bool isWeekend = (col == 0 || col == 6);
        bool isLeave = (day == 14 || day == 15); // 模拟 14-15 号请假
        bool isWork = !isWeekend && !isLeave;

        QString style = "font-size: 12px; border-radius: 16px; border: none; font-weight: 500; ";
        if (day == today) {
            style += "background: #3b82f6; color: white; font-weight: bold; ";
        } else if (isLeave) {
            style += "background: #ffedd5; color: #9a3412;"; // 警示橙
        } else if (isWork) {
            style += "background: #dcfce7; color: #166534;"; // 清新绿
        } else {
            style += "background: #f1f5f9; color: #64748b;"; // 极简灰
        }
        
        dayLabel->setStyleSheet(style);
        grid->addWidget(dayLabel, row, col);
    }
    calLayout->addLayout(grid);

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
    
    auto addStat = [&](const QString &label, const QString &val, const QString &color) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *l = new QLabel(label);
        l->setStyleSheet("color: #606266; font-size: 13px; border: none; background: transparent;");
        QLabel *v = new QLabel(val);
        v->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; border: none; background: transparent;").arg(color));
        row->addWidget(l);
        row->addStretch();
        row->addWidget(v);
        mLayout->addLayout(row);
    };
    addStat("正常出勤", "18 天", "#67c23a");
    addStat("迟到/早退", "1 次", "#e6a23c");
    addStat("请假假天", "0 天", "#909399");

    contentLayout->addWidget(todayCard);
    contentLayout->addWidget(monthCalendarCard);
    contentLayout->addWidget(monthCard);
    contentLayout->addStretch();
    scroll->setWidget(content);
    return scroll;
}
void EmployeeDetailDrawer::setEmployee(const EmployeeInfo &info)
{
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
    
    QPixmap pixmap(effectivePath);
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
    painter.drawPixmap(0, 0, pixmap.scaled(avatarSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    m_avatarLabel->setPixmap(target);
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

bool EmployeeDetailDrawer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avatarLabel && event->type() == QEvent::MouseButtonRelease) {
        emit avatarClicked(m_currentEmployee.imgPath);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}
