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
#include <QButtonGroup>

// --- 复刻：全行圆角边框选中委托 ---
class PetRowDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillRect(opt.rect, Qt::white);

        if (opt.state & QStyle::State_Selected) {
            bool isFirst = (index.column() == 0);
            bool isLast = (index.column() == index.model()->columnCount() - 1);
            QRect rect = opt.rect.adjusted(1, 4, -1, -4);
            int radius = 8;
            QColor borderColor("#3b82f6");
            QColor bgColor("#eff6ff");

            painter->fillRect(opt.rect, bgColor);
            painter->setPen(QPen(borderColor, 2));
            
            if (isFirst) {
                QPainterPath path;
                path.moveTo(opt.rect.right() + 1, rect.top()); 
                path.lineTo(rect.left() + radius, rect.top());
                path.arcTo(QRect(rect.left(), rect.top(), radius*2, radius*2), 90, 90);
                path.lineTo(rect.left(), rect.bottom() - radius);
                path.arcTo(QRect(rect.left(), rect.bottom() - radius*2, radius*2, radius*2), 180, 90);
                path.lineTo(opt.rect.right() + 1, rect.bottom());
                painter->drawPath(path);
            } else if (isLast) {
                QPainterPath path;
                path.moveTo(opt.rect.left() - 1, rect.top());
                path.lineTo(rect.right() - radius, rect.top());
                path.arcTo(QRect(rect.right() - radius*2, rect.top(), radius*2, radius*2), 90, -90);
                path.lineTo(rect.right(), rect.bottom() - radius);
                path.arcTo(QRect(rect.right() - radius*2, rect.bottom() - radius*2, radius*2, radius*2), 0, -90);
                path.lineTo(opt.rect.left() - 1, rect.bottom());
                painter->drawPath(path);
            } else {
                painter->drawLine(QPoint(opt.rect.left() - 1, rect.top()), QPoint(opt.rect.right() + 1, rect.top()));
                painter->drawLine(QPoint(opt.rect.left() - 1, rect.bottom()), QPoint(opt.rect.right() + 1, rect.bottom()));
            }
        } else {
            painter->setPen(QPen(QColor("#f1f5f9"), 1));
            painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());
        }

        // 只有非 CellWidget 的列才绘制默认文本
        if (!index.model()->data(index, Qt::UserRole + 1).toBool()) {
            painter->setPen(QColor((opt.state & QStyle::State_Selected) ? "#1e40af" : "#303133"));
            QFont font = painter->font();
            font.setWeight(opt.state & QStyle::State_Selected ? QFont::Bold : QFont::Normal);
            font.setPointSize(10);
            painter->setFont(font);
            QRect textRect = opt.rect.adjusted(10, 0, -10, 0);
            painter->drawText(textRect, opt.displayAlignment | Qt::AlignVCenter, opt.text);
        }
        
        painter->restore();
    }
};

PetModule::PetModule(UserRole role, QWidget *parent) : QWidget(parent),
    prevBtn(nullptr), nextBtn(nullptr), pageLabel(nullptr), m_currentPage(1), m_pageSize(10),
    jumpEdit(nullptr), jumpValidator(nullptr), jumpBtn(nullptr), m_role(role)
{
    m_drawer = new PetRecordDrawer(this);
    connect(m_drawer, &PetRecordDrawer::logAdded, this, &PetModule::onLogAdded);
    connect(m_drawer, &PetRecordDrawer::closeRequested, this, [=](){ m_drawer->hideDrawer(); });
    connect(m_drawer, &PetRecordDrawer::editRequested, this, &PetModule::onEditPetFromDrawer);
    
    this->setObjectName("PetModule");
    this->setStyleSheet("#PetModule QPushButton { color: #64748b; text-align: center; font-weight: 700; }");

    setupUI();
    
    // 初始化悬浮气泡
    m_floatingTooltip = new FloatingTooltip(this);
    
    // 开启鼠标追踪并安装事件过滤器，确保气泡响应灵敏
    petTable->setMouseTracking(true);
    petTable->viewport()->setMouseTracking(true);
    petTable->viewport()->installEventFilter(this);

    // 双击单元格进入编辑
    connect(petTable, &QTableWidget::cellDoubleClicked, this, [this](int /*row*/, int /*col*/) {
        onEditPet();
    });

    
    if (petTable->rowCount() > 0) {
        onCurrentCellChanged(0, 0, -1, -1);
        petTable->selectRow(0);
    }

    // 触发从服务器拉取真实数据
    PetDataManager::instance()->requestPetList();
}

