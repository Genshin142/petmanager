#include "petmodule.h"
#include "petdatamanager.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QLineEdit>
#include <QComboBox>
#include <QFrame>
#include <QAbstractItemView>
#include <QFont>
#include <QColor>
#include "addpetdialog.h"
#include "custommessagedialog.h"
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QCheckBox>
#include <QFile>
#include <QToolTip>
#include <QApplication>
#include <QHelpEvent>
#include <QStyledItemDelegate>

PetModule::PetModule(QWidget *parent) : QWidget(parent), m_currentPage(1), m_pageSize(10)
{
    m_drawer = new PetRecordDrawer(this);
    connect(m_drawer, &PetRecordDrawer::logAdded, this, &PetModule::onLogAdded);
    connect(m_drawer, &PetRecordDrawer::closeRequested, this, [=](){ m_drawer->hideDrawer(); });
    
    this->setObjectName("PetModule");
    this->setStyleSheet("#PetModule QPushButton { color: #606266; text-align: center; }");

    setupUI();
    
    // 初始化悬浮气泡
    m_floatingTooltip = new FloatingTooltip(this);
    
    // 应用自定义代理到以往病例(7)和饮食禁忌(8)
    auto *delegate = new CustomTooltipDelegate(this);
    petTable->setItemDelegateForColumn(7, delegate);
    petTable->setItemDelegateForColumn(8, delegate);
    
    // 开启鼠标追踪并安装事件过滤器，确保气泡响应灵敏
    petTable->setMouseTracking(true);
    petTable->viewport()->setMouseTracking(true);
    petTable->viewport()->installEventFilter(this);

    // 双击单元格进入编辑
    connect(petTable, &QTableWidget::cellDoubleClicked, this, [this](int /*row*/, int col) {
        if (col == 7 || col == 8) {
            onEditPet();
        }
    });
    
    if (petTable->rowCount() > 0) {
        onCurrentCellChanged(0, 0, -1, -1);
        petTable->selectRow(0);
    }
}

