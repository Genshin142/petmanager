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
#include <QIntValidator>
#include <QTimer>
#include <QFileDialog>
#include <QEvent>
#include <QButtonGroup>
#include <QProgressBar>
#include <QDateTime>
#include <QPropertyAnimation>
#include <QGraphicsBlurEffect>
#include <QTextEdit>
#include <QWheelEvent>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include "custom_calendar_edit.h"
#include "productdatamanager.h"

ProductModule::ProductModule(UserRole role, QWidget *parent) : QWidget(parent), m_role(role) {
    m_currentPage = 1;
    m_pageSize = 10;
    m_recCurrentPage = 1;
    m_recPageSize = 10;
    m_detailDrawer = nullptr;
    m_backdrop = nullptr;
    m_drawerAnim = nullptr;
    setupUI();
}

bool ProductModule::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        if (watched == m_mainPreview) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            QStringList imgList = m_mainPreview->property("imgList").toStringList();
            if (imgList.size() > 1) {
                int currentIndex = m_mainPreview->property("imgIndex").toInt();
                if (wheelEvent->angleDelta().y() > 0) {
                    // 向上滚，上一张
                    currentIndex = (currentIndex - 1 + imgList.size()) % imgList.size();
                } else {
                    // 向下滚，下一张
                    currentIndex = (currentIndex + 1) % imgList.size();
                }
                QString path = imgList[currentIndex];
                m_mainPreview->setPixmap(QPixmap(path).scaled(290, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                m_mainPreview->setProperty("imgPath", path);
                m_mainPreview->setProperty("imgIndex", currentIndex);
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        QLabel *label = qobject_cast<QLabel*>(watched);
        if (label && label->property("imgPath").isValid()) {
            QString path = label->property("imgPath").toString();
            
            // 模拟宠物模块：全屏半透明预览
            // 关键修复：即便在对话框内触发，预览也应覆盖整个主窗口，但父节点绑定在 label->window() 确保层级在上
            QWidget *mainWin = this->window();
            QWidget *currentWin = label->window();
            QDialog *preview = new QDialog(currentWin, Qt::FramelessWindowHint);
            
            // 铺满主窗口
            preview->setGeometry(mainWin->geometry());
            preview->setAttribute(Qt::WA_TranslucentBackground);
            
            QVBoxLayout *layout = new QVBoxLayout(preview);
            layout->setContentsMargins(0, 0, 0, 0);
            
            // 全屏背景遮罩 - 调深阴影
            QFrame *bg = new QFrame();
            bg->setStyleSheet("background-color: rgba(0, 0, 0, 235);");
            layout->addWidget(bg);
            
            QVBoxLayout *bgLayout = new QVBoxLayout(bg);
            bgLayout->setContentsMargins(0, 0, 0, 0);
            bgLayout->setAlignment(Qt::AlignCenter);
            
            QLabel *imgLabel = new QLabel();
            QPixmap pix(path);
            if (!pix.isNull()) {
                // 调整显示尺寸，避免过大显得突兀
                imgLabel->setPixmap(pix.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            imgLabel->setStyleSheet("border: none; background: white; border-radius: 10px; padding: 20px;");
            imgLabel->setAlignment(Qt::AlignCenter);
            bgLayout->addWidget(imgLabel);
            
            // 点击背景任意位置关闭
            bg->installEventFilter(this);
            bg->setProperty("isPreviewBg", true);
            bg->setProperty("previewDlg", QVariant::fromValue((void*)preview));
            bg->setCursor(Qt::PointingHandCursor);
            
            // 全局坐标同步
            preview->raise();
            
            connect(preview, &QDialog::finished, preview, &QDialog::deleteLater);
            preview->show();
            return true;
        }
        
        // 点击预览背景关闭
        if (watched->property("isPreviewBg").toBool()) {
            void* ptr = watched->property("previewDlg").value<void*>();
            if (ptr) {
                QDialog *dlg = static_cast<QDialog*>(ptr);
                dlg->close();
                return true;
            }
        }
        
        // 点击遮罩层关闭抽屉
        if (watched == m_backdrop) {
            closeDrawer();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ProductModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(0);

    // 全局左右 Master-Detail 布局
    QHBoxLayout *globalMasterDetail = new QHBoxLayout();
    globalMasterDetail->setContentsMargins(0, 0, 0, 0);
    globalMasterDetail->setSpacing(0);

    m_mainTabs = new QTabWidget();
    m_mainTabs->setStyleSheet(
        "QTabWidget::pane { border: none; background: white; } " // 移除边框，让整体感更强
        "QTabBar::tab { background: #f5f7fa; color: #909399; padding: 12px 25px; border: 1px solid #e4e7ed; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; } "
        "QTabBar::tab:selected { background: white; color: #409eff; border-bottom-color: white; } "
        "QTabBar::tab:hover:!selected { background: #fafafa; }"
    );

    // Tab 1: 库存看板
    QWidget *inventoryTab = new QWidget();
    QVBoxLayout *invLayout = new QVBoxLayout(inventoryTab);
    invLayout->setContentsMargins(15, 20, 15, 20);
    invLayout->setSpacing(20);

    // --- 原有的库存看板代码迁移过来 ---
    // 1. 顶部标题与快速搜索
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("商品进销存看板");
    titleLabel->setStyleSheet("font-size: 20px; color: #303133;");
    
    QHBoxLayout *filterLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索商品名称、编号、分类...");
    searchEdit->setFixedWidth(260);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    // -- 左侧：批量删除 --
    QPushButton *batchDeleteBtn = new QPushButton("批量删除");
    batchDeleteBtn->setCursor(Qt::PointingHandCursor);
    batchDeleteBtn->setFixedHeight(32);
    batchDeleteBtn->setStyleSheet(
        "QPushButton { background-color: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 6px; font-size: 12px; padding: 0 15px; }"
        "QPushButton:hover { background-color: #f56c6c; color: white; }"
    );
    connect(batchDeleteBtn, &QPushButton::clicked, this, &ProductModule::onBatchDelete);
    filterLayout->addWidget(batchDeleteBtn);

    filterLayout->addStretch();

    // 新增：分类胶囊按钮组
    QHBoxLayout *capsuleLayout = new QHBoxLayout();
    capsuleLayout->setSpacing(8);
    QStringList categories = {"全部", "主粮", "零食", "洗护", "玩具"};
    for (const QString &cat : categories) {
        QPushButton *btn = new QPushButton(cat);
        btn->setCheckable(true);
        btn->setFixedSize(60, 28);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { padding: 2px 10px; background: #f5f7fa; color: #909399; border-radius: 14px; border: 1px solid #e4e7ed; font-size: 12px; font-weight: bold; } "
            "QPushButton:hover { background: #f0f2f5; } "
            "QPushButton:checked { padding: 2px 10px; background: #409eff; color: white; border: none; }"
        );
        if (cat == "全部") btn->setChecked(true);
        
        // 互斥逻辑
        static QButtonGroup *group = new QButtonGroup(this);
        group->addButton(btn);
        
        connect(btn, &QPushButton::clicked, this, [=](){
            searchEdit->setText(cat == "全部" ? "" : cat);
        });
        capsuleLayout->addWidget(btn);
    }
    filterLayout->addLayout(capsuleLayout);
    filterLayout->addSpacing(20);

    filterLayout->addWidget(searchEdit);
    filterLayout->addSpacing(10);

    QPushButton *stockInBtn = new QPushButton("+ 商品入库登记");
    stockInBtn->setFixedHeight(32);
    stockInBtn->setCursor(Qt::PointingHandCursor);
    stockInBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 16px; font-size: 13px; border: none; padding: 0 10px; } "
        "QPushButton:hover { background: #66b1ff; } "
    );
    connect(stockInBtn, &QPushButton::clicked, this, &ProductModule::onStockIn);
    if (m_role == UserRole::STAFF) stockInBtn->setVisible(false);
    stockInBtn->setFixedWidth(130);
    filterLayout->addWidget(stockInBtn);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    // 移除了此处对 filterLayout 的添加，稍后将其移至卡片下方

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
        valLabel = new QLabel("0"); valLabel->setStyleSheet("color: #303133; font-size: 22px; border: none; background: transparent;");
        vl->addWidget(tl); vl->addWidget(valLabel);
        vl->addStretch();

        cl->addWidget(iconLabel); cl->addSpacing(15); cl->addLayout(vl); cl->addStretch();
        return card;
    };

    statLayout->addWidget(createStatCard("📦", "商品品种", varietyLabel, "#409eff"));
    statLayout->addWidget(createStatCard("⚠️", "库存预警", lowStockLabel, "#f56c6c"));
    statLayout->addWidget(createStatCard("💰", "总货值估算", totalValueLabel, "#67c23a"));
    
    // 3. 商品列表
    prodTable = new QTableWidget();
    prodTable->setColumnCount(10);
    prodTable->setHorizontalHeaderLabels({"选择", "图片", "条形码", "商品名称", "规格单位", "成本价", "销售价", "当前库存", "库存状态", "操作"});
    
    prodTable->setShowGrid(false);
    prodTable->setAlternatingRowColors(false);
    prodTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    prodTable->setSelectionMode(QAbstractItemView::SingleSelection);
    prodTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    prodTable->setFocusPolicy(Qt::NoFocus);
    prodTable->verticalHeader()->setVisible(false);
    prodTable->verticalHeader()->setDefaultSectionSize(55);

    prodTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background: white; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QTableWidget::item:selected { background-color: #b3d8ff; } " 
        "QHeaderView::section { background: #f5f7fa; padding: 10px; border: none;  font-weight: bold; } "
    );

    prodTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    prodTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    prodTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed); // 选择
    prodTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed); // 图片
    prodTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Fixed); // 操作
    prodTable->setColumnWidth(0, 45);  // 选择
    prodTable->setColumnWidth(1, 46);  // 图片
    prodTable->setColumnWidth(9, 80);  // 操作按钮
    
    // 隐藏/显示成本列
    if (m_role != UserRole::ADMIN) {
        prodTable->hideColumn(5);
    }

    // 4. 底部统计与分页
    QFrame *statFrame = new QFrame();
    statFrame->setFixedHeight(45);
    statFrame->setStyleSheet("QFrame { background: #f8f9fb; border-top: 1px solid #ebeef5; padding: 0 12px; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(statFrame);
    footerLayout->addStretch();

    // 1. 跳转组
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

    // 2. 翻页组
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

    // 绑定分页逻辑
    connect(searchEdit, &QLineEdit::textChanged, this, &ProductModule::onSearchChanged);
    connect(prevBtn, &QPushButton::clicked, this, &ProductModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &ProductModule::onNextPage);
    connect(jumpBtn, &QPushButton::clicked, this, &ProductModule::onJumpPage);
    connect(jumpEdit, &QLineEdit::returnPressed, this, &ProductModule::onJumpPage);

    // 左侧：组装表格与分页
    QWidget *tableContainer = new QWidget();
    QVBoxLayout *tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(10);
    tableLayout->addWidget(prodTable);
    tableLayout->addWidget(statFrame);

    invLayout->addLayout(headerLayout);
    invLayout->addLayout(statLayout);
    invLayout->addLayout(filterLayout);
    invLayout->addWidget(tableContainer);

    m_mainTabs->addTab(inventoryTab, "库存看板");

    // Tab 2: 入库记录回溯
    QWidget *recordTab = new QWidget();
    setupRecordTab(recordTab);
    m_mainTabs->addTab(recordTab, "入库记录回溯");

    // 设置全局详情抽屉
    setupDetailDrawer();
    m_detailDrawer->show();
    m_detailDrawer->setFixedWidth(600);

    globalMasterDetail->addWidget(m_mainTabs, 1);
    globalMasterDetail->addWidget(m_detailDrawer, 0);

    mainLayout->addLayout(globalMasterDetail);

    // 切换标签页时控制详情面板
    connect(m_mainTabs, &QTabWidget::currentChanged, this, [=](int index){
        if (index == 0) {
            m_btnModifyInfo->setVisible(true);
            m_lblDrawerHeaderTitle->setText("📦 商品详情");
            // 联动左侧列表选中
            if (prodTable->currentRow() >= 0) {
                QString barcode = prodTable->item(prodTable->currentRow(), 2)->text();
                updateDetailDrawer(ProductDataManager::instance()->getProduct(barcode));
            }
        } else {
            m_btnModifyInfo->setVisible(false);
            m_lblDrawerHeaderTitle->setText("📜 记录详情 (只读)");
            // 联动右侧记录选中
            if (recordTable->currentRow() >= 0) {
                QString barcode = recordTable->item(recordTable->currentRow(), 3)->text();
                updateDetailDrawer(ProductDataManager::instance()->getProduct(barcode));
            }
        }
    });

    // 绑定信号：行选中时刷新并滑出抽屉
    connect(prodTable, &QTableWidget::itemSelectionChanged, this, [=](){
        int row = prodTable->currentRow();
        if (row >= 0) {
            QString barcode = prodTable->item(row, 2)->text();
            ProductInfo info = ProductDataManager::instance()->getProduct(barcode);
            updateDetailDrawer(info);
            openDrawer();
        }
    });



    // 注入数据 (从单例获取)
    auto all = ProductDataManager::instance()->allProducts();
    for (const auto &p : all) {
        addProductRow(p);
    }

    updateStats();
    updatePagination();
    
    // 默认选中第一行，展示详情
    if (prodTable->rowCount() > 0) {
        prodTable->selectRow(0);
    }
}

