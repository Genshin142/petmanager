#include "productmodule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QHeaderView>
#include <QScrollBar>
#include <QDate>
#include <QComboBox>
#include <QFrame>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QMessageBox>
#include <QScrollBar>

ProductModule::ProductModule(UserRole role, QWidget *parent) : QWidget(parent), m_role(role) {
    setupUI();
}

void ProductModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(0);

    QTabWidget *tabs = new QTabWidget();
    tabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #ebeef5; top: -1px; background: white; border-radius: 0 0 8px 8px; } "
        "QTabBar::tab { background: #f5f7fa; color: #909399; padding: 12px 25px; border: 1px solid #e4e7ed; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; font-weight: bold; margin-right: 2px; } "
        "QTabBar::tab:selected { background: white; color: #409eff; border-bottom-color: white; } "
        "QTabBar::tab:hover:!selected { background: #fafafa; }"
    );

    // Tab 1: 库存看板
    QWidget *inventoryTab = new QWidget();
    QVBoxLayout *invLayout = new QVBoxLayout(inventoryTab);
    invLayout->setContentsMargins(20, 20, 20, 20);
    invLayout->setSpacing(20);

    // --- 原有的库存看板代码迁移过来 ---
    // 1. 顶部标题与快速搜索
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("商品进销存看板");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #303133;");
    
    QHBoxLayout *filterLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索商品名称、编号、分类...");
    searchEdit->setFixedWidth(260);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    filterLayout->addWidget(searchEdit);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addLayout(filterLayout);

    // 2. 统计概览
    QHBoxLayout *statLayout = new QHBoxLayout();
    auto createStatCard = [&](const QString &icon, const QString &title, QLabel* &valLabel, const QString &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; }");
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 25));
        shadow->setOffset(0, 2);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *cl = new QHBoxLayout(card);
        cl->setContentsMargins(20, 15, 20, 15);
        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(50, 50); // 稍微小一点，适合 90 高度
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("font-size: 24px; color: %1; background: #f5f7fa; border-radius: 10px; border: none;").arg(color));
        
        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(2);
        QLabel *tl = new QLabel(title); tl->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        valLabel = new QLabel("0"); valLabel->setStyleSheet("color: #303133; font-size: 22px; font-weight: bold; border: none; background: transparent;");
        vl->addWidget(tl); vl->addWidget(valLabel);
        vl->addStretch();

        cl->addWidget(iconLabel); cl->addSpacing(15); cl->addLayout(vl); cl->addStretch();
        return card;
    };

    statLayout->addWidget(createStatCard("📦", "商品品种", varietyLabel, "#409eff"));
    statLayout->addWidget(createStatCard("⚠️", "库存预警", lowStockLabel, "#f56c6c"));
    statLayout->addWidget(createStatCard("💰", "总货值估算", totalValueLabel, "#67c23a"));
    mainLayout->addLayout(statLayout);

    // 3. 商品列表
    prodTable = new QTableWidget();
    prodTable->setColumnCount(6);
    prodTable->setHorizontalHeaderLabels({"条形码", "商品名称", "规格单位", "单价/零售", "当前库存", "库存状态"});
    
    prodTable->setShowGrid(false);
    prodTable->setAlternatingRowColors(true);
    prodTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    prodTable->verticalHeader()->setVisible(false);
    prodTable->verticalHeader()->setDefaultSectionSize(55);

    prodTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background: white; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background: #f5f7fa; padding: 10px; border: none; font-weight: bold; } "
    );

    prodTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    prodTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    prodTable->setColumnWidth(0, 120);
    prodTable->setColumnWidth(4, 80);


    // 4. 底部动作
    QPushButton *stockInBtn = new QPushButton("+ 商品入库登记 / 新货上架");
    stockInBtn->setMinimumHeight(45);
    stockInBtn->setCursor(Qt::PointingHandCursor);
    stockInBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 6px; font-weight: bold; font-size: 14px; border: none; } "
        "QPushButton:hover { background: #66b1ff; } "
    );
    invLayout->addLayout(headerLayout);
    invLayout->addLayout(statLayout);
    invLayout->addWidget(prodTable);
    invLayout->addWidget(stockInBtn);

    connect(stockInBtn, &QPushButton::clicked, this, &ProductModule::onStockIn);
    if (m_role == UserRole::STAFF) stockInBtn->setVisible(false);

    tabs->addTab(inventoryTab, "库存看板");

    // Tab 2: 入库记录回溯
    QWidget *recordTab = new QWidget();
    setupRecordTab(recordTab);
    tabs->addTab(recordTab, "入库记录回溯");

    mainLayout->addWidget(tabs);

    // 注入数据
    addProductRow("690123456789", "皇家基础全价猫粮 2kg", "袋", 199.00, 5, 10);
    addProductRow("690987654321", "小鲜肉混合猫砂 6L", "包", 35.00, 42, 20);
    addProductRow("001234567890", "宠物专用免洗手套", "盒", 15.00, 3, 5);
    addProductRow("400112233445", "伊丽莎白圈 M号", "个", 25.00, 12, 5);
    addProductRow("400556677889", "除臭喷剂 300ml", "瓶", 45.00, 18, 10);

    updateStats();
}