void PetModule::setupUI()
{
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    QWidget *mainWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // --- 顶部统计与标题容器 (复刻会员模块风格) ---
    QFrame *topContainer = new QFrame();
    topContainer->setObjectName("TopStatisticsContainer");
    topContainer->setFixedHeight(160); // 限制高度
    topContainer->setStyleSheet("#TopStatisticsContainer { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QVBoxLayout *topLayout = new QVBoxLayout(topContainer);
    topLayout->setContentsMargins(25, 15, 25, 15);
    topLayout->setSpacing(12);

    // 1. 顶部标题
    QLabel *titleLabel = new QLabel("宠物数字化健康档案中心");
    titleLabel->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold; border: none; background: transparent;");
    topLayout->addWidget(titleLabel);

    // ═══════════════════════════════════════════
    // 2. 统计卡片行
    // ═══════════════════════════════════════════
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(15);
    topLayout->addLayout(statLayout);

    mainLayout->addWidget(topContainer);
    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &outValueLabel, const QColor &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(80);
        card->setStyleSheet("QFrame { background: #f8fafc; border-radius: 8px; border: 1px solid #f1f5f9; } ");
        
        QHBoxLayout *l = new QHBoxLayout(card);
        l->setContentsMargins(20, 10, 20, 10);

        QLabel *iconLabel = new QLabel(icon);
        if (icon.isEmpty()) {
            iconLabel->hide();
        } else {
            iconLabel->setFixedSize(40, 40);
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setStyleSheet(QString("font-size: 20px; color: %1; background: white; border-radius: 8px; border: 1px solid #f1f5f9;").arg(color.name()));
        }
        if (!icon.isEmpty()) {
            l->addWidget(iconLabel);
            l->addSpacing(12);
        }

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);
        
        QLabel *labelTitle = new QLabel(label);
        labelTitle->setStyleSheet("color: #94a3b8; font-size: 12px; border: none; background: transparent;");
        
        outValueLabel = new QLabel("0");
        outValueLabel->setStyleSheet("font-size: 20px; color: #1e293b; border: none; background: transparent; font-weight: bold;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(outValueLabel);
        textLayout->addStretch();

        l->addLayout(textLayout);
        l->addStretch();

        return card;
    };
    statLayout->addWidget(createStatCard("", "在册宠物", totalPetsLabel, QColor("#3b82f6")));
    statLayout->addWidget(createStatCard("", "在店寄养", boardingPetsLabel, QColor("#10b981")));
    statLayout->addWidget(createStatCard("", "洗护进行中", groomingPetsLabel, QColor("#f59e0b")));
    // 已由 topLayout 管理

    // --- 3. 操作中控台 (搜索 + 状态筛选) ---
    QFrame *operationCard = new QFrame();
    operationCard->setObjectName("OperationCard");
    operationCard->setStyleSheet("#OperationCard { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QHBoxLayout *operationLayout = new QHBoxLayout(operationCard);
    operationLayout->setContentsMargins(25, 12, 25, 12);
    operationLayout->setSpacing(0);

    // -- 搜索栏 --
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索宠物名称、品种、主人姓名...");
    searchEdit->setFixedWidth(280); 
    searchEdit->setFixedHeight(36);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    connect(searchEdit, &QLineEdit::textChanged, this, &PetModule::onSearch);
    operationLayout->addWidget(searchEdit);
    operationLayout->addSpacing(12);
    
    // -- 状态筛选按钮组 --
    m_statusGroup = new QButtonGroup(this);
    m_statusGroup->setExclusive(true);
    
    QWidget *filterBox = new QWidget();
    QHBoxLayout *filterLayout = new QHBoxLayout(filterBox);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);
    
    QStringList statuses = {"全部", "已预约", "寄养中", "洗护中", "待接走", "接送中", "在家"};
    for (int i = 0; i < statuses.size(); ++i) {
        QPushButton *btn = new QPushButton(statuses[i]);
        btn->setCheckable(true);
        btn->setAutoExclusive(true); // 启用自动互斥，简化逻辑
        btn->setFixedHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 18px; padding: 0 16px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );
        if (i == 0) btn->setChecked(true);
        m_statusGroup->addButton(btn, i);
        filterLayout->addWidget(btn);
    }
    connect(m_statusGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &PetModule::onStatusFilterChanged);
    
    operationLayout->addWidget(filterBox);
    operationLayout->addStretch();
    
    mainLayout->addWidget(operationCard);

    petTable = new QTableWidget();
    petTable->setColumnCount(7);
    petTable->setHorizontalHeaderLabels({
        "宠物ID", "宠物信息", "所属主人", "基本属性", "状态", "入店时间", "操作"
    });
    petTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    petTable->setItemDelegate(new PetRowDelegate(petTable));
    
    // 优化列宽分配，使其更加均匀美观
    petTable->setColumnWidth(0, 80);  // ID
    petTable->setColumnWidth(4, 100); // 状态
    petTable->setColumnWidth(5, 180); // 入店时间
    petTable->setColumnWidth(6, 180); // 操作 (加宽以容纳彻底删除按钮)

    petTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    petTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // 信息
    petTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // 主人
    petTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch); // 属性
    petTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    petTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    petTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);

    // 4. 表格卡片容器 (12px 圆角)
    QFrame *tableCard = new QFrame();
    tableCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #ebeef5; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 5, 0, 0);
    
    petTable->setStyleSheet(
        "QTableWidget { border: none; background: white; outline: none; border-radius: 12px; } "
        "QHeaderView { border: none; background: transparent; border-radius: 12px 12px 0 0; }"
    );
    petTable->setShowGrid(false);
    petTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    petTable->setSelectionMode(QAbstractItemView::SingleSelection);
    petTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    petTable->setFocusPolicy(Qt::NoFocus);
    petTable->verticalHeader()->setVisible(false);
    petTable->verticalHeader()->setDefaultSectionSize(60);
    connect(petTable, &QTableWidget::currentCellChanged, this, &PetModule::onCurrentCellChanged);

    tableLayout->addWidget(petTable);

    mainLayout->addWidget(tableCard);

    // 6. 统计栏（集成在 tableCard 底部）
    QFrame *statFrame = new QFrame();
    statFrame->setFixedHeight(50);
    // 背景改为白色，移除 border
    statFrame->setStyleSheet("QFrame { background: white; border: none; padding: 0 12px; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(statFrame);
    tableLayout->addWidget(statFrame);

    prevBtn = new QPushButton("上一页");
    nextBtn = new QPushButton("下一页");
    pageLabel = new QLabel("第 1 页 / 共 1 页");

    QString pageStyle = "QPushButton { height: 28px; border: 1px solid #e2e8f0; border-radius: 6px; background: white; color: #64748b; font-size: 12px; padding: 0 12px; text-align: center; font-weight: bold; } "
                        "QPushButton:hover { border-color: #3b82f6; color: #3b82f6; background: #eff6ff; } "
                        "QPushButton:disabled { background: white; color: #cbd5e1; border-color: #f1f5f9; }";
    prevBtn->setStyleSheet(pageStyle);
    nextBtn->setStyleSheet(pageStyle);

    prevBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setCursor(Qt::PointingHandCursor);
    pageLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: 600; margin: 0 10px;");

    footerLayout->addStretch();
    footerLayout->addWidget(prevBtn);
    footerLayout->addSpacing(20);
    footerLayout->addWidget(pageLabel);
    footerLayout->addSpacing(20);
    footerLayout->addWidget(nextBtn);
    
    rootLayout->addWidget(mainWidget, 1);
    
    // 恢复右侧常驻面板：设置为固定宽度且默认显示
    m_drawer->setFixedWidth(450);
    m_drawer->show(); 
    rootLayout->addWidget(m_drawer);

    connect(m_drawer, &PetRecordDrawer::avatarClicked, this, &PetModule::showBigImage);
    
    // --- 初始化全屏大图预览层 ---
    m_imagePreviewOverlay = new QWidget(this);
    m_imagePreviewOverlay->setObjectName("PetPreviewOverlay");
    m_imagePreviewOverlay->setStyleSheet("#PetPreviewOverlay { background-color: rgba(0, 0, 0, 215); }");
    m_imagePreviewOverlay->hide();
    m_imagePreviewOverlay->installEventFilter(this); // 点击遮罩任意位置关闭
    
    QVBoxLayout *previewL = new QVBoxLayout(m_imagePreviewOverlay);
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    previewL->addWidget(m_previewLabel);

    connect(prevBtn, &QPushButton::clicked, this, &PetModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &PetModule::onNextPage);

    // 数据同步增强：对接中央数据管理器
    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, &PetModule::refreshTablePreservingSelection);
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
            refreshTablePreservingSelection();
        });
    });

    refreshTable();
    updateStats();
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
    if (m_isRefreshing) return;
    m_isRefreshing = true;

    petTable->setRowCount(0);
    QList<PetInfo> pets = PetDataManager::instance()->allPets();

    // 1. 过滤逻辑
    QString kw = searchEdit->text().trimmed().toLower();
    QString statusFilter = m_statusGroup->checkedButton() ? m_statusGroup->checkedButton()->text() : "全部";

    QList<PetInfo> filtered;
    for (const auto &info : pets) {
        bool match = true;
        if (!kw.isEmpty()) {
            if (!info.name.toLower().contains(kw) && !info.ownerName.toLower().contains(kw) && 
                !info.id.toLower().contains(kw) && !info.ownerId.toLower().contains(kw)) {
                match = false;
            }
        }
        
        if (match && statusFilter != "全部") {
            if (info.status != statusFilter) match = false;
        }

        if (match) filtered.append(info);
    }

    // 2. 排序逻辑：活跃的在前，注销的沉底
    std::sort(filtered.begin(), filtered.end(), [](const PetInfo &a, const PetInfo &b) {
        if (a.isActive != b.isActive) return a.isActive > b.isActive;
        return a.id < b.id;
    });

    // 3. 分页逻辑
    int total = filtered.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    updatePagination(); // 更新页码显示

    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, total);

    for (int i = start; i < end; ++i) {
        addPetRow(filtered[i]);
    }

    updateStats();
    m_isRefreshing = false;
}