void ProductModule::setupRecordTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(15, 20, 15, 20); // 严格对齐看板页布局
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

    initDateGroup(sYearCombo, sMonthCombo, sDayCombo, QDate(2026, 1, 1));
    
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
        "font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif; " /* 明确指定字体 */
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
    filterLayout->addStretch();

    layout->addLayout(filterLayout);

    // History record table
    recordTable = new QTableWidget();
    recordTable->setColumnCount(7); // 增加一列：图片
    recordTable->setHorizontalHeaderLabels({QStringLiteral("图片"), "操作时间", "商品名称", "条形码", "入库数量", "供应商", "操作员"});
    recordTable->setShowGrid(false);
    recordTable->setAlternatingRowColors(false);
    recordTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    recordTable->setSelectionMode(QAbstractItemView::SingleSelection);
    recordTable->setFocusPolicy(Qt::NoFocus);
    recordTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recordTable->verticalHeader()->setVisible(false);
    recordTable->verticalHeader()->setDefaultSectionSize(45);
    recordTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    recordTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    
    recordTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background: white; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QTableWidget::item:selected { background-color: #b3d8ff; } " 
        "QHeaderView::section { background: #f5f7fa; padding: 10px; border: none; color: #606266;  font-weight: bold; } "
    );
    
    layout->addWidget(recordTable); // 核心修复：重新添加表格到布局

    // 分页页脚 (严格对齐看板页布局：跳转组在前，翻页组在后)
    QHBoxLayout *pageLayout = new QHBoxLayout();
    pageLayout->setContentsMargins(10, 5, 10, 5);
    pageLayout->setSpacing(8);

    // 1. 跳转组
    recJumpEdit = new QLineEdit();
    recJumpEdit->setFixedWidth(36);
    recJumpEdit->setMaxLength(3);
    recJumpEdit->setFixedHeight(24);
    recJumpEdit->setAlignment(Qt::AlignCenter);
    recJumpEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 0; font-size: 13px; background: white; margin: 0; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    recJumpValidator = new QIntValidator(1, 1, this);
    recJumpEdit->setValidator(recJumpValidator);

    QLabel *recJumpPrefix = new QLabel("跳转到第");
    recJumpPrefix->setStyleSheet("color: #606266; font-size: 13px; margin: 0; padding: 0;");
    QLabel *recJumpSuffix = new QLabel("页");
    recJumpSuffix->setStyleSheet("color: #606266; font-size: 13px; margin: 0; padding: 0;");

    recJumpBtn = new QPushButton("确认");
    recJumpBtn->setCursor(Qt::PointingHandCursor);
    recJumpBtn->setFixedSize(44, 24);
    recJumpBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; font-size: 12px; padding: 2px 0; text-align: center; margin: 0; }"
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"
    );

    QWidget *jumpGroup = new QWidget();
    jumpGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *jumpLayout = new QHBoxLayout(jumpGroup);
    jumpLayout->setContentsMargins(0, 0, 0, 0);
    jumpLayout->setSpacing(2);
    jumpLayout->addWidget(recJumpPrefix);
    jumpLayout->addWidget(recJumpEdit);
    jumpLayout->addWidget(recJumpSuffix);
    jumpLayout->addWidget(recJumpBtn);

    // 2. 翻页组
    recPrevBtn = new QPushButton("上一页");
    recNextBtn = new QPushButton("下一页");
    recPageLabel = new QLabel("第 1 页 / 共 1 页");

    QString recPageStyle = "QPushButton { height: 24px; border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; font-size: 12px; padding: 0 8px; text-align: center; margin: 0; } "
                           "QPushButton:hover { border-color: #409eff; color: #409eff; } "
                           "QPushButton:disabled { background: #f5f7fa; color: #c0c4cc; border-color: #e4e7ed; }";
    recPrevBtn->setStyleSheet(recPageStyle);
    recNextBtn->setStyleSheet(recPageStyle);
    recPrevBtn->setCursor(Qt::PointingHandCursor);
    recNextBtn->setCursor(Qt::PointingHandCursor);
    recPageLabel->setStyleSheet("color: #909399; font-size: 13px; margin: 0; padding: 0 4px;");

    QWidget *pageGroup = new QWidget();
    pageGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *pageLayoutWrapper = new QHBoxLayout(pageGroup);
    pageLayoutWrapper->setContentsMargins(0, 0, 0, 0);
    pageLayoutWrapper->setSpacing(2);
    pageLayoutWrapper->addWidget(recPrevBtn);
    pageLayoutWrapper->addWidget(recPageLabel);
    pageLayoutWrapper->addWidget(recNextBtn);

    pageLayout->addStretch();
    pageLayout->addWidget(jumpGroup);
    pageLayout->addSpacing(8);
    pageLayout->addWidget(pageGroup);
    layout->addLayout(pageLayout);

    connect(recPrevBtn, &QPushButton::clicked, this, &ProductModule::onRecPrevPage);
    connect(recNextBtn, &QPushButton::clicked, this, &ProductModule::onRecNextPage);
    connect(recJumpBtn, &QPushButton::clicked, this, &ProductModule::onRecJumpPage);
    connect(recJumpEdit, &QLineEdit::returnPressed, this, &ProductModule::onRecJumpPage);

    // 联动详情面板：选中记录时显示商品详情
    connect(recordTable, &QTableWidget::itemSelectionChanged, this, [=](){
        int row = recordTable->currentRow();
        if (row >= 0 && m_mainTabs->currentIndex() == 1) {
            QString barcode = recordTable->item(row, 3)->text(); // 条形码在第3列
            ProductInfo info = ProductDataManager::instance()->getProduct(barcode);
            updateDetailDrawer(info);
            openDrawer();
        }
    });

    // Inject sample records
    StockInRecord r1 = {QDateTime::currentDateTime().addDays(-2).toString("yyyy-MM-dd HH:mm:ss"), "皇家基础全价猫粮 2kg", "690123456789", 10, "皇家宠物食品有限公司", "店长admin", "E:/QT/work/PetManager/images/stores/default.png"};
    StockInRecord r2 = {QDateTime::currentDateTime().addDays(-5).toString("yyyy-MM-dd HH:mm:ss"), "小鲜肉混合猫砂 6L", "690987654321", 50, "中宠贸易实业", "营业员staff", "E:/QT/work/PetManager/images/stores/default.png"};
    m_records << r1 << r2;
    onFilterRecords();
}

