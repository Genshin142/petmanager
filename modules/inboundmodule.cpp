#include "inboundmodule.h"
#include "productdatamanager.h"
#include "inboundregistrationdialog.h"
#include <QHeaderView>
#include <QDateTime>
#include <QWheelEvent>
#include <QDebug>
#include "custom_calendar_edit.h"
#include <QGraphicsDropShadowEffect>

InboundModule::InboundModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
    updateRecordList();
    updateStats();

    // 监听数据变化，实现跨模块同步
    connect(ProductDataManager::instance(), &ProductDataManager::productDataChanged, this, [this](){
        updateRecordList();
        updateStats();
    });
}

void InboundModule::setupUI()
{
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(20);

    QWidget *masterPanel = new QWidget();
    QVBoxLayout *masterLayout = new QVBoxLayout(masterPanel);
    masterLayout->setContentsMargins(0, 0, 0, 0);
    masterLayout->setSpacing(20);

    // --- 0. Header: Title ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("商品入库登记中心");
    titleLabel->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    masterLayout->addLayout(headerLayout);

    // --- 1. Stats: Cards ---
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
        if (icon.isEmpty()) {
            iconLabel->hide();
        } else {
            iconLabel->setFixedSize(50, 50); iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setStyleSheet(QString("font-size: 24px; color: %1; background: #f5f7fa; border-radius: 10px; border: none;").arg(color));
        }
        QVBoxLayout *vl = new QVBoxLayout(); vl->setSpacing(2);
        QLabel *tl = new QLabel(title); tl->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        valLabel = new QLabel("0"); valLabel->setStyleSheet("color: #303133; font-size: 24px; border: none; background: transparent;");
        vl->addWidget(tl); vl->addWidget(valLabel); vl->addStretch();
        if (!icon.isEmpty()) {
            cl->addWidget(iconLabel); cl->addSpacing(15);
        }
        cl->addLayout(vl); cl->addStretch();
        return card;
    };
    statLayout->addWidget(createStatCard("", "总入库品种", m_totalCategoriesLabel, "#409eff"));
    statLayout->addWidget(createStatCard("", "今日入库量", m_todayItemsLabel, "#67c23a"));
    statLayout->addWidget(createStatCard("", "待上架批次", m_pendingShelvesLabel, "#e6a23c"));
    masterLayout->addLayout(statLayout);

    // --- 2. Filter Bar ---
    QWidget *filterBar = new QWidget();
    filterBar->setStyleSheet("background: #f8fafc; border-radius: 12px;"); // 去除边框，使用浅背景
    QHBoxLayout *filterLayout = new QHBoxLayout(filterBar);
    filterLayout->setContentsMargins(15, 10, 15, 10);
    filterLayout->setSpacing(15);

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
    QStringList cats = {"全部", "主食", "零食", "玩具", "洗护"};
    for (int i = 0; i < cats.size(); ++i) {
        QPushButton *btn = new QPushButton(cats[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(34);
        btn->setMinimumWidth(64);
        btn->setStyleSheet("QPushButton { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 17px; "
                           "color: #64748b; font-size: 13px; font-weight: bold; padding: 0 15px; } "
                           "QPushButton:hover:!checked { background: #f1f5f9; border-color: #cbd5e1; } "
                           "QPushButton:checked { background: #3b82f6; border-color: #3b82f6; color: white; }");
        if (i == 0) btn->setChecked(true);
        m_categoryGroup->addButton(btn, i);
        filterLayout->addWidget(btn);
    }

    filterLayout->addStretch();

    // New Registration Button
    QPushButton *newBtn = new QPushButton("新增入库登记");
    newBtn->setFixedHeight(36);
    newBtn->setFixedWidth(140); // 移除 emoji 后缩小宽度
    newBtn->setStyleSheet("QPushButton { background: #3b82f6; color: white; font-weight: bold; border-radius: 6px; padding: 0 10px; border: none; } "
                           "QPushButton:hover { background: #2563eb; } "
                           "QPushButton:pressed { background: #1d4ed8; }");
    filterLayout->addWidget(newBtn);

    masterLayout->addWidget(filterBar);

    // --- 2. Table: Audit View ---
    m_recordTable = new QTableWidget();
    m_recordTable->setColumnCount(8); 
    m_recordTable->setHorizontalHeaderLabels({"入库日期", "商品名称", "条形码", "生产日期", "分类", "供应商", "数量", "状态"});
    m_recordTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    
    m_recordTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_recordTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // 商品名称
    m_recordTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch); // 供应商
    
    m_recordTable->setColumnWidth(0, 155); // 入库日期
    m_recordTable->setColumnWidth(2, 120); // 条形码
    m_recordTable->setColumnWidth(3, 100); // 生产日期
    m_recordTable->setColumnWidth(4, 60);  // 分类
    m_recordTable->setColumnWidth(6, 60);  // 数量
    m_recordTable->setColumnWidth(7, 80);  // 状态
    
    m_recordTable->verticalHeader()->setDefaultSectionSize(42); // 增加行高以确保垂直居中效果
    m_recordTable->verticalHeader()->setVisible(false); // 隐藏行号
    
    m_recordTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recordTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recordTable->setStyleSheet("QTableWidget { background: white; border: 1px solid #e2e8f0; border-radius: 12px; } "
                                "QHeaderView::section { background: #f8fafc; border: none; border-bottom: 1px solid #e2e8f0; font-weight: bold; padding: 10px; color: #475569; } "
                                "QTableWidget::item { padding: 8px; }");
    
    masterLayout->addWidget(m_recordTable);

    // --- 3. Detail Drawer ---
    setupDetailDrawer();

    rootLayout->addWidget(masterPanel, 4); // 调整比例，增大右侧
    rootLayout->addWidget(m_detailDrawer, 3);

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
    m_previewImg = new QLabel(); 

    // Connections
    connect(newBtn, &QPushButton::clicked, this, &InboundModule::onNewRegistration);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &InboundModule::onFilterChanged);
    connect(m_startDateEdit, &CustomCalendarEdit::dateChanged, this, &InboundModule::onFilterChanged);
    connect(m_endDateEdit, &CustomCalendarEdit::dateChanged, this, &InboundModule::onFilterChanged);
    connect(m_categoryGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &InboundModule::onFilterChanged);
    connect(m_recordTable, &QTableWidget::itemSelectionChanged, this, &InboundModule::onRecordSelected);
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
}