void PetModule::setupUI()
{
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 15, 0); // 整体左移一点，预留右边缘间距
    rootLayout->setSpacing(0);

    QWidget *mainWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("宠物数字化健康档案中心");
    titleLabel->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold;");

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    QHBoxLayout *statLayout = new QHBoxLayout();
    auto createStatCard = [&](const QString &icon, const QString &title, QLabel* &valLabel, const QString &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; } ");
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15); shadow->setColor(QColor(0, 0, 0, 30)); shadow->setOffset(0, 2);
        card->setGraphicsEffect(shadow);
        QHBoxLayout *cl = new QHBoxLayout(card);
        cl->setContentsMargins(20, 15, 20, 15);
        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(50, 50); iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("font-size: 24px; color: %1; background: #f5f7fa; border-radius: 10px; border: none;").arg(color));
        QVBoxLayout *vl = new QVBoxLayout(); vl->setSpacing(2);
        QLabel *tl = new QLabel(title); tl->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        valLabel = new QLabel("0"); valLabel->setStyleSheet("color: #303133; font-size: 24px; border: none; background: transparent;");
        vl->addWidget(tl); vl->addWidget(valLabel); vl->addStretch();
        cl->addWidget(iconLabel); cl->addSpacing(15); cl->addLayout(vl); cl->addStretch();
        return card;
    };
    statLayout->addWidget(createStatCard("🐾", "在册宠物", totalPetsLabel, "#409eff"));
    statLayout->addWidget(createStatCard("🏠", "在店寄养", boardingPetsLabel, "#67c23a"));
    statLayout->addWidget(createStatCard("🛁", "洗护进行中", groomingPetsLabel, "#e6a23c"));
    mainLayout->addLayout(statLayout);

    QHBoxLayout *operationLayout = new QHBoxLayout();
    QPushButton *batchDeleteBtn = new QPushButton("批量删除");
    batchDeleteBtn->setCursor(Qt::PointingHandCursor);
    batchDeleteBtn->setFixedHeight(32);
    batchDeleteBtn->setStyleSheet(
        "QPushButton { background-color: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 6px; font-size: 12px; padding: 0 15px; }"
        "QPushButton:hover { background-color: #f56c6c; color: white; }"
    );
    connect(batchDeleteBtn, &QPushButton::clicked, this, &PetModule::onBatchDelete);
    operationLayout->addWidget(batchDeleteBtn);
    
    operationLayout->addStretch();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索宠物名称、品种、主人姓名...");
    searchEdit->setFixedWidth(280); searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet("QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } QLineEdit:focus { border-color: #409eff; outline: none; }");
    connect(searchEdit, &QLineEdit::textChanged, this, &PetModule::onSearch);
    operationLayout->addWidget(searchEdit);
    mainLayout->addLayout(operationLayout);

    petTable = new QTableWidget();
    petTable->setColumnCount(12);
    petTable->setHorizontalHeaderLabels({
        "选择", "宠物ID", "宠物信息", "所属主人", "基本属性", "健康状态", 
        "疫苗接种", "以往病例", "饮食禁忌", "在店状态", "入店时间", "操作"
    });
    
    petTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    petTable->setColumnWidth(0, 48);
    petTable->setColumnWidth(1, 60);
    petTable->setColumnWidth(2, 180);
    petTable->setColumnWidth(3, 100);
    petTable->setColumnWidth(4, 110);
    petTable->setColumnWidth(5, 80);
    petTable->setColumnWidth(6, 100);
    petTable->setColumnWidth(7, 100);
    petTable->setColumnWidth(8, 120);
    petTable->setColumnWidth(9, 115);
    petTable->setColumnWidth(10, 95);
    petTable->setColumnWidth(11, 150);
    petTable->horizontalHeader()->setSectionResizeMode(11, QHeaderView::Stretch);

    petTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background-color: white; alternate-background-color: white; color: black; outline: none; selection-background-color: #b3d8ff; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; padding: 5px; } "
        "QTableWidget::item:selected { background-color: #b3d8ff; color: black; } " 
        "QTableWidget::item:focus { background-color: transparent; } "
        "QHeaderView::section { background-color: #f5f7fa; padding: 12px; border: none; color: #606266; font-size: 13px;  font-weight: bold; } "
    );

    petTable->setShowGrid(false);
    petTable->setAlternatingRowColors(false);
    petTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    petTable->setSelectionMode(QAbstractItemView::SingleSelection);
    petTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    petTable->setFocusPolicy(Qt::NoFocus);
    petTable->verticalHeader()->setVisible(false);
    petTable->verticalHeader()->setDefaultSectionSize(65);

    connect(petTable, &QTableWidget::currentCellChanged, this, &PetModule::onCurrentCellChanged);

    mainLayout->addWidget(petTable);

    QFrame *statFrame = new QFrame();
    statFrame->setFixedHeight(45);
    statFrame->setStyleSheet("QFrame { background: #f8f9fb; border-top: 1px solid #ebeef5; padding: 0 12px; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(statFrame);
    footerLayout->addStretch();

    jumpEdit = new QLineEdit();
    jumpEdit->setFixedWidth(36);
    jumpEdit->setMaxLength(3);
    jumpEdit->setFixedHeight(24);
    jumpEdit->setAlignment(Qt::AlignCenter);
    jumpEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 0; font-size: 13px; background: white; margin: 0; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    jumpValidator = new QIntValidator(1, 1, this);
    jumpEdit->setValidator(jumpValidator);

    QLabel *jumpPrefix = new QLabel("跳转到第");
    jumpPrefix->setStyleSheet("color: #606266; font-size: 13px; margin: 0; padding: 0;");
    QLabel *jumpSuffix = new QLabel("页");
    jumpSuffix->setStyleSheet("color: #606266; font-size: 13px; margin: 0; padding: 0;");

    jumpBtn = new QPushButton("确认");
    jumpBtn->setCursor(Qt::PointingHandCursor);
    jumpBtn->setFixedSize(44, 24);
    jumpBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; font-size: 12px; padding: 2px 0; text-align: center; margin: 0; }"
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"
    );

    QWidget *jumpGroup = new QWidget();
    jumpGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *jumpLayout = new QHBoxLayout(jumpGroup);
    jumpLayout->setContentsMargins(0, 0, 0, 0);
    jumpLayout->setSpacing(2);
    jumpLayout->addWidget(jumpPrefix);
    jumpLayout->addWidget(jumpEdit);
    jumpLayout->addWidget(jumpSuffix);
    jumpLayout->addWidget(jumpBtn);

    prevBtn = new QPushButton("上一页");
    nextBtn = new QPushButton("下一页");
    pageLabel = new QLabel("第 1 页 / 共 1 页");

    QString pageStyle = "QPushButton { height: 24px; border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; font-size: 12px; padding: 0 8px; text-align: center; margin: 0; } "
                        "QPushButton:hover { border-color: #409eff; color: #409eff; } "
                        "QPushButton:disabled { background: #f5f7fa; color: #c0c4cc; border-color: #e4e7ed; }";
    prevBtn->setStyleSheet(pageStyle);
    nextBtn->setStyleSheet(pageStyle);
    prevBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setCursor(Qt::PointingHandCursor);
    pageLabel->setStyleSheet("color: #909399; font-size: 13px; margin: 0; padding: 0 4px;");

    QWidget *pageGroup = new QWidget();
    pageGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *pageLayout = new QHBoxLayout(pageGroup);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(2);
    pageLayout->addWidget(prevBtn);
    pageLayout->addWidget(pageLabel);
    pageLayout->addWidget(nextBtn);

    footerLayout->addWidget(jumpGroup);
    footerLayout->addSpacing(8);
    footerLayout->addWidget(pageGroup);

    mainLayout->addWidget(statFrame);
    
    rootLayout->addWidget(mainWidget, 1);
    
    // 恢复右侧常驻面板：设置为固定宽度且默认显示
    m_drawer->setFixedWidth(450);
    m_drawer->show(); 
    rootLayout->addWidget(m_drawer);

    connect(m_drawer, &PetRecordDrawer::avatarClicked, this, &PetModule::showBigImage);
    
    // --- 初始化全屏大图预览层 ---
    m_imagePreviewOverlay = new QWidget(this);
    m_imagePreviewOverlay->setObjectName("PetPreviewOverlay");
    m_imagePreviewOverlay->setStyleSheet("#PetPreviewOverlay { background-color: rgba(0, 0, 0, 180); }");
    m_imagePreviewOverlay->hide();
    m_imagePreviewOverlay->installEventFilter(this); // 点击遮罩任意位置关闭
    
    QVBoxLayout *previewL = new QVBoxLayout(m_imagePreviewOverlay);
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    previewL->addWidget(m_previewLabel);

    connect(prevBtn, &QPushButton::clicked, this, &PetModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &PetModule::onNextPage);
    connect(jumpBtn, &QPushButton::clicked, this, &PetModule::onJumpPage);
    connect(jumpEdit, &QLineEdit::returnPressed, this, &PetModule::onJumpPage);

    // 数据同步增强：对接中央数据管理器
    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, [this]() {
        QTimer::singleShot(50, this, [this]() {
            refreshTable();
            updateStats();
        });
    });
    connect(PetDataManager::instance(), &PetDataManager::petDataChanged, this, [this](const QString &id) {
        QTimer::singleShot(50, this, [this, id]() {
            // 数据一致性修复：同步更新档案抽屉
            if (m_drawer->isVisible()) {
                 PetInfo info = PetDataManager::instance()->getPet(id);
                 QList<PetActivityLog> logs = PetDataManager::instance()->getLogs(id);
                 QList<PetMedia> media = PetDataManager::instance()->getMedia(id);
                 QList<FosterBatch> batches = PetDataManager::instance()->getHistoryBatches(id);
                 // 如果 ID 匹配，则原位刷新抽屉内容
                 m_drawer->setPet(info, logs, media, batches);
            }
            refreshTable();
        });
    });

    refreshTable();
    updateStats();
    updatePagination();
}