void ProductModule::setupRecordTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // 顶部筛选栏 (胶囊风格美化)
    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->setContentsMargins(10, 5, 10, 5);
    filterLayout->setSpacing(12);

    // 1. 搜索框
    recordSearch = new QLineEdit();
    recordSearch->setPlaceholderText("搜索商品/条码/供应商...");
    recordSearch->setFixedWidth(220);
    recordSearch->setFixedHeight(36);
    recordSearch->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 18px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; }"
    );
    connect(recordSearch, &QLineEdit::textChanged, this, &ProductModule::onFilterRecords);

    QLabel *dateLabel = new QLabel("日期：");
    dateLabel->setStyleSheet("color: #606266; font-size: 13px;");

    // 2. 日期选择器 (三元组合 QComboBox)
    auto styleCombo = [](QComboBox* cb, int w) {
        cb->setFixedHeight(36);
        cb->setFixedWidth(w);
        cb->setStyleSheet(
            "QComboBox {"
            "   border: 1px solid #dcdfe6;"
            "   border-radius: 4px;"
            "   padding: 4px 20px 4px 10px;"
            "   font-size: 13px;"
            "   background-color: #f5f7fa;"
            "   color: #606266;"
            "}"
            "QComboBox:focus {"
            "   border: 1px solid #409eff;"
            "   background-color: #ffffff;"
            "}"
            "QComboBox::drop-down {"
            "   subcontrol-origin: padding;"
            "   subcontrol-position: top right;"
            "   width: 24px;"
            "   border: none;"
            "}"
            "QComboBox::down-arrow {"
            "   image: url(:/images/chevron-down.svg);"
            "   width: 12px;"
            "   height: 12px;"
            "   margin-right: 5px;"
            "}"
            "QComboBox QAbstractItemView {"
            "   border: 1px solid #e4e7ed; background-color: #ffffff; border-radius: 4px;"
            "   selection-background-color: #f5f7fa; selection-color: #409eff; outline: none;"
            "}"
            "QComboBox QAbstractItemView::item { height: 35px; padding-left: 10px; color: #606266; }"
            "QComboBox QAbstractItemView::item:selected { background-color: #f5f7fa; color: #409eff; border-left: 3px solid #409eff; }"
        );
        cb->view()->verticalScrollBar()->setStyleSheet(
            "QScrollBar:vertical { width: 0px; background: transparent; margin: 0px; } "
            "QScrollBar::handle:vertical { background: #dcdfe6; border-radius: 4px; min-height: 20px; } "
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        );
    };

    auto initDateGroup = [&](QComboBox*& y, QComboBox*& m, QComboBox*& d, const QDate& initialDate) {
        y = new QComboBox();
        m = new QComboBox();
        d = new QComboBox();
        
        for (int i = 2020; i <= 2030; ++i) y->addItem(QString::number(i) + "年", i);
        for (int i = 1; i <= 12; ++i) m->addItem(QString::number(i) + "月", i);
        
        styleCombo(y, 115);
        styleCombo(m, 100);
        styleCombo(d, 100);
        
        y->setCurrentText(QString::number(initialDate.year()) + "年");
        m->setCurrentIndex(initialDate.month() - 1);
        
        updateDays(y, m, d);
        d->setCurrentIndex(initialDate.day() - 1);
        
        connect(y, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](){ updateDays(y, m, d); onFilterRecords(); });
        connect(m, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](){ updateDays(y, m, d); onFilterRecords(); });
        connect(d, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](){ onFilterRecords(); });
    };

    initDateGroup(sYearCombo, sMonthCombo, sDayCombo, QDate::currentDate().addDays(-30));
    
    QLabel *toLabel = new QLabel("至");
    toLabel->setStyleSheet("color: #606266;");

    initDateGroup(eYearCombo, eMonthCombo, eDayCombo, QDate::currentDate());

    // 3. 操作按钮 (Pill 风格，增加宽度解决遮挡，合并用户优化)
    QPushButton *filterBtn = new QPushButton("统计筛选");
    filterBtn->setFixedSize(120, 36);
    filterBtn->setCursor(Qt::PointingHandCursor);
    filterBtn->setStyleSheet(
        "QPushButton { "
        "background: #409eff; "
        "color: white; "
        "border-radius: 18px; "
        "border: none; "
        "font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif; " /* 明确指定字体 */
        "font-weight: bold; "
        "font-size: 13px; "
        "padding: 0px;"
        "text-align: center;"
        "} "
        "QPushButton:hover { background: #66b1ff; }"
        );
    connect(filterBtn, &QPushButton::clicked, this, &ProductModule::onFilterRecords);

    QPushButton *resetBtn = new QPushButton("重置条件");
    resetBtn->setFixedSize(120, 36);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setStyleSheet(
        "QPushButton { "
        "background: white; "
        "color: #606266; "
        "border-radius: 18px; "
        "border: 1px solid #dcdfe6; "
        "font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif; "
        "font-size: 13px;"
        "padding: 0px;"
        "text-align: center;"
        "} "
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"
    );
    connect(resetBtn, &QPushButton::clicked, this, &ProductModule::onResetRecords);

    filterLayout->addWidget(recordSearch);
    filterLayout->addSpacing(10);
    filterLayout->addWidget(dateLabel);
    filterLayout->addWidget(sYearCombo);
    filterLayout->addWidget(sMonthCombo);
    filterLayout->addWidget(sDayCombo);
    filterLayout->addWidget(toLabel);
    filterLayout->addWidget(eYearCombo);
    filterLayout->addWidget(eMonthCombo);
    filterLayout->addWidget(eDayCombo);
    filterLayout->addSpacing(15);
    filterLayout->addWidget(filterBtn);
    filterLayout->addWidget(resetBtn);
    filterLayout->addStretch();

    layout->addLayout(filterLayout);

    // History record table
    recordTable = new QTableWidget();
    recordTable->setColumnCount(6);
    recordTable->setHorizontalHeaderLabels({"操作时间", "商品名称", "条形码", "入库数量", "供应商", "操作员"});
    recordTable->setShowGrid(false);
    recordTable->setAlternatingRowColors(true);
    recordTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    recordTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recordTable->verticalHeader()->setVisible(false);
    recordTable->verticalHeader()->setDefaultSectionSize(45);
    recordTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    recordTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    
    recordTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background: white; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background: #f5f7fa; padding: 10px; border: none; font-weight: bold; color: #606266; } "
    );
    
    layout->addWidget(recordTable);

    // Inject sample records
    StockInRecord r1 = {QDateTime::currentDateTime().addDays(-2).toString("yyyy-MM-dd HH:mm:ss"), "皇家基础全价猫粮 2kg", "690123456789", 10, "皇家宠物食品有限公司", "店长admin"};
    StockInRecord r2 = {QDateTime::currentDateTime().addDays(-5).toString("yyyy-MM-dd HH:mm:ss"), "小鲜肉混合猫砂 6L", "690987654321", 50, "中宠贸易实业", "营业员staff"};
    addRecordRow(r1);
    addRecordRow(r2);
    m_records << r1 << r2;
}

