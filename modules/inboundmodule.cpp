#include "inboundmodule.h"
#include "productdatamanager.h"
#include "inboundregistrationdialog.h"
#include "custommessagedialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QWheelEvent>
#include <QDebug>
#include "custom_calendar_edit.h"
#include <QProgressBar>
#include <QGraphicsDropShadowEffect>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>

// --- 复刻：全行圆角边框选中委托 ---
class InboundRowDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (opt.state & QStyle::State_MouseOver) {
            painter->fillRect(opt.rect, QColor("#ecf5ff"));
        } else {
            painter->fillRect(opt.rect, Qt::white);
        }

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
            QString text = index.model()->data(index, Qt::DisplayRole).toString();
            if (text.isEmpty()) text = opt.text; // 双重保障
            
            painter->setPen(QColor((opt.state & QStyle::State_Selected) ? "#1e40af" : "#303133"));
            QFont font = painter->font();
            font.setWeight(opt.state & QStyle::State_Selected ? QFont::Bold : QFont::Normal);
            font.setPointSize(10);
            painter->setFont(font);
            
            QRect textRect = opt.rect.adjusted(10, 0, -10, 0);
            painter->drawText(textRect, Qt::AlignCenter, text);
        }
        
        painter->restore();
    }
};

InboundModule::InboundModule(UserRole role, QWidget *parent) 
    : QWidget(parent), m_selectedCategory("全部"), m_currentPage(1), m_pageSize(10), m_role(role)
{
    setupUI();
    updateRecordList();
    updateStats();

    // 监听数据变化，实现跨模块同步
    connect(ProductDataManager::instance(), &ProductDataManager::productDataChanged, this, [this](){
        updateRecordList();
        updateStats();
    });

    connect(ProductDataManager::instance(), &ProductDataManager::inboundListReceived, this, [this](){
        updateRecordList();
        updateStats();
    });

    connect(ProductDataManager::instance(), &ProductDataManager::shelveResult, this, [this](bool success, const QString &msg){
        if (success) {
            CustomMessageDialog::showSuccess(this, "成功", "商品已成功上架并存入档案库");
            ProductDataManager::instance()->requestInboundList(); // 刷新列表
        } else {
            CustomMessageDialog::showWarning(this, "上架失败", msg);
        }
    });

    // 初始请求
    ProductDataManager::instance()->requestInboundList();
    ProductDataManager::instance()->requestProductList();
}