void PetModule::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 每次进入模块时，确保首行被选中以同步侧边栏数据
    if (petTable->rowCount() > 0) {
        petTable->selectRow(0);
        onCurrentCellChanged(0, 0, -1, -1);
    }
}

void PetModule::refreshTable()
{
    petTable->setRowCount(0);
    QList<PetInfo> allPets = PetDataManager::instance()->allPets();
    for (const auto &info : allPets) {
        addPetRow(info);
    }
    updatePagination();

    // 关键修复：刷新表格后默认选中首行，确保详情抽屉同步刷新
    if (petTable->rowCount() > 0) {
        petTable->selectRow(0);
        onCurrentCellChanged(0, 0, -1, -1);
    }
}

void PetModule::addPet(const PetInfo &info)
{
    addPetRow(info);
    updateStats();
}

void PetModule::addPetRow(const PetInfo &info)
{
    
    int row = petTable->rowCount();
    petTable->insertRow(row);

    QWidget *chkWidget = new QWidget();
    QHBoxLayout *chkLayout = new QHBoxLayout(chkWidget);
    chkLayout->setContentsMargins(0, 0, 0, 0);
    QCheckBox *chkBox = new QCheckBox();
    chkLayout->addWidget(chkBox, 0, Qt::AlignCenter);
    petTable->setCellWidget(row, 0, chkWidget);

    QTableWidgetItem *idItem = new QTableWidgetItem(info.id);
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setForeground(QColor("#909399"));
    petTable->setItem(row, 1, idItem);

    QWidget *infoWidget = new QWidget();
    QHBoxLayout *infoLayout = new QHBoxLayout(infoWidget);
    infoLayout->setContentsMargins(10, 5, 10, 5);
    infoLayout->setSpacing(12);

    QLabel *avatarImg = new QLabel();
    avatarImg->setFixedSize(45, 45);
    avatarImg->setStyleSheet("border-radius: 22px; background: #f0f2f5; ");
    avatarImg->setCursor(Qt::PointingHandCursor);
    avatarImg->setProperty("avatarPath", info.avatarPath);
    avatarImg->installEventFilter(this);
    
    QPixmap pix(info.avatarPath); 
    if (pix.isNull()) pix.load(":/images/load_img.jpg"); 
    
    QPixmap target(45, 45);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, 45, 45);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, 45, 45, pix.scaled(45, 45, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    avatarImg->setPixmap(target);
    
    QVBoxLayout *nameV = new QVBoxLayout();
    nameV->setSpacing(2);
    QLabel *nameL = new QLabel(info.name);
    nameL->setStyleSheet("font-weight: bold; color: #303133; font-size: 14px;");
    QLabel *breedL = new QLabel(info.breed);
    breedL->setStyleSheet("color: #909399; font-size: 12px;");
    nameV->addWidget(nameL); nameV->addWidget(breedL);
    
    infoLayout->addWidget(avatarImg);
    infoLayout->addLayout(nameV);
    infoLayout->addStretch();

    infoWidget->setProperty("row", row); // 保存行号用于手动选中
    infoWidget->installEventFilter(this);
    infoWidget->setCursor(Qt::PointingHandCursor);
    
    // 关键修复：容器层不能设为穿透，否则子控件头像无法点中
    nameL->setAttribute(Qt::WA_TransparentForMouseEvents);
    breedL->setAttribute(Qt::WA_TransparentForMouseEvents);
    petTable->setCellWidget(row, 2, infoWidget);

    QTableWidgetItem *ownerItem = new QTableWidgetItem(QString("%1 (%2)").arg(info.ownerName, info.ownerId));
    ownerItem->setTextAlignment(Qt::AlignCenter);
    petTable->setItem(row, 3, ownerItem);

    QString genderIcon = (info.gender == "公") ? "♂" : "♀";
    QTableWidgetItem *attrItem = new QTableWidgetItem(QString("%1 %2 · %3").arg(genderIcon, info.species, info.age));
    attrItem->setTextAlignment(Qt::AlignCenter);
    petTable->setItem(row, 4, attrItem);

    QTableWidgetItem *healthItem = new QTableWidgetItem(info.health);
    healthItem->setTextAlignment(Qt::AlignCenter);
    if (info.health != "健康") healthItem->setForeground(QColor("#e6a23c"));
    petTable->setItem(row, 5, healthItem);

    QWidget *vacWrap = new QWidget();
    QHBoxLayout *vacL = new QHBoxLayout(vacWrap);
    vacL->setContentsMargins(0,0,0,0); 
    vacL->setAlignment(Qt::AlignCenter);
    
    QPushButton *vacBtn = new QPushButton();
    vacBtn->setText(info.vaccine);
    vacBtn->setFixedSize(105, 26);
    vacBtn->setCursor(Qt::PointingHandCursor);
    
    if (info.vaccine == "未接种") {
        vacBtn->setStyleSheet(
            "QPushButton { background-color: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 4px; font-size: 11px; font-weight: bold; padding: 0; text-align: center; } "
            "QPushButton:hover { background-color: #f56c6c; color: white; }"
        );
    } else {
        vacBtn->setStyleSheet(
            "QPushButton { background-color: #ecf5ff; color: #409eff; border: 1px solid #b3d8ff; border-radius: 4px; font-size: 11px; font-weight: bold; padding: 0; text-align: center; } "
            "QPushButton:hover { background-color: #409eff; color: white; }"
        );
    }
    
    connect(vacBtn, &QPushButton::clicked, this, [this, info]() {
        VaccineDetailDialog dlg(info.name, PetDataManager::instance()->getVaccines(info.id), this->window());
        dlg.exec();
    });
    
    vacL->addWidget(vacBtn);
    petTable->setCellWidget(row, 6, vacWrap);

    QTableWidgetItem *medItem = new QTableWidgetItem(info.medicalHistory);
    medItem->setTextAlignment(Qt::AlignCenter);
    medItem->setFont(QFont("Microsoft YaHei", 8));
    if (info.medicalHistory != "无" && info.medicalHistory != "暂无病史") medItem->setForeground(QColor("#f56c6c"));
    petTable->setItem(row, 7, medItem);

    QTableWidgetItem *dietItem = new QTableWidgetItem(info.dietary);
    dietItem->setTextAlignment(Qt::AlignCenter);
    dietItem->setFont(QFont("Microsoft YaHei", 8));
    if (info.dietary != "常规饮食") dietItem->setForeground(QColor("#e6a23c"));
    petTable->setItem(row, 8, dietItem);

    QWidget *statusWrap = new QWidget();
    QHBoxLayout *statusL = new QHBoxLayout(statusWrap);
    statusL->setContentsMargins(0, 0, 0, 0); statusL->setAlignment(Qt::AlignCenter);
    
    QLabel *statusTag = new QLabel(info.status);
    statusTag->setFixedSize(90, 28);
    statusTag->setAlignment(Qt::AlignCenter);
    
    QString bgColor, textColor, borderColor;
    if (info.status == "寄养中") { bgColor = "#ecf5ff"; textColor = "#409eff"; borderColor = "#b3d8ff"; }
    else if (info.status == "洗护中") { bgColor = "#fdf6ec"; textColor = "#e6a23c"; borderColor = "#faecd8"; }
    else if (info.status == "离店") { bgColor = "#f4f4f5"; textColor = "#909399"; borderColor = "#e4e7ed"; }
    else { bgColor = "#f0f9eb"; textColor = "#67c23a"; borderColor = "#e1f3d8"; }

    statusTag->setStyleSheet(QString(
        "background-color: %1; color: %2; border: 1px solid %3; border-radius: 14px; font-size: 12px; font-weight: bold;"
    ).arg(bgColor, textColor, borderColor));

    statusL->addWidget(statusTag);
    petTable->setCellWidget(row, 9, statusWrap);

    QTableWidgetItem *timeItem = new QTableWidgetItem(info.joinTime);
    timeItem->setTextAlignment(Qt::AlignCenter);
    timeItem->setForeground(QColor("#909399"));
    petTable->setItem(row, 10, timeItem);

    QWidget *btnWrap = new QWidget();
    btnWrap->setMinimumWidth(140);
    QHBoxLayout *btnL = new QHBoxLayout(btnWrap);
    btnL->setContentsMargins(5, 0, 5, 0); btnL->setSpacing(8); btnL->setAlignment(Qt::AlignCenter);
    
    auto createActBtn = [&](const QString &text, const QString &style) {
        QPushButton *b = new QPushButton(text);
        b->setFixedSize(60, 28); b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(style);
        return b;
    };
    QPushButton *editB = createActBtn("修改", "QPushButton { background: #f0f7ff; color: #409eff; border: 1px solid #b3d8ff; border-radius: 3px; font-size: 11px; padding: 0; text-align: center; } QPushButton:hover { background: #409eff; color: white; }");
    QPushButton *delB = createActBtn("删除", "QPushButton { background: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 3px; font-size: 11px; padding: 0; text-align: center; } QPushButton:hover { background: #f56c6c; color: white; }");
    connect(editB, &QPushButton::clicked, this, &PetModule::onEditPet);
    connect(delB, &QPushButton::clicked, this, &PetModule::onDeletePet);
    btnL->addWidget(editB); btnL->addWidget(delB);
    petTable->setCellWidget(row, 11, btnWrap);

    updatePagination();
}