void PetModule::refreshTablePreservingSelection()
{
    QString selectedId;
    int row = petTable->currentRow();
    if (row >= 0 && petTable->item(row, 0)) {
        selectedId = petTable->item(row, 0)->text();
    }

    refreshTable();

    if (!selectedId.isEmpty()) {
        for (int i = 0; i < petTable->rowCount(); ++i) {
            if (petTable->item(i, 0)->text() == selectedId) {
                petTable->setCurrentCell(i, 0);
                break;
            }
        }
    }
}

void PetModule::addPet(const PetInfo &info)
{
    Q_UNUSED(info);
    refreshTable();
    updateStats();
}

void PetModule::addPetRow(const PetInfo &info)
{
    int row = petTable->rowCount();
    petTable->insertRow(row);

    QTableWidgetItem *idItem = new QTableWidgetItem(info.id);
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setForeground(QColor("#909399"));
    petTable->setItem(row, 0, idItem);

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
    nameV->setAlignment(Qt::AlignVCenter);
    QLabel *nameL = new QLabel();
    QString genderSym = (info.gender == "公" || info.gender == "雄" || info.gender == "M") ? "♂" : "♀";
    QString genderCol = (genderSym == "♂") ? "#409EFF" : "#F56C6C";
    nameL->setText(QString("%1 <span style='color:%2; font-weight:bold;'>%3</span>")
                   .arg(info.name).arg(genderCol).arg(genderSym));
    nameL->setStyleSheet("font-weight: bold; color: #303133; font-size: 14px; border: none; background: transparent;");
    QLabel *breedL = new QLabel(info.breed);
    breedL->setStyleSheet("color: #909399; font-size: 12px; border: none; background: transparent;");
    nameV->addWidget(nameL); nameV->addWidget(breedL);
    
    infoLayout->addStretch();
    infoLayout->addWidget(avatarImg);
    infoLayout->addLayout(nameV);
    infoLayout->addStretch();

    infoWidget->setProperty("row", row); // 保存行号用于手动选中
    infoWidget->installEventFilter(this);
    infoWidget->setCursor(Qt::PointingHandCursor);
    
    // 关键修复：容器层不能设为穿透，否则子控件头像无法点中
    nameL->setAttribute(Qt::WA_TransparentForMouseEvents);
    breedL->setAttribute(Qt::WA_TransparentForMouseEvents);
    petTable->setCellWidget(row, 1, infoWidget);

    QTableWidgetItem *ownerItem = new QTableWidgetItem(QString("%1 (%2)").arg(info.ownerName, info.ownerId));
    ownerItem->setTextAlignment(Qt::AlignCenter);
    petTable->setItem(row, 2, ownerItem);

    QTableWidgetItem *attrItem = new QTableWidgetItem(QString("%1 · %2").arg(info.species, info.age));
    attrItem->setTextAlignment(Qt::AlignCenter);
    petTable->setItem(row, 3, attrItem);

    QWidget *statusWrap = new QWidget();
    QHBoxLayout *statusL = new QHBoxLayout(statusWrap);
    statusL->setContentsMargins(0, 0, 0, 0); statusL->setAlignment(Qt::AlignCenter);
    
    QLabel *statusTag = new QLabel(info.status);
    statusTag->setFixedHeight(22);
    statusTag->setFixedWidth(80);
    statusTag->setAlignment(Qt::AlignCenter);
    
    QString bgColor, textColor, borderColor;
    if (info.status == "寄养中") { bgColor = "#eff6ff"; textColor = "#1e40af"; borderColor = "#d9ecff"; }
    else if (info.status == "洗护中") { bgColor = "#fff7e6"; textColor = "#e6a23c"; borderColor = "#f5dab1"; }
    else if (info.status == "已预约") { bgColor = "#f0f9eb"; textColor = "#67c23a"; borderColor = "#c2e7b0"; }
    else if (info.status == "待接走") { bgColor = "#fef0f0"; textColor = "#f56c6c"; borderColor = "#fbc4c4"; }
    else if (info.status == "接送中") { bgColor = "#f4f4f5"; textColor = "#6b7280"; borderColor = "#e4e4e7"; }
    else if (info.status == "已注销") { bgColor = "#f4f4f5"; textColor = "#94a3b8"; borderColor = "#e2e8f0"; }
    else { bgColor = "#f4f4f5"; textColor = "#909399"; borderColor = "#e9e9eb"; }

    statusTag->setStyleSheet(QString(
        "background-color: %1; color: %2; border: 1px solid %3; border-radius: 11px; font-size: 11px; font-weight: bold; padding: 0 12px;"
    ).arg(bgColor, textColor, borderColor));

    statusL->addWidget(statusTag);
    petTable->setCellWidget(row, 4, statusWrap);

    QTableWidgetItem *timeItem = new QTableWidgetItem(info.joinTime);
    timeItem->setTextAlignment(Qt::AlignCenter);
    timeItem->setForeground(QColor("#909399"));
    petTable->setItem(row, 5, timeItem);

    QWidget *btnWrap = new QWidget();
    QHBoxLayout *btnL = new QHBoxLayout(btnWrap);
    btnL->setContentsMargins(0, 0, 0, 0); 
    btnL->setSpacing(8); 
    btnL->setAlignment(Qt::AlignCenter);
    
    QPushButton *delBtn = new QPushButton();
    delBtn->setObjectName("TableDeleteBtn");
    delBtn->setFixedSize(60, 28);
    delBtn->setCursor(Qt::PointingHandCursor);
    
    QVBoxLayout *btnInnerL = new QVBoxLayout(delBtn);
    btnInnerL->setContentsMargins(0, 0, 0, 0);
    QLabel *btnText = new QLabel();
    btnText->setAlignment(Qt::AlignCenter);
    btnText->setStyleSheet("font-size: 11px; font-weight: bold; background: transparent; border: none;");
    btnInnerL->addWidget(btnText);

    if (info.isActive) {
        delBtn->setStyleSheet(
            "QPushButton#TableDeleteBtn { background: #fef0f0; border: 1px solid #fbc4c4; border-radius: 4px; } "
            "QPushButton#TableDeleteBtn:hover { background: #f56c6c; border-color: #f56c6c; } "
        );
        btnText->setText("删除");
        btnText->setStyleSheet(btnText->styleSheet() + "color: #f56c6c;");
        connect(delBtn, &QPushButton::clicked, this, &PetModule::onDeletePet);
    } else {
        delBtn->setStyleSheet(
            "QPushButton#TableDeleteBtn { background: #f0f9eb; border: 1px solid #c2e7b0; border-radius: 4px; } "
            "QPushButton#TableDeleteBtn:hover { background: #67c23a; border-color: #67c23a; } "
        );
        btnText->setText("恢复");
        btnText->setStyleSheet(btnText->styleSheet() + "color: #67c23a;");
        connect(delBtn, &QPushButton::clicked, this, &PetModule::onRestorePet);

        // 管理员专属：彻底删除按钮
        if (m_role == ADMIN) {
            QPushButton *hardDelBtn = new QPushButton();
            hardDelBtn->setFixedSize(75, 28);
            hardDelBtn->setCursor(Qt::PointingHandCursor);
            hardDelBtn->setStyleSheet(
                "QPushButton { background: #fff1f0; border: 1px solid #ffa39e; border-radius: 4px; } "
                "QPushButton:hover { background: #cf1322; border-color: #cf1322; } "
                "QPushButton:hover QLabel { color: white; }"
            );
            
            QVBoxLayout *hbtnInnerL = new QVBoxLayout(hardDelBtn);
            hbtnInnerL->setContentsMargins(0, 0, 0, 0);
            QLabel *hbtnText = new QLabel("彻底删除");
            hbtnText->setAlignment(Qt::AlignCenter);
            hbtnText->setStyleSheet("font-size: 11px; font-weight: bold; color: #cf1322; background: transparent; border: none;");
            hbtnInnerL->addWidget(hbtnText);
            
            // 悬停变色逻辑由 QSS 处理背景，这里保持文字颜色同步 (可选)
            
            hardDelBtn->setProperty("petId", info.id);
            connect(hardDelBtn, &QPushButton::clicked, this, &PetModule::onHardDeletePet);
            btnL->addWidget(hardDelBtn);
        }
    }
    
    delBtn->setProperty("petId", info.id);
    btnL->addWidget(delBtn);
    petTable->setCellWidget(row, 6, btnWrap);
    petTable->setItem(row, 6, new QTableWidgetItem("")); 
}

