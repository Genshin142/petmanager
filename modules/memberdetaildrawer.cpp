#include "memberdetaildrawer.h"
#include <QPainter>
#include <QDate>
#include <QRandomGenerator>
#include <QMouseEvent>
#include <QPainterPath>
#include "petdatamanager.h"
#include "custommessagedialog.h"

MemberDetailDrawer::MemberDetailDrawer(QWidget *parent) : QWidget(parent), m_isOpened(true)
{
    setupUI();
    setupImagePreview();
    setFixedWidth(380);
}

void MemberDetailDrawer::setupUI()
{
    setObjectName("MemberDetailDrawer");
    setStyleSheet("#MemberDetailDrawer { background-color: white; border-left: 1px solid #ebeef5; } "
                  "QLabel { border: none; background: transparent; padding: 0; margin: 0; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- 1. 头部 (Avatar & Base Info) ---
    QWidget *header = new QWidget();
    header->setFixedHeight(160); // 增加总高度
    header->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #ffffff, stop:1 #f8faff);");
    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(20, 45, 20, 10); // 大幅增加顶部边距，使内容下沉

    QVBoxLayout *nameIdLayout = new QVBoxLayout();
    m_nameLabel = new QLabel("未选择会员");
    m_nameLabel->setStyleSheet("font-size: 22px; color: #303133; font-weight: bold;"); // 姓名也稍微加大
    
    QHBoxLayout *badgeLayout = new QHBoxLayout();
    m_levelLabel = new QLabel("普通会员");
    m_levelLabel->setStyleSheet("background: #f0f9eb; color: #67c23a; padding: 4px 10px; border-radius: 4px; font-size: 13px; font-weight: bold;");
    
    m_idLabel = new QLabel("ID: --");
    m_idLabel->setStyleSheet("color: #909399; font-size: 13px; margin-left: 10px;");
    
    badgeLayout->addWidget(m_levelLabel);
    badgeLayout->addWidget(m_idLabel);
    
    m_petCountLabel = new QLabel("宠物资产: 0");
    m_petCountLabel->setStyleSheet("background: #fdf6ec; color: #e6a23c; padding: 4px 10px; border-radius: 4px; font-size: 13px; font-weight: bold; margin-left: 10px;");
    badgeLayout->addWidget(m_petCountLabel);
    
    badgeLayout->addStretch();
    
    nameIdLayout->addWidget(m_nameLabel);
    nameIdLayout->addSpacing(15); // 增加姓名与标签之间的距离
    nameIdLayout->addLayout(badgeLayout);
    
    headerLayout->addLayout(nameIdLayout);
    headerLayout->addStretch();

    // --- 2. 导航栏 (Tab Bar) ---
    QWidget *tabWidget = new QWidget();
    tabWidget->setFixedHeight(46);
    tabWidget->setStyleSheet("border-bottom: 1px solid #ebeef5; background: white;");
    QHBoxLayout *tabLayout = new QHBoxLayout(tabWidget);
    tabLayout->setContentsMargins(15, 0, 15, 0);
    tabLayout->setSpacing(10);

    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->setExclusive(true);

    QStringList tabs = {"档案", "宠物", "消费"};
    for (int i = 0; i < tabs.size(); ++i) {
        QPushButton *btn = new QPushButton(tabs[i]);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(44);
        btn->setMinimumWidth(70);
        btn->setStyleSheet(
            "QPushButton { border: none; font-size: 14px; color: #606266; background: transparent; padding: 0 5px 4px 5px; } "
            "QPushButton:hover { color: #409eff; } "
            "QPushButton:checked { color: #409eff; font-weight: bold; border-bottom: 3px solid #409eff; padding-bottom: 1px; }"
        );
        m_tabGroup->addButton(btn, i);
        tabLayout->addWidget(btn);
    }

    mainLayout->addWidget(header);
    mainLayout->addWidget(tabWidget);

    // --- 3. 内容区 (Stacked Widget) ---
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->addWidget(createProfilePage()); // Index 0
    m_stackedWidget->addWidget(createPetPage());     // Index 1
    m_stackedWidget->addWidget(createOrderPage());   // Index 2

    mainLayout->addWidget(m_stackedWidget);

    connect(m_tabGroup, &QButtonGroup::idClicked, m_stackedWidget, &QStackedWidget::setCurrentIndex);
    
    connect(m_addPetBtn, &QPushButton::clicked, this, [this](){
        emit sig_addPetRequested(m_currentMember.id, m_currentMember.name);
    });

    m_tabGroup->button(0)->setChecked(true);
}

QWidget* MemberDetailDrawer::createProfilePage()
{
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; } ");
    
    QWidget *content = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(16);

    auto createGroupCard = [&](const QString &title, QVBoxLayout* &cardLayout) {
        QFrame *card = new QFrame();
        card->setStyleSheet("background: #fcfcfd; border-radius: 8px; border: 1px solid #ebeef5;");
        QVBoxLayout *mainV = new QVBoxLayout(card);
        mainV->setContentsMargins(15, 15, 15, 15);
        mainV->setSpacing(12);
        
        QLabel *tLabel = new QLabel(title);
        tLabel->setStyleSheet("color: #303133; font-size: 14px; font-weight: bold; margin-bottom: 5px; border: none; background: transparent;");
        mainV->addWidget(tLabel);
        
        cardLayout = new QVBoxLayout();
        cardLayout->setSpacing(10);
        mainV->addLayout(cardLayout);
        return card;
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

    // 1. 身份信息
    QVBoxLayout *baseL;
    contentLayout->addWidget(createGroupCard("身份与联系", baseL));
    addDetailRow(baseL, "性别", m_valGender);
    addDetailRow(baseL, "生日", m_valBirthday);
    addDetailRow(baseL, "联系电话", m_valPhone);

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
    
    QWidget *content = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(content);
    l->setContentsMargins(16, 16, 16, 16);
    l->setSpacing(15);
    
    // 按钮栏
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_addPetBtn = new QPushButton("+ 新增宠物档案");
    m_addPetBtn->setCursor(Qt::PointingHandCursor);
    m_addPetBtn->setFixedHeight(36);
    m_addPetBtn->setStyleSheet(
        "QPushButton { background: #67c23a; color: white; border-radius: 6px; font-size: 13px; font-weight: bold; padding: 0 15px; } "
        "QPushButton:hover { background: #85ce61; } "
        "QPushButton:pressed { background: #5daf34; }"
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
    QWidget *w = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(16, 16, 16, 16);
    
    QLabel *empty = new QLabel("暂无消费记录");
    empty->setAlignment(Qt::AlignCenter);
    empty->setStyleSheet("color: #909399; font-size: 13px;");
    l->addWidget(empty);
    return w;
}

void MemberDetailDrawer::setMember(const MemberInfo &info, const QString &lastVisit, const QString &pets)
{
    m_currentMember = info;
    
    m_nameLabel->setText(info.name);
    
    // 更新等级颜色
    if (info.level.contains("黄金")) {
        m_levelLabel->setStyleSheet("background: #fdf6ec; color: #e6a23c; padding: 4px 10px; border-radius: 4px; font-size: 13px; font-weight: bold;");
    } else if (info.level.contains("铂金")) {
        m_levelLabel->setStyleSheet("background: #ecf5ff; color: #409eff; padding: 4px 10px; border-radius: 4px; font-size: 13px; font-weight: bold;");
    } else if (info.level.contains("钻石")) {
        m_levelLabel->setStyleSheet("background: #f0f7ff; color: #a066ff; padding: 4px 10px; border-radius: 4px; font-size: 13px; font-weight: bold;");
    } else {
        m_levelLabel->setStyleSheet("background: #f0f9eb; color: #67c23a; padding: 4px 10px; border-radius: 4px; font-size: 13px; font-weight: bold;");
    }
    m_levelLabel->setText(info.level);

    m_idLabel->setText("ID: " + info.id);
    
    // 统计宠物数量并更新标签
    QStringList petList = pets.split("/", Qt::SkipEmptyParts);
    int count = 0;
    for(const QString &p : petList) {
        if(!p.trimmed().isEmpty() && p.trimmed() != "暂无") count++;
    }
    m_petCountLabel->setText(QString("宠物资产: %1").arg(count));
    // 如果没有宠物，使用中性色；有宠物使用温暖的橙色
    if(count == 0) {
        m_petCountLabel->setStyleSheet("background: #f4f4f5; color: #909399; padding: 4px 10px; border-radius: 4px; font-size: 13px; font-weight: bold; margin-left: 10px;");
    } else {
        m_petCountLabel->setStyleSheet("background: #fdf6ec; color: #e6a23c; padding: 4px 10px; border-radius: 4px; font-size: 13px; font-weight: bold; margin-left: 10px;");
    }

    m_valGender->setText(info.gender);
    m_valBirthday->setText(info.birthday);
    m_valPhone->setText(info.phone);
    m_valLevel->setText(info.level);
    m_valBalance->setText(QString("¥ %1").arg(info.balance, 0, 'f', 2));
    m_valPoints->setText(QString::number(info.points));
    m_valTotalConsume->setText(QString("¥ %1").arg(info.consume_amt, 0, 'f', 2));

    // --- 模拟生成历史到店记录 (基于最后到店日期进行回溯) ---
    QDate lastDate = QDate::fromString(lastVisit, "yyyy-MM-dd");
    if (!lastDate.isValid()) lastDate = QDate::currentDate();

    QString historyHtml = "";
    for (int i = 0; i < 4; ++i) {
        QDate historicalDate = lastDate.addDays(- (int)QRandomGenerator::global()->bounded(30) - (i * 30));
        QString color = (i == 0) ? "#409eff" : "#606266"; // 调深历史记录颜色，增强对比
        QString weight = (i == 0) ? "bold" : "normal";
        QString label = (i == 0) ? " <span style='font-size: 10px; background: #ecf5ff; padding: 1px 4px; border-radius: 2px;'>[最新]</span>" : "";
        
        historyHtml += QString("<div style='margin-bottom: 10px; color: %1;'> "
                               "<span style='font-weight: %2;'>• %3</span>%4"
                               "</div>")
                       .arg(color, weight, historicalDate.toString("yyyy-MM-dd"), label);
        
        if (i == 0) m_valLastVisit->setText(lastVisit);
    }
    m_valLastVisit->setText(historyHtml);
    m_valLastVisit->setTextFormat(Qt::RichText);
    
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
            
            QPixmap pix(pInfo.avatarPath);
            if (pix.isNull()) pix.load(":/images/load_img.jpg");
            
            QSize size(54, 54);
            QPixmap target(size);
            target.fill(Qt::transparent);
            QPainter painter(&target);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addEllipse(0, 0, size.width(), size.height());
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, size.width(), size.height(), pix.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            pAvatar->setPixmap(target);
            
            // B. 信息列
            QVBoxLayout *pInfoCol = new QVBoxLayout();
            pInfoCol->setSpacing(4);

            // 第一行：姓名 + 性别
            QHBoxLayout *pTitleRow = new QHBoxLayout();
            QLabel *pNameL = new QLabel(pInfo.name);
            pNameL->setStyleSheet("font-size: 14px; font-weight: bold; color: #303133; border:none;");
            
            QLabel *pGenderL = new QLabel(pInfo.gender == "公" ? "♂" : "♀");
            pGenderL->setStyleSheet(pInfo.gender == "公" ? "color: #409eff; font-weight: bold;" : "color: #f56c6c; font-weight: bold;");
            
            pTitleRow->addWidget(pNameL);
            pTitleRow->addWidget(pGenderL);
            pTitleRow->addStretch();
            
            // 第二行：ID
            QLabel *pIdLabel = new QLabel("ID: " + pInfo.id);
            pIdLabel->setStyleSheet("color: #909399; font-size: 11px; border:none;");
            
            // 第三行：状态 + 房间号
            QHBoxLayout *pStatusRow = new QHBoxLayout();
            QString statusStyle;
            bool isFostering = (pInfo.status == "在店 (寄养中)");

            if (isFostering) {
                statusStyle = "background: #fdf6ec; color: #e6a23c; border: 1px solid #f5dab1;";
            } else if (pInfo.status.contains("在店")) {
                statusStyle = "background: #f0f9eb; color: #67c23a; border: 1px solid #e1f3d8;";
            } else {
                statusStyle = "background: #f4f4f5; color: #909399; border: 1px solid #e9e9eb;";
            }

            QLabel *pStatusL = new QLabel(pInfo.status.split(" ").first());
            pStatusL->setStyleSheet(statusStyle + "padding: 1px 6px; border-radius: 3px; font-size: 10px; font-weight: bold;");
            pStatusRow->addWidget(pStatusL);

            if (isFostering) {
                // 模拟房间号，后续可从 pInfo 扩展字段
                QLabel *pRoomL = new QLabel("房间: B-102"); 
                pRoomL->setStyleSheet("background: #ecf5ff; color: #409eff; padding: 1px 6px; border-radius: 3px; font-size: 10px; font-weight: bold; margin-left: 5px;");
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
    m_imagePreviewOverlay = new QWidget(this->window()); // 挂载到顶层窗口
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
    QPixmap pix(path);
    if (pix.isNull()) pix.load(":/images/load_img.jpg");
    
    m_imagePreviewOverlay->setGeometry(this->window()->rect());
    
    int maxDim = qMin(this->window()->width(), this->window()->height()) * 0.8;
    m_previewLabel->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    m_imagePreviewOverlay->show();
    m_imagePreviewOverlay->raise();
}

void MemberDetailDrawer::hideBigImage()
{
    m_imagePreviewOverlay->hide();
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

void MemberDetailDrawer::showDrawer()
{
    m_isOpened = true;
    setFixedWidth(380);
}

void MemberDetailDrawer::hideDrawer()
{
    // If you want it always visible, hideDrawer could do nothing
    // but for flexibility we keep it as an instant close if needed
    m_isOpened = false;
    setFixedWidth(0);
}