void PetModule::updateStats()
{
    int total = petTable->rowCount();
    int boarding = 0;
    int grooming = 0;

    for (int i = 0; i < total; ++i) {
        if (petTable->isRowHidden(i)) continue;
        QWidget *w = petTable->cellWidget(i, 9);
        if (w) {
            QLabel *tag = w->findChild<QLabel*>();
            if (tag) {
                QString st = tag->text();
                if (st == "寄养中") boarding++;
                else if (st == "洗护中") grooming++;
            }
        }
    }

    int visibleCount = 0;
    for (int i = 0; i < total; ++i)
        if (!petTable->isRowHidden(i)) visibleCount++;

    totalPetsLabel->setText(QString("%1只").arg(visibleCount));
    boardingPetsLabel->setText(QString("%1只").arg(boarding));
    groomingPetsLabel->setText(QString("%1只").arg(grooming));
}

void PetModule::filterByMemberAndHighlightPet(const QString &memberName, const QString &petName)
{
    for (int i = 0; i < petTable->rowCount(); ++i) {
        for (int j = 0; j < petTable->columnCount(); ++j) {
            QTableWidgetItem *item = petTable->item(i, j);
            if (item) {
                item->setBackground(QBrush());
            }
        }
    }

    searchEdit->clear();
    onSearch("");

    petTable->clearSelection();
    petTable->clearFocus();
    petTable->setCurrentItem(nullptr);
    
    for (int i = 0; i < petTable->rowCount(); ++i) {
        if (!petTable->isRowHidden(i)) {
            QTableWidgetItem *nameItem = petTable->item(i, 2);
            QTableWidgetItem *ownerItem = petTable->item(i, 3);

            if (nameItem && ownerItem) {
                QString currentPetName = nameItem->text();
                if (currentPetName == petName && ownerItem->text().startsWith(memberName + " ")) {
                    petTable->scrollToItem(nameItem, QAbstractItemView::PositionAtCenter);
                    petTable->selectRow(i);
                    break;
                }
            }
        }
    }
}