void ProductModule::addRecordRow(const StockInRecord &record) {
    int row = recordTable->rowCount();
    recordTable->insertRow(row);

    auto setItem = [&](int col, const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        recordTable->setItem(row, col, item);
    };

    setItem(0, record.dateTime);
    setItem(1, record.productName);
    setItem(2, record.barcode);
    setItem(3, QString::number(record.quantity));
    setItem(4, record.supplier);
    setItem(5, record.operatorName);
}

void ProductModule::updateDays(QComboBox *y, QComboBox *m, QComboBox *d) {
    int year = y->currentData().toInt();
    int month = m->currentData().toInt();
    int oldDay = d->currentData().toInt();
    if (oldDay <= 0) oldDay = d->currentText().remove("日").toInt();
    
    QDate date(year, month, 1);
    int daysInMonth = date.daysInMonth();
    
    d->clear();
    for (int i = 1; i <= daysInMonth; ++i) {
        d->addItem(QString("%1日").arg(i), i);
    }
    
    int index = d->findData(oldDay);
    if (index != -1) d->setCurrentIndex(index);
    else d->setCurrentIndex(d->count() - 1);
    d->blockSignals(false);
}

void ProductModule::addProductRow(const QString &barcode, const QString &name, const QString &spec, 
                                 double price, int stock, int minStock) {
    int row = prodTable->rowCount();
    prodTable->insertRow(row);

    auto setItem = [&](int col, const QString &text, bool isBold = false) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        if (isBold) item->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        prodTable->setItem(row, col, item);
    };

    setItem(0, barcode);
    setItem(1, name, true);
    setItem(2, spec);
    setItem(3, QString("￥%1").arg(price, 0, 'f', 2));
    
    // 库存数值项
    QTableWidgetItem *stockItem = new QTableWidgetItem(QString::number(stock));
    stockItem->setTextAlignment(Qt::AlignCenter);
    if (stock <= minStock) stockItem->setForeground(QColor("#f56c6c"));
    prodTable->setItem(row, 4, stockItem);

    // 状态标签
    QWidget *tagContainer = new QWidget();
    QHBoxLayout *tagLayout = new QHBoxLayout(tagContainer);
    tagLayout->setContentsMargins(0, 0, 0, 0);
    tagLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *tag = new QLabel();
    QString baseStyle = "padding: 2px 10px; border-radius: 10px; font-size: 11px; font-weight: bold; ";
    if (stock <= 0) {
        tag->setText("缺货");
        tag->setStyleSheet(baseStyle + "background: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4;");
    } else if (stock <= minStock) {
        tag->setText("库存告急");
        tag->setStyleSheet(baseStyle + "background: #fff7e6; color: #fa8c16; border: 1px solid #ffd591;");
    } else {
        tag->setText("充足");
        tag->setStyleSheet(baseStyle + "background: #f0f9eb; color: #67c23a; border: 1px solid #e1f3d8;");
    }
    tagLayout->addWidget(tag);
    prodTable->setCellWidget(row, 5, tagContainer);
}