void ProductModule::addRecordRow(const StockInRecord &record) {
    int row = recordTable->rowCount();
    recordTable->insertRow(row);

    // 0. 图片列
    QWidget *imgContainer = new QWidget();
    QHBoxLayout *imgLayout = new QHBoxLayout(imgContainer);
    imgLayout->setContentsMargins(5, 5, 5, 5);
    imgLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *imgLabel = new QLabel();
    imgLabel->setFixedSize(40, 40);
    imgLabel->setCursor(Qt::PointingHandCursor);
    imgLabel->setProperty("imgPath", record.imgPath);
    
    QPixmap pix(record.imgPath);
    if (!pix.isNull()) {
        imgLabel->setPixmap(pix.scaled(40, 40, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        imgLabel->setStyleSheet("border-radius: 4px; border: none; background: transparent;");
    } else {
        imgLabel->setText(QStringLiteral("无"));
        imgLabel->setStyleSheet("font-size: 10px; color: #909399; background: #f5f7fa; border-radius: 4px;");
    }
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLayout->addWidget(imgLabel);
    recordTable->setCellWidget(row, 0, imgContainer);
    imgLabel->installEventFilter(this);

    auto setItem = [&](int col, const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        recordTable->setItem(row, col, item);
    };

    setItem(1, record.dateTime);
    setItem(2, record.productName);
    setItem(3, record.barcode);
    setItem(4, QString::number(record.quantity));
    setItem(5, record.supplier);
    setItem(6, record.operatorName);
}

void ProductModule::updateDays(QComboBox *y, QComboBox *m, QComboBox *d) {
    if (!y || !m || !d) return;
    d->blockSignals(true);
    
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
    else d->setCurrentIndex(0); // 默认指向 1 日而非最后一天
    
    d->blockSignals(false);
}

void ProductModule::addProductRow(const ProductInfo &info) {
    int row = prodTable->rowCount();
    prodTable->insertRow(row);
    
    QString imgPath = info.images.isEmpty() ? "E:/QT/work/PetManager/images/stores/default.png" : info.images.first();

    // 0. 选择勾选框
    QWidget *chkWidget = new QWidget();
    QHBoxLayout *chkLayout = new QHBoxLayout(chkWidget);
    chkLayout->setContentsMargins(0, 0, 0, 0);
    QCheckBox *chkBox = new QCheckBox();
    chkLayout->addWidget(chkBox, 0, Qt::AlignCenter);
    prodTable->setCellWidget(row, 0, chkWidget);

    // 1. 图片列
    QWidget *imgContainer = new QWidget();
    QHBoxLayout *imgLayout = new QHBoxLayout(imgContainer);
    imgLayout->setContentsMargins(5, 5, 5, 5);
    imgLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *imgLabel = new QLabel();
    imgLabel->setFixedSize(40, 40);
    imgLabel->setCursor(Qt::PointingHandCursor);
    imgLabel->setProperty("imgPath", imgPath);
    
    QPixmap pix(imgPath);
    if (!pix.isNull()) {
        imgLabel->setPixmap(pix.scaled(40, 40, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        imgLabel->setStyleSheet("border-radius: 4px; border: none; background: transparent;");
    } else {
        imgLabel->setText("无图片");
        imgLabel->setStyleSheet("font-size: 10px; color: #909399; background: #f5f7fa; border-radius: 4px;");
    }
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLayout->addWidget(imgLabel);
    prodTable->setCellWidget(row, 1, imgContainer);

    // 安装事件过滤器以便点击图片预览
    imgLabel->installEventFilter(this);

    auto setItem = [&](int col, const QString &text, bool isBold = false) {
        Q_UNUSED(isBold);
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        QFont font("Microsoft YaHei", 9);
        item->setFont(font);
        prodTable->setItem(row, col, item);
    };

    setItem(2, info.barcode);
    setItem(3, info.name, true);
    setItem(4, info.spec);
    setItem(5, QString("￥%1").arg(info.costPrice, 0, 'f', 2));
    setItem(6, QString("￥%1").arg(info.price, 0, 'f', 2));
    
    // 库存数值项 (第 7 列)
    QTableWidgetItem *stockItem = new QTableWidgetItem(QString::number(info.stock));
    stockItem->setTextAlignment(Qt::AlignCenter);
    stockItem->setData(Qt::UserRole, info.minStock); // 存储预警阈值
    if (info.stock <= info.minStock) stockItem->setForeground(QColor("#f56c6c"));
    prodTable->setItem(row, 7, stockItem);

    // 状态标签 (第 7 列)
    QWidget *tagContainer = new QWidget();
    tagContainer->setStyleSheet("background: transparent;");
    QHBoxLayout *tagLayout = new QHBoxLayout(tagContainer);
    tagLayout->setContentsMargins(0, 0, 0, 0);
    tagLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *tag = new QLabel();
    QString baseStyle = "padding: 2px 10px; border-radius: 10px; font-size: 11px; ";
    if (info.stock <= 0) {
        tag->setText("缺货");
        tag->setStyleSheet(baseStyle + "background: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4;");
    } else if (info.stock <= info.minStock) {
        tag->setText("库存告急");
        tag->setStyleSheet(baseStyle + "background: #fff7e6; color: #fa8c16; border: 1px solid #ffd591;");
    } else {
        tag->setText("充足");
        tag->setStyleSheet(baseStyle + "background: #f0f9eb; color: #67c23a; border: 1px solid #e1f3d8;");
    }
    tagLayout->addWidget(tag);
    prodTable->setCellWidget(row, 8, tagContainer);

    // 9. 操作列 (编辑与删除按钮)
    QWidget *optContainer = new QWidget();
    optContainer->setStyleSheet("background: transparent;");
    QHBoxLayout *optLayout = new QHBoxLayout(optContainer);
    optLayout->setContentsMargins(0, 0, 15, 0); // 右边距 15px
    optLayout->setSpacing(0);
    optLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter); // 右对齐


    QPushButton *delBtn = new QPushButton("删除");
    delBtn->setFixedSize(50, 26);
    delBtn->setCursor(Qt::PointingHandCursor);
    delBtn->setStyleSheet(
        "QPushButton { background: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 3px; font-size: 11px; padding: 0; text-align: center; } "
        "QPushButton:hover { background: #f56c6c; color: white; }"
    );
    connect(delBtn, &QPushButton::clicked, this, &ProductModule::onDeleteProduct);

    optLayout->addWidget(delBtn);
    prodTable->setCellWidget(row, 9, optContainer);

    // 同步刷新分页（仅当控件已初始化）
    if (pageLabel) updatePagination();
}

void ProductModule::onStockIn() {
    ProductInfo emptyInfo;
    emptyInfo.barcode = "";
    emptyInfo.name = "";
    emptyInfo.stock = 0;
    emptyInfo.minStock = 5;
    emptyInfo.productionDate = QDate::currentDate().toString("yyyy-MM-dd");
    emptyInfo.shelfLifeDays = 365;
    
    showProductEditDialog(emptyInfo, true);
}

void ProductModule::onEditProduct() {
    int row = prodTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请先在列表中选择要修改的商品");
        return;
    }

    QString barcode = prodTable->item(row, 2)->text();
    ProductInfo info = ProductDataManager::instance()->getProduct(barcode);
    if (info.barcode.isEmpty()) return;

    showProductEditDialog(info, false);
}

void ProductModule::showProductEditDialog(const ProductInfo &info, bool isNew) {
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(isNew ? "新商品入库登记" : "修改商品档案");
    dialog->setWindowIcon(QIcon());
    dialog->setMinimumWidth(600);
    dialog->setStyleSheet("QDialog { background: #f8f9fa; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header (Pro Max Style)
    QWidget *header = new QWidget();
    header->setFixedHeight(70);
    header->setStyleSheet("background: white; border-bottom: 1px solid #e4e7ed;");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(25, 0, 25, 0);
    
    QVBoxLayout *titleArea = new QVBoxLayout();
    titleArea->setSpacing(2);
    titleArea->setAlignment(Qt::AlignVCenter);
    
    QLabel *headerTitle = new QLabel(isNew ? "新商品入库登记" : "编辑商品详情");
    headerTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #1a1a1a;");
    
    QLabel *headerSub = new QLabel(isNew ? "请输入新商品的基础资料与库存信息" : "正在修改: " + info.name);
    headerSub->setStyleSheet("font-size: 12px; color: #909399;");
    
    titleArea->addWidget(headerTitle);
    titleArea->addWidget(headerSub);
    
    headerLayout->addLayout(titleArea);
    headerLayout->addStretch();
    mainLayout->addWidget(header);

    // Content Scroll Area
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: #f8f9fa; }");
    
    QWidget *content = new QWidget();
    content->setObjectName("dialogContent");
    content->setStyleSheet("QWidget#dialogContent { background: #f8f9fa; }");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(30, 25, 30, 25);
    contentLayout->setSpacing(24);
    
    auto createSectionHeader = [&](const QString &title, QBoxLayout *targetLayout = nullptr) {
        QWidget *container = new QWidget();
        QVBoxLayout *l = new QVBoxLayout(container);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(8);

        QHBoxLayout *h = new QHBoxLayout();
        QLabel *lbl = new QLabel(title);
        lbl->setStyleSheet("font-weight: bold; color: #303133; font-size: 15px;");
        h->addWidget(lbl);
        h->addStretch();
        l->addLayout(h);
        
        if (targetLayout) targetLayout->addWidget(container);
        else contentLayout->addWidget(container);
        return container;
    };

    auto createPremiumRow = [&](const QString &label, QWidget *widget, const QString &desc = "", QBoxLayout *targetLayout = nullptr) {
        QWidget *container = new QWidget();
        QVBoxLayout *rowContainer = new QVBoxLayout(container);
        rowContainer->setContentsMargins(0, 0, 0, 0);
        rowContainer->setSpacing(8);
        
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
        
        rowContainer->addWidget(lbl);
        rowContainer->addWidget(widget);
        
        if (!desc.isEmpty()) {
            QLabel *d = new QLabel(desc);
            d->setStyleSheet("font-size: 11px; color: #909399;");
            rowContainer->addWidget(d);
        }
        
        if (targetLayout) targetLayout->addWidget(container);
        else contentLayout->addWidget(container);
        return container;
    };

    QString editStyle = "QLineEdit, QDateEdit, QSpinBox, QDoubleSpinBox, QComboBox, QTextEdit { "
                       "background: white; border: 1px solid #dcdfe6; border-radius: 8px; padding: 8px 12px; color: #606266; font-size: 13px; } "
                       "QLineEdit:focus, QDateEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QTextEdit:focus { "
                       "border-color: #409eff; background: #fbfdff; } "
                       "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 30px; border: none; } "
                       "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
                       "QComboBox QAbstractItemView { border: 1px solid #e4e7ed; background: white; border-radius: 4px; selection-background-color: #f5f7fa; selection-color: #409eff; }";

    // 1. 核心识别 (条码在新建时必须，在修改时禁用)
    createSectionHeader("唯一标识");
    QLineEdit *barcodeEdit = new QLineEdit(info.barcode);
    barcodeEdit->setPlaceholderText("扫描或输入条形码...");
    barcodeEdit->setStyleSheet(editStyle);
    if (!isNew) {
        barcodeEdit->setEnabled(false);
        barcodeEdit->setStyleSheet(editStyle + "background: #f5f7fa; color: #c0c4cc;");
    }
    createPremiumRow("条形码 (不可更改)", barcodeEdit, "条形码作为商品的唯一身份ID，登记后不可修改。");

    // 2. 基础描述
    createSectionHeader("基础信息");
    QLineEdit *nameEdit = new QLineEdit(info.name);
    nameEdit->setPlaceholderText("请输入商品完整名称...");
    nameEdit->setStyleSheet(editStyle);
    createPremiumRow("商品名称", nameEdit);

    QHBoxLayout *grid1 = new QHBoxLayout();
    grid1->setSpacing(20);
    
    QVBoxLayout *v1 = new QVBoxLayout();
    QComboBox *categoryCombo = new QComboBox();
    categoryCombo->addItems({"主食", "玩具", "零食", "洗护"});
    if (!info.category.isEmpty()) categoryCombo->setCurrentText(info.category);
    categoryCombo->setStyleSheet(editStyle);
    QLabel *l1 = new QLabel("所属分类"); l1->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v1->addWidget(l1); v1->addWidget(categoryCombo);
    
    QVBoxLayout *v2 = new QVBoxLayout();
    QLineEdit *specEdit = new QLineEdit(info.spec);
    specEdit->setPlaceholderText("如：10kg/袋");
    specEdit->setStyleSheet(editStyle);
    QLabel *l2 = new QLabel("规格单位"); l2->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v2->addWidget(l2); v2->addWidget(specEdit);
    
    grid1->addLayout(v1);
    grid1->addLayout(v2);
    contentLayout->addLayout(grid1);

    QLineEdit *originEdit = new QLineEdit(info.origin);
    originEdit->setStyleSheet(editStyle);
    createPremiumRow("产地/品牌信息", originEdit);

    QHBoxLayout *grid1_5 = new QHBoxLayout();
    grid1_5->setSpacing(20);
    
    QVBoxLayout *v1_5_a = new QVBoxLayout();
    QLineEdit *supplierEdit = new QLineEdit(info.supplier);
    supplierEdit->setPlaceholderText("请输入供货厂商名称...");
    supplierEdit->setStyleSheet(editStyle);
    QLabel *l1_5_a = new QLabel("供货厂商"); l1_5_a->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v1_5_a->addWidget(l1_5_a); v1_5_a->addWidget(supplierEdit);
    
    QVBoxLayout *v1_5_b = new QVBoxLayout();
    QLineEdit *supplierContactEdit = new QLineEdit(info.supplierPhone);
    supplierContactEdit->setPlaceholderText("请输入联系人或电话...");
    supplierContactEdit->setStyleSheet(editStyle);
    QLabel *l1_5_b = new QLabel("联系方式"); l1_5_b->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v1_5_b->addWidget(l1_5_b); v1_5_b->addWidget(supplierContactEdit);
    
    grid1_5->addLayout(v1_5_a);
    grid1_5->addLayout(v1_5_b);
    contentLayout->addLayout(grid1_5);

    // 3. 经营财务
    createSectionHeader("经营与库存");
    QHBoxLayout *grid2 = new QHBoxLayout();
    grid2->setSpacing(20);

    QVBoxLayout *v3 = new QVBoxLayout();
    QLineEdit *priceEdit = new QLineEdit(QString::number(info.price, 'f', 2));
    priceEdit->setPlaceholderText("0.00");
    priceEdit->setStyleSheet(editStyle);
    QLabel *l3 = new QLabel("销售单价"); l3->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v3->addWidget(l3); v3->addWidget(priceEdit);

    QVBoxLayout *v4 = new QVBoxLayout();
    QLineEdit *costEdit = new QLineEdit(QString::number(info.costPrice, 'f', 2));
    costEdit->setPlaceholderText("0.00");
    costEdit->setStyleSheet(editStyle);
    QLabel *l4 = new QLabel("进货成本"); l4->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v4->addWidget(l4); v4->addWidget(costEdit);

    grid2->addLayout(v3);
    grid2->addLayout(v4);
    contentLayout->addLayout(grid2);

    QHBoxLayout *grid3 = new QHBoxLayout();
    grid3->setSpacing(20);

    QVBoxLayout *v5 = new QVBoxLayout();
    QLineEdit *stockEdit = new QLineEdit(QString::number(info.stock));
    stockEdit->setPlaceholderText("0");
    stockEdit->setStyleSheet(editStyle);
    QLabel *l5 = new QLabel(isNew ? "初始库存量" : "当前总库存"); l5->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v5->addWidget(l5); v5->addWidget(stockEdit);

    QVBoxLayout *v6 = new QVBoxLayout();
    QLineEdit *warningEdit = new QLineEdit(QString::number(info.minStock));
    warningEdit->setPlaceholderText("5");
    warningEdit->setStyleSheet(editStyle);
    QLabel *l6 = new QLabel("库存预警线"); l6->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v6->addWidget(l6); v6->addWidget(warningEdit);

    grid3->addLayout(v5);
    grid3->addLayout(v6);
    contentLayout->addLayout(grid3);

    // 4. 数字化质控
    QWidget *qualityContainer = new QWidget();
    QVBoxLayout *qualityLayout = new QVBoxLayout(qualityContainer);
    qualityLayout->setContentsMargins(0, 0, 0, 0);
    qualityLayout->setSpacing(24);
    contentLayout->addWidget(qualityContainer);

    createSectionHeader("数字化质控", qualityLayout);

    QHBoxLayout *grid4 = new QHBoxLayout();
    grid4->setSpacing(20);

    QVBoxLayout *v7 = new QVBoxLayout();
    CustomCalendarEdit *prodDateEdit = new CustomCalendarEdit();
    if (!info.productionDate.isEmpty()) prodDateEdit->setText(info.productionDate);
    else prodDateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    prodDateEdit->setStyleSheet(editStyle);
    QLabel *l7 = new QLabel("生产日期"); l7->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v7->addWidget(l7); v7->addWidget(prodDateEdit);

    QVBoxLayout *v8 = new QVBoxLayout();
    QLineEdit *shelfLifeEdit = new QLineEdit(QString::number(info.shelfLifeDays));
    shelfLifeEdit->setPlaceholderText("365");
    shelfLifeEdit->setStyleSheet(editStyle);
    QLabel *l8 = new QLabel("保质期时长 (天)"); l8->setStyleSheet("font-weight: 600; color: #606266; font-size: 13px;");
    v8->addWidget(l8); v8->addWidget(shelfLifeEdit);

    grid4->addLayout(v7);
    grid4->addLayout(v8);
    qualityLayout->addLayout(grid4);

    QLineEdit *storageEdit = new QLineEdit(info.storageReq);
    storageEdit->setPlaceholderText("如：阴凉、干燥、通风保存");
    storageEdit->setStyleSheet(editStyle);
    createPremiumRow("存储环境要求", storageEdit, "", qualityLayout);

    // 5. 导购支持
    createSectionHeader("导购决策辅助");
    QLineEdit *tagsEdit = new QLineEdit(info.tags.join(", "));
    tagsEdit->setPlaceholderText("用逗号分隔卖点，如：低敏, 美毛, 零添加");
    tagsEdit->setStyleSheet(editStyle);
    createPremiumRow("卖点标签 (Tags)", tagsEdit, "这些标签将以彩色卡片形式显示在详情页顶部。");

    QTextEdit *ingredEdit = new QTextEdit(info.ingredients);
    ingredEdit->setPlaceholderText("请输入主要原料组成，复杂配料将自动支持截断显示...");
    ingredEdit->setFixedHeight(100);
    ingredEdit->setStyleSheet(editStyle);
    QWidget *ingredRow = createPremiumRow("成分配料表", ingredEdit);

    // 媒体图片
    createSectionHeader("媒体资料");
    QStringList *selectedImgPaths = new QStringList(info.images);
    QWidget *photoPanel = new QWidget();
    photoPanel->setStyleSheet("background: #f8f9fa; border: none; padding: 0;");
    QVBoxLayout *photoPanelLayout = new QVBoxLayout(photoPanel);
    photoPanelLayout->setContentsMargins(0, 0, 0, 0);
    photoPanelLayout->setSpacing(12);
    
    QScrollArea *imgScroll = new QScrollArea();
    imgScroll->setFixedHeight(70);
    imgScroll->setWidgetResizable(true);
    imgScroll->setFrameShape(QFrame::NoFrame);
    QWidget *imgContainer = new QWidget();
    QHBoxLayout *imgLayout = new QHBoxLayout(imgContainer);
    imgLayout->setContentsMargins(0, 0, 0, 0);
    imgLayout->setSpacing(8);
    imgScroll->setWidget(imgContainer);
    
    QPushButton *uploadBtn = new QPushButton("➕ 添加商品图片");
    uploadBtn->setFixedHeight(40);
    uploadBtn->setCursor(Qt::PointingHandCursor);
    uploadBtn->setStyleSheet("QPushButton { background: #f5f7fa; color: #409eff; border: 1px dashed #409eff; border-radius: 4px; font-weight: bold; } "
                           "QPushButton:hover { background: #ecf5ff; }");

    auto refreshImgList = [=]() {
        QLayoutItem *child;
        while ((child = imgLayout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
        if (selectedImgPaths->isEmpty()) {
            QLabel *empty = new QLabel("尚未上传图片");
            empty->setStyleSheet("color: #909399; font-size: 12px;");
            imgLayout->addWidget(empty);
        } else {
            for (const QString &p : *selectedImgPaths) {
                QLabel *pLabel = new QLabel();
                pLabel->setFixedSize(54, 54);
                pLabel->setPixmap(QPixmap(p).scaled(54, 54, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                pLabel->setStyleSheet("border: none; border-radius: 6px; background: white;");
                pLabel->setAlignment(Qt::AlignCenter);
                imgLayout->addWidget(pLabel);
            }
        }
        imgLayout->addStretch();
    };
    refreshImgList();

    connect(uploadBtn, &QPushButton::clicked, [=]() {
        QStringList paths = QFileDialog::getOpenFileNames(dialog, "选择图片", "", "Images (*.png *.jpg *.jpeg)");
        if (!paths.isEmpty()) {
            *selectedImgPaths = paths;
            refreshImgList();
        }
    });

    auto updateVisibility = [=](const QString &cat) {
        bool isToy = (cat == "玩具");
        qualityContainer->setVisible(!isToy);
        ingredRow->setVisible(!isToy);

        // 动态更新标签占位符
        QString placeholder = "用逗号分隔卖点，如：";
        if (cat == "主食") placeholder += "高肉含量, 天然无谷, 美毛, 低敏";
        else if (cat == "玩具") placeholder += "耐咬, 益智, 互动性强, 安全无毒";
        else if (cat == "零食") placeholder += "适口性好, 磨牙补钙, 冻干工艺, 独立包装";
        else if (cat == "洗护") placeholder += "植萃配方, 柔顺亮泽, 祛味持久, 温和不刺激";
        else placeholder = "用逗号分隔多个卖点标签 (Tags)";
        
        tagsEdit->setPlaceholderText(placeholder);
    };
    connect(categoryCombo, &QComboBox::currentTextChanged, updateVisibility);
    updateVisibility(categoryCombo->currentText());

    photoPanelLayout->addWidget(imgScroll);
    photoPanelLayout->addWidget(uploadBtn);
    contentLayout->addWidget(photoPanel);

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);

    // Footer
    QWidget *footer = new QWidget();
    footer->setFixedHeight(90);
    footer->setStyleSheet("background: white; border-top: 1px solid #e4e7ed;");
    QHBoxLayout *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(30, 24, 30, 24);
    
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setFixedSize(100, 42);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet("QPushButton { padding: 4px 20px; background: white; border: 1px solid #dcdfe6; border-radius: 21px; color: #606266; font-weight: bold; } "
                           "QPushButton:hover { background: #f5f7fa; border-color: #c0c4cc; }");
    
    QPushButton *saveBtn = new QPushButton(isNew ? "立即入库" : "同步资料");
    saveBtn->setFixedSize(140, 42);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet("QPushButton { padding: 4px 20px; background: #409eff; color: white; border-radius: 21px; font-weight: bold; font-size: 14px; box-shadow: 0 4px 12px rgba(64,158,255,0.3); } "
                         "QPushButton:hover { background: #66b1ff; }");
    
    footerLayout->addStretch();
    footerLayout->addWidget(cancelBtn);
    footerLayout->addSpacing(15);
    footerLayout->addWidget(saveBtn);
    mainLayout->addWidget(footer);

    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, [=]() mutable {
        ProductInfo nInfo = info;
        nInfo.barcode = barcodeEdit->text().trimmed();
        nInfo.name = nameEdit->text().trimmed();
        
        if (nInfo.barcode.isEmpty() || nInfo.name.isEmpty()) {
            QMessageBox::warning(dialog, "提示", "商品条码和名称为必填项！");
            return;
        }

        // 检查条码冲突 (仅在新建模式)
        if (isNew) {
            ProductInfo existing = ProductDataManager::instance()->getProduct(nInfo.barcode);
            if (!existing.barcode.isEmpty()) {
                QMessageBox::warning(dialog, "条码冲突", QString("系统中已存在条码为 [%1] 的商品 (%2)。\n请直接搜索该商品进行“修改”或“补货”操作。").arg(nInfo.barcode, existing.name));
                return;
            }
        }

        nInfo.category = categoryCombo->currentText().trimmed();
        nInfo.origin = originEdit->text().trimmed();
        nInfo.supplier = supplierEdit->text().trimmed();
        nInfo.supplierPhone = supplierContactEdit->text().trimmed();
        nInfo.spec = specEdit->text().trimmed();
        nInfo.price = priceEdit->text().toDouble();
        nInfo.costPrice = costEdit->text().toDouble();
        nInfo.stock = stockEdit->text().toInt();
        nInfo.minStock = warningEdit->text().toInt();
        nInfo.productionDate = prodDateEdit->text().trimmed();
        nInfo.shelfLifeDays = shelfLifeEdit->text().toInt();
        nInfo.storageReq = storageEdit->text().trimmed();
        nInfo.ingredients = ingredEdit->toPlainText().trimmed();
        nInfo.tags = tagsEdit->text().split(",", Qt::SkipEmptyParts);
        for(QString &t : nInfo.tags) t = t.trimmed();
        nInfo.images = *selectedImgPaths;

        if (isNew) {
            ProductDataManager::instance()->addProduct(nInfo);
            addProductRow(nInfo);
            
            // 记录入库历史
            StockInRecord record;
            record.dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            record.productName = nInfo.name;
            record.barcode = nInfo.barcode;
            record.quantity = nInfo.stock;
            record.supplier = nInfo.supplier.isEmpty() ? "自主入库" : nInfo.supplier;
            record.operatorName = "管理员";
            m_records.append(record);
            addRecordRow(record);
        } else {
            ProductDataManager::instance()->addProduct(nInfo);
            // 更新现有行
            int row = prodTable->currentRow();
            if (row >= 0) {
                prodTable->item(row, 3)->setText(nInfo.name);
                prodTable->item(row, 4)->setText(nInfo.spec);
                prodTable->item(row, 6)->setText(QString("￥%1").arg(nInfo.price, 0, 'f', 2));
                prodTable->item(row, 7)->setText(QString::number(nInfo.stock));
                prodTable->item(row, 7)->setData(Qt::UserRole, nInfo.minStock);
                
                // 更新状态标签
                QWidget *tagWidget = prodTable->cellWidget(row, 8);
                if (tagWidget) {
                    QLabel *tag = tagWidget->findChild<QLabel*>();
                    if (tag) {
                        QString baseStyle = "padding: 2px 10px; border-radius: 10px; font-size: 11px; ";
                        if (nInfo.stock <= 0) {
                            tag->setText("缺货");
                            tag->setStyleSheet(baseStyle + "background: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4;");
                        } else if (nInfo.stock <= nInfo.minStock) {
                            tag->setText("库存告急");
                            tag->setStyleSheet(baseStyle + "background: #fff7e6; color: #fa8c16; border: 1px solid #ffd591;");
                        } else {
                            tag->setText("充足");
                            tag->setStyleSheet(baseStyle + "background: #f0f9eb; color: #67c23a; border: 1px solid #e1f3d8;");
                        }
                    }
                }
            }
            updateDetailDrawer(nInfo);
        }

        updateStats();
        QMessageBox::information(dialog, "成功", isNew ? "新商品已成功入库！" : "商品档案已同步更新！");
        dialog->accept();
    });

    dialog->exec();
}

void ProductModule::updateStats() {
    int varieties = prodTable->rowCount();
    int lowStock = 0;
    double totalValue = 0;

    for (int i = 0; i < varieties; ++i) {
        // 列索引修正：5:成本, 6:售价, 7:库存
        QTableWidgetItem *priceItem = prodTable->item(i, 6);
        QTableWidgetItem *stockItem = prodTable->item(i, 7);
        
        if (!priceItem || !stockItem) continue;

        int stock = stockItem->text().toInt();
        double price = priceItem->text().remove(QStringLiteral("￥")).toDouble();
        int minStock = stockItem->data(Qt::UserRole).toInt();
        
        totalValue += (price * stock);

        // 使用属性判定的预警逻辑
        if (stock <= minStock) {
            lowStock++;
        }
    }

    varietyLabel->setText(QStringLiteral("%1种").arg(varieties));
    lowStockLabel->setText(QStringLiteral("%1项").arg(lowStock));
    totalValueLabel->setText(QStringLiteral("￥%1").arg(totalValue, 0, 'f', 2));
}

void ProductModule::onFilterRecords() {
    m_recCurrentPage = 1;
    updateRecordPagination();
}

void ProductModule::onResetRecords() {
    // 禁用信号以避免多次触发过滤
    recordSearch->blockSignals(true);
    sYearCombo->blockSignals(true);
    sMonthCombo->blockSignals(true);
    eYearCombo->blockSignals(true);
    eMonthCombo->blockSignals(true);

    recordSearch->clear();
    QDate startInitial(2026, 1, 1);
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

void ProductModule::onPreviewImage()
{
    // 该槽函数目前主要通过 eventFilter 触发内部逻辑
}



void ProductModule::onPrevPage()
{
    if (m_currentPage > 1) {
        m_currentPage--;
        updatePagination();
    }
}

void ProductModule::onRecPrevPage()
{
    if (m_recCurrentPage > 1) {
        m_recCurrentPage--;
        updateRecordPagination();
    }
}

void ProductModule::onNextPage()
{
    int total = prodTable->rowCount();
    QString kw = searchEdit->text().trimmed().toLower();
    int visibleCount = 0;
    for (int i = 0; i < total; ++i) {
        bool match = false;
        if (kw.isEmpty()) match = true;
        else {
            for (int col : {2, 3, 4}) { // 条码、名称、规格偏移
                QTableWidgetItem *item = prodTable->item(i, col);
                if (item && item->text().toLower().contains(kw)) { match = true; break; }
            }
        }
        if (match) visibleCount++;
    }
    int totalPages = qMax(1, (visibleCount + m_pageSize - 1) / m_pageSize);
    if (m_currentPage < totalPages) {
        m_currentPage++;
        updatePagination();
    }
}

void ProductModule::onRecNextPage()
{
    // 获取当前过滤后的总数
    QDate start(sYearCombo->currentData().toInt(), sMonthCombo->currentData().toInt(), sDayCombo->currentData().toInt());
    QDate end(eYearCombo->currentData().toInt(), eMonthCombo->currentData().toInt(), eDayCombo->currentData().toInt());
    QString kw = recordSearch->text().trimmed().toLower();

    int visibleCount = 0;
    for (const auto &record : m_records) {
        QDate rd = QDateTime::fromString(record.dateTime, "yyyy-MM-dd HH:mm:ss").date();
        if (rd >= start && rd <= end) {
            if (kw.isEmpty() || record.productName.contains(kw) || record.barcode.contains(kw) || record.supplier.contains(kw)) {
                visibleCount++;
            }
        }
    }

    int totalPages = qMax(1, (visibleCount + m_recPageSize - 1) / m_recPageSize);
    if (m_recCurrentPage < totalPages) {
        m_recCurrentPage++;
        updateRecordPagination();
    }
}

void ProductModule::onJumpPage()
{
    int page = jumpEdit->text().toInt();
    if (page < 1) return;
    m_currentPage = page;
    updatePagination();
    jumpEdit->clear();
    jumpEdit->clearFocus();
}

void ProductModule::onRecJumpPage()
{
    int page = recJumpEdit->text().toInt();
    if (page < 1) return;
    m_recCurrentPage = page;
    updateRecordPagination();
    recJumpEdit->clear();
    recJumpEdit->clearFocus();
}

void ProductModule::onSearchChanged(const QString &)
{
    m_currentPage = 1;
    updatePagination();
}

void ProductModule::updatePagination()
{
    int total = prodTable->rowCount();
    QString kw = searchEdit->text().trimmed().toLower();
    
    QList<int> visibleRows;
    for (int i = 0; i < total; ++i) {
        bool match = false;
        if (kw.isEmpty()) {
            match = true;
        } else {
            for (int col : {2, 3, 4}) { // 搜索列偏移
                QTableWidgetItem *item = prodTable->item(i, col);
                if (item && item->text().toLower().contains(kw)) {
                    match = true;
                    break;
                }
            }
        }
        if (match) visibleRows.append(i);
        prodTable->setRowHidden(i, true);
    }

    int totalVisible = visibleRows.size();
    int totalPages = qMax(1, (totalVisible + m_pageSize - 1) / m_pageSize);
    
    if (jumpValidator) jumpValidator->setTop(totalPages);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, totalVisible);

    for (int i = start; i < end; ++i) {
        prodTable->setRowHidden(visibleRows[i], false);
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

void ProductModule::updateRecordPagination() {
    QDate start(sYearCombo->currentData().toInt(), sMonthCombo->currentData().toInt(), sDayCombo->currentData().toInt());
    QDate end(eYearCombo->currentData().toInt(), eMonthCombo->currentData().toInt(), eDayCombo->currentData().toInt());
    QString kw = recordSearch->text().trimmed().toLower();

    QList<StockInRecord> visibleRecords;
    for (const auto &record : m_records) {
        QDate rd = QDateTime::fromString(record.dateTime, "yyyy-MM-dd HH:mm:ss").date();
        if (rd >= start && rd <= end) {
            if (kw.isEmpty() || record.productName.toLower().contains(kw) || 
                record.barcode.toLower().contains(kw) || record.supplier.toLower().contains(kw)) {
                visibleRecords.append(record);
            }
        }
    }

    int totalVisible = visibleRecords.size();
    int totalPages = qMax(1, (totalVisible + m_recPageSize - 1) / m_recPageSize);

    if (m_recCurrentPage > totalPages) m_recCurrentPage = totalPages;
    if (m_recCurrentPage < 1) m_recCurrentPage = 1;
    if (recJumpValidator) recJumpValidator->setTop(totalPages);

    int startIdx = (m_recCurrentPage - 1) * m_recPageSize;
    int endIdx = qMin(startIdx + m_recPageSize, totalVisible);

    recordTable->setRowCount(0);
    for (int i = startIdx; i < endIdx; ++i) {
        addRecordRow(visibleRecords[i]);
    }

    recPageLabel->setText(QStringLiteral("第 %1 页 / 共 %2 页").arg(m_recCurrentPage).arg(totalPages));
    recPrevBtn->setEnabled(m_recCurrentPage > 1);
    recNextBtn->setEnabled(m_recCurrentPage < totalPages);

    if (totalVisible == 0) {
        recPrevBtn->setEnabled(false);
        recNextBtn->setEnabled(false);
        recPageLabel->setText(QStringLiteral("第 0 页 / 共 0 页"));
    }
}

void ProductModule::onDeleteProduct() {
    QMessageBox::information(this, "业务提醒", "【删除商品】\n如果该商品已有出入库业务流水，为了保障经营数据完整性，系统强烈建议将其标记为“已下架”而非硬删除。");
}

void ProductModule::onBatchDelete()
{
    QList<int> checkedRows;
    for (int i = prodTable->rowCount() - 1; i >= 0; --i) {
        QWidget *w = prodTable->cellWidget(i, 0); // 勾选列在第 0 列
        if (w) {
            QCheckBox *cb = w->findChild<QCheckBox*>();
            if (cb && cb->isChecked()) {
                checkedRows.append(i);
            }
        }
    }
    
    if (checkedRows.isEmpty()) {
        QMessageBox::warning(this, "批量操作", "请先勾选需要删除的商品记录。");
        return;
    }

    if (QMessageBox::question(this, "批量删除", QString("确定要删除选中的 %1 个商品档案吗？此操作不可撤销。").arg(checkedRows.size()),
                               QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        for (int row : checkedRows) {
            prodTable->removeRow(row);
        }
        updateStats();
        updatePagination();
    }
}

int ProductModule::getLowStockCount() const
{
    int count = 0;
    for (int i = 0; i < prodTable->rowCount(); ++i) {
        QTableWidgetItem *stockItem = prodTable->item(i, 6);
        if (stockItem) {
            int stock = stockItem->text().toInt();
            int minStock = stockItem->data(Qt::UserRole).toInt();
            if (stock <= minStock) {
                count++;
            }
        }
    }
    return count;
}

void ProductModule::setupDetailDrawer() {
    // ===== 抽屉面板（侧边布局） =====
    m_detailDrawer = new QWidget(this);
    m_detailDrawer->setStyleSheet(
        "QWidget#drawerPanel {"
        "  background: white;"
        "  border-left: 1px solid #e4e7ed;"
        "}"
    );
    m_detailDrawer->setObjectName("drawerPanel");
    m_detailDrawer->hide();
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_detailDrawer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ===== 抽屉头部（关闭按钮 + 标题 + 编辑按钮） =====
    QWidget *drawerHeader = new QWidget();
    drawerHeader->setFixedHeight(52);
    drawerHeader->setStyleSheet("background: #fafbfc; border-bottom: 1px solid #ebeef5;");
    QHBoxLayout *headerLayout = new QHBoxLayout(drawerHeader);
    headerLayout->setContentsMargins(20, 0, 12, 0);
    
    m_lblDrawerHeaderTitle = new QLabel("📦 商品详情");
    m_lblDrawerHeaderTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #303133; background: transparent;");
    
    m_btnModifyInfo = new QPushButton("✎ 修改资料");
    m_btnModifyInfo->setCursor(Qt::PointingHandCursor);
    m_btnModifyInfo->setStyleSheet(
        "QPushButton { background: transparent; color: #409eff; border: none; font-size: 13px; font-weight: bold; padding: 4px 8px; border-radius: 4px; }"
        "QPushButton:hover { background: #ecf5ff; }"
    );
    connect(m_btnModifyInfo, &QPushButton::clicked, this, &ProductModule::onEditProduct);
    
    QPushButton *closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(32, 32);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #909399; border: none; border-radius: 16px; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background: #f56c6c; color: white; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &ProductModule::closeDrawer);
    
    headerLayout->addWidget(m_lblDrawerHeaderTitle);
    headerLayout->addStretch();
    
    QPushButton *prevBtn = new QPushButton("上一个");
    prevBtn->setCursor(Qt::PointingHandCursor);
    prevBtn->setStyleSheet("QPushButton { background: transparent; color: #606266; border: 1px solid #dcdfe6; font-size: 12px; padding: 4px 10px; border-radius: 4px; }"
                           "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #c0c4cc; }");
    connect(prevBtn, &QPushButton::clicked, this, [=]() {
        int row = prodTable->currentRow();
        while (row > 0) {
            row--;
            if (!prodTable->isRowHidden(row)) {
                prodTable->selectRow(row);
                break;
            }
        }
    });

    QPushButton *nextBtn = new QPushButton("下一个");
    nextBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setStyleSheet("QPushButton { background: transparent; color: #606266; border: 1px solid #dcdfe6; font-size: 12px; padding: 4px 10px; border-radius: 4px; }"
                           "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #c0c4cc; }");
    connect(nextBtn, &QPushButton::clicked, this, [=]() {
        int row = prodTable->currentRow();
        while (row >= 0 && row < prodTable->rowCount() - 1) {
            row++;
            if (!prodTable->isRowHidden(row)) {
                prodTable->selectRow(row);
                break;
            }
        }
    });

    headerLayout->addWidget(prevBtn);
    headerLayout->addWidget(nextBtn);
    headerLayout->addSpacing(10);
    headerLayout->addWidget(m_btnModifyInfo);
    headerLayout->addWidget(closeBtn);
    mainLayout->addWidget(drawerHeader);

    // ===== 顶部图片区域 =====
    QWidget *topHeader = new QWidget();
    topHeader->setFixedHeight(280);
    topHeader->setStyleSheet("background: white;");
    QVBoxLayout *topLayout = new QVBoxLayout(topHeader);
    topLayout->setContentsMargins(24, 16, 24, 10);
    
    m_mainPreview = new QLabel();
    m_mainPreview->setMinimumHeight(200);
    m_mainPreview->setStyleSheet("background: #f5f7fa; border-radius: 12px; border: 1px solid #e4e7ed;");
    m_mainPreview->setAlignment(Qt::AlignCenter);
    m_mainPreview->setCursor(Qt::PointingHandCursor);
    m_mainPreview->installEventFilter(this);
    m_mainPreview->setScaledContents(false);
    
    m_thumbContainer = new QWidget();
    QHBoxLayout *thumbLayout = new QHBoxLayout(m_thumbContainer);
    thumbLayout->setContentsMargins(0, 5, 0, 0);
    thumbLayout->setSpacing(10);
    thumbLayout->setAlignment(Qt::AlignCenter);

    topLayout->addWidget(m_mainPreview);
    topLayout->addWidget(m_thumbContainer);
    mainLayout->addWidget(topHeader);

    // ===== 滚动内容区域 =====
    m_detailScroll = new QScrollArea();
    m_detailScroll->setWidgetResizable(true);
    m_detailScroll->setFrameShape(QFrame::NoFrame);
    m_detailScroll->setStyleSheet("QScrollArea { border: none; background: #fafbfc; }"); // 稍微带点底色区分内容
    
    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: #fafbfc;");
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(24, 16, 24, 40);
    scrollLayout->setSpacing(20);

    // 1. 标题与品牌
    m_lblDetailName = new QLabel("未选择商品");
    m_lblDetailName->setWordWrap(true);
    m_lblDetailName->setStyleSheet("font-size: 22px; font-weight: 800; color: #303133;");
    m_lblDetailBrand = new QLabel("");
    m_lblDetailBrand->setStyleSheet("color: #409eff; font-weight: bold; font-size: 14px; background: #ecf5ff; padding: 4px 10px; border-radius: 12px;");
    
    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->addWidget(m_lblDetailName);
    titleLayout->addStretch();
    scrollLayout->addLayout(titleLayout);
    
    QHBoxLayout *brandLayout = new QHBoxLayout();
    brandLayout->addWidget(m_lblDetailBrand);
    brandLayout->addStretch();
    scrollLayout->addLayout(brandLayout);
    
    // 卖点标签区
    m_tagContainer = new QWidget();
    QHBoxLayout *tagLay = new QHBoxLayout(m_tagContainer);
    tagLay->setContentsMargins(0, 0, 0, 0);
    tagLay->setSpacing(8);
    tagLay->setAlignment(Qt::AlignLeft);
    scrollLayout->addWidget(m_tagContainer);

    // ================= 垂直卡片流 =================
    // 2. 核心交易区 (The Buy Box)
    QWidget *financeCard = new QWidget();
    financeCard->setStyleSheet("background: white; border-radius: 8px; border: 1px solid #f0f2f5; padding: 12px;");
    QVBoxLayout *financeLayout = new QVBoxLayout(financeCard);
    financeLayout->setSpacing(10);
    financeLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *financeTitle = new QLabel("经营与财务");
    financeTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #303133; border: none; background: transparent;");
    financeLayout->addWidget(financeTitle);
    
    auto addFinanceRow = [&](const QString &label, QLabel* &valLabel, const QString &color = "#303133", bool isBold = false) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *l = new QLabel(label); l->setStyleSheet("color: #909399; font-size: 13px; background: transparent; border: none;");
        valLabel = new QLabel("-"); 
        valLabel->setStyleSheet(QString("color: %1; font-size: 13px; background: transparent; border: none; %2").arg(color).arg(isBold ? "font-weight: bold;" : ""));
        row->addWidget(l); row->addStretch(); row->addWidget(valLabel);
        financeLayout->addLayout(row);
    };
    
    addFinanceRow("零售指导价", m_lblSalePrice, "#f56c6c", true);
    if (m_role == ADMIN) {
        addFinanceRow("进货成本价", m_lblCostPrice);
        addFinanceRow("预估毛利率", m_lblGrossMargin, "#67c23a", true);
    }
    
    QFrame *line = new QFrame(); line->setFrameShape(QFrame::HLine); line->setStyleSheet("background: #f0f2f5;");
    financeLayout->addWidget(line);
    
    addFinanceRow("当前可用库存", m_lblCurrentStock, "#409eff", true);
    addFinanceRow("预警阈值", m_lblMinStock);
    
    // 补货警告横幅 (内嵌于卡片，紧贴库存下方)
    m_restockBanner = new QWidget();
    m_restockBanner->setStyleSheet("background: #fef0f0; border: 1px solid #fde2e2; border-radius: 6px; padding: 8px 12px; margin-top: 5px;");
    QHBoxLayout *bannerLayout = new QHBoxLayout(m_restockBanner);
    bannerLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *bannerText = new QLabel("该商品可用库存已低于预警线，系统已自动加入今日补货待办！");
    bannerText->setStyleSheet("color: #f56c6c; font-weight: bold; font-size: 12px; border: none; background: transparent;");
    bannerLayout->addWidget(bannerText);
    bannerLayout->addStretch();
    m_restockBanner->hide();
    financeLayout->addWidget(m_restockBanner);
    
    scrollLayout->addWidget(financeCard);

    // 3. 销售兵器库 (The Hook)
    QWidget *salesCard = new QWidget();
    salesCard->setStyleSheet("background: white; border-radius: 8px; border: 1px solid #f0f2f5; padding: 12px;");
    auto addSalesRow = [&](QVBoxLayout *layout, QLabel* &valLabel, const QString &label) {
        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(8);
        QLabel *l = new QLabel(label + "："); 
        l->setStyleSheet("color: #909399; font-size: 12px; background: transparent; border: none; min-width: 65px;");
        valLabel = new QLabel("-");
        valLabel->setWordWrap(true);
        valLabel->setStyleSheet("color: #606266; font-size: 13px; background: transparent; border: none;");
        row->addWidget(l, 0, Qt::AlignTop);
        row->addWidget(valLabel, 1, Qt::AlignTop);
        layout->addLayout(row);
    };
    
    QVBoxLayout *salesLayout = new QVBoxLayout(salesCard);
    salesLayout->setSpacing(8);
    salesLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *salesTitle = new QLabel("销售话术辅助");
    salesTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #303133; margin-bottom: 5px; border: none; background: transparent;");
    salesLayout->addWidget(salesTitle);
    
    addSalesRow(salesLayout, m_lblSuitablePets, "适用对象");
    addSalesRow(salesLayout, m_lblPairingSuggestion, "推荐搭配");
    
    scrollLayout->addWidget(salesCard);

    // 4. 数字化质控 (The Reassurance)
    QWidget *qcCard = new QWidget();
    qcCard->setStyleSheet("background: white; border-radius: 8px; border: 1px solid #f0f2f5; padding: 12px;");
    QVBoxLayout *qcLayout = new QVBoxLayout(qcCard);
    qcLayout->setSpacing(10);
    qcLayout->setContentsMargins(0, 0, 0, 0);
    
    QHBoxLayout *qcHeaderLayout = new QHBoxLayout();
    QLabel *qcTitle = new QLabel("数字化质控");
    qcTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #303133; border: none; background: transparent;");
    m_lblHealthScore = new QLabel("A");
    m_lblHealthScore->setFixedSize(24, 24);
    m_lblHealthScore->setAlignment(Qt::AlignCenter);
    m_lblHealthScore->setStyleSheet("background: #67c23a; color: white; border-radius: 12px; font-weight: 900; font-size: 13px;");
    qcHeaderLayout->addWidget(qcTitle);
    qcHeaderLayout->addStretch();
    qcHeaderLayout->addWidget(m_lblHealthScore);
    qcLayout->addLayout(qcHeaderLayout);
    
    m_expiryBar = new QProgressBar();
    m_expiryBar->setFixedHeight(8);
    m_expiryBar->setTextVisible(false);
    m_expiryBar->setStyleSheet("QProgressBar { background: #f0f2f5; border-radius: 4px; border: none; } QProgressBar::chunk { border-radius: 4px; background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #a8edea, stop:1 #fed6e3); }");
    qcLayout->addWidget(m_expiryBar);

    QHBoxLayout *ed = new QHBoxLayout();
    m_lblDetailDate = new QLabel("生产：-"); m_lblDetailDate->setStyleSheet("color: #909399; font-size: 11px; border: none; background: transparent;");
    m_lblDetailExpiry = new QLabel("剩余：-"); m_lblDetailExpiry->setStyleSheet("font-weight: bold; font-size: 11px; border: none; background: transparent;");
    ed->addWidget(m_lblDetailDate); ed->addStretch(); ed->addWidget(m_lblDetailExpiry);
    qcLayout->addLayout(ed);
    
    m_lblDetailStorage = new QLabel("-");
    m_lblDetailStorage->setWordWrap(true);
    m_lblDetailStorage->setStyleSheet("color: #e6a23c; font-size: 12px; font-weight: bold; padding: 8px; background: #fdf6ec; border-radius: 6px; border: none;");
    qcLayout->addWidget(m_lblDetailStorage);
    scrollLayout->addWidget(qcCard);

    // 5. 基础规格参数 (The Details)
    QWidget *baseCard = new QWidget();
    baseCard->setStyleSheet("background: white; border-radius: 8px; border: 1px solid #f0f2f5; padding: 12px;");
    QVBoxLayout *baseInfoLayout = new QVBoxLayout(baseCard);
    baseInfoLayout->setSpacing(8);
    baseInfoLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *baseTitle = new QLabel("详细参数");
    baseTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #303133; margin-bottom: 5px; border: none; background: transparent;");
    baseInfoLayout->addWidget(baseTitle);
    
    auto addRow = [&](const QString &label, QLabel* &valLabel) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *l = new QLabel(label + "："); l->setStyleSheet("color: #909399; min-width: 60px; font-size: 12px; background: transparent; border: none;");
        valLabel = new QLabel("-"); valLabel->setStyleSheet("color: #606266; font-weight: 500; font-size: 12px; background: transparent; border: none;");
        valLabel->setWordWrap(true);
        row->addWidget(l); row->addWidget(valLabel, 1);
        baseInfoLayout->addLayout(row);
    };
    addRow("商品条码", m_lblDetailBarcode);
    addRow("产地信息", m_lblDetailOrigin);
    addRow("规格单位", m_lblDetailSpec);
    addRow("供货厂商", m_lblDetailSupplier);
    addRow("联系方式", m_lblDetailSupplierContact);
    scrollLayout->addWidget(baseCard);

    // 6. 原料组成与配方 (智能截断)
    m_nutritionContainer = new QWidget();
    m_nutritionContainer->setStyleSheet("background: white; border-radius: 8px; border: 1px solid #f0f2f5; padding: 12px;");
    QVBoxLayout *nutriLayout = new QVBoxLayout(m_nutritionContainer);
    nutriLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *nutriTitle = new QLabel("原料组成与配方");
    nutriTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #303133; margin-bottom: 5px; border: none; background: transparent;");
    nutriLayout->addWidget(nutriTitle);
    
    m_lblDetailIngredients = new QLabel("-");
    m_lblDetailIngredients->setWordWrap(true);
    m_lblDetailIngredients->setStyleSheet("color: #606266; font-size: 13px; line-height: 1.6; background: transparent; padding-top: 8px; border: none;");
    nutriLayout->addWidget(m_lblDetailIngredients);
    
    scrollLayout->addWidget(m_nutritionContainer);

    // ================= 底部：动态流转记录 =================
    // 流转记录卡片
    QWidget *flowCard = new QWidget();
    flowCard->setStyleSheet("background: white; border-radius: 8px; border: 1px solid #ebeef5; padding: 16px;");
    QVBoxLayout *flowLayout = new QVBoxLayout(flowCard);
    flowLayout->setContentsMargins(0, 0, 0, 0);
    flowLayout->setSpacing(10);
    
    QLabel *flowTitle = new QLabel("近期流转动态");
    flowTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #303133; border: none; background: transparent;");
    flowLayout->addWidget(flowTitle);
    
    m_flowRecordsLayout = new QVBoxLayout();
    m_flowRecordsLayout->setSpacing(8);
    flowLayout->addLayout(m_flowRecordsLayout);
    
    scrollLayout->addWidget(flowCard);

    m_detailScroll->setWidget(scrollContent);
    mainLayout->addWidget(m_detailScroll);
    
}

void ProductModule::openDrawer() {
    m_detailDrawer->show();
}

void ProductModule::closeDrawer() {
    m_detailDrawer->hide();
}

void ProductModule::updateDetailDrawer(const ProductInfo &info) {
    if (info.barcode.isEmpty()) return;

    // 1. 文本与基础信息更新
    m_lblDetailName->setText(info.name);
    m_lblDetailBrand->setText("品牌：" + info.brand);
    
    QString shortName = info.name.length() > 15 ? info.name.left(15) + "..." : info.name;
    m_lblDrawerHeaderTitle->setText(QString("%1 | 📦 库存: %2").arg(shortName).arg(info.stock));
    
    // 1.1 卖点标签动态生成 (决策外挂：一眼看清核心优势)
    QLayoutItem *tagItem;
    while ((tagItem = m_tagContainer->layout()->takeAt(0)) != nullptr) {
        if (tagItem->widget()) tagItem->widget()->deleteLater();
        delete tagItem;
    }
    
    for (const QString &tagText : info.tags) {
        QLabel *tag = new QLabel(tagText);
        QString style = "padding: 3px 10px; border-radius: 4px; font-size: 11px; font-weight: bold; ";
        if (tagText.contains("低敏") || tagText.contains("高肉")) {
            style += "background: #fdf6ec; color: #e6a23c; border: 1px solid #f5dab1;";
        } else if (tagText.contains("美毛") || tagText.contains("亮毛")) {
            style += "background: #f0f9eb; color: #67c23a; border: 1px solid #c2e7b0;";
        } else {
            style += "background: #ecf5ff; color: #409eff; border: 1px solid #b3d8ff;";
        }
        tag->setStyleSheet(style);
        m_tagContainer->layout()->addWidget(tag);
    }
    ((QHBoxLayout*)m_tagContainer->layout())->addStretch();
    m_lblDetailBarcode->setText(info.barcode);
    m_lblDetailOrigin->setText(info.origin);
    m_lblDetailSpec->setText(info.spec);
    m_lblDetailSupplier->setText(info.supplier);
    m_lblDetailSupplierContact->setText(info.supplierPhone.isEmpty() ? "-" : info.supplierPhone);
    m_lblDetailIngredients->setText(info.ingredients.isEmpty() ? "-" : info.ingredients);
    m_lblDetailStorage->setText("存储要求：" + info.storageReq);
    
    // 2. 经营与财务数据更新
    m_lblSalePrice->setText(QString("¥ %1").arg(info.price, 0, 'f', 2));
    if (m_role == ADMIN) {
        m_lblCostPrice->setText(QString("¥ %1").arg(info.costPrice, 0, 'f', 2));
        double margin = 0.0;
        if (info.price > 0) {
            margin = ((info.price - info.costPrice) / info.price) * 100.0;
        }
        m_lblGrossMargin->setText(QString("%1%").arg(margin, 0, 'f', 1));
    }
    m_lblCurrentStock->setText(QString::number(info.stock));
    m_lblMinStock->setText(QString::number(info.minStock));
    

    
    // 3. 销售建议更新
    m_lblSuitablePets->setText(info.suitablePets.isEmpty() ? "暂无建议" : info.suitablePets);
    m_lblPairingSuggestion->setText(info.pairingSuggestion.isEmpty() ? "暂无建议" : info.pairingSuggestion);

    // 4. 效期监控计算与健康度评价
    QDate prodDate = QDate::fromString(info.productionDate, "yyyy-MM-dd");
    QDate expiryDate = prodDate.addDays(info.shelfLifeDays);
    int totalDays = info.shelfLifeDays;
    int remainingDays = QDate::currentDate().daysTo(expiryDate);
    m_lblDetailDate->setText("生产日期：" + info.productionDate);

    if (remainingDays < 0) {
        m_lblDetailExpiry->setText("❗ 已过期");
        m_lblDetailExpiry->setStyleSheet("color: #f56c6c; font-weight: bold; font-size: 11px;");
        m_expiryBar->setValue(100);
        m_expiryBar->setStyleSheet("QProgressBar::chunk { background: #909399; }");
        m_lblHealthScore->setText("D");
        m_lblHealthScore->setStyleSheet("background: #909399; color: white; border-radius: 12px; font-weight: 900; font-size: 13px;");
    } else {
        m_lblDetailExpiry->setText(QString("剩余 %1 天").arg(remainingDays));
        int progress = ((totalDays - remainingDays) * 100) / totalDays;
        m_expiryBar->setValue(progress);
        
        double healthRatio = (double)remainingDays / totalDays;
        if (remainingDays < 30 || healthRatio < 0.1) {
            m_lblDetailExpiry->setStyleSheet("color: #f56c6c; font-weight: bold; font-size: 11px;");
            m_expiryBar->setStyleSheet("QProgressBar::chunk { background: #f56c6c; }");
            m_lblHealthScore->setText("C");
            m_lblHealthScore->setStyleSheet("background: #f56c6c; color: white; border-radius: 12px; font-weight: 900; font-size: 13px;");
        } else if (remainingDays < 90 || healthRatio < 0.3) {
            m_lblDetailExpiry->setStyleSheet("color: #e6a23c; font-weight: bold; font-size: 11px;");
            m_expiryBar->setStyleSheet("QProgressBar::chunk { background: #e6a23c; }");
            m_lblHealthScore->setText("B");
            m_lblHealthScore->setStyleSheet("background: #e6a23c; color: white; border-radius: 12px; font-weight: 900; font-size: 13px;");
        } else {
            m_lblDetailExpiry->setStyleSheet("color: #67c23a; font-weight: bold; font-size: 11px;");
            m_expiryBar->setStyleSheet("QProgressBar::chunk { background: #67c23a; }");
            m_lblHealthScore->setText("A");
            m_lblHealthScore->setStyleSheet("background: #67c23a; color: white; border-radius: 12px; font-weight: 900; font-size: 13px;");
        }
    }


    
    // 6. 流转记录动态更新
    // 清除旧的记录
    QLayoutItem *childItem;
    while ((childItem = m_flowRecordsLayout->takeAt(0)) != nullptr) {
        if (childItem->widget()) childItem->widget()->deleteLater();
        delete childItem;
    }
    
    // 控制补货横幅
    m_restockBanner->setVisible(info.stock <= info.minStock);
    
    // 找出最多3条该商品的入库记录
    int count = 0;
    for (int i = 0; i < m_records.size() && count < 3; ++i) {
        if (m_records[i].barcode == info.barcode) {
            QWidget *recWidget = new QWidget();
            recWidget->setStyleSheet("background: #fafbfc; border-radius: 6px; padding: 8px;");
            QHBoxLayout *rl = new QHBoxLayout(recWidget);
            rl->setContentsMargins(5, 5, 5, 5);
            
            QLabel *lblType = new QLabel("入库");
            lblType->setStyleSheet("color: #409eff; font-size: 12px; font-weight: bold; border: none; background: transparent;");
            
            QLabel *lblDate = new QLabel(m_records[i].dateTime);
            lblDate->setStyleSheet("color: #909399; font-size: 12px; border: none; background: transparent;");
            
            QLabel *lblQty = new QLabel(QString("+%1 %2").arg(m_records[i].quantity).arg(info.spec));
            lblQty->setStyleSheet("color: #67c23a; font-size: 13px; font-weight: bold; border: none; background: transparent;");
            
            rl->addWidget(lblType);
            rl->addWidget(lblDate);
            rl->addStretch();
            rl->addWidget(lblQty);
            
            m_flowRecordsLayout->addWidget(recWidget);
            count++;
        }
    }
    
    if (count == 0) {
        QLabel *emptyLbl = new QLabel("暂无近期流转记录");
        emptyLbl->setStyleSheet("color: #c0c4cc; font-size: 12px;");
        emptyLbl->setAlignment(Qt::AlignCenter);
        m_flowRecordsLayout->addWidget(emptyLbl);
    }

    // 7. 画廊主图与缩略图更新
    QLayoutItem *child;
    while ((child = m_thumbContainer->layout()->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    if (!info.images.isEmpty()) {
        m_mainPreview->setPixmap(QPixmap(info.images.first()).scaled(290, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_mainPreview->setProperty("imgPath", info.images.first());
        m_mainPreview->setProperty("imgList", info.images);
        m_mainPreview->setProperty("imgIndex", 0);
        int index = 0;
        for (const QString &path : info.images) {
            QPushButton *btn = new QPushButton();
            btn->setFixedSize(40, 40);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(QString("QPushButton { border: 1px solid #dcdfe6; border-radius: 4px; background-image: url(%1); background-position: center; background-repeat: no-repeat; }").arg(path));
            int capturedIndex = index;
            connect(btn, &QPushButton::clicked, this, [=](){
                m_mainPreview->setPixmap(QPixmap(path).scaled(290, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                m_mainPreview->setProperty("imgPath", path);
                m_mainPreview->setProperty("imgIndex", capturedIndex);
            });
            m_thumbContainer->layout()->addWidget(btn);
            index++;
        }
    } else {
        m_mainPreview->setText("暂无图片");
        m_mainPreview->setPixmap(QPixmap());
    }
}