void PetModule::onSearch(const QString &keyword)
{
    QString kw = keyword.trimmed().toLower();
    for (int i = 0; i < petTable->rowCount(); ++i) {
        if (kw.isEmpty()) {
            petTable->setRowHidden(i, false);
            continue;
        }
        bool match = false;
        QList<int> searchCols = {1, 2, 3, 4};
        for (int col : searchCols) {
            QTableWidgetItem *item = petTable->item(i, col);
            if (item && item->text().toLower().contains(kw)) {
                match = true;
                break;
            }
        }
        petTable->setRowHidden(i, !match);
    }
    m_currentPage = 1;
    updateStats();
    updatePagination();
}

void PetModule::onEditPet()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    for (int i = 0; i < petTable->rowCount(); ++i) {
        QWidget *w = petTable->cellWidget(i, 11);
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
             QString petId = petTable->item(i, 1)->text();
             PetInfo info = PetDataManager::instance()->getPet(petId);
             if (info.id.isEmpty()) break;
             
             AddPetDialog dlg(this);
             dlg.setPetInfo(info);
             if (dlg.exec() == QDialog::Accepted) {
                  PetInfo newInfo = dlg.getPetInfo();
                  PetDataManager::instance()->updatePet(newInfo);
                  // 移除手动 removeRow 和 addPetRow，由 PetDataManager 信号触发 refreshTable 保持排序
                  updateStats();
             }
             break;
        }
    }
}

