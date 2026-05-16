#include "memberdetaildrawer.h"
#include "../utils/imageutils.h"
#include <QPainter>
#include <QDate>
#include <QRandomGenerator>
#include <QMouseEvent>
#include <QPainterPath>
#include "petdatamanager.h"
#include "custommessagedialog.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

MemberDetailDrawer::MemberDetailDrawer(QWidget *parent) : QWidget(parent), m_imagePreviewOverlay(nullptr), m_previewLabel(nullptr), m_isOpened(false)

{
    setupUI();
    setupImagePreview();
    setFixedWidth(0);
    
    m_animation = new QPropertyAnimation(this, "sideWidth");
    m_animation->setDuration(300);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
}

void MemberDetailDrawer::setupUI()
{
    setObjectName("MemberDetailDrawer");
    setStyleSheet("#MemberDetailDrawer { background-color: white; border-left: 1px solid #ebeef5; } "
                  "QLabel { border: none; background: transparent; padding: 0; margin: 0; }");

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20); // 统一外边距，和左侧保持一致
    outerLayout->setSpacing(0);

    QFrame *container = new QFrame();
    container->setObjectName("DrawerContainer");
    container->setStyleSheet("#DrawerContainer { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    
    // 开启子控件裁切以保证圆角（关键）
    container->setAttribute(Qt::WA_StyledBackground);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- 占位界面 ---
    m_emptyWidget = new QWidget();
    QVBoxLayout *emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    QLabel *emptyIcon = new QLabel("👤");
    emptyIcon->setStyleSheet("font-size: 48px; color: #dcdfe6;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    QLabel *emptyText = new QLabel("暂无会员数据\n请在左侧列表选择或录入新会员");
    emptyText->setStyleSheet("color: #909399; font-size: 14px; line-height: 1.5;");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyLayout->addStretch();
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addSpacing(20);
    emptyLayout->addWidget(emptyText);
    emptyLayout->addStretch();
    mainLayout->addWidget(m_emptyWidget);

    // --- 内容界面 ---
    m_contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    mainLayout->addWidget(m_contentWidget);
    
    outerLayout->addWidget(container);

    // --- 1. 头部 (Avatar & Base Info) ---
    QWidget *header = new QWidget();
    header->setFixedHeight(200); 
    // 取消渐变，改为纯白，并保持顶部圆角以匹配大容器
    header->setStyleSheet("QWidget { background: white; border-top-left-radius: 12px; border-top-right-radius: 12px; border: none; }");
    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(20, 15, 20, 0); // 将底部边距从 10 减小到 0
    // --- 顶部工具栏 ---
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addStretch();
    QPushButton *closeBtn = new QPushButton("×");
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { border: none; font-size: 24px; color: #909399; background: transparent; } QPushButton:hover { color: #f56c6c; }");
    connect(closeBtn, &QPushButton::clicked, this, [=](){ this->hide(); });
    topBar->addWidget(closeBtn);
    headerLayout->addLayout(topBar);
    headerLayout->addSpacing(4); // 从 10 减小到 4

    QVBoxLayout *nameIdLayout = new QVBoxLayout();
    
    QHBoxLayout *nameRow = new QHBoxLayout();
    m_nameLabel = new QLabel("未选择会员");
    m_nameLabel->setStyleSheet("font-size: 24px; color: #303133; font-weight: bold;"); 
    
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
    
    m_editBtn->hide(); 
    
    nameRow->addWidget(m_nameLabel);
    
    m_idLabel = new QLabel("ID: --");
    m_idLabel->setStyleSheet("color: #94a3b8; font-size: 14px; margin-left: 10px; font-weight: normal;");
    nameRow->addWidget(m_idLabel);

    nameRow->addStretch(); // 重点：推到最右侧
    // m_editBtn 不再加入布局，改用绝对定位
    
    QHBoxLayout *badgeLayout = new QHBoxLayout();
    m_levelLabel = new QLabel("普通会员");
    m_levelLabel->setObjectName("LevelBadge");
    m_levelLabel->setStyleSheet("QLabel#LevelBadge { background: #dcfce7; color: #166534; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; border: 1px solid #dcfce7; }");
    badgeLayout->addWidget(m_levelLabel);
    
    m_statusLabel = new QLabel("正常");
    m_statusLabel->setObjectName("StatusBadge");
    m_statusLabel->setStyleSheet("QLabel#StatusBadge { background: #eff6ff; color: #3b82f6; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; margin-left: 10px; border: 1px solid #dbeafe; }");
    badgeLayout->addWidget(m_statusLabel);
    
    m_petCountLabel = new QLabel("宠物数量: 0");
    m_petCountLabel->setObjectName("PetBadge");
    m_petCountLabel->setStyleSheet("QLabel#PetBadge { background: #ffedd5; color: #9a3412; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; margin-left: 10px; border: 1px solid #ffedd5; }");
    badgeLayout->addWidget(m_petCountLabel);
    badgeLayout->addStretch();
    
    // 核心组装逻辑：确保没有重复 addWidget
    nameIdLayout->addLayout(nameRow);
    nameIdLayout->addSpacing(12); 
    nameIdLayout->addLayout(badgeLayout);
    
    headerLayout->addLayout(nameIdLayout);
    headerLayout->addStretch();

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

    QStringList tabs = {"档案", "宠物", "消费"};
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

    // --- 3. 居中并限制宽度 ---
    QWidget *tabContainer = new QWidget();
    tabContainer->setFixedHeight(60); // 固定高度确保位置一致
    QHBoxLayout *tabContainerLayout = new QHBoxLayout(tabContainer);
    tabContainerLayout->setContentsMargins(0, 10, 0, 10);
    tabContainerLayout->addStretch();
    tabWidget->setFixedWidth(280); // 增加宽度以适应更长的文本
    tabContainerLayout->addWidget(tabWidget);
    tabContainerLayout->addStretch();

    contentLayout->addWidget(header);
    contentLayout->addWidget(tabContainer);

    // --- 3. 内容区 (Stacked Widget) ---
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->setStyleSheet("QStackedWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    m_stackedWidget->addWidget(createProfilePage()); // Index 0
    m_stackedWidget->addWidget(createPetPage());     // Index 1
    m_stackedWidget->addWidget(createOrderPage());   // Index 2

    contentLayout->addWidget(m_stackedWidget);

    connect(m_tabGroup, &QButtonGroup::idClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    
    connect(m_addPetBtn, &QPushButton::clicked, this, [this](){
        emit sig_addPetRequested(m_currentMember.id, m_currentMember.name);
    });

    connect(m_editBtn, &QPushButton::clicked, this, [this](){
        emit sig_editMemberRequested(m_currentMember);
    });

    connect(m_viewMoreVisitBtn, &QPushButton::clicked, this, [this](){
        m_stackedWidget->setCurrentIndex(2); // 切换到“消费”标签页
        m_tabGroup->button(2)->setChecked(true);
    });

    m_tabGroup->button(0)->setChecked(true);

    m_editBtn->setParent(container);
    m_editBtn->move(297, 21);
    m_editBtn->raise();

    showEmptyState(true);
}

void MemberDetailDrawer::showEmptyState(bool empty)
{
    m_emptyWidget->setVisible(empty);
    m_contentWidget->setVisible(!empty);
    m_editBtn->setVisible(!empty);
}

QWidget* MemberDetailDrawer::createProfilePage()
{
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    // 强制 ScrollArea 及其 viewport 均为白色，且底部保持圆角
    scroll->setStyleSheet("QScrollArea { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
                          "QScrollArea > QWidget > QWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    
    QWidget *content = new QWidget();
    content->setStyleSheet("background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(24, 16, 24, 16); // 增加一点左右边距
    contentLayout->setSpacing(20); // 增加分组间的间距

    auto createGroupCard = [&](const QString &title, QVBoxLayout* &cardLayout) {
        QWidget *group = new QWidget();
        QVBoxLayout *groupL = new QVBoxLayout(group);
        groupL->setContentsMargins(0, 0, 0, 0);
        groupL->setSpacing(12);

        QLabel *tLabel = new QLabel(title);
        tLabel->setStyleSheet("color: #334155; font-size: 15px; font-weight: bold; margin-left: 4px;");
        groupL->addWidget(tLabel);

        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; }");
        cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(12);
        
        groupL->addWidget(card);
        return group;
    };

    auto addDetailRow = [&](QVBoxLayout *layout, const QString &label, QLabel* &valLabel) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *titleL = new QLabel(label);
        titleL->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        titleL->setFixedWidth(70);
        valLabel = new QLabel("--");
        valLabel->setStyleSheet("color: #303133; font-size: 13px; border: none; background: transparent;");
        valLabel->setWordWrap(true);
        row->addWidget(titleL);
        row->addWidget(valLabel, 1);
        layout->addLayout(row);
    };

    // 1. 身份信息
    QVBoxLayout *baseL;
    contentLayout->addWidget(createGroupCard("身份与联系", baseL));
    addDetailRow(baseL, "性别", m_valGender);
    addDetailRow(baseL, "生日", m_valBirthday);
    addDetailRow(baseL, "联系电话", m_valPhone);
    addDetailRow(baseL, "当前状态", m_valStatus);

    // 2. 账户资产
    QVBoxLayout *assetL;
    contentLayout->addWidget(createGroupCard("账户资产", assetL));
    addDetailRow(assetL, "会员等级", m_valLevel);
    addDetailRow(assetL, "余额", m_valBalance);
    addDetailRow(assetL, "可用积分", m_valPoints);
    addDetailRow(assetL, "累计消费", m_valTotalConsume);

    // 3. 到店足迹
    QVBoxLayout *recL;
    contentLayout->addWidget(createGroupCard("到店足迹", recL));
    
    m_valLastVisit = new QLabel("--");
    m_valLastVisit->setWordWrap(true);
    m_valLastVisit->setStyleSheet("color: #606266; font-size: 13px; line-height: 1.6; border: none; background: transparent;");
    recL->addWidget(m_valLastVisit);

    m_viewMoreVisitBtn = new QPushButton("查看完整历史记录 >");
    m_viewMoreVisitBtn->setCursor(Qt::PointingHandCursor);
    m_viewMoreVisitBtn->setStyleSheet(
        "QPushButton { color: #409eff; font-size: 12px; border: none; background: transparent; text-align: left; padding: 0; margin-top: 5px; } "
        "QPushButton:hover { color: #66b1ff; text-decoration: underline; }"
    );
    recL->addWidget(m_viewMoreVisitBtn);

    contentLayout->addStretch();
    scroll->setWidget(content);
    return scroll;
}

QWidget* MemberDetailDrawer::createPetPage()
{
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    // 统一背景与圆角
    scroll->setStyleSheet("QScrollArea { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
                          "QScrollArea > QWidget > QWidget { background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    
    QWidget *content = new QWidget();
    content->setStyleSheet("background: white; border: none; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QVBoxLayout *l = new QVBoxLayout(content);
    l->setContentsMargins(24, 16, 24, 16);
    l->setSpacing(15);
    
    // 按钮栏
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_addPetBtn = new QPushButton("新增宠物档案");
    m_addPetBtn->setCursor(Qt::PointingHandCursor);
    m_addPetBtn->setFixedHeight(36);
    m_addPetBtn->setStyleSheet(
        "QPushButton { background: #166534; color: white; border-radius: 6px; font-size: 13px; font-weight: bold; padding: 0 15px; } "
        "QPushButton:hover { background: #15803d; } "
        "QPushButton:pressed { background: #14532d; }"
    );
    btnLayout->addWidget(m_addPetBtn);
    btnLayout->addStretch();
    l->addLayout(btnLayout);

    QFrame *card = new QFrame();
    card->setStyleSheet("background: #fcfcfd; border-radius: 8px; border: 1px solid #ebeef5;");
    QVBoxLayout *cardL = new QVBoxLayout(card);
    
    QLabel *title = new QLabel("关联宠物列表");
    title->setStyleSheet("color: #303133; font-size: 14px; font-weight: bold; border: none; background: transparent;");

    m_petCardsLayout = new QVBoxLayout();
    m_petCardsLayout->setSpacing(12);
    m_petCardsLayout->setContentsMargins(0, 5, 0, 0);
    
    m_petListLabel = new QLabel("暂无宠物信息");
    m_petListLabel->setStyleSheet("color: #909399; font-size: 13px;");
    m_petListLabel->hide(); // 默认隐藏，由 setMember 控制显示
    
    cardL->addWidget(title);
    cardL->addLayout(m_petCardsLayout);
    cardL->addWidget(m_petListLabel);
    cardL->addStretch();
    
    l->addWidget(card);
    l->addStretch();
    
    scroll->setWidget(content);
    return scroll;
}

QWidget* MemberDetailDrawer::createOrderPage()
{
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: white; border: none; }");

    QWidget *content = new QWidget();
    content->setObjectName("OrderPageContent");
    content->setStyleSheet("QWidget#OrderPageContent { background: white; border: none; }");
    m_orderListLayout = new QVBoxLayout(content);
    m_orderListLayout->setContentsMargins(24, 16, 24, 16);
    m_orderListLayout->setSpacing(15);
    
    m_orderEmptyLabel = new QLabel("暂无消费记录");
    m_orderEmptyLabel->setAlignment(Qt::AlignCenter);
    m_orderEmptyLabel->setStyleSheet("color: #94a3b8; font-size: 13px; margin-top: 50px;");
    m_orderListLayout->addWidget(m_orderEmptyLabel);
    m_orderListLayout->addStretch();
    
    scroll->setWidget(content);
    return scroll;
}

void MemberDetailDrawer::setMemberInfo(const MemberInfo &info)
{
    m_currentMember = info;
    
    // 如果没有数据，显示占位状态
    if (info.id.isEmpty()) {
        m_nameLabel->setText("暂无会员数据");
        m_idLabel->setText("");
        // 使用头文件中定义的正确变量名
        if (m_levelLabel) m_levelLabel->hide();
        if (m_statusLabel) m_statusLabel->hide();
        // 隐藏或清空其他详细字段
        for (auto *label : findChildren<QLabel*>()) {
            if (label->property("isField").toBool()) label->setText("--");
        }
        return;
    }

    if (m_levelLabel) m_levelLabel->show();
    if (m_statusLabel) m_statusLabel->show();
    m_nameLabel->setText(info.name);
}

void MemberDetailDrawer::setMember(const MemberInfo &info, const QString &lastVisit, const QString &pets)
{
    showEmptyState(false);
    Q_UNUSED(lastVisit);
    Q_UNUSED(pets);
    m_currentMember = info;
    m_nameLabel->setText(info.name);
    m_editBtn->show();
    
    // 更新等级颜色
    if (info.level.contains("黄金")) {
        m_levelLabel->setStyleSheet("background: #ffedd5; color: #9a3412; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    } else if (info.level.contains("铂金")) {
        m_levelLabel->setStyleSheet("background: #e0f2fe; color: #0369a1; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    } else if (info.level.contains("钻石")) {
        m_levelLabel->setStyleSheet("background: #f3e8ff; color: #7e22ce; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    } else {
        m_levelLabel->setStyleSheet("background: #dcfce7; color: #166534; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    }
    m_levelLabel->setText(info.level);

    m_idLabel->setText("ID: " + info.id);

    // 更新状态标签样式
    if (info.status == "已注销") {
        m_statusLabel->setStyleSheet("background: #f1f5f9; color: #64748b; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; margin-left: 10px; border: 1px solid #e2e8f0;");
    } else {
        m_statusLabel->setStyleSheet("background: #eff6ff; color: #3b82f6; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; margin-left: 10px; border: 1px solid #dbeafe;");
    }
    m_statusLabel->setText(info.status);
    if (m_valStatus) m_valStatus->setText(info.status);
    
    // 统计宠物数量并更新标签（从 PetDataManager 动态获取最新数据）
    int count = 0;
    QList<PetInfo> allPets = PetDataManager::instance()->allPets();
    for(const auto &pet : allPets) {
        if (pet.ownerId == info.id) count++;
    }
    m_petCountLabel->setText(QString("宠物数量: %1").arg(count));
    // 如果没有宠物，使用中性色；有宠物使用温暖的橙色
    if(count == 0) {
        m_petCountLabel->setStyleSheet("QLabel#PetBadge { background: #f1f5f9; color: #64748b; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; margin-left: 10px; border: 1px solid #f1f5f9; }");
    } else {
        m_petCountLabel->setStyleSheet("QLabel#PetBadge { background: #ffedd5; color: #9a3412; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; margin-left: 10px; border: 1px solid #ffedd5; }");
    }

    m_valGender->setText(info.gender.isEmpty() ? "未知" : info.gender);
    m_valBirthday->setText(info.birthday.isEmpty() ? "未填写" : info.birthday);
    m_valPhone->setText(info.phone);
    m_valLevel->setText(info.level);
    m_valBalance->setText(QString("¥ %1").arg(info.balance, 0, 'f', 2));
    m_valPoints->setText(QString::number(info.points));
    m_valTotalConsume->setText(QString("¥ %1").arg(info.consume_amt, 0, 'f', 2));

    // --- 真实到店记录 (从 PetDataManager 获取该会员的所有已支付订单) ---
    QList<OrderInfo> allOrders = PetDataManager::instance()->getOrders(QDate(2020, 1, 1), QDate::currentDate(), "", "全部");
    QList<OrderInfo> memberOrders;
    for (const auto &o : allOrders) {
        if (o.memberId == info.id && o.status == "Paid") {
            memberOrders.append(o);
        }
    }

    QString historyHtml = "";
    if (memberOrders.isEmpty()) {
        historyHtml = "<div style='color: #94a3b8; font-style: italic; margin-top: 15px; text-align: center;'>暂无到店足迹</div>";
    } else {
        // 取最近的 5 条记录
        int displayCount = qMin(5, (int)memberOrders.size());
        for (int i = 0; i < displayCount; ++i) {
            const auto &o = memberOrders[i];
            QString dateStr = o.createTime.left(10);
            QString timeStr = o.createTime.mid(11, 5);
            
            // 1. 确定业务类型与颜色
            QString accentColor = "#94a3b8"; // 默认灰色
            QString typeName = "其他消费";
            if (o.sourceModule == "Boarding") {
                accentColor = "#f59e0b"; // 琥珀色
                typeName = "寄养服务";
            } else if (o.sourceModule == "Appointment") {
                accentColor = "#3b82f6"; // 蓝色
                typeName = "洗护/美容";
            } else if (o.sourceModule == "Product") {
                accentColor = "#10b981"; // 绿色
                typeName = "商品零售";
            } else if (o.sourceModule == "Transport") {
                accentColor = "#8b5cf6"; // 紫色
                typeName = "接送服务";
            }
            
            // 2. 解析明细摘要 (统一处理 JSON 或 纯文本)
            QString summary = "";
            QJsonDocument itemDoc = QJsonDocument::fromJson(o.itemDetails.toUtf8());
            if (itemDoc.isArray()) {
                QJsonArray items = itemDoc.array();
                QStringList itemNames;
                for (int j = 0; j < items.size(); ++j) {
                    QJsonObject itemObj = items[j].toObject();
                    QString name = itemObj["name"].toString();
                    QString petName = itemObj["petName"].toString();
                    
                    if (o.sourceModule == "Product") {
                        int count = itemObj.contains("count") ? itemObj["count"].toInt() : 1;
                        name = QString("%1 <span style='color:#94a3b8;font-size:11px;'>x%2</span>").arg(name).arg(count);
                    }
                    
                    if (!petName.isEmpty()) {
                        itemNames << QString("%1 <span style='color:#94a3b8;font-weight:normal;'>(%2)</span>").arg(name, petName);
                    } else {
                        itemNames << name;
                    }
                }
                summary = itemNames.join(", ");
            } else {
                summary = o.itemDetails;
                if (summary.contains("+")) summary = summary.split("+").first();
            }
            if (summary.isEmpty()) summary = "业务结算";
            if (summary.length() > 40) summary = summary.left(37) + "...";

            // 3. 构造模版 HTML (使用带背景色的 table 形成卡片包裹视觉)
            QString itemHtml = QString(
                "<table width='100%' cellpadding='10' cellspacing='0' bgcolor='#f8fafc' style='border: 1px solid #f1f5f9;'>"
                "  <tr>"
                "    <td colspan='2' style='padding-bottom: 0px;'>"
                "      <span style='color: %1;'>●</span>"
                "      <span style='color: #64748b; font-weight: bold; font-size: 11px; text-transform: uppercase;'>&nbsp;%2</span>"
                "      %3"
                "    </td>"
                "  </tr>"
                "  <tr>"
                "    <td colspan='2' style='color: #1e293b; font-size: 13px; font-weight: 600; padding-top: 6px; padding-bottom: 8px;'>%4</td>"
                "  </tr>"
                "  <tr>"
                "    <td style='color: #94a3b8; font-size: 11px;'>%5 &nbsp;%6</td>"
                "    <td align='right' style='color: #1e293b; font-weight: 800; font-size: 13px;'>¥ %7</td>"
                "  </tr>"
                "</table>"
                "<div style='height: 12px;'></div>" // 卡片之间的间隔
            ).arg(
                accentColor, typeName,
                (i == 0 ? "<span style='font-size: 9px; color: #3b82f6; font-weight: 800; margin-left: 8px;'>NEW</span>" : ""),
                summary,
                dateStr, timeStr,
                QString::number(o.finalAmount, 'f', 2)
            );
            historyHtml += itemHtml;
            if (i == 0) m_valLastVisit->setText(dateStr);
        }
    }
    
    m_valLastVisit->setText(historyHtml);
    m_valLastVisit->setTextFormat(Qt::RichText);

    // --- 4. 更新“消费”标签页的完整列表 ---
    // 清理旧项
    QLayoutItem *orderChild;
    while ((orderChild = m_orderListLayout->takeAt(0)) != nullptr) {
        if (orderChild->widget()) delete orderChild->widget();
        delete orderChild;
    }
    
    if (memberOrders.isEmpty()) {
        m_orderEmptyLabel = new QLabel("暂无消费记录");
        m_orderEmptyLabel->setAlignment(Qt::AlignCenter);
        m_orderEmptyLabel->setStyleSheet("color: #94a3b8; font-size: 13px; margin-top: 50px;");
        m_orderListLayout->addWidget(m_orderEmptyLabel);
    } else {
        // 显示所有订单
        for (const auto &o : memberOrders) {
            QString dateStr = o.createTime.left(10);
            QString timeStr = o.createTime.mid(11, 5);
            
            QString accentColor = "#94a3b8";
            QString typeName = "其他消费";
            if (o.sourceModule == "Boarding") { accentColor = "#f59e0b"; typeName = "寄养服务"; }
            else if (o.sourceModule == "Appointment") { accentColor = "#3b82f6"; typeName = "洗护/美容"; }
            else if (o.sourceModule == "Product") { accentColor = "#10b981"; typeName = "商品零售"; }
            else if (o.sourceModule == "Transport") { accentColor = "#8b5cf6"; typeName = "接送服务"; }
            
            QFrame *oCard = new QFrame();
            oCard->setStyleSheet("QFrame { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 12px; }");
            QVBoxLayout *oL = new QVBoxLayout(oCard);
            oL->setContentsMargins(16, 14, 16, 14);
            oL->setSpacing(8);
            
            // 1. 顶部行：业务类型 + 金额 (金额放在右边)
            QHBoxLayout *topRow = new QHBoxLayout();
            topRow->setAlignment(Qt::AlignVCenter);
            
            QLabel *iconL = new QLabel();
            iconL->setFixedSize(8, 8);
            iconL->setStyleSheet(QString("background: %1; border-radius: 4px; border: none;").arg(accentColor));
            
            QLabel *titleL = new QLabel(typeName);
            titleL->setStyleSheet("color: #1e3a8a; font-weight: 900; font-size: 14px; background: transparent; border: none;");
            
            QLabel *orderNoL = new QLabel("#" + o.id.right(8));
            orderNoL->setStyleSheet("color: #94a3b8; font-size: 11px; background: transparent; border: none; margin-left: 8px;");
            
            topRow->addWidget(iconL);
            topRow->addSpacing(6);
            topRow->addWidget(titleL);
            topRow->addStretch();
            
            // 状态标签 (QLabel with proper layout padding)
            QLabel *statusL = new QLabel();
            if (o.status == "Paid") {
                statusL->setText("已结清");
                statusL->setStyleSheet("background: #dcfce7; color: #166534; padding: 4px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; border: none;");
            } else if (o.status == "Unpaid") {
                statusL->setText("待结算");
                statusL->setStyleSheet("background: #fef3c7; color: #b45309; padding: 4px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; border: none;");
            } else {
                statusL->setText("已作废");
                statusL->setStyleSheet("background: #f1f5f9; color: #64748b; padding: 4px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; border: none;");
            }
            topRow->addWidget(statusL);
            topRow->addSpacing(8);

            QLabel *amtL = new QLabel(QString("¥ %1").arg(o.finalAmount, 0, 'f', 2));
            amtL->setStyleSheet("color: #1d4ed8; font-weight: 900; font-size: 18px; background: transparent; border: none;");
            topRow->addWidget(amtL);
            
            oL->addLayout(topRow);
            
            // 2. 中间行：消费明细 (包含宠物名称)
            QJsonDocument doc = QJsonDocument::fromJson(o.itemDetails.toUtf8());
            int totalDays = 0;
            if (doc.isArray()) {
                QJsonArray arr = doc.array();
                for (int j=0; j<arr.size(); ++j) {
                    QJsonObject obj = arr[j].toObject();
                    QString name = obj["name"].toString();
                    QString petName = obj["petName"].toString();
                    int count = obj["count"].toInt();
                    if (obj.contains("duration")) {
                        totalDays = qMax(totalDays, obj["duration"].toInt());
                    }
                    
                    QHBoxLayout *itemRow = new QHBoxLayout();
                    itemRow->setAlignment(Qt::AlignVCenter);
                    itemRow->setSpacing(8);
                    
                    QLabel *nameL = new QLabel(name);
                    nameL->setStyleSheet("color: #475569; font-size: 13px; font-weight: 500; background: transparent; border: none;");
                    itemRow->addWidget(nameL);
                    
                    if (!petName.isEmpty()) {
                        QLabel *petL = new QLabel(petName);
                        petL->setStyleSheet("background: #e0e7ff; color: #4f46e5; padding: 2px 6px; border-radius: 4px; font-size: 11px; border: none;");
                        itemRow->addWidget(petL);
                    }
                    
                    itemRow->addStretch();
                    
                    if (o.sourceModule == "Product") {
                        int displayCount = (count > 0) ? count : 1;
                        QLabel *countL = new QLabel(QString("x%1").arg(displayCount));
                        countL->setStyleSheet("color: #94a3b8; font-size: 13px; font-weight: bold; background: transparent; border: none;");
                        itemRow->addWidget(countL);
                    }
                    
                    oL->addLayout(itemRow);
                }
            } else {
                QLabel *bodyL = new QLabel(o.itemDetails);
                bodyL->setStyleSheet("color: #475569; font-size: 13px; font-weight: 500; background: transparent; border: none;");
                bodyL->setWordWrap(true);
                oL->addWidget(bodyL);
            }
            
            // 3. 底部行：日期时间 + 订单号
            QHBoxLayout *bottomRow = new QHBoxLayout();
            bottomRow->setContentsMargins(0, 0, 0, 0);
            bottomRow->setAlignment(Qt::AlignVCenter);

            QString dateInfo = QString("%1 %2").arg(dateStr, timeStr);
            if (o.sourceModule == "Boarding" && totalDays > 0) {
                QDateTime endDt = QDateTime::fromString(o.createTime, "yyyy-MM-dd HH:mm:ss");
                QDateTime startDt = endDt.addDays(-totalDays);
                dateInfo = QString("入店: %1 &nbsp;&nbsp; 离店: %2 <span style='color: #94a3b8;'>(共 %3 天)</span>")
                            .arg(startDt.toString("yyyy-MM-dd"), endDt.toString("yyyy-MM-dd")).arg(totalDays);
            }

            QLabel *dateL = new QLabel(dateInfo);
            dateL->setStyleSheet("color: #94a3b8; font-size: 11px; background: transparent; border: none;");
            
            bottomRow->addWidget(dateL);
            bottomRow->addStretch();
            bottomRow->addWidget(orderNoL);
            
            oL->addLayout(bottomRow);
            
            m_orderListLayout->addWidget(oCard);
        }
    }
    m_orderListLayout->addStretch();

    // --- 更新宠物卡片列表 ---
    QLayoutItem *child;
    while ((child = m_petCardsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    // 从 PetDataManager 获取该会员真实的宠物列表
    QList<PetInfo> realPets = PetDataManager::instance()->getPetsByOwner(info.id);

    if (realPets.isEmpty()) {
        m_petListLabel->show();
    } else {
        m_petListLabel->hide();
        for (const PetInfo &pInfo : realPets) {
            // 创建卡片容器
            QFrame *pCard = new QFrame();
            pCard->setProperty("petId", pInfo.id); 
            pCard->setProperty("petName", pInfo.name); // 存储姓名用于提示框
            pCard->setObjectName("PetCard");
            pCard->setCursor(Qt::PointingHandCursor);
            pCard->setStyleSheet("QFrame#PetCard { background: #ffffff; border: 1px solid #f0f2f5; border-radius: 10px; } "
                                 "QFrame#PetCard:hover { border-color: #409eff; background: #fcfdfe; }");
            pCard->installEventFilter(this);

            QHBoxLayout *pLayout = new QHBoxLayout(pCard);
            pLayout->setContentsMargins(12, 12, 12, 12);
            pLayout->setSpacing(15);

            // A. 头像 (真实路径)
            QLabel *pAvatar = new QLabel();
            pAvatar->setFixedSize(54, 54);
            pAvatar->setProperty("avatarPath", pInfo.avatarPath); // 存储路径用于放大
            pAvatar->setStyleSheet("border: none; background: transparent;");
            pAvatar->installEventFilter(this);
            
            QPixmap srcPix = ImageUtils::loadPixmap(pInfo.avatarPath);
            if (srcPix.isNull()) srcPix.load(":/images/load_img.jpg");
            
            QSize size(54, 54);
            QPixmap target(size);
            target.fill(Qt::transparent);
            QPainter painter(&target);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            QPainterPath path;
            path.addEllipse(0, 0, size.width(), size.height());
            painter.setClipPath(path);
            
            QPixmap scaled = srcPix.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            int x = (size.width() - scaled.width()) / 2;
            int y = (size.height() - scaled.height()) / 2;
            painter.drawPixmap(x, y, scaled);
            pAvatar->setPixmap(target);
            
            // B. 信息列
            QVBoxLayout *pInfoCol = new QVBoxLayout();
            pInfoCol->setSpacing(4);

            // 第一行：姓名 + 性别
            QHBoxLayout *pTitleRow = new QHBoxLayout();
            QLabel *pNameL = new QLabel(pInfo.name);
            pNameL->setStyleSheet("font-size: 14px; font-weight: bold; color: #303133; border:none;");
            
            QLabel *pGenderL = new QLabel(pInfo.gender == "公" ? "♂" : "♀");
            pGenderL->setFixedSize(22, 22);
            pGenderL->setAlignment(Qt::AlignCenter);
            pGenderL->setStyleSheet(pInfo.gender == "公" ? 
                "color: #409eff; background: #ecf5ff; border: 1px solid #d9ecff; border-radius: 11px; font-weight: bold; font-size: 14px;" : 
                "color: #f56c6c; background: #fef0f0; border: 1px solid #fde2e2; border-radius: 11px; font-weight: bold; font-size: 14px;");
            
            pTitleRow->addWidget(pNameL);
            pTitleRow->addSpacing(5);
            pTitleRow->addWidget(pGenderL);
            pTitleRow->addStretch();
            
            // 第二行：ID + 品种
            QLabel *pIdLabel = new QLabel(QString("ID: %1  |  %2").arg(pInfo.id, pInfo.breed));
            pIdLabel->setStyleSheet("color: #909399; font-size: 11px; border: none; background: transparent;");
            
            // 第三行：状态 + 房间号
            QHBoxLayout *pStatusRow = new QHBoxLayout();
            QString statusStyle;
            bool isFostering = (pInfo.status == "在店 (寄养中)");

            if (isFostering) {
                statusStyle = "background: #ffedd5; color: #9a3412;";
            } else if (pInfo.status.contains("在店")) {
                statusStyle = "background: #dcfce7; color: #166534;";
            } else {
                statusStyle = "background: #f1f5f9; color: #64748b;";
            }

            QLabel *pStatusL = new QLabel(pInfo.status.split(" ").first());
            pStatusL->setStyleSheet(statusStyle + "padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
            pStatusRow->addWidget(pStatusL);

            if (isFostering) {
                // 模拟房间号，后续可从 pInfo 扩展字段
                QLabel *pRoomL = new QLabel("房间: B-102"); 
                pRoomL->setStyleSheet("background: #e0f2fe; color: #0369a1; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; margin-left: 5px;");
                pStatusRow->addWidget(pRoomL);
            }
            pStatusRow->addStretch();

            pInfoCol->addLayout(pTitleRow);
            pInfoCol->addWidget(pIdLabel);
            pInfoCol->addLayout(pStatusRow);

            pLayout->addWidget(pAvatar);
            pLayout->addLayout(pInfoCol);
            pLayout->addStretch();

            m_petCardsLayout->addWidget(pCard);
        }
    }
}

void MemberDetailDrawer::setupImagePreview()
{
    QWidget *win = this->window();
    if (!win) return;
    m_imagePreviewOverlay = new QWidget(win); // 挂载到顶层窗口
    m_imagePreviewOverlay->setObjectName("DrawerPreviewOverlay");
    m_imagePreviewOverlay->setStyleSheet("#DrawerPreviewOverlay { background-color: rgba(0, 0, 0, 215); }");
    m_imagePreviewOverlay->hide();
    m_imagePreviewOverlay->installEventFilter(this);
    
    QVBoxLayout *previewL = new QVBoxLayout(m_imagePreviewOverlay);
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    previewL->addWidget(m_previewLabel);
}

void MemberDetailDrawer::showBigImage(const QString &path)
{
    if (path.isEmpty()) return;
    
    // 延迟初始化：只有在真正需要显示且 window() 可用时才创建
    if (!m_imagePreviewOverlay) {
        setupImagePreview();
    }
    
    if (!m_imagePreviewOverlay || !m_previewLabel) return;
    
    m_currentPreviewPath = path;
    QPixmap pix = ImageUtils::loadPixmap(path);
    if (!pix.isNull()) {
        m_previewLabel->setPixmap(pix.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        QWidget *win = this->window();
        if (win) {
            m_imagePreviewOverlay->setGeometry(win->rect());
            m_imagePreviewOverlay->show();
            m_imagePreviewOverlay->raise();
        }
    }
}

void MemberDetailDrawer::hideBigImage()
{
    if (m_imagePreviewOverlay) {
        m_imagePreviewOverlay->hide();
    }
}

bool MemberDetailDrawer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        // 1. 点击头像放大
        if (obj->inherits("QLabel") && !obj->property("avatarPath").toString().isEmpty()) {
            showBigImage(obj->property("avatarPath").toString());
            return true;
        }
        
        // 2. 点击卡片跳转
        if (obj->objectName() == "PetCard") {
            QString petId = obj->property("petId").toString();
            QString petName = obj->property("petName").toString(); 
            
            if (CustomMessageDialog::confirm(this, "模块跳转", 
                QString("是否跳转到宠物模块并查看宠物 [ %1 ] 的详细信息？").arg(petName))) 
            {
                emit sig_jumpToPetRequested(petId);
            }
            return true;
        }
        
        // 3. 点击遮罩层关闭预览
        if (obj == m_imagePreviewOverlay) {
            hideBigImage();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void MemberDetailDrawer::updateBalance(double newBalance)
{
    m_currentMember.balance = newBalance;
    if (m_valBalance) {
        m_valBalance->setText(QString("¥ %1").arg(newBalance, 0, 'f', 2));
    }
}

void MemberDetailDrawer::showDrawer()
{
    if (m_isOpened) return;
    m_isOpened = true;
    m_animation->stop();
    m_animation->setStartValue(width());
    m_animation->setEndValue(450); 
    m_animation->start();
}
 
void MemberDetailDrawer::hideDrawer()
{
    m_isOpened = false;
    m_animation->stop();
    m_animation->setStartValue(width());
    m_animation->setEndValue(0);
    m_animation->start();
}