void PetModule::updateStats()
{
    int total = petTable->rowCount();
    int boarding = 0;
    int grooming = 0;

    for (int i = 0; i < total; ++i) {
        QWidget *w = petTable->cellWidget(i, 4); // 在店状态列索引为4
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
    Q_UNUSED(keyword);
    m_currentPage = 1;
    refreshTable();
}

void PetModule::onEditPet()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    for (int i = 0; i < petTable->rowCount(); ++i) {
        QWidget *w = petTable->cellWidget(i, 6);
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
             QString petId = petTable->item(i, 0)->text();
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
    QString id = btn->property("petId").toString();
    
    if (CustomMessageDialog::confirm(this, "删除确认", "确定要将该宠物档案注销吗？\n注销后信息将保留，但默认不显示活跃状态。")) {
        PetDataManager::instance()->removePet(id);
        // refreshTablePreservingSelection 会被信号触发
    }
}

void PetModule::onRestorePet()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString id = btn->property("petId").toString();
    
    PetDataManager::instance()->restorePet(id);
}

void PetModule::onHardDeletePet()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString id = btn->property("petId").toString();
    
    if (CustomMessageDialog::confirm(this, "彻底删除确认", "确定要彻底删除该宠物档案吗？\n此操作不可恢复，会员中心也将不再显示该宠物信息。")) {
        PetDataManager::instance()->hardDeletePet(id);
    }
}