void InboundModule::setupUI()
{
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    QWidget *masterPanel = new QWidget();
    QVBoxLayout *masterLayout = new QVBoxLayout(masterPanel);
    masterLayout->setContentsMargins(20, 20, 20, 20);
    masterLayout->setSpacing(20);

    // --- 0 & 1. 顶部统计与标题容器 ---
    QFrame *topContainer = new QFrame();
    topContainer->setObjectName("TopStatisticsContainer");
    topContainer->setFixedHeight(160);
    topContainer->setStyleSheet("#TopStatisticsContainer { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QVBoxLayout *topLayout = new QVBoxLayout(topContainer);
    topLayout->setContentsMargins(25, 15, 25, 15);
    topLayout->setSpacing(12);

    // --- 0. Header: Title ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("商品入库登记中心");
    titleLabel->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold; border: none; background: transparent;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    topLayout->addLayout(headerLayout);

    // --- 1. Stats: Cards ---
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(15);
    auto createStatCard = [&](const QString &icon, const QString &title, QLabel* &valLabel, const QString &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(80);
        card->setStyleSheet("QFrame { background: #f8fafc; border-radius: 8px; border: 1px solid #f1f5f9; } ");
        QHBoxLayout *cl = new QHBoxLayout(card);
        cl->setContentsMargins(20, 10, 20, 10);
        QLabel *iconLabel = new QLabel(icon);
        if (icon.isEmpty()) {
            iconLabel->hide();
        } else {
            iconLabel->setFixedSize(40, 40); iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setStyleSheet(QString("font-size: 20px; color: %1; background: white; border-radius: 8px; border: 1px solid #f1f5f9;").arg(color));
        }
        QVBoxLayout *vl = new QVBoxLayout(); vl->setSpacing(2);
        QLabel *tl = new QLabel(title); tl->setStyleSheet("color: #94a3b8; font-size: 12px; border: none; background: transparent;");
        valLabel = new QLabel("0"); valLabel->setStyleSheet("color: #1e293b; font-size: 20px; border: none; background: transparent; font-weight: bold;");
        vl->addWidget(tl); vl->addWidget(valLabel); vl->addStretch();
        if (!icon.isEmpty()) {
            cl->addWidget(iconLabel); cl->addSpacing(12);
        }
        cl->addLayout(vl); cl->addStretch();
        return card;
    };
    statLayout->addWidget(createStatCard("", "总入库品种", m_totalCategoriesLabel, "#409eff"));
    statLayout->addWidget(createStatCard("", "今日入库量", m_todayItemsLabel, "#67c23a"));
    statLayout->addWidget(createStatCard("", "待上架批次", m_pendingShelvesLabel, "#e6a23c"));
    statLayout->addWidget(createStatCard("", "总入库成本", m_totalCostLabel, "#9333ea"));
    topLayout->addLayout(statLayout);
    
    masterLayout->addWidget(topContainer);

    // --- 操作中控台 (Operation Console) ---
    QFrame *filterBar = new QFrame();
    filterBar->setFixedHeight(64);
    filterBar->setStyleSheet("QFrame { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QHBoxLayout *filterLayout = new QHBoxLayout(filterBar);
    filterLayout->setContentsMargins(15, 0, 15, 0);
    filterLayout->setSpacing(12);

    // Search
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("搜索商品名称 / 条形码...");
    m_searchEdit->setFixedWidth(240);
    m_searchEdit->setFixedHeight(36);
    m_searchEdit->setStyleSheet("QLineEdit { background: white; border: 1px solid #dcdfe6; border-radius: 6px; padding-left: 10px; color: #606266; } "
                               "QLineEdit:hover { border-color: #c0c4cc; } "
                               "QLineEdit:focus { border-color: #409eff; }");
    filterLayout->addWidget(m_searchEdit);

    // Date Range
    QLabel *dateLabel = new QLabel("时间范围:");
    dateLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold; border: none;"); // 明确去除 label 边框
    filterLayout->addWidget(dateLabel);

    m_startDateEdit = new CustomCalendarEdit();
    m_endDateEdit = new CustomCalendarEdit();
    m_startDateEdit->setText(QDate::currentDate().addDays(-7).toString("yyyy-MM-dd"));
    m_endDateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    m_startDateEdit->setFixedHeight(36);
    m_endDateEdit->setFixedHeight(36);
    m_startDateEdit->setFixedWidth(120);
    m_endDateEdit->setFixedWidth(120);
    
    // 初始限制：不能超过今天
    m_startDateEdit->setMaximumDate(QDate::currentDate());
    m_endDateEdit->setMaximumDate(QDate::currentDate());
    // 联动限制：结束日期不能小于开始日期
    m_endDateEdit->setMinimumDate(QDate::currentDate().addDays(-7));
    
    QString dateStyle = "CustomCalendarEdit { background: white; border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; color: #606266; } "
                        "CustomCalendarEdit:hover { border-color: #c0c4cc; } "
                        "CustomCalendarEdit:focus { border-color: #409eff; }";
    m_startDateEdit->setStyleSheet(dateStyle);
    m_endDateEdit->setStyleSheet(dateStyle);
    
    filterLayout->addWidget(m_startDateEdit);
    QLabel *toLabel = new QLabel("至");
    toLabel->setStyleSheet("border: none; color: #64748b;");
    filterLayout->addWidget(toLabel);
    filterLayout->addWidget(m_endDateEdit);

    filterLayout->addSpacing(10);

    // Category Buttons
    m_categoryGroup = new QButtonGroup(this);
    m_categoryGroup->setExclusive(true);
    QStringList cats = {"全部", "主食", "零食", "玩具", "洗护"};
    for (int i = 0; i < cats.size(); ++i) {
        QPushButton *btn = new QPushButton(cats[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 18px; padding: 0 15px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );
        if (i == 0) btn->setChecked(true);
        m_categoryGroup->addButton(btn, i);
        filterLayout->addWidget(btn);
    }

    filterLayout->addStretch();

    // New Registration Button
    QPushButton *newBtn = new QPushButton("新增入库登记");
    newBtn->setFixedHeight(36);
    newBtn->setFixedWidth(140);
    newBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #3b82f6; color: #3b82f6; border-radius: 8px; font-size: 13px; padding: 0 20px; font-weight: bold; } "
        "QPushButton:hover { background: #eff6ff; }"
    );
    filterLayout->addWidget(newBtn);

    masterLayout->addWidget(filterBar);

    // --- 2. Table: Audit View ---
    m_recordTable = new QTableWidget();
    m_recordTable->setColumnCount(11); 
    m_recordTable->setHorizontalHeaderLabels({"图片", "入库日期", "商品名称", "条形码", "售价", "生产日期", "分类", "入库数量", "当前库存", "状态", "操作"});
    
    QHeaderView *h = m_recordTable->horizontalHeader();
    h->setDefaultAlignment(Qt::AlignCenter);
    h->setSectionResizeMode(QHeaderView::Interactive);
    
    // 0. 图片
    h->setSectionResizeMode(0, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(0, 60);
    
    // 1. 入库日期
    h->setSectionResizeMode(1, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(1, 160);
    
    // 2. 商品名称 (弹性拉伸)
    h->setSectionResizeMode(2, QHeaderView::Stretch);
    
    // 3. 条形码
    h->setSectionResizeMode(3, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(3, 140);
    
    // 4. 售价 (新增)
    h->setSectionResizeMode(4, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(4, 90);
    
    // 5. 生产日期
    h->setSectionResizeMode(5, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(5, 110);

    // 6. 分类
    h->setSectionResizeMode(6, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(6, 80);

    // 7. 入库数量
    h->setSectionResizeMode(7, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(7, 70);
    
    // 8. 当前库存 (新增)
    h->setSectionResizeMode(8, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(8, 80);

    // 9. 状态
    h->setSectionResizeMode(9, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(9, 90);

    // 10. 操作
    h->setSectionResizeMode(10, QHeaderView::Fixed);
    m_recordTable->setColumnWidth(10, 160);

    
    m_recordTable->setItemDelegate(new InboundRowDelegate(m_recordTable));
    
    m_recordTable->setShowGrid(false);
    m_recordTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recordTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_recordTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recordTable->setFocusPolicy(Qt::NoFocus);
    m_recordTable->verticalHeader()->setVisible(false);
    m_recordTable->verticalHeader()->setDefaultSectionSize(60); 
    // 3. 表格卡片容器 (12px 圆角)
    QFrame *tableCard = new QFrame();
    tableCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 5, 0, 0);

    m_recordTable->setStyleSheet("QTableWidget { border: none; background: white; outline: none; border-radius: 6px; } "
                                 "QHeaderView { border: none; background: transparent; border-radius: 12px 12px 0 0; }");
    
    tableLayout->addWidget(m_recordTable);

    // 4. 底部统计与分页
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
    pageLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold; margin: 0 10px;");

    footerLayout->addStretch();
    footerLayout->addWidget(prevBtn);
    footerLayout->addSpacing(20);
    footerLayout->addWidget(pageLabel);
    footerLayout->addSpacing(20);
    footerLayout->addWidget(nextBtn);

    masterLayout->addWidget(tableCard);

    connect(prevBtn, &QPushButton::clicked, this, &InboundModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &InboundModule::onNextPage);

    // --- 3. Detail Drawer ---
    setupDetailDrawer();

    rootLayout->addWidget(masterPanel, 1);
    rootLayout->addWidget(m_detailDrawer);

    // --- 4. Backdrop Overlay (Full-screen style like ProductModule) ---
    m_backdrop = new QWidget(); // Parent will be set to window() on show
    m_backdrop->setStyleSheet("background: rgba(0,0,0,215);");
    m_backdrop->hide();
    m_backdrop->installEventFilter(this);

    QVBoxLayout *backLayout = new QVBoxLayout(m_backdrop);
    backLayout->setContentsMargins(0, 0, 0, 0);
    backLayout->setSpacing(20);
    
    backLayout->addStretch();
    
    m_largePreviewImg = new QLabel();
    m_largePreviewImg->setAlignment(Qt::AlignCenter);
    m_largePreviewImg->setStyleSheet("border: none; background: transparent; padding: 0;");
    m_largePreviewImg->installEventFilter(this);
    backLayout->addWidget(m_largePreviewImg, 0, Qt::AlignCenter);

    m_largeDotsContainer = new QWidget();
    m_largeDotsLayout = new QHBoxLayout(m_largeDotsContainer);
    m_largeDotsLayout->setContentsMargins(0, 0, 0, 0);
    m_largeDotsLayout->setAlignment(Qt::AlignCenter);
    backLayout->addWidget(m_largeDotsContainer);
    
    backLayout->addStretch();

    // Fake preview for sync
    // Connections
    connect(newBtn, &QPushButton::clicked, this, &InboundModule::onNewRegistration);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &InboundModule::onFilterChanged);
    connect(m_startDateEdit, &CustomCalendarEdit::dateChanged, this, &InboundModule::onFilterChanged);
    connect(m_endDateEdit, &CustomCalendarEdit::dateChanged, this, &InboundModule::onFilterChanged);
    connect(m_categoryGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &InboundModule::onFilterChanged);
    connect(m_recordTable, &QTableWidget::itemSelectionChanged, this, &InboundModule::onRecordSelected);
    
    // 数据同步
    connect(ProductDataManager::instance(), &ProductDataManager::productDataChanged, this, [this](){
        updateRecordList();
        onRecordSelected(); // 刷新抽屉按钮状态
    });
}

void InboundModule::updateRecordList()
{
    m_recordTable->setRowCount(0);
    QList<StockInRecord> allRecords = ProductDataManager::instance()->getAllRecords();

    QString searchText = m_searchEdit->text().trimmed().toLower();
    QDate start = QDate::fromString(m_startDateEdit->text(), "yyyy-MM-dd");
    QDate end = QDate::fromString(m_endDateEdit->text(), "yyyy-MM-dd");
    QString catFilter = m_selectedCategory;

    // 1. 过滤
    QList<StockInRecord> filtered;
    for (const auto &rec : allRecords) {
        // Date Filter (兼容 ISO 和 数据库格式)
        QDateTime dt = QDateTime::fromString(rec.dateTime, "yyyy-MM-dd HH:mm:ss");
        if (!dt.isValid()) dt = QDateTime::fromString(rec.dateTime, Qt::ISODate);
        
        QDate recDate = dt.date();
        if (recDate.isValid() && (recDate < start || recDate > end)) continue;

        // Search Filter
        if (!searchText.isEmpty()) {
            if (!rec.productName.toLower().contains(searchText) && !rec.barcode.contains(searchText))
                continue;
        }

        // Category Filter (归一化处理：只看 "/" 后面的部分)
        if (catFilter != "全部") {
            QString actualCat = rec.category.trimmed();
            QString mainCat = actualCat.contains('/') ? actualCat.split('/').last().trimmed() : actualCat;
            
            // 精确匹配或以后缀匹配（兼容“猫零食”匹配“零食”按钮）
            if (mainCat != catFilter && !actualCat.endsWith(catFilter)) continue;
        }

        filtered.append(rec);
    }

    // 2. 排序逻辑：活跃在前，已作废沉底
    std::sort(filtered.begin(), filtered.end(), [](const StockInRecord &a, const StockInRecord &b) {
        if (a.isActive != b.isActive) return a.isActive > b.isActive;
        return a.dateTime > b.dateTime; // 同状态按时间倒序
    });

    // 3. 分页
    int total = filtered.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    int beginIdx = (m_currentPage - 1) * m_pageSize;
    int endIdx = qMin(beginIdx + m_pageSize, total);

    for (int i = beginIdx; i < endIdx; ++i) {
        const auto &rec = filtered[i];
        
        // 解析日期用于显示
        QDateTime displayDt = QDateTime::fromString(rec.dateTime, "yyyy-MM-dd HH:mm:ss");
        if (!displayDt.isValid()) displayDt = QDateTime::fromString(rec.dateTime, Qt::ISODate);

        int row = m_recordTable->rowCount();
        m_recordTable->insertRow(row);

        auto createItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            item->setFont(QFont("Microsoft YaHei", 9));
            return item;
        };

        // 列0：图片 - 优化：使用共享缓存，避免重复 Base64 解码
        QLabel *imgLabel = new QLabel();
        imgLabel->setFixedSize(40, 40);
        imgLabel->setStyleSheet("background: #f8fafc; border-radius: 4px; border: 1px solid #e2e8f0;");
        imgLabel->setCursor(Qt::PointingHandCursor);
        imgLabel->installEventFilter(this);
        imgLabel->setProperty("isTableImg", true);
        imgLabel->setProperty("imgPaths", rec.imgPaths);
        imgLabel->setProperty("barcode", rec.barcode);

        // 尝试从缓存获取已解码的图片
        QPixmap cachedPix = ProductDataManager::instance()->getProductPixmap(rec.barcode);
        if (!cachedPix.isNull()) {
            imgLabel->setPixmap(cachedPix.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else if (!rec.imgData.isEmpty()) {
            // 如果缓存没有（理论上登录时已预加载），则回退到手动加载并存入缓存
            QByteArray ba = QByteArray::fromBase64(rec.imgData.toUtf8());
            QPixmap pix;
            if (pix.loadFromData(ba)) {
                imgLabel->setPixmap(pix.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                imgLabel->setText("损坏");
            }
        } else {
            imgLabel->setText("无图");
            imgLabel->setAlignment(Qt::AlignCenter);
            imgLabel->setStyleSheet("font-size: 10px; color: #94a3b8; background: #f8fafc; border-radius: 4px;");
        }
        QWidget *imgWrap = new QWidget();
        QHBoxLayout *imgL = new QHBoxLayout(imgWrap);
        imgL->setContentsMargins(0, 0, 0, 0);
        imgL->setAlignment(Qt::AlignCenter);
        imgL->addWidget(imgLabel);
        m_recordTable->setCellWidget(row, 0, imgWrap);
        m_recordTable->setItem(row, 0, new QTableWidgetItem()); // 必须先创建 item，否则下面的 item(row, 0) 为空指针
        m_recordTable->item(row, 0)->setData(Qt::UserRole + 1, true);

        m_recordTable->setItem(row, 1, createItem(displayDt.toString("yyyy-MM-dd HH:mm")));
        QTableWidgetItem *nameItem = createItem(rec.productName);
        nameItem->setData(Qt::UserRole, rec.id); // 存储 ID 用于精准操作
        m_recordTable->setItem(row, 2, nameItem);
        m_recordTable->setItem(row, 3, createItem(rec.barcode));
        
        // 优先显示本次入库登记的拟定售价
        QString priceStr = (rec.salePrice > 0) ? QString("¥%1").arg(rec.salePrice, 0, 'f', 2) : "-";
        
        // 如果入库单没价格，再尝试找商品档案
        if (priceStr == "-") {
            ProductInfo pInfo = ProductDataManager::instance()->getProduct(rec.barcode);
            if (!pInfo.barcode.isEmpty() && pInfo.price > 0) {
                priceStr = QString("¥%1").arg(pInfo.price, 0, 'f', 2);
            }
        }
        
        auto *priceItem = createItem(priceStr);
        priceItem->setForeground(QBrush(QColor("#3b82f6")));
        priceItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
        m_recordTable->setItem(row, 4, priceItem);

        ProductInfo pInfo = ProductDataManager::instance()->getProduct(rec.barcode);
        bool isLowStock = !pInfo.barcode.isEmpty() && pInfo.stock <= pInfo.minStock;

        m_recordTable->setItem(row, 2, createItem(isLowStock ? "⚠️ " + rec.productName : rec.productName));
        m_recordTable->setItem(row, 3, createItem(rec.barcode));

        // ... (Price handled above)

        m_recordTable->setItem(row, 5, createItem(rec.productionDate));
        m_recordTable->setItem(row, 6, createItem(rec.category.isEmpty() ? "未分类" : rec.category));
        m_recordTable->setItem(row, 7, createItem(QString::number(rec.quantity)));
        
        // 获取当前最新库存 (从商品档案中获取)
        QString stockStr = pInfo.barcode.isEmpty() ? "-" : QString::number(pInfo.stock);
        auto *stockItem = createItem(isLowStock ? "⚠️ " + stockStr : stockStr);
        if (isLowStock) {
            stockItem->setForeground(QBrush(QColor("#ef4444"))); // 库存预警变红
            stockItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
        }
        m_recordTable->setItem(row, 8, stockItem);
        
        QWidget *tagWrapper = new QWidget();
        QHBoxLayout *tagLayout = new QHBoxLayout(tagWrapper);
        tagLayout->setContentsMargins(0, 0, 0, 0);
        tagLayout->setSpacing(0);
        
        QLabel *statusTag = new QLabel(rec.isShelved ? "已上架" : "待上架");
        statusTag->setFixedHeight(22);
        statusTag->setFixedWidth(65);
        statusTag->setAlignment(Qt::AlignCenter);
        
        QString tagStyle = "border-radius: 11px; font-size: 11px; font-weight: bold; border: 1px solid %1; background-color: %2; color: %3;";
        
        if (!rec.isActive) {
            // 灰色 - 已作废
            statusTag->setText("已作废");
            statusTag->setStyleSheet(tagStyle.arg("#e2e8f0", "#f4f4f5", "#94a3b8"));
        } else if (!rec.isShelved) {
            // 橙色 - 对应 Element UI Warning 风格
            statusTag->setStyleSheet(tagStyle.arg("#f5dab1", "#fff7e6", "#e6a23c"));
        } else {
            // 绿色 - 对应 Element UI Success 风格 (会员模块充值按钮样式)
            statusTag->setStyleSheet(tagStyle.arg("#c2e7b0", "#f0f9eb", "#67c23a"));
        }
        tagLayout->addWidget(statusTag, 0, Qt::AlignCenter); 
        
        m_recordTable->setCellWidget(row, 9, tagWrapper);
        m_recordTable->setItem(row, 9, new QTableWidgetItem()); // 占位
        m_recordTable->item(row, 9)->setData(Qt::UserRole + 1, true); // 标记此列已有 Widget

        // 列8：操作
        QWidget *btnWrap = new QWidget();
        QHBoxLayout *btnL = new QHBoxLayout(btnWrap);
        btnL->setContentsMargins(0, 0, 0, 0);
        btnL->setAlignment(Qt::AlignCenter);
        
        QPushButton *delBtn = new QPushButton();
        delBtn->setObjectName("TableDeleteBtn");
        delBtn->setFixedSize(60, 28);
        delBtn->setCursor(Qt::PointingHandCursor);
        QLabel *btnText = new QLabel();
        btnText->setAlignment(Qt::AlignCenter);
        btnText->setStyleSheet("font-size: 11px; font-weight: bold; background: transparent; border: none;");

        QVBoxLayout *btnInnerL = new QVBoxLayout(delBtn);
        btnInnerL->setContentsMargins(0, 0, 0, 0);

        if (rec.isActive) {
            delBtn->setStyleSheet(
                "QPushButton#TableDeleteBtn { background: #fef0f0; border: 1px solid #fbc4c4; border-radius: 4px; } "
                "QPushButton#TableDeleteBtn:hover { background: #f56c6c; border-color: #f56c6c; } "
            );
            btnText->setText("删除");
            btnText->setStyleSheet(btnText->styleSheet() + "color: #f56c6c;");
            connect(delBtn, &QPushButton::clicked, this, &InboundModule::onDeleteRecord);
        } else {
            delBtn->setStyleSheet(
                "QPushButton#TableDeleteBtn { background: #f0f9eb; border: 1px solid #c2e7b0; border-radius: 4px; } "
                "QPushButton#TableDeleteBtn:hover { background: #67c23a; border-color: #67c23a; } "
            );
            btnText->setText("恢复");
            btnText->setStyleSheet(btnText->styleSheet() + "color: #67c23a;");
            connect(delBtn, &QPushButton::clicked, this, &InboundModule::onRestoreRecord);
            
            // 管理员专属：彻底删除
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
                
                hardDelBtn->setProperty("dateTime", rec.dateTime);
                hardDelBtn->setProperty("barcode", rec.barcode);
                connect(hardDelBtn, &QPushButton::clicked, this, &InboundModule::onHardDeleteRecord);
                btnL->addWidget(hardDelBtn);
            }
        }
        btnInnerL->addWidget(btnText);
        
        // 如果用户希望悬停时文字还是红色，我们就不在 CSS 里写 :hover QLabel { color: white; }
        // 保持现状或根据需要调整

        // 将关键数据绑定到按钮，方便删除时定位
        delBtn->setProperty("dateTime", rec.dateTime);
        delBtn->setProperty("barcode", rec.barcode);
        
        btnL->addWidget(delBtn);
        m_recordTable->setCellWidget(row, 10, btnWrap);
        m_recordTable->setItem(row, 10, new QTableWidgetItem());
        m_recordTable->item(row, 10)->setData(Qt::UserRole + 1, true);
    }

    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);

    // 默认选中第一行
    if (m_recordTable->rowCount() > 0) {
        m_recordTable->setCurrentCell(0, 0);
        onRecordSelected();
    }
    updateStats();
}

void InboundModule::onDeleteRecord()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString dateTime = btn->property("dateTime").toString();
    QString barcode = btn->property("barcode").toString();

    if (CustomMessageDialog::confirm(this, "作废确认", "确定要作废这条入库记录吗？\n作废后数据将保留并移至列表末尾。")) {
        ProductDataManager::instance()->removeRecord(dateTime, barcode);
        updateRecordList(); // 重新加载数据
    }
}

void InboundModule::onRestoreRecord()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString dateTime = btn->property("dateTime").toString();
    QString barcode = btn->property("barcode").toString();

    ProductDataManager::instance()->restoreRecord(dateTime, barcode);
    updateRecordList();
}

void InboundModule::onHardDeleteRecord()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString dateTime = btn->property("dateTime").toString();
    QString barcode = btn->property("barcode").toString();

    if (CustomMessageDialog::confirm(this, "彻底删除确认", "确定要彻底删除这条记录吗？\n如果该商品已上架，对应的商品档案也将被同步删除。此操作不可恢复！")) {
        ProductDataManager::instance()->hardDeleteRecord(dateTime, barcode);
        updateRecordList();
    }
}

void InboundModule::onPrevPage() {
    if (m_currentPage > 1) {
        m_currentPage--;
        updateRecordList();
    }
}

void InboundModule::onNextPage() {
    m_currentPage++;
    updateRecordList();
}

void InboundModule::onFilterChanged()
{
    // 处理分类选择 (从 ButtonGroup 获取当前选中的按钮)
    if (m_categoryGroup) {
        QPushButton *checkedBtn = qobject_cast<QPushButton*>(m_categoryGroup->checkedButton());
        if (checkedBtn) {
            m_selectedCategory = checkedBtn->text();
        }
    }

    // 动态维护范围限制
    QDate start = QDate::fromString(m_startDateEdit->text(), "yyyy-MM-dd");
    QDate end = QDate::fromString(m_endDateEdit->text(), "yyyy-MM-dd");
    
    m_endDateEdit->setMinimumDate(start);
    m_startDateEdit->setMaximumDate(qMin(end, QDate::currentDate()));

    m_currentPage = 1;
    updateRecordList();
    updateStats();
}

void InboundModule::updateStats()
{
    auto allProducts = ProductDataManager::instance()->allProducts();
    m_totalCategoriesLabel->setText(QString("%1 种").arg(allProducts.size()));

    auto allRecords = ProductDataManager::instance()->getAllRecords();
    int todayTotal = 0;
    QString todayStr = QDate::currentDate().toString("yyyy-MM-dd");
    for (const auto &rec : allRecords) {
        if (rec.dateTime.startsWith(todayStr)) {
            todayTotal += rec.quantity;
        }
    }
    m_todayItemsLabel->setText(QString("%1 件").arg(todayTotal));

    auto pending = ProductDataManager::instance()->getUnlistedInboundItems();
    m_pendingShelvesLabel->setText(QString("%1 批").arg(pending.size()));

    double totalCost = 0;
    for (const auto &rec : allRecords) {
        if (rec.isActive) {
            totalCost += (rec.quantity * rec.costPrice);
        }
    }
    m_totalCostLabel->setText(QString("¥ %1").arg(QString::number(totalCost, 'f', 2)));
}

void InboundModule::setupDetailDrawer()
{
    m_detailDrawer = new QWidget();
    m_detailDrawer->setObjectName("detailDrawer");
    // 移除外层背景和左边框，仅保留内部卡片视觉
    m_detailDrawer->setStyleSheet("QWidget#detailDrawer { background: transparent; border: none; }");
    m_detailDrawer->setFixedWidth(450); 
    
    QVBoxLayout *outerLayout = new QVBoxLayout(m_detailDrawer);
    outerLayout->setContentsMargins(20, 20, 20, 20); // 关键：在这里留出和左侧一致的外边距
    outerLayout->setSpacing(0);

    // 内部圆角卡片容器
    m_drawerContainer = new QFrame();
    m_drawerContainer->setObjectName("DrawerContainer");
    m_drawerContainer->setStyleSheet("#DrawerContainer { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    m_drawerContainer->setAttribute(Qt::WA_StyledBackground);
    
    QVBoxLayout *l = new QVBoxLayout(m_drawerContainer);
    l->setContentsMargins(20, 30, 20, 30); // 增加上下内边距
    l->setSpacing(25); // 增加间距
    outerLayout->addWidget(m_drawerContainer);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");
    
    QWidget *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    m_detailContentLayout = new QVBoxLayout(content);
    m_detailContentLayout->setContentsMargins(0, 0, 0, 0);
    m_detailContentLayout->setSpacing(20);
    
    scroll->setWidget(content);
    l->addWidget(scroll);

    // 底部操作区 (上架按钮)
    m_shelveBtn = new QPushButton("确认上架并开售");
    m_shelveBtn->setFixedHeight(48);
    m_shelveBtn->setCursor(Qt::PointingHandCursor);
    m_shelveBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border: none; border-radius: 12px; font-size: 15px; font-weight: bold; } "
        "QPushButton:hover { background: #2563eb; } "
        "QPushButton:disabled { background: #e2e8f0; color: #94a3b8; }"
    );
    l->addWidget(m_shelveBtn);
    m_shelveBtn->hide();

    m_unshelveBtn = new QPushButton("下架商品");
    m_unshelveBtn->setFixedHeight(48);
    m_unshelveBtn->setCursor(Qt::PointingHandCursor);
    m_unshelveBtn->setStyleSheet(
        "QPushButton { background: white; color: #ef4444; border: 1px solid #ef4444; border-radius: 12px; font-size: 15px; font-weight: bold; } "
        "QPushButton:hover { background: #fef2f2; } "
    );
    l->addWidget(m_unshelveBtn);
    m_unshelveBtn->hide();

    // 初始化编辑与上架按钮
    m_editBtn = new QPushButton("修改资料", m_drawerContainer);
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
    
    m_editBtn->move(297, 21);
    m_editBtn->raise();
    m_editBtn->hide();
    
    connect(m_editBtn, &QPushButton::clicked, this, &InboundModule::onEditProductFromDrawer);
    connect(m_shelveBtn, &QPushButton::clicked, this, &InboundModule::onShelveFromDrawer);
    connect(m_unshelveBtn, &QPushButton::clicked, this, [this](){
        int row = m_recordTable->currentRow();
        if (row < 0) return;
        QTableWidgetItem *nameItem = m_recordTable->item(row, 2);
        if (!nameItem) return;
        int id = nameItem->data(Qt::UserRole).toInt();
        if (id > 0) {
            ProductDataManager::instance()->unshelveProduct(id);
        }
    });
}

void InboundModule::onNewRegistration()
{
    InboundRegistrationDialog dlg(this);
    connect(&dlg, &InboundRegistrationDialog::recordAdded, this, &InboundModule::updateRecordList);
    dlg.exec();
}

void InboundModule::onRecordSelected()
{
    int row = m_recordTable->currentRow();
    if (row < 0) return;

    // 清除现有内容
    // 使用递归函数彻底清理布局，防止子布局残留导致重叠
    std::function<void(QLayout*)> clearLayout = [&](QLayout* layout) {
        if (!layout) return;
        while (QLayoutItem* item = layout->takeAt(0)) {
            if (QWidget* widget = item->widget()) {
                widget->deleteLater();
            } else if (QLayout* childLayout = item->layout()) {
                clearLayout(childLayout);
            }
            delete item;
        }
    };
    clearLayout(m_detailContentLayout);

    QString dateTime = m_recordTable->item(row, 1)->text(); 
    QString prodName = m_recordTable->item(row, 2)->text();
    QString barcode = m_recordTable->item(row, 3)->text();
    
    QList<StockInRecord> records = ProductDataManager::instance()->getAllRecords();
    StockInRecord rec;
    for(const auto& r : records) {
        if(r.barcode == barcode && r.dateTime.contains(dateTime)) {
            rec = r;
            break;
        }
    }

    // 获取商品档案信息以补充详情
    ProductInfo pInfo = ProductDataManager::instance()->getProduct(barcode);

    // 关键修复：计算派生字段
    QDate prodDate = QDate::fromString(rec.productionDate, "yyyy-MM-dd");
    int lifeDays = rec.shelfLifeDays > 0 ? rec.shelfLifeDays : pInfo.shelfLifeDays;
    QDate expiryDate = prodDate.isValid() ? prodDate.addDays(lifeDays) : QDate();

    // --- 1. Hero Section (Product Header) ---
    QWidget *heroSection = new QWidget();
    heroSection->setStyleSheet("background: transparent; margin-bottom: 5px;");
    QHBoxLayout *heroLayout = new QHBoxLayout(heroSection);
    heroLayout->setContentsMargins(0, 0, 0, 15);
    heroLayout->setSpacing(15);

    // Image Stack
    QWidget *imgWrapper = new QWidget();
    QVBoxLayout *imgStack = new QVBoxLayout(imgWrapper);
    imgStack->setContentsMargins(0,0,0,0);
    imgStack->setSpacing(5);

    m_detailPreviewImg = new QLabel();
    m_detailPreviewImg->setFixedSize(95, 95); 
    m_detailPreviewImg->setStyleSheet("background: #f8fafc; border-radius: 12px; border: 1px solid #e2e8f0;");
    m_detailPreviewImg->installEventFilter(this);
    
    m_detailDotsContainer = new QWidget();
    m_detailDotsContainer->setFixedHeight(12);
    m_detailDotsLayout = new QHBoxLayout(m_detailDotsContainer);
    m_detailDotsLayout->setContentsMargins(0,0,0,0);
    m_detailDotsLayout->setAlignment(Qt::AlignCenter);
    m_detailDotsLayout->setSpacing(4);

    imgStack->addWidget(m_detailPreviewImg);
    imgStack->addWidget(m_detailDotsContainer);

    m_detailImagePaths = rec.imgPaths;
    if (!rec.imgData.isEmpty() && !m_detailImagePaths.contains(rec.imgData)) {
        m_detailImagePaths.prepend(rec.imgData); // 优先显示数据库里的图
    }
    m_detailImgIndex = 0;
    switchDetailImage(false);

    // Text Info
    QVBoxLayout *textInfo = new QVBoxLayout();
    textInfo->setSpacing(6);
    textInfo->setAlignment(Qt::AlignVCenter);
    
    QLabel *nameLabel = new QLabel(prodName);
    // 关键修复：增加行高和显示完整性
    nameLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1e293b; border: none; line-height: 1.4;");
    nameLabel->setWordWrap(true);
    nameLabel->setMinimumHeight(50); // 确保多行显示时不会被挤压
    
    QLabel *barcodeLabel = new QLabel("条码：" + barcode);
    barcodeLabel->setStyleSheet("font-size: 12px; color: #94a3b8; border: none;");
    
    textInfo->addWidget(nameLabel);
    textInfo->addWidget(barcodeLabel);

    heroLayout->addWidget(imgWrapper);
    heroLayout->addLayout(textInfo, 1);
    heroLayout->addStretch(); 

    m_detailContentLayout->addWidget(heroSection);

    // --- 1.1 预警横幅 (新增) ---
    if (!pInfo.barcode.isEmpty() && pInfo.stock <= pInfo.minStock) {
        QWidget *warningBanner = new QWidget();
        warningBanner->setFixedHeight(40);
        warningBanner->setStyleSheet("background: #fef2f2; border: 1px solid #fee2e2; border-radius: 8px; margin: 0 5px 15px 5px;");
        QHBoxLayout *warnL = new QHBoxLayout(warningBanner);
        warnL->setContentsMargins(12, 0, 12, 0);
        
        QLabel *warnIcon = new QLabel("⚠️");
        QLabel *warnText = new QLabel(QString("库存严重不足！当前 %1 (预警值 %2)").arg(pInfo.stock).arg(pInfo.minStock));
        warnText->setStyleSheet("color: #b91c1c; font-size: 13px; font-weight: bold; border: none; background: transparent;");
        
        warnL->addWidget(warnIcon);
        warnL->addWidget(warnText);
        warnL->addStretch();
        
        m_detailContentLayout->addWidget(warningBanner);
    }
    
    // 控制操作按钮显示
    if (rec.isActive) {
        if (!rec.isShelved) {
            m_shelveBtn->show();
            m_unshelveBtn->hide();
        } else {
            m_shelveBtn->hide();
            m_unshelveBtn->show();
        }
    } else {
        m_shelveBtn->hide();
        m_unshelveBtn->hide();
    }

    // --- 2. Grouped Details Area (Redesigned from 'Boxes' to 'List') ---
    auto createDetailGroup = [&](const QString &title, const QList<QPair<QString, QString>> &items) {
        QWidget *groupContainer = new QWidget();
        QVBoxLayout *containerL = new QVBoxLayout(groupContainer);
        containerL->setContentsMargins(0, 0, 0, 0);
        containerL->setSpacing(10);

        QLabel *titleL = new QLabel(title);
        titleL->setStyleSheet("color: #334155; font-size: 15px; font-weight: bold; margin-left: 4px;");
        containerL->addWidget(titleL);

        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; }");
        QVBoxLayout *cardL = new QVBoxLayout(card);
        cardL->setContentsMargins(16, 16, 16, 16);
        cardL->setSpacing(12);

        for (int i = 0; i < items.size(); ++i) {
            QHBoxLayout *row = new QHBoxLayout();
            
            QLabel *label = new QLabel(items[i].first);
            label->setStyleSheet("color: #94a3b8; font-size: 13px; border: none; background: transparent;");
            
            QLabel *val = new QLabel(items[i].second.isEmpty() ? "-" : items[i].second);
            val->setStyleSheet("color: #1e293b; font-size: 13px; font-weight: 600; border: none; background: transparent;");
            
            row->addWidget(label);
            row->addStretch();
            row->addWidget(val);
            cardL->addLayout(row);
        }
        cardL->setSpacing(14); // 增加行间距以补偿分割线的移除
        containerL->addWidget(card);
        return groupContainer;
    };

    QList<QPair<QString, QString>> inboundInfo;
    inboundInfo << qMakePair(QString("入库单号"), rec.inboundNo);
    inboundInfo << qMakePair(QString("入库数量"), QString::number(rec.quantity));
    inboundInfo << qMakePair(QString("当前总库存"), pInfo.barcode.isEmpty() ? "-" : QString::number(pInfo.stock));
    inboundInfo << qMakePair(QString("库存预警值"), pInfo.barcode.isEmpty() ? "未设" : QString::number(pInfo.minStock));
    inboundInfo << qMakePair(QString("规格单位"), rec.spec.isEmpty() ? "-" : rec.spec);
    inboundInfo << qMakePair(QString("入库时间"), rec.dateTime);
    inboundInfo << qMakePair(QString("零售标价"), (rec.salePrice > 0) ? QString("¥ %1").arg(rec.salePrice, 0, 'f', 2) : 
                               (pInfo.price > 0 ? QString("¥ %1").arg(pInfo.price, 0, 'f', 2) : "未设价格"));
    inboundInfo << qMakePair(QString("经办人"), rec.operatorName.isEmpty() ? "-" : rec.operatorName);
    
    m_detailContentLayout->addWidget(createDetailGroup("物流与库存记录", inboundInfo));

    // --- 3. 供应链与生产信息 (带保质期进度条) ---
    QWidget *supGroup = new QWidget();
    QVBoxLayout *supL = new QVBoxLayout(supGroup);
    supL->setContentsMargins(0, 0, 0, 0);
    supL->setSpacing(10);

    QLabel *supTitle = new QLabel("供应链与生产信息");
    supTitle->setStyleSheet("color: #334155; font-size: 15px; font-weight: bold; margin-left: 4px;");
    supL->addWidget(supTitle);

    QFrame *supCard = new QFrame();
    supCard->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; }");
    QVBoxLayout *supCardL = new QVBoxLayout(supCard);
    supCardL->setContentsMargins(16, 16, 16, 16);
    supCardL->setSpacing(14);

    auto addSupRow = [&](const QString &l, const QString &v) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *lbl = new QLabel(l);
        lbl->setStyleSheet("color: #94a3b8; font-size: 13px; border: none; background: transparent;");
        QLabel *val = new QLabel(v.isEmpty() ? "-" : v);
        val->setStyleSheet("color: #1e293b; font-size: 13px; font-weight: 600; border: none; background: transparent;");
        row->addWidget(lbl);
        row->addStretch();
        row->addWidget(val);
        supCardL->addLayout(row);
    };

    addSupRow("供应商", rec.supplier.isEmpty() ? "未知" : rec.supplier);
    addSupRow("供应商电话", rec.supplierPhone.isEmpty() ? "-" : rec.supplierPhone);
    addSupRow("生产日期", rec.productionDate.isEmpty() ? "-" : rec.productionDate);
    addSupRow("保质期", QString::number(lifeDays) + " 天");
    addSupRow("到期日期", expiryDate.isValid() ? expiryDate.toString("yyyy-MM-dd") : "未设定");

    // 进度条逻辑
    if (expiryDate.isValid()) {
        QDate prodDate = QDate::fromString(rec.productionDate, "yyyy-MM-dd");
        if (prodDate.isValid()) {
            qint64 total = prodDate.daysTo(expiryDate);
            qint64 passed = prodDate.daysTo(QDate::currentDate());
            double percent = 0;
            if (total > 0) percent = qBound(0.0, (double)passed / total * 100.0, 100.0);

            QVBoxLayout *progLayout = new QVBoxLayout();
            progLayout->setSpacing(6);
            progLayout->setContentsMargins(0, 5, 0, 0);

            QHBoxLayout *progHeader = new QHBoxLayout();
            QLabel *progTitle = new QLabel("保质期消耗进度");
            progTitle->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; border: none; background: transparent;");
            QLabel *progVal = new QLabel(QString("%1%").arg(percent, 0, 'f', 1));
            progVal->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; border: none; background: transparent;").arg(percent > 90 ? "#ef4444" : (percent > 70 ? "#f59e0b" : "#3b82f6")));
            progHeader->addWidget(progTitle);
            progHeader->addStretch();
            progHeader->addWidget(progVal);

            QProgressBar *bar = new QProgressBar();
            bar->setFixedHeight(8);
            bar->setRange(0, 100);
            bar->setValue((int)percent);
            bar->setTextVisible(false);
            
            QString barColor = "#3b82f6";
            if (percent >= 100) barColor = "#ef4444";
            else if (percent > 80) barColor = "#f59e0b";

            bar->setStyleSheet(QString(
                "QProgressBar { background: #e2e8f0; border-radius: 4px; border: none; } "
                "QProgressBar::chunk { background: %1; border-radius: 4px; }"
            ).arg(barColor));

            progLayout->addLayout(progHeader);
            progLayout->addWidget(bar);
            supCardL->addLayout(progLayout);
        }
    }

    supL->addWidget(supCard);
    m_detailContentLayout->addWidget(supGroup);
    m_detailContentLayout->addStretch();
}

void InboundModule::switchDetailImage(bool next)
{
    if (m_detailImagePaths.isEmpty()) {
        m_detailPreviewImg->setText("无图");
        m_detailPreviewImg->setAlignment(Qt::AlignCenter);
        m_detailDotsContainer->hide();
        return;
    }
    if (next) m_detailImgIndex = (m_detailImgIndex + 1) % m_detailImagePaths.size();
    QString imgSource = m_detailImagePaths[m_detailImgIndex];
    QPixmap pix;
    if (imgSource.length() > 500) { // 简单判断是否为 Base64
        QByteArray ba = QByteArray::fromBase64(imgSource.toUtf8());
        pix.loadFromData(ba);
    } else {
        pix.load(imgSource);
    }
    
    if (pix.isNull()) {
        m_detailPreviewImg->setText("图片丢失");
    } else {
        m_detailPreviewImg->setPixmap(pix.scaled(m_detailPreviewImg->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        if (m_backdrop->isVisible()) {
            int maxDim = qMin(this->window()->width(), this->window()->height()) * 0.8;
            m_largePreviewImg->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    updateDetailDots();
}

void InboundModule::updateDetailDots()
{
    QLayoutItem *child;
    while ((child = m_detailDotsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    if (m_detailImagePaths.size() <= 1) { m_detailDotsContainer->hide(); return; }
    m_detailDotsContainer->show();
    for (int i = 0; i < m_detailImagePaths.size(); ++i) {
        QFrame *dot = new QFrame();
        dot->setFixedSize(8, 8);
        dot->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        dot->setStyleSheet(i == m_detailImgIndex ? "background: #3b82f6; border-radius: 4px;" : "background: #e2e8f0; border-radius: 4px;");
        m_detailDotsLayout->addWidget(dot, 0, Qt::AlignCenter);
    }
}

void InboundModule::showLargePreview()
{
    if (m_imagePaths.isEmpty()) return;
    
    // 照搬商品模块逻辑：覆盖整个主窗口并居中
    QWidget *mainWin = this->window();
    m_backdrop->setParent(mainWin);
    m_backdrop->setGeometry(0, 0, mainWin->width(), mainWin->height());
    
    m_backdrop->show();
    m_backdrop->raise();
    switchImage(false);
}

void InboundModule::switchImage(bool next)
{
    if (m_imagePaths.isEmpty()) return;
    if (next) m_currentImgIndex = (m_currentImgIndex + 1) % m_imagePaths.size();
    QString imgSource = m_imagePaths[m_currentImgIndex];
    QPixmap pix;
    if (imgSource.length() > 500) {
        pix.loadFromData(QByteArray::fromBase64(imgSource.toUtf8()));
    } else {
        pix.load(imgSource);
    }
    
    int maxDim = qMin(this->window()->width(), this->window()->height()) * 0.8;
    m_largePreviewImg->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    updateLargeDots();
}

void InboundModule::updateLargeDots()
{
    QLayoutItem *child;
    while ((child = m_largeDotsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        QLabel *dot = new QLabel();
        dot->setFixedSize(8, 8);
        dot->setStyleSheet(i == m_currentImgIndex ? "background: white; border-radius: 4px;" : "background: rgba(255,255,255,100); border-radius: 4px;");
        m_largeDotsLayout->addWidget(dot);
    }
}

bool InboundModule::eventFilter(QObject *watched, QEvent *event)
{
    QLabel *watchedLabel = qobject_cast<QLabel*>(watched);
    
    // 1. 处理详情页主图
    if (watched == m_detailPreviewImg) {
        if (event->type() == QEvent::Wheel) {
            switchDetailImage(static_cast<QWheelEvent*>(event)->angleDelta().y() < 0);
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            m_imagePaths = m_detailImagePaths;
            m_currentImgIndex = m_detailImgIndex;
            showLargePreview();
            return true;
        }
    }
    
    // 2. 处理表格中的小图 (点击放大)
    if (watchedLabel && watchedLabel->property("isTableImg").toBool()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QStringList paths = watchedLabel->property("imgPaths").toStringList();
            QString base64Data = watchedLabel->property("imgData").toString();
            
            m_imagePaths = paths;
            if (!base64Data.isEmpty() && !m_imagePaths.contains(base64Data)) {
                m_imagePaths.prepend(base64Data);
            }
            m_currentImgIndex = 0;
            showLargePreview();
            return true;
        }
    }

    // 3. 处理大图预览背景及大图 (点击关闭/滚动切换)
    if (watched == m_backdrop || watched == m_largePreviewImg) {
        if (event->type() == QEvent::Wheel) {
            switchImage(static_cast<QWheelEvent*>(event)->angleDelta().y() < 0);
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            m_backdrop->hide();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void InboundModule::onEditProductFromDrawer()
{
    int row = m_recordTable->currentRow();
    if (row < 0) return;

    // 获取唯一标识：入库时间 + 条码 (注意列索引的变化)
    QString dateTime = m_recordTable->item(row, 1)->text(); // 入库日期在第 1 列
    QString barcode = m_recordTable->item(row, 3)->text();  // 条形码在第 3 列
    
    // 1. 获取入库记录
    QList<StockInRecord> records = ProductDataManager::instance()->getAllRecords();
    StockInRecord rec;
    bool found = false;
    for(const auto& r : records) {
        // 兼容处理：dateTime 可能包含秒，text() 可能只显示到分
        if(r.dateTime.contains(dateTime) && r.barcode == barcode) {
            rec = r;
            found = true;
            break;
        }
    }
    
    if (!found) return;

    // 2. 获取商品档案信息（用于填充分类等）
    ProductInfo pInfo = ProductDataManager::instance()->getProduct(barcode);

    // 3. 打开登记对话框并设置为编辑模式
    InboundRegistrationDialog dlg(this);
    dlg.setEditMode(rec, pInfo);
    
    connect(&dlg, &InboundRegistrationDialog::recordUpdated, this, [this](){
        updateRecordList();
        onRecordSelected(); // 刷新详情面板
    });

    dlg.exec();
}

void InboundModule::onShelveFromDrawer()
{
    int row = m_recordTable->currentRow();
    if (row < 0) return;

    QTableWidgetItem *nameItem = m_recordTable->item(row, 2);
    if (!nameItem) return;
    
    int inboundId = nameItem->data(Qt::UserRole).toInt();
    if (inboundId <= 0) {
        CustomMessageDialog::showWarning(this, "错误", "无法解析记录ID，请刷新列表后再试。");
        return;
    }

    ProductDataManager::instance()->shelveProduct(inboundId);
}