void PetModule::onDeletePet()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    for (int i = 0; i < petTable->rowCount(); ++i) {
        QWidget *w = petTable->cellWidget(i, 11);
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
            QString petId = petTable->item(i, 1)->text();
            if (CustomMessageDialog::confirm(this, "删除确认", "确定要永久删除该宠物档案吗？")) {
                PetDataManager::instance()->removePet(petId);
            }
            break;
        }
    }
}

void PetModule::onBatchDelete()
{
    QList<int> checkedRows;
    for (int i = petTable->rowCount() - 1; i >= 0; --i) {
        QWidget *w = petTable->cellWidget(i, 0);
        if (w) {
            QCheckBox *cb = w->findChild<QCheckBox*>();
            if (cb && cb->isChecked()) {
                checkedRows.append(i);
            }
        }
    }
    
    if (checkedRows.isEmpty()) {
        CustomMessageDialog::showWarning(this, "批量操作", "请先勾选需要删除的宠物档案。");
        return;
    }

    if (CustomMessageDialog::confirm(this, "批量删除", QString("确定要删除选中的 %1 个宠物档案吗？此操作不可撤销。").arg(checkedRows.size()))) {
        for (int row : checkedRows) {
            QString petId = petTable->item(row, 1)->text();
            PetDataManager::instance()->removePet(petId);
        }
    }
}