void ProductModule::onStockIn() {
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("商品入库登记");
    dialog->setMinimumWidth(400);
    dialog->setStyleSheet("QDialog { background: white; border-radius: 8px; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 表单区域
    QVBoxLayout *formLayout = new QVBoxLayout();
    
    auto createFormRow = [&](const QString &labelText, QWidget *widget) {
        QHBoxLayout *rowLayout = new QHBoxLayout();
        QLabel *label = new QLabel(labelText);
        label->setFixedWidth(80);
        label->setStyleSheet("color: #303133; font-weight: bold;");
        rowLayout->addWidget(label);
        rowLayout->addWidget(widget);
        formLayout->addLayout(rowLayout);
    };

    // 条码输入
    QLineEdit *barcodeEdit = new QLineEdit();
    barcodeEdit->setPlaceholderText("请输入商品条形码");
    barcodeEdit->setMinimumHeight(35);
    barcodeEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; } "
        "QLineEdit:focus { border-color: #409eff; }"
    );
    createFormRow("条形码", barcodeEdit);

    // 商品名输入
    QLineEdit *nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("请输入商品名称");
    nameEdit->setMinimumHeight(35);
    nameEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; } "
        "QLineEdit:focus { border-color: #409eff; }"
    );
    createFormRow("商品名称", nameEdit);

    // 数量输入
    QSpinBox *quantitySpin = new QSpinBox();
    quantitySpin->setMinimum(1);
    quantitySpin->setMaximum(9999);
    quantitySpin->setButtonSymbols(QAbstractSpinBox::NoButtons); // 隐藏调节按钮
    quantitySpin->setMinimumHeight(35);
    quantitySpin->setStyleSheet(
        "QSpinBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; } "
        "QSpinBox:focus { border-color: #409eff; }"
    );
    createFormRow("入库数量", quantitySpin);

    // 供应商输入
    QLineEdit *supplierEdit = new QLineEdit();
    supplierEdit->setPlaceholderText("请输入供应商");
    supplierEdit->setMinimumHeight(35);
    supplierEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; } "
        "QLineEdit:focus { border-color: #409eff; }"
    );
    createFormRow("供应商", supplierEdit);

    // 操作员选择 (改为 QComboBox)
    QComboBox *operatorCombo = new QComboBox();
    operatorCombo->setMinimumHeight(35);
    // 美化下拉列表样式 (QAbstractItemView)
    operatorCombo->setStyleSheet(
        "QComboBox {"
        "   border: 1px solid #dcdfe6;"
        "   border-radius: 6px;"
        "   padding: 0 12px;"
        "   padding-right: 30px;"
        "   background: #ffffff;"
        "   color: #606266;"
        "   font-size: 13px;"
        "}"
        "QComboBox:hover { border-color: #c0c4cc; }"
        "QComboBox:focus { border-color: #409eff; }"
        "QComboBox::drop-down {"
        "   subcontrol-origin: padding;"
        "   subcontrol-position: top right;"
        "   width: 30px;"
        "   border: none;"
        "}"
        "QComboBox::down-arrow {"
        "   image: url(:/images/chevron-down.svg);"
        "   width: 14px;"
        "   height: 14px;"
        "}"
        /* 重点：美化下拉列表视图 */
        "QComboBox QAbstractItemView {"
        "   border: 1px solid #e4e7ed;"
        "   background-color: #ffffff;"
        "   border-radius: 4px;"
        "   selection-background-color: #f5f7fa;"
        "   selection-color: #409eff;"
        "   outline: none;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "   height: 35px;"
        "   padding-left: 10px;"
        "   color: #606266;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "   background-color: #f5f7fa;"
        "   color: #409eff;"
        "}"
    );
    
    // 强制下拉列表遵循 QSS (某些系统下需要)
    operatorCombo->view()->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical { width: 8px; background: transparent; margin: 0px; } "
        "QScrollBar::handle:vertical { background: #dcdfe6; border-radius: 4px; min-height: 20px; } "
        "QScrollBar::handle:vertical:hover { background: #c0c4cc; } "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );

    // 填充真实员工数据
    operatorCombo->addItem("请选择操作员...", "");
    operatorCombo->addItem("王经理 (1001)");
    operatorCombo->addItem("张晓明 (1002)");
    operatorCombo->addItem("李晓华 (1003)");
    operatorCombo->addItem("赵强 (1004)");
    
    createFormRow("操作员", operatorCombo);

    mainLayout->addLayout(formLayout);

    // 按钮区域
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setMinimumHeight(35);
    cancelBtn->setStyleSheet(
        "QPushButton { background: #f5f7fa; color: #606266; border-radius: 6px; border: 1px solid #dcdfe6; } "
        "QPushButton:hover { background: #e4e7ed; }"
    );

    QPushButton *confirmBtn = new QPushButton("确认入库");
    confirmBtn->setMinimumHeight(35);
    confirmBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 6px; border: none; font-weight: bold; } "
        "QPushButton:hover { background: #66b1ff; }"
    );

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addSpacing(10);
    btnLayout->addWidget(confirmBtn);
    mainLayout->addLayout(btnLayout);

    // 连接信号
    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
    connect(confirmBtn, &QPushButton::clicked, [=]() {
        QString barcode = barcodeEdit->text().trimmed();
        QString name = nameEdit->text().trimmed();
        int quantity = quantitySpin->value();
        QString supplier = supplierEdit->text().trimmed();
        QString operatorName = operatorCombo->currentIndex() <= 0 ? "" : operatorCombo->currentText();

        if (barcode.isEmpty() || name.isEmpty() || supplier.isEmpty() || operatorName.isEmpty()) {
            QMessageBox::warning(dialog, "提示", "请填写完整的入库信息并选择操作员");
            return;
        }

        // 查找是否存在该商品
        bool found = false;
        int targetRow = -1;
        for (int i = 0; i < prodTable->rowCount(); ++i) {
            if (prodTable->item(i, 0)->text() == barcode) {
                found = true;
                targetRow = i;
                break;
            }
        }

        if (found && targetRow >= 0) {
            // 更新现有商品库存
            int currentStock = prodTable->item(targetRow, 4)->text().toInt();
            int newStock = currentStock + quantity;
            prodTable->item(targetRow, 4)->setText(QString::number(newStock));

            // Update stock status
            QWidget *tagContainer = prodTable->cellWidget(targetRow, 5);
            if (tagContainer) {
                QLabel *tag = tagContainer->findChild<QLabel*>();
                if (tag) {
                    QString baseStyle = "padding: 2px 10px; border-radius: 10px; font-size: 11px; font-weight: bold; ";
                    // Assuming minStock is 10 for existing items for simplicity
                    if (newStock <= 0) {
                        tag->setText("缺货");
                        tag->setStyleSheet(baseStyle + "background: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4;");
                    } else if (newStock <= 10) { 
                        tag->setText("库存告急");
                        tag->setStyleSheet(baseStyle + "background: #fff7e6; color: #fa8c16; border: 1px solid #ffd5f1;");
                    } else {
                        tag->setText("充足");
                        tag->setStyleSheet(baseStyle + "background: #f0f9eb; color: #67c23a; border: 1px solid #e1f3d8;");
                    }
                }
            }
        } else {
            // Add new product
            addProductRow(barcode, name, "个", 0.00, quantity, 10);
        }

        // Record stock-in history
        StockInRecord record;
        record.dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        record.productName = name;
        record.barcode = barcode;
        record.quantity = quantity;
        record.supplier = supplier;
        record.operatorName = operatorName;
        
        m_records.append(record);
        addRecordRow(record);

        // Update statistics
        updateStats();

        QMessageBox::information(dialog, "成功", "商品入库成功并已记录历史");
        dialog->accept();
    });

    dialog->exec();
    delete dialog;
}