void PetModule::selectPetById(const QString &petId)
{
    for (int i = 0; i < petTable->rowCount(); ++i) {
        if (petTable->item(i, 0)->text() == petId) { // ID 位于第 0 列
            petTable->setCurrentCell(i, 0);
            petTable->scrollToItem(petTable->item(i, 0));
            return;
        }
    }
}

void PetModule::onPrevPage()
{
    if (m_currentPage > 1) {
        m_currentPage--;
        refreshTable();
    }
}

void PetModule::onNextPage()
{
    m_currentPage++;
    refreshTable();
}

void PetModule::onJumpPage()
{
}

void PetModule::updatePagination()
{
    pageLabel->setText(QString("第 %1 页").arg(m_currentPage));
    prevBtn->setEnabled(m_currentPage > 1);
}

#include <QDebug>
bool PetModule::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        // 1. 如果点击的是头像，直接放大
        if (watched->property("avatarPath").isValid()) {
            qDebug() << "[PetModule] Avatar Clicked! Path:" << watched->property("avatarPath").toString();
            showBigImage(watched->property("avatarPath").toString());
            return true;
        }
        
        // 2. 如果点击的是头像周边的信息区域，手动选中该行
        if (watched->property("row").isValid()) {
            int row = watched->property("row").toInt();
            qDebug() << "[PetModule] Info Widget Clicked! Row:" << row;
            petTable->selectRow(row);
            // 这里也可以顺便触发详情刷新逻辑
            onCurrentCellChanged(row, 0, -1, -1);
            return true;
        }
    }
    
    // 3. 处理遮罩层点击（关闭预览）
    if (watched == m_imagePreviewOverlay && event->type() == QEvent::MouseButtonPress) {
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
    qDebug() << "[PetModule] showBigImage called with path:" << path;
    
    QPixmap pix(path);
    if (pix.isNull()) {
        qDebug() << "[PetModule] Target image is null, loading default.";
        pix.load(":/images/load_img.jpg");
    }
    
    // 确保遮罩覆盖整个 PetModule 区域
    m_imagePreviewOverlay->setGeometry(rect());
    
    // 统一标准：取窗口最小维度的 80%
    int maxDim = qMin(this->window()->width(), this->window()->height()) * 0.8;
    m_previewLabel->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    m_imagePreviewOverlay->show();
    m_imagePreviewOverlay->raise(); // 确保遮罩层在最顶层展示
    qDebug() << "[PetModule] overlay shown successfully.";
}

void PetModule::hideBigImage()
{
    m_imagePreviewOverlay->hide();
}

void PetModule::onCurrentCellChanged(int row, int column, int prevRow, int prevCol)
{
    Q_UNUSED(prevRow); Q_UNUSED(prevCol); Q_UNUSED(column);
    if (row < 0) return;

    QTableWidgetItem *idItem = petTable->item(row, 0);
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

void PetModule::onEditPetFromDrawer(const PetInfo &info)
{
    AddPetDialog dlg(this);
    dlg.setPetInfo(info);
    if (dlg.exec() == QDialog::Accepted) {
        PetInfo newInfo = dlg.getPetInfo();
        PetDataManager::instance()->updatePet(newInfo);
        
        updateStats();
        // 同步刷新抽屉详情
        m_drawer->setPet(newInfo, PetDataManager::instance()->getLogs(newInfo.id));
    }
}

void PetModule::onStatusFilterChanged(int /*id*/)
{
    m_currentPage = 1;
    updatePagination();
    updateStats();
}