void PetModule::onPrevPage()
{
    if (m_currentPage > 1) {
        m_currentPage--;
        updatePagination();
    }
}

void PetModule::onNextPage()
{
    int total = petTable->rowCount();
    QString kw = searchEdit->text().trimmed().toLower();
    int visibleCount = 0;
    if (kw.isEmpty()) {
        visibleCount = total;
    } else {
        for (int i = 0; i < total; ++i) {
            for (int col : {1, 2, 3, 4}) {
                QTableWidgetItem *item = petTable->item(i, col);
                if (item && item->text().toLower().contains(kw)) {
                    visibleCount++;
                    break;
                }
            }
        }
    }
    int totalPages = qMax(1, (visibleCount + m_pageSize - 1) / m_pageSize);
    if (m_currentPage < totalPages) {
        m_currentPage++;
        updatePagination();
    }
}

void PetModule::onJumpPage()
{
    int page = jumpEdit->text().toInt();
    if (page < 1) return;
    
    m_currentPage = page;
    updatePagination();
    jumpEdit->clear();
    jumpEdit->clearFocus();
}

void PetModule::updatePagination()
{
    int total = petTable->rowCount();
    QString kw = searchEdit->text().trimmed().toLower();
    
    QList<int> visibleRows;
    for (int i = 0; i < total; ++i) {
        bool match = false;
        if (kw.isEmpty()) {
            match = true;
        } else {
            QList<int> searchCols = {1, 2, 3, 4};
            for (int col : searchCols) {
                QTableWidgetItem *item = petTable->item(i, col);
                if (item && item->text().toLower().contains(kw)) {
                    match = true;
                    break;
                }
            }
        }
        if (match) visibleRows.append(i);
        petTable->setRowHidden(i, true);
    }

    int totalVisible = visibleRows.size();
    int totalPages = qMax(1, (totalVisible + m_pageSize - 1) / m_pageSize);
    
    if (jumpValidator) jumpValidator->setTop(totalPages);

    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, totalVisible);

    for (int i = start; i < end; ++i) {
        petTable->setRowHidden(visibleRows[i], false);
    }

    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);

    if (totalVisible == 0) {
        prevBtn->setEnabled(false);
        nextBtn->setEnabled(false);
        pageLabel->setText("第 0 页 / 共 0 页");
    }
}