void InboundModule::setupDetailDrawer()
{
    m_detailDrawer = new QWidget();
    m_detailDrawer->setObjectName("detailDrawer");
    m_detailDrawer->setStyleSheet("QWidget#detailDrawer { background: white; border-left: 1px solid #f1f5f9; }");
    
    QVBoxLayout *l = new QVBoxLayout(m_detailDrawer);
    l->setContentsMargins(25, 25, 25, 25);
    l->setSpacing(20);

    m_drawerTitle = new QLabel("入库详情记录");
    m_drawerTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #1e293b;");
    l->addWidget(m_drawerTitle);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");
    
    QWidget *content = new QWidget();
    m_detailContentLayout = new QVBoxLayout(content);
    m_detailContentLayout->setContentsMargins(0, 0, 0, 0);
    m_detailContentLayout->setSpacing(20);
    
    scroll->setWidget(content);
    l->addWidget(scroll);
}

void InboundModule::onNewRegistration()
{
    InboundRegistrationDialog dlg(this);
    connect(&dlg, &InboundRegistrationDialog::recordAdded, this, &InboundModule::updateRecordList);
    dlg.exec();
}

void InboundModule::onFilterChanged()
{
    // 动态维护范围限制：右边不能小于左边，左边不能大于右边
    QDate start = QDate::fromString(m_startDateEdit->text(), "yyyy-MM-dd");
    QDate end = QDate::fromString(m_endDateEdit->text(), "yyyy-MM-dd");
    
    m_endDateEdit->setMinimumDate(start);
    m_startDateEdit->setMaximumDate(qMin(end, QDate::currentDate()));

    updateRecordList();
}