void ProductModule::updateStats() {
    int varieties = prodTable->rowCount();
    int lowStock = 0;
    double totalValue = 0;

    for (int i = 0; i < varieties; ++i) {
        int stock = prodTable->item(i, 4)->text().toInt();
        QString priceStr = prodTable->item(i, 3)->text().remove("￥");
        double price = priceStr.toDouble();
        
        totalValue += (price * stock);

        QWidget *w = prodTable->cellWidget(i, 5);
        if (w) {
            QLabel *lbl = w->findChild<QLabel*>();
            if (lbl && (lbl->text() == "缺货" || lbl->text() == "库存告急")) {
                lowStock++;
            }
        }
    }

    varietyLabel->setText(QString("%1种").arg(varieties));
    lowStockLabel->setText(QString("%1项").arg(lowStock));
    totalValueLabel->setText(QString("￥%1").arg(totalValue, 0, 'f', 2));
}

void ProductModule::onFilterRecords() {
    QDate start(sYearCombo->currentData().toInt(), sMonthCombo->currentData().toInt(), sDayCombo->currentData().toInt());
    QDate end(eYearCombo->currentData().toInt(), eMonthCombo->currentData().toInt(), eDayCombo->currentData().toInt());
    QString kw = recordSearch->text().trimmed().toLower();

    recordTable->setRowCount(0);
    for (const auto &record : m_records) {
        QDate recordDate = QDateTime::fromString(record.dateTime, "yyyy-MM-dd HH:mm:ss").date();
        
        bool dateMatch = (recordDate >= start && recordDate <= end);
        bool kwMatch = kw.isEmpty() || 
                       record.productName.toLower().contains(kw) || 
                       record.barcode.toLower().contains(kw) || 
                       record.supplier.toLower().contains(kw);

        if (dateMatch && kwMatch) {
            addRecordRow(record);
        }
    }
}