bool PetModule::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        // 1. 如果点击的是头像，直接放大
        if (watched->property("avatarPath").isValid()) {
            showBigImage(watched->property("avatarPath").toString());
            return true;
        }
        
        // 2. 如果点击的是头像周边的信息区域，手动选中该行
        if (watched->property("row").isValid()) {
            int row = watched->property("row").toInt();
            petTable->selectRow(row);
            // 这里也可以顺便触发详情刷新逻辑
            onCurrentCellChanged(row, 0, -1, -1);
            return true;
        }
    }
    
    // 3. 处理遮罩层点击（关闭预览）
    if (watched == m_imagePreviewOverlay && event->type() == QEvent::MouseButtonRelease) {
        hideBigImage();
        return true;
    }

    // 鼠标离开表格区域时隐藏气泡
    if (watched == petTable->viewport() && event->type() == QEvent::Leave) {
        if (m_floatingTooltip) m_floatingTooltip->hide();
    }

    // 关键：实时追踪鼠标，实现“操作流极快”的即时气泡提示
    if (watched == petTable->viewport() && event->type() == QEvent::MouseMove) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        QModelIndex index = petTable->indexAt(me->pos());
        
        if (index.isValid() && (index.column() == 7 || index.column() == 8)) {
            QString text = index.data(Qt::DisplayRole).toString().trimmed();
            
            // 【关键修复3】：智能判断。使用单元格实际字体计算宽度
            QTableWidgetItem *item = petTable->item(index.row(), index.column());
            QFont font = item ? item->font() : petTable->font();
            QFontMetrics fm(font);
            int availableWidth = petTable->columnWidth(index.column()) - 15;
            bool isTruncated = fm.horizontalAdvance(text) > availableWidth;

            // 只有当“文字真正被截断” 或者是 “有意义的特殊内容” 时，才显示气泡
            // 如果你希望只有截断才显示，可以去掉后面的条件；但通常有意义的内容悬停显示更符合直觉
            if ((isTruncated || (text != "无" && text != "常规饮食" && text != "暂无病史")) && !text.isEmpty()) {
                if (m_lastHoveredIndex != index) {
                    m_floatingTooltip->showText(me->globalPosition().toPoint(), text);
                    m_lastHoveredIndex = index;
                }
                return false; 
            }
        }
        
        // 鼠标移出目标列、移到空白处、或者文字没有被截断时，平滑隐藏气泡
        if (m_floatingTooltip && m_floatingTooltip->isVisible()) {
            m_floatingTooltip->hide();
            m_lastHoveredIndex = QModelIndex();
        }
    }
    
    return QWidget::eventFilter(watched, event);
}

void PetModule::showBigImage(const QString &path)
{
    if (path.isEmpty()) return;
    
    QPixmap pix(path);
    if (pix.isNull()) pix.load(":/images/load_img.jpg");
    
    // 确保遮罩覆盖整个 PetModule 区域
    m_imagePreviewOverlay->setGeometry(rect());
    
    // 限制预览图最大尺寸为模块宽度的 80%
    int maxWidth = width() * 0.8;
    int maxHeight = height() * 0.8;
    m_previewLabel->setPixmap(pix.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    m_imagePreviewOverlay->show();
    m_imagePreviewOverlay->raise(); // 确保遮罩层在最顶层展示
}

void PetModule::hideBigImage()
{
    m_imagePreviewOverlay->hide();
}

void PetModule::onCurrentCellChanged(int row, int column, int prevRow, int prevCol)
{
    Q_UNUSED(prevRow); Q_UNUSED(prevCol); Q_UNUSED(column);
    if (row < 0) return;

    QTableWidgetItem *idItem = petTable->item(row, 1);
    if (!idItem) return;

    QString petId = idItem->text();
    PetInfo info = PetDataManager::instance()->getPet(petId);
    if (!info.id.isEmpty()) {
        QList<FosterBatch> batches = PetDataManager::instance()->getHistoryBatches(petId);
        m_drawer->setPet(info, PetDataManager::instance()->getLogs(petId), PetDataManager::instance()->getMedia(petId), batches);
    }
}

void PetModule::onLogAdded(const QString &petId, const PetActivityLog &log)
{
    PetDataManager::instance()->addActivityLog(petId, log);
    onCurrentCellChanged(petTable->currentRow(), 0, -1, -1);
}

void PetModule::onQuickAction()
{
    // 该槽函数预留用于处理来自详情抽屉或外部的快捷指令
    // 目前通过 onLogAdded 已能覆盖大部分业务逻辑
}

void PetModule::updateRowStatus(int row)
{
    if (row < 0 || row >= petTable->rowCount()) return;
    
    // 同步刷新指定行的 UI 表现（如颜色、图标等）
    // 这里的逻辑已集成在 addPetRow 和状态 ComboBox 的 lambda 中
}