void InboundModule::updateRecordList()
{
    QList<StockInRecord> allRecords = ProductDataManager::instance()->getAllRecords();
    m_recordTable->setRowCount(0);

    QString searchText = m_searchEdit->text().trimmed().toLower();
    QDate start = QDate::fromString(m_startDateEdit->text(), "yyyy-MM-dd");
    QDate end = QDate::fromString(m_endDateEdit->text(), "yyyy-MM-dd");
    QString catFilter = m_categoryGroup->checkedButton() ? m_categoryGroup->checkedButton()->text() : "全部";

    for (const auto &rec : allRecords) {
        // Date Filter
        QDate recDate = QDateTime::fromString(rec.dateTime, "yyyy-MM-dd HH:mm:ss").date();
        if (recDate < start || recDate > end) continue;

        // Search Filter
        if (!searchText.isEmpty()) {
            if (!rec.productName.toLower().contains(searchText) && !rec.barcode.contains(searchText))
                continue;
        }

        // Category Filter
        ProductInfo info = ProductDataManager::instance()->getProduct(rec.barcode);
        if (catFilter != "全部" && info.category != catFilter) continue;

        // Add to table
        int row = m_recordTable->rowCount();
        m_recordTable->insertRow(row);

        auto createItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        m_recordTable->setItem(row, 0, createItem(rec.dateTime));
        m_recordTable->setItem(row, 1, createItem(rec.productName));
        m_recordTable->setItem(row, 2, createItem(rec.barcode));
        m_recordTable->setItem(row, 3, createItem(rec.productionDate));
        m_recordTable->setItem(row, 4, createItem(info.category.isEmpty() ? "未分类" : info.category));
        m_recordTable->setItem(row, 5, createItem(rec.supplier.isEmpty() ? "未知" : rec.supplier));
        m_recordTable->setItem(row, 6, createItem(QString::number(rec.quantity)));
        
        QWidget *tagWrapper = new QWidget();
        QHBoxLayout *tagLayout = new QHBoxLayout(tagWrapper);
        tagLayout->setContentsMargins(0, 0, 0, 0);
        tagLayout->setSpacing(0);
        
        QLabel *statusTag = new QLabel(rec.isShelved ? "已上架" : "待上架");
        statusTag->setFixedSize(65, 24); 
        statusTag->setAlignment(Qt::AlignCenter);
        tagLayout->addWidget(statusTag, 0, Qt::AlignCenter); 
        
        if (!rec.isShelved) {
            statusTag->setStyleSheet("background: #fff7e6; color: #fa8c16; border-radius: 4px; font-size: 11px;");
        } else {
            statusTag->setStyleSheet("background: #f0f9eb; color: #67c23a; border-radius: 4px; font-size: 11px;");
        }
        
        m_recordTable->setCellWidget(row, 7, tagWrapper);
    }

    // 默认选中第一行
    if (m_recordTable->rowCount() > 0) {
        m_recordTable->setCurrentCell(0, 0);
        onRecordSelected();
    } else {
        // 如果没有数据，清空详情面板（可选）
        // clearDetailPanel(); // 如果有这个函数的话
    }
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

    QString dateTime = m_recordTable->item(row, 0)->text();
    QString prodName = m_recordTable->item(row, 1)->text();
    QString barcode = m_recordTable->item(row, 2)->text();
    QString qtyStr = m_recordTable->item(row, 6)->text();

    QList<StockInRecord> records = ProductDataManager::instance()->getAllRecords();
    StockInRecord rec;
    for(const auto& r : records) {
        if(r.dateTime == dateTime && r.barcode == barcode) {
            rec = r;
            break;
        }
    }

    // --- 1. Hero Section: Unified Layout ---
    QWidget *heroSection = new QWidget();
    heroSection->setStyleSheet("background: white; margin-bottom: 5px;");
    QHBoxLayout *heroLayout = new QHBoxLayout(heroSection);
    heroLayout->setContentsMargins(0, 0, 0, 15);
    heroLayout->setSpacing(15);

    // Image Stack
    QWidget *imgWrapper = new QWidget();
    QVBoxLayout *imgStack = new QVBoxLayout(imgWrapper);
    imgStack->setContentsMargins(0,0,0,0);
    imgStack->setSpacing(5);

    m_detailPreviewImg = new QLabel();
    m_detailPreviewImg->setFixedSize(90, 90); // 稍微缩小一点以适配紧凑布局
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
    m_detailImgIndex = 0;
    switchDetailImage(false);

    // Text Info
    QVBoxLayout *textInfo = new QVBoxLayout();
    textInfo->setSpacing(4);
    textInfo->setAlignment(Qt::AlignVCenter);
    
    QLabel *nameLabel = new QLabel(prodName);
    nameLabel->setStyleSheet("font-size: 18px; font-weight: 800; color: #1e293b; border: none;");
    nameLabel->setWordWrap(true);
    
    QLabel *barcodeLabel = new QLabel("条码：" + barcode);
    barcodeLabel->setStyleSheet("font-size: 12px; color: #94a3b8; border: none;");
    
    textInfo->addWidget(nameLabel);
    textInfo->addWidget(barcodeLabel);

    heroLayout->addWidget(imgWrapper);
    heroLayout->addLayout(textInfo);
    heroLayout->addStretch(); // 将内容推向左侧

    m_detailContentLayout->addWidget(heroSection);

    // --- 2. Grid Area: Electronic Voucher ---
    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(12);

    auto createDetailCard = [&](const QString &label, const QString &val) {
        QFrame *f = new QFrame();
        f->setStyleSheet("background: #f8fafc; border-radius: 10px; border: 1px solid #f1f5f9;");
        QVBoxLayout *l = new QVBoxLayout(f);
        l->setContentsMargins(12, 10, 12, 10);
        l->setSpacing(4);
        
        QLabel *title = new QLabel(label);
        title->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: 600; border: none;");
        QLabel *value = new QLabel(val.isEmpty() ? "-" : val);
        value->setStyleSheet("color: #1e293b; font-size: 13px; font-weight: 700; border: none;");
        value->setWordWrap(true);
        
        l->addWidget(title);
        l->addWidget(value);
        return f;
    };

    ProductInfo pInfo = ProductDataManager::instance()->getProduct(barcode);
    QDate prodDate = QDate::fromString(rec.productionDate, "yyyy-MM-dd");
    QDate expiryDate = prodDate.isValid() ? prodDate.addDays(pInfo.shelfLifeDays) : QDate();
    QString batchId = "IN" + QDateTime::fromString(rec.dateTime, "yyyy-MM-dd HH:mm:ss").toString("yyyyMMdd") + barcode.right(4);

    grid->addWidget(createDetailCard("进货单号", batchId), 0, 0);
    grid->addWidget(createDetailCard("入库数量", QString::number(rec.quantity)), 0, 1);
    grid->addWidget(createDetailCard("规格单位", rec.spec), 1, 0);
    grid->addWidget(createDetailCard("经办人", rec.operatorName), 1, 1);
    grid->addWidget(createDetailCard("生产日期", rec.productionDate), 2, 0);
    grid->addWidget(createDetailCard("保质期", QString::number(pInfo.shelfLifeDays) + " 天"), 2, 1);
    grid->addWidget(createDetailCard("到期日期", expiryDate.isValid() ? expiryDate.toString("yyyy-MM-dd") : "未设定"), 3, 0);
    grid->addWidget(createDetailCard("供应商", rec.supplier.isEmpty() ? "未知" : rec.supplier), 3, 1);
    grid->addWidget(createDetailCard("联系电话", rec.supplierPhone.isEmpty() ? "未填写" : rec.supplierPhone), 4, 0);
    grid->addWidget(createDetailCard("入库时间", rec.dateTime), 4, 1);

    m_detailContentLayout->addLayout(grid);
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
    QPixmap pix(m_detailImagePaths[m_detailImgIndex]);
    m_detailPreviewImg->setPixmap(pix.scaled(m_detailPreviewImg->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    if (m_backdrop->isVisible()) m_largePreviewImg->setPixmap(pix.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
    QPixmap pix(m_imagePaths[m_currentImgIndex]);
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