void ProductModule::onResetRecords() {
    // 禁用信号以避免多次触发过滤
    recordSearch->blockSignals(true);
    sYearCombo->blockSignals(true);
    sMonthCombo->blockSignals(true);
    eYearCombo->blockSignals(true);
    eMonthCombo->blockSignals(true);

    recordSearch->clear();
    QDate startInitial = QDate::currentDate().addDays(-30);
    sYearCombo->setCurrentText(QString::number(startInitial.year()) + "年");
    sMonthCombo->setCurrentIndex(startInitial.month() - 1);
    updateDays(sYearCombo, sMonthCombo, sDayCombo);
    sDayCombo->setCurrentIndex(startInitial.day() - 1);

    QDate endInitial = QDate::currentDate();
    eYearCombo->setCurrentText(QString::number(endInitial.year()) + "年");
    eMonthCombo->setCurrentIndex(endInitial.month() - 1);
    updateDays(eYearCombo, eMonthCombo, eDayCombo);
    eDayCombo->setCurrentIndex(endInitial.day() - 1);
    
    recordSearch->blockSignals(false);
    sYearCombo->blockSignals(false);
    sMonthCombo->blockSignals(false);
    eYearCombo->blockSignals(false);
    eMonthCombo->blockSignals(false);

    // 重新统一过滤
    onFilterRecords();
}
