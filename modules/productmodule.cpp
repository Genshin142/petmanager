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
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include "custom_calendar_edit.h"
#include "productdatamanager.h"
#include "fostermodule.h"

// --- 复刻：全行圆角边框选中委托 ---
class ProductRowDelegate : public QStyledItemDelegate {
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

        // 绘制默认文本（针对非 CellWidget 列）
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

ProductModule::ProductModule(UserRole role, QWidget *parent) : QWidget(parent),
    prevBtn(nullptr), nextBtn(nullptr), pageLabel(nullptr), m_currentPage(1), m_pageSize(10),
    jumpEdit(nullptr), jumpValidator(nullptr), jumpBtn(nullptr),
    m_role(role), m_detailDrawer(nullptr), m_backdrop(nullptr), m_drawerAnim(nullptr)
{
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
            QStringList imgList = label->property("imgList").toStringList();
            int index = label->property("imgIndex").toInt();
            QString path = label->property("imgPath").toString();
            
            if (imgList.isEmpty()) imgList << path;
            
            // 使用标准化的全屏预览（支持滚轮切换）
            (new FullImagePreviewDialog(imgList, index, this->window()))->show();
            return true;
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

    // Tab 1: 档案看板
    QWidget *inventoryTab = new QWidget();
    QVBoxLayout *invLayout = new QVBoxLayout(inventoryTab);
    invLayout->setContentsMargins(15, 20, 15, 20);
    invLayout->setSpacing(20);

    // --- 原有的库存看板代码迁移过来 ---
    // 1. 顶部标题与快速搜索
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("商品档案管理中心");
    titleLabel->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold;");
    
    // --- 操作中控台 (Operation Console) ---
    QFrame *operationCard = new QFrame();
    operationCard->setFixedHeight(64);
    operationCard->setStyleSheet("QFrame { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QHBoxLayout *filterLayout = new QHBoxLayout(operationCard);
    filterLayout->setContentsMargins(15, 0, 15, 0);
    filterLayout->setSpacing(10);
    
    // -- 1. 搜索栏 (左侧) --
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索商品名称、条形码、分类...");
    searchEdit->setFixedWidth(260);
    searchEdit->setFixedHeight(36);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    connect(searchEdit, &QLineEdit::textChanged, this, &ProductModule::onSearchChanged);
    filterLayout->addWidget(searchEdit);
    filterLayout->addSpacing(10);

    // -- 2. 分类切换 (左侧) --
    m_categoryGroup = new QButtonGroup(this);
    m_categoryGroup->setExclusive(true);
    QStringList categories = {"全部", "宠物主食", "零食罐头", "清洁用品", "宠物玩具", "洗护医疗"};
    for (int i = 0; i < categories.size(); ++i) {
        QString cat = categories[i];
        QPushButton *btn = new QPushButton(cat);
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setFixedHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 18px; padding: 0 15px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );
        if (cat == "全部") btn->setChecked(true);
        m_categoryGroup->addButton(btn, i);
        connect(btn, &QPushButton::clicked, this, [=](){
            searchEdit->setText(cat == "全部" ? "" : cat);
        });
        filterLayout->addWidget(btn);
    }

    filterLayout->addStretch();

    // -- 3. 右侧：操作按钮 --
    QPushButton *listingBtn = new QPushButton("商品上架");
    listingBtn->setFixedHeight(36);
    listingBtn->setCursor(Qt::PointingHandCursor);
    listingBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 6px; font-size: 13px; border: none; padding: 0 20px; } "
        "QPushButton:hover { background: #66b1ff; } "
    );
    connect(listingBtn, &QPushButton::clicked, this, &ProductModule::onListing);
    if (m_role == UserRole::STAFF) listingBtn->setVisible(false);
    filterLayout->addWidget(listingBtn);



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

        if (!icon.isEmpty()) {
            cl->addWidget(iconLabel);
            cl->addSpacing(15);
        }
        cl->addLayout(vl);
        cl->addStretch();
        return card;
    };

    statLayout->addWidget(createStatCard("", "商品品种", varietyLabel, "#409eff"));
    statLayout->addWidget(createStatCard("", "库存预警", lowStockLabel, "#f56c6c"));
    statLayout->addWidget(createStatCard("", "总货值估算", totalValueLabel, "#67c23a"));
    
    // 3. 商品列表
    prodTable = new QTableWidget();
    prodTable->setColumnCount(9);
    prodTable->setHorizontalHeaderLabels({"图片", "条形码", "商品名称", "规格单位", "成本价", "销售价", "当前库存", "库存状态", "操作"});
    prodTable->setItemDelegate(new ProductRowDelegate(prodTable));
    
    prodTable->setShowGrid(false);
    prodTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    prodTable->setSelectionMode(QAbstractItemView::SingleSelection);
    prodTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    prodTable->setFocusPolicy(Qt::NoFocus);
    prodTable->verticalHeader()->setVisible(false);
    prodTable->verticalHeader()->setDefaultSectionSize(60); // 统一行高 60px，与订单管理对齐

    prodTable->setStyleSheet(
        "QTableWidget { border: none; background: white; outline: none; } "
        
    );
    connect(prodTable, &QTableWidget::cellDoubleClicked, this, &ProductModule::onShowBatchDetails);

    prodTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    prodTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
      // 选择
    prodTable->setColumnWidth(0, 80);  // 图片
    prodTable->setColumnWidth(1, 120); // 条形码
    prodTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch); // 商品名称拉伸
    prodTable->setColumnWidth(8, 100); // 操作按钮
    
    
    prodTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    prodTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    prodTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Fixed);
    
    // 隐藏/显示成本列
    if (m_role != UserRole::ADMIN) {
        prodTable->hideColumn(4);
    }

    // 4. 底部统计与分页
    QFrame *statFrame = new QFrame();
    statFrame->setFixedHeight(50);
    // 背景改为白色，移除 border
    statFrame->setStyleSheet("QFrame { background: white; border: none; padding: 0 12px; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(statFrame);

    prevBtn = new QPushButton("上一页");
    nextBtn = new QPushButton("下一页");
    pageLabel = new QLabel("第 1 页 / 共 1 页");

    QString pageStyle = "QPushButton { height: 28px; border: 1px solid #e2e8f0; border-radius: 6px; background: white; color: #64748b; font-size: 12px; padding: 0 12px; text-align: center; font-weight: bold; } "
                        "QPushButton:hover { border-color: #3b82f6; color: #3b82f6; background: #eff6ff; } "
                        "QPushButton:disabled { background: #f8fafc; color: #cbd5e1; border-color: #f1f5f9; }";
    prevBtn->setStyleSheet(pageStyle);
    nextBtn->setStyleSheet(pageStyle);

    prevBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setCursor(Qt::PointingHandCursor);
    pageLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold; margin: 0 10px;");

    QWidget *pageGroup = new QWidget();
    QHBoxLayout *pageLayout = new QHBoxLayout(pageGroup);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(5); 
    pageLayout->addWidget(prevBtn);
    pageLayout->addWidget(pageLabel);
    pageLayout->addWidget(nextBtn);

    footerLayout->addStretch(); // 关键：左侧弹簧推向右侧
    footerLayout->addWidget(pageGroup);
    footerLayout->addSpacing(15); 

    // 绑定分页逻辑
    connect(prevBtn, &QPushButton::clicked, this, &ProductModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &ProductModule::onNextPage);

    // 左侧：组装表格与分页
    QWidget *tableContainer = new QWidget();
    QVBoxLayout *tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(10);
    tableLayout->addWidget(prodTable);
    tableLayout->addWidget(statFrame);

    invLayout->addLayout(headerLayout);
    invLayout->addLayout(statLayout);
    invLayout->addWidget(operationCard);
    invLayout->addWidget(tableContainer);

    m_mainTabs->addTab(inventoryTab, "库存看板");

    // 设置全局详情抽屉
    setupDetailDrawer();
    m_detailDrawer->setFixedWidth(450); // 统一宽度为 450px

    globalMasterDetail->addWidget(m_mainTabs, 1);
    globalMasterDetail->addWidget(m_detailDrawer, 0);

    mainLayout->addLayout(globalMasterDetail);

    // 联动详情面板
    connect(m_mainTabs, &QTabWidget::currentChanged, this, [=](int index){
        Q_UNUSED(index);
        m_btnModifyInfo->setVisible(true);
        m_lblDrawerHeaderTitle->setText("商品详情");
    });

    // 绑定信号：行选中时刷新并滑出抽屉
    connect(prodTable, &QTableWidget::itemSelectionChanged, this, [=](){
        int row = prodTable->currentRow();
        if (row >= 0) {
            auto item1 = prodTable->item(row, 1);
            if (!item1) return;
            QString barcode = item1->text();
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


void ProductModule::addProductRow(const ProductInfo &info) {
    int row = prodTable->rowCount();
    prodTable->insertRow(row);
    
    QString imgPath = info.images.isEmpty() ? "E:/QT/work/PetManager/images/stores/default.png" : info.images.first();

    // 0. 选择勾选框

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
    prodTable->setCellWidget(row, 0, imgContainer);

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

    setItem(1, info.barcode);
    setItem(2, info.name, true);
    setItem(3, info.spec);
    setItem(4, QString("￥%1").arg(info.costPrice, 0, 'f', 2));
    setItem(5, QString("￥%1").arg(info.price, 0, 'f', 2));
    
    // 库存数值项 (第 7 列)
    QTableWidgetItem *stockItem = new QTableWidgetItem(QString::number(info.stock));
    stockItem->setTextAlignment(Qt::AlignCenter);
    stockItem->setData(Qt::UserRole, info.minStock); // 存储预警阈值
    if (info.stock <= info.minStock) stockItem->setForeground(QColor("#f56c6c"));
    prodTable->setItem(row, 6, stockItem);

    // 状态标签 (第 8 列)
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
    prodTable->setCellWidget(row, 7, tagContainer);

    // 9. 操作列 (编辑与下架按钮)
    QWidget *optContainer = new QWidget();
    optContainer->setStyleSheet("background: transparent;");
    QHBoxLayout *optLayout = new QHBoxLayout(optContainer);
    optLayout->setContentsMargins(0, 0, 15, 0); // 右边距 15px
    optLayout->setSpacing(0);
    optLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter); // 右对齐

    QPushButton *delBtn = new QPushButton(info.isActive ? "下架" : "已下架");
    delBtn->setFixedSize(50, 26);
    delBtn->setCursor(info.isActive ? Qt::PointingHandCursor : Qt::ArrowCursor);
    delBtn->setEnabled(info.isActive);
    delBtn->setStyleSheet(
        info.isActive ? 
        "QPushButton { background: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 3px; font-size: 11px; padding: 0; text-align: center; } "
        "QPushButton:hover { background: #f56c6c; color: white; }" :
        "QPushButton { background: #f5f7fa; color: #c0c4cc; border: 1px solid #e4e7ed; border-radius: 3px; font-size: 11px; padding: 0; text-align: center; }"
    );
    connect(delBtn, &QPushButton::clicked, this, &ProductModule::onDeleteProduct);

    optLayout->addWidget(delBtn);
    prodTable->setCellWidget(row, 8, optContainer);
    
    // 如果已下架，整行变灰
    if (!info.isActive) {
        for (int i = 0; i < prodTable->columnCount(); ++i) {
            QTableWidgetItem *it = prodTable->item(row, i);
            if (it) it->setForeground(QColor("#c0c4cc"));
        }
        tag->setText("已下架");
        tag->setStyleSheet(baseStyle + "background: #f4f4f5; color: #909399; border: 1px solid #e9e9eb;");
    }

    // 同步刷新分页（仅当控件已初始化）
    if (pageLabel) updatePagination();
}

void ProductModule::onListing() {
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

    QString barcode = prodTable->item(row, 1)->text();
    ProductInfo info = ProductDataManager::instance()->getProduct(barcode);
    if (info.barcode.isEmpty()) return;

    showProductEditDialog(info, false);
}

void ProductModule::showProductEditDialog(const ProductInfo &info, bool isNew) {
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(isNew ? "新商品上架登记" : "修改商品档案");
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
    
    QLabel *headerTitle = new QLabel(isNew ? "新商品上架登记" : "编辑商品详情");
    headerTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #1a1a1a;");
    
    QLabel *headerSub = new QLabel(isNew ? "请补全并确认新商品的上架档案信息" : "正在修改: " + info.name);
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
        lbl->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
        
        rowContainer->addWidget(lbl);
        rowContainer->addWidget(widget);
        
        if (!desc.isEmpty()) {
            QLabel *d = new QLabel(desc);
            d->setStyleSheet("font-size: 11px; color: #909399;");
            rowContainer->addWidget(d);
        }
        
        if (targetLayout) targetLayout->addWidget(container);
        else contentLayout->addWidget(container);
        
        // 如果是新建模式，隐藏已在入库阶段确定的物流信息
        if (isNew && (label == "商品名称" || label == "规格单位" || label == "进货成本" || 
                      label == "供货厂商" || label == "初始库存量" || label == "生产日期" || 
                      label == "产地/品牌信息" || label == "所属分类" || label == "保质期时长 (天)")) {
            container->hide();
        }
        
        return container;
    };

    QString editStyle = "QLineEdit, QDateEdit, QSpinBox, QDoubleSpinBox, QComboBox, QTextEdit { "
                       "background: white; border: 1px solid #dcdfe6; border-radius: 8px; padding: 8px 12px; color: #606266; font-size: 13px; } "
                       "QLineEdit:focus, QDateEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QTextEdit:focus { "
                       "border-color: #409eff; background: #fbfdff; } "
                       "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 30px; border: none; } "
                       "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
                       "QComboBox QAbstractItemView { border: 1px solid #e4e7ed; background: white; border-radius: 4px; selection-background-color: #f5f7fa; selection-color: #409eff; }";

    // 1. 核心识别 (新建时从下拉列表选，修改时只读)
    createSectionHeader("选择待上架商品资料");
    
    QComboBox *barcodeCombo = nullptr;
    QLineEdit *barcodeDisplay = nullptr;
    
    if (isNew) {
        barcodeCombo = new QComboBox();
        barcodeCombo->setStyleSheet(editStyle);
        barcodeCombo->addItem("-- 请选择待上架商品 --", "");
        QList<StockInRecord> unlisted = ProductDataManager::instance()->getUnlistedInboundItems();
        for (const auto &r : unlisted) {
            QString displayText = QString("%1 [%2] (生产日期: %3)")
                                   .arg(r.productName)
                                   .arg(r.barcode)
                                   .arg(r.productionDate);
            QString compositeKey = QString("%1|%2").arg(r.barcode).arg(r.productionDate);
            barcodeCombo->addItem(displayText, compositeKey);
        }
        createPremiumRow("待上架列表", barcodeCombo, "系统已为您筛选出所有已入库但未创建档案的条码。");
    } else {
        barcodeDisplay = new QLineEdit(info.barcode);
        barcodeDisplay->setEnabled(false);
        barcodeDisplay->setStyleSheet(editStyle + "background: #f5f7fa; color: #c0c4cc;");
        createPremiumRow("条形码 (不可更改)", barcodeDisplay);
    }

    // 2. 基础描述
    QWidget *basicHeader = createSectionHeader("基础信息");
    if (isNew) basicHeader->hide();
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
    QLabel *l1 = new QLabel("所属分类"); l1->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v1->addWidget(l1); v1->addWidget(categoryCombo);
    
    QVBoxLayout *v2 = new QVBoxLayout();
    QLineEdit *specEdit = new QLineEdit(info.spec);
    specEdit->setPlaceholderText("如：10kg/袋");
    specEdit->setStyleSheet(editStyle);
    QLabel *l2 = new QLabel("规格单位"); l2->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v2->addWidget(l2); v2->addWidget(specEdit);
    
    grid1->addLayout(v1);
    grid1->addLayout(v2);
    QWidget *grid1Container = new QWidget();
    grid1Container->setLayout(grid1);
    contentLayout->addWidget(grid1Container);
    if (isNew) grid1Container->hide();

    QLineEdit *originEdit = new QLineEdit(info.origin);
    originEdit->setStyleSheet(editStyle);
    createPremiumRow("产地/品牌信息", originEdit);

    QHBoxLayout *grid1_5 = new QHBoxLayout();
    grid1_5->setSpacing(20);
    
    QVBoxLayout *v1_5_a = new QVBoxLayout();
    QLineEdit *supplierEdit = new QLineEdit(info.supplier);
    supplierEdit->setPlaceholderText("请输入供货厂商名称...");
    supplierEdit->setStyleSheet(editStyle);
    QLabel *l1_5_a = new QLabel("供货厂商"); l1_5_a->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v1_5_a->addWidget(l1_5_a); v1_5_a->addWidget(supplierEdit);
    
    QVBoxLayout *v1_5_b = new QVBoxLayout();
    QLineEdit *supplierContactEdit = new QLineEdit(info.supplierPhone);
    supplierContactEdit->setPlaceholderText("请输入联系人或电话...");
    supplierContactEdit->setStyleSheet(editStyle);
    QLabel *l1_5_b = new QLabel("联系方式"); l1_5_b->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v1_5_b->addWidget(l1_5_b); v1_5_b->addWidget(supplierContactEdit);
    
    QWidget *supplierRow = createPremiumRow("供货商资料", new QWidget()); // 占位以获取容器
    supplierRow->layout()->setContentsMargins(0,0,0,0);
    QHBoxLayout *suppH = new QHBoxLayout();
    suppH->addLayout(v1_5_a);
    suppH->addLayout(v1_5_b);
    static_cast<QVBoxLayout*>(supplierRow->layout())->addLayout(suppH);
    if (isNew) supplierRow->hide(); // 新建时从入库带出，直接隐藏整个供货商区域

    // 3. 经营财务
    QWidget *financeHeader = createSectionHeader("经营与库存");
    if (isNew) financeHeader->hide();
    QHBoxLayout *grid2 = new QHBoxLayout();
    grid2->setSpacing(20);

    QVBoxLayout *v3 = new QVBoxLayout();
    QLineEdit *priceEdit = new QLineEdit(QString::number(info.price, 'f', 2));
    priceEdit->setPlaceholderText("0.00");
    priceEdit->setStyleSheet(editStyle);
    QLabel *l3 = new QLabel("销售单价"); l3->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v3->addWidget(l3); v3->addWidget(priceEdit);

    QVBoxLayout *v4 = new QVBoxLayout();
    QLineEdit *costEdit = new QLineEdit(QString::number(info.costPrice, 'f', 2));
    costEdit->setPlaceholderText("0.00");
    costEdit->setStyleSheet(editStyle);
    QLabel *l4 = new QLabel("进货成本"); l4->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v4->addWidget(l4); v4->addWidget(costEdit);

    grid2->addLayout(v3);
    if (!isNew) grid2->addLayout(v4); // 进货成本仅在编辑模式显示
    contentLayout->addLayout(grid2);

    QHBoxLayout *grid3 = new QHBoxLayout();
    grid3->setSpacing(20);

    QVBoxLayout *v5 = new QVBoxLayout();
    QLineEdit *stockEdit = new QLineEdit(QString::number(info.stock));
    stockEdit->setPlaceholderText("0");
    stockEdit->setStyleSheet(editStyle);
    QLabel *l5 = new QLabel(isNew ? "初始库存量" : "当前总库存"); l5->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v5->addWidget(l5); v5->addWidget(stockEdit);

    QVBoxLayout *v6 = new QVBoxLayout();
    QLineEdit *warningEdit = new QLineEdit(QString::number(info.minStock));
    warningEdit->setPlaceholderText("5");
    warningEdit->setStyleSheet(editStyle);
    QLabel *l6 = new QLabel("库存预警线"); l6->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v6->addWidget(l6); v6->addWidget(warningEdit);

    if (!isNew) grid3->addLayout(v5); // 初始库存仅在编辑模式显示
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
    QLabel *l7 = new QLabel("生产日期"); l7->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v7->addWidget(l7); v7->addWidget(prodDateEdit);

    QVBoxLayout *v8 = new QVBoxLayout();
    QLineEdit *shelfLifeEdit = new QLineEdit(QString::number(info.shelfLifeDays));
    shelfLifeEdit->setPlaceholderText("365");
    shelfLifeEdit->setStyleSheet(editStyle);
    QLabel *l8 = new QLabel("保质期时长 (天)"); l8->setStyleSheet("font-weight: bold; color: #606266; font-size: 13px;");
    v8->addWidget(l8); v8->addWidget(shelfLifeEdit);

    if (!isNew) {
        grid4->addLayout(v7);
        grid4->addLayout(v8);
        qualityLayout->addLayout(grid4);
    } else {
        l8->hide();
        shelfLifeEdit->hide();
        // 新建模式下，生产日期和保质期均已由入库单确定，故不将此行(grid4)加入布局，避免产生空白间距
    }

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
    QWidget *mediaHeader = createSectionHeader("媒体资料");
    if (isNew) mediaHeader->hide();
    QStringList *selectedImgPaths = new QStringList(info.images);
    QWidget *photoPanel = new QWidget();
    if (isNew) photoPanel->hide();
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

    // 联动逻辑 (由于涉及较多 Widget 引用，放在所有 Widget 初始化之后)
    if (isNew && barcodeCombo) {
        connect(barcodeCombo, &QComboBox::currentIndexChanged, this, [=](int index){
            QString compositeKey = barcodeCombo->itemData(index).toString();
            if (compositeKey.isEmpty()) {
                nameEdit->clear();
                // ... (existing clear logic)
                return;
            }
            
            QStringList parts = compositeKey.split("|");
            QString barcode = parts.at(0);
            QString prodDate = parts.size() > 1 ? parts.at(1) : "";
            
            // 查找对应的入库记录 (精准匹配日期)
            QList<StockInRecord> recs = ProductDataManager::instance()->getAllRecords();
            for (const auto &r : recs) {
                if (r.barcode == barcode && r.productionDate == prodDate) {
                    nameEdit->setText(r.productName);
                    nameEdit->setEnabled(false);
                    nameEdit->setStyleSheet(editStyle + "background: #f5f7fa; color: #909399;");
                    
                    costEdit->setText(QString::number(r.costPrice, 'f', 2));
                    costEdit->setEnabled(false);
                    costEdit->setStyleSheet(editStyle + "background: #f5f7fa; color: #909399;");
                    
                    supplierEdit->setText(r.supplier);
                    supplierEdit->setEnabled(false);
                    supplierEdit->setStyleSheet(editStyle + "background: #f5f7fa; color: #909399;");
                    
                    stockEdit->setText(QString::number(r.quantity));
                    stockEdit->setEnabled(false);
                    stockEdit->setStyleSheet(editStyle + "background: #f5f7fa; color: #909399;");
                    
                    prodDateEdit->setText(r.productionDate);
                    prodDateEdit->setEnabled(false);
                    prodDateEdit->setStyleSheet(editStyle + "background: #f5f7fa; color: #909399;");
                    
                    specEdit->setText(r.spec);
                    specEdit->setEnabled(false);
                    specEdit->setStyleSheet(editStyle + "background: #f5f7fa; color: #909399;");

                    shelfLifeEdit->setText(QString::number(r.shelfLifeDays));
                    shelfLifeEdit->setEnabled(false);
                    shelfLifeEdit->setStyleSheet(editStyle + "background: #f5f7fa; color: #909399;");

                    *selectedImgPaths = r.imgPaths; // 同步入库时拍摄的照片

                    // 触发动态字段更新
                    updateVisibility(r.category);
                    break;
                }
            }
        });
    }

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
    
    QPushButton *saveBtn = new QPushButton(isNew ? "确认并立即上架" : "同步资料");
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
        nInfo.name = nameEdit->text().trimmed();
        if (isNew) {
            if (!barcodeCombo || barcodeCombo->currentData().toString().isEmpty()) {
                QMessageBox::warning(dialog, "提示", "请先从下拉列表中选择一个待上架的条码");
                return;
            }
            nInfo.barcode = barcodeCombo->currentData().toString();
        } else {
            nInfo.barcode = barcodeDisplay->text().trimmed();
        }

        if (nInfo.name.isEmpty()) {
            QMessageBox::warning(dialog, "提示", "商品名称不能为空");
            return;
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
        nInfo.isActive = true;

        if (isNew) {
            // 从选中的入库记录补全被隐藏的信息 (精准匹配)
            QString compositeKey = barcodeCombo->currentData().toString();
            QStringList parts = compositeKey.split("|");
            QString barcode = parts.at(0);
            QString prodDate = parts.size() > 1 ? parts.at(1) : "";

            QList<StockInRecord> recs = ProductDataManager::instance()->getAllRecords();
            for (const auto &r : recs) {
                if (r.barcode == barcode && r.productionDate == prodDate) {
                    nInfo.barcode = r.barcode;
                    nInfo.name = r.productName;
                    nInfo.spec = r.spec;
                    nInfo.costPrice = r.costPrice;
                    nInfo.supplier = r.supplier;
                    nInfo.stock = r.quantity;
                    nInfo.productionDate = r.productionDate;
                    nInfo.origin = r.origin;
                    nInfo.category = r.category;
                    nInfo.shelfLifeDays = r.shelfLifeDays;
                    break;
                }
            }
            ProductDataManager::instance()->addProduct(nInfo);
            ProductDataManager::instance()->markRecordAsShelved(nInfo.barcode, nInfo.productionDate);
            addProductRow(nInfo);
        } else {
            ProductDataManager::instance()->addProduct(nInfo);
            // 更新现有行
            updatePagination(); 
            updateDetailDrawer(nInfo);
        }

        updateStats();
        QMessageBox::information(dialog, "成功", isNew ? "商品档案已成功创建并上架！" : "商品档案已同步更新！");
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
        QTableWidgetItem *priceItem = prodTable->item(i, 5);
        QTableWidgetItem *stockItem = prodTable->item(i, 6);
        
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

void ProductModule::onNextPage()
{
    int total = prodTable->rowCount();
    QString kw = searchEdit->text().trimmed().toLower();
    int visibleCount = 0;
    for (int i = 0; i < total; ++i) {
        bool match = false;
        if (kw.isEmpty()) match = true;
        else {
            for (int col : {1, 2, 3}) { // 条码、名称、规格偏移
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

void ProductModule::onJumpPage()
{
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
            for (int col : {1, 2, 3}) { // 搜索列偏移
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
    
    // if (jumpValidator) jumpValidator->setTop(totalPages);
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
    
    // 自动选中第一行
    if (totalVisible > 0) {
        prodTable->selectRow(visibleRows[start]);
    } else {
        prodTable->clearSelection();
    }

    if (totalVisible == 0) {
        prevBtn->setEnabled(false);
        nextBtn->setEnabled(false);
        pageLabel->setText("第 0 页 / 共 0 页");
    }
}

void ProductModule::onDeleteProduct() {
    int row = prodTable->currentRow();
    if (row < 0) return;

    auto item1 = prodTable->item(row, 1);
    if (!item1) return;
    QString barcode = item1->text();
    ProductInfo info = ProductDataManager::instance()->getProduct(barcode);
    if (info.barcode.isEmpty()) return;

    if (!info.isActive) return;

    if (QMessageBox::question(this, "确认下架", 
                              QString("确定要下架商品 [%1] 吗？\n下架后该商品将无法进行前台销售，但档案资料仍将保留。").arg(info.name),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        info.isActive = false;
        ProductDataManager::instance()->updateProduct(info);
        
        // 刷新列表显示
        // 这里为了简单，我们直接重设整行样式，或者刷新表格
        updatePagination(); 
        updateStats();
        QMessageBox::information(this, "已下架", "商品已成功下架。");
    }
}


int ProductModule::getLowStockCount() const
{
    int count = 0;
    for (int i = 0; i < prodTable->rowCount(); ++i) {
        QTableWidgetItem *stockItem = prodTable->item(i, 6); // 库存数值项在第 7 列
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
    
    m_lblDrawerHeaderTitle = new QLabel("商品详情");
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
    m_lblDetailName->setStyleSheet("font-size: 22px; font-weight: bold; color: #303133;");
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
    m_lblHealthScore->setStyleSheet("background: #67c23a; color: white; border-radius: 12px; font-weight: bold; font-size: 13px;");
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
        valLabel = new QLabel("-"); valLabel->setStyleSheet("color: #606266; font-size: 12px; background: transparent; border: none;");
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
    m_lblDrawerHeaderTitle->setText(QString("%1 | 库存: %2").arg(shortName).arg(info.stock));
    
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
        m_lblHealthScore->setStyleSheet("background: #909399; color: white; border-radius: 12px; font-weight: bold; font-size: 13px;");
    } else {
        m_lblDetailExpiry->setText(QString("剩余 %1 天").arg(remainingDays));
        int progress = ((totalDays - remainingDays) * 100) / totalDays;
        m_expiryBar->setValue(progress);
        
        double healthRatio = (double)remainingDays / totalDays;
        if (remainingDays < 30 || healthRatio < 0.1) {
            m_lblDetailExpiry->setStyleSheet("color: #f56c6c; font-weight: bold; font-size: 11px;");
            m_expiryBar->setStyleSheet("QProgressBar::chunk { background: #f56c6c; }");
            m_lblHealthScore->setText("C");
            m_lblHealthScore->setStyleSheet("background: #f56c6c; color: white; border-radius: 12px; font-weight: bold; font-size: 13px;");
        } else if (remainingDays < 90 || healthRatio < 0.3) {
            m_lblDetailExpiry->setStyleSheet("color: #e6a23c; font-weight: bold; font-size: 11px;");
            m_expiryBar->setStyleSheet("QProgressBar::chunk { background: #e6a23c; }");
            m_lblHealthScore->setText("B");
            m_lblHealthScore->setStyleSheet("background: #e6a23c; color: white; border-radius: 12px; font-weight: bold; font-size: 13px;");
        } else {
            m_lblDetailExpiry->setStyleSheet("color: #67c23a; font-weight: bold; font-size: 11px;");
            m_expiryBar->setStyleSheet("QProgressBar::chunk { background: #67c23a; }");
            m_lblHealthScore->setText("A");
            m_lblHealthScore->setStyleSheet("background: #67c23a; color: white; border-radius: 12px; font-weight: bold; font-size: 13px;");
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
    QList<StockInRecord> allRecs = ProductDataManager::instance()->getAllRecords();
    for (int i = 0; i < allRecs.size() && count < 3; ++i) {
        if (allRecs[i].barcode == info.barcode) {
            QWidget *recWidget = new QWidget();
            recWidget->setStyleSheet("background: #fafbfc; border-radius: 6px; padding: 8px;");
            QHBoxLayout *rl = new QHBoxLayout(recWidget);
            rl->setContentsMargins(5, 5, 5, 5);
            
            QLabel *lblType = new QLabel("入库");
            lblType->setStyleSheet("color: #409eff; font-size: 12px; font-weight: bold; border: none; background: transparent;");
            
            QLabel *lblDate = new QLabel(allRecs[i].dateTime);
            lblDate->setStyleSheet("color: #909399; font-size: 12px; border: none; background: transparent;");
            
            QLabel *lblQty = new QLabel(QString("+%1 %2").arg(allRecs[i].quantity).arg(info.spec));
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

void ProductModule::onShowBatchDetails(int row, int col) {
    Q_UNUSED(col);
    if (row < 0) return;
    QString barcode = prodTable->item(row, 1)->text();
    ProductInfo pInfo = ProductDataManager::instance()->getProduct(barcode);
    QList<StockBatch> batches = ProductDataManager::instance()->getBatchesForProduct(barcode);

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QString("库存批次详情 - %1").arg(pInfo.name));
    dialog->resize(800, 450);
    dialog->setStyleSheet("QDialog { background: white; }");

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // 头部汇总
    QLabel *summary = new QLabel(QString("商品：%1  |  条码：%2  |  当前总库存：%3 %4")
                                 .arg(pInfo.name).arg(pInfo.barcode).arg(pInfo.stock).arg(pInfo.spec));
    summary->setStyleSheet("font-size: 15px; font-weight: bold; color: #303133; padding: 10px; background: #f5f7fa; border-radius: 8px;");
    layout->addWidget(summary);

    QTableWidget *batchTable = new QTableWidget();
    batchTable->setColumnCount(7);
    batchTable->setHorizontalHeaderLabels({"批次ID", "生产日期", "保质期", "到期日期", "入库量", "剩余量", "状态"});
    batchTable->verticalHeader()->setVisible(false);
    batchTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    batchTable->setSelectionMode(QAbstractItemView::NoSelection);
    batchTable->setShowGrid(false);
    batchTable->setStyleSheet("QTableWidget { border: none; }");
    batchTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QDate today = QDate::currentDate();

    for (const auto &b : batches) {
        int r = batchTable->rowCount();
        batchTable->insertRow(r);
        
        batchTable->setItem(r, 0, new QTableWidgetItem(b.batchId));
        batchTable->setItem(r, 1, new QTableWidgetItem(b.productionDate));
        batchTable->setItem(r, 2, new QTableWidgetItem(QString("%1天").arg(b.shelfLifeDays)));
        batchTable->setItem(r, 3, new QTableWidgetItem(b.expiryDate));
        batchTable->setItem(r, 4, new QTableWidgetItem(QString::number(b.initialQty)));
        batchTable->setItem(r, 5, new QTableWidgetItem(QString::number(b.currentQty)));

        // 状态与预警逻辑
        QDate expiryDate = QDate::fromString(b.expiryDate, "yyyy-MM-dd");
        qint64 daysToExpiry = today.daysTo(expiryDate);

        QLabel *statusLabel = new QLabel();
        statusLabel->setAlignment(Qt::AlignCenter);
        QString statusStyle = "border-radius: 4px; font-size: 11px; margin: 4px; padding: 2px;";

        if (daysToExpiry < 0) {
            statusLabel->setText("已过期");
            statusLabel->setStyleSheet(statusStyle + "background: #fef0f0; color: #f56c6c;");
            // 整行标红
            for(int i=0; i<6; ++i) {
                if (batchTable->item(r, i)) batchTable->item(r, i)->setForeground(QColor("#f56c6c"));
            }
        } else if (daysToExpiry <= 30) {
            statusLabel->setText(QString("预警 (剩%1天)").arg(daysToExpiry));
            statusLabel->setStyleSheet(statusStyle + "background: #fff7e6; color: #fa8c16;");
            for(int i=0; i<6; ++i) {
                if (batchTable->item(r, i)) batchTable->item(r, i)->setForeground(QColor("#fa8c16"));
            }
        } else {
            statusLabel->setText("正常");
            statusLabel->setStyleSheet(statusStyle + "background: #f0f9eb; color: #67c23a;");
        }
        batchTable->setCellWidget(r, 6, statusLabel);
    }

    layout->addWidget(batchTable);

    QPushButton *closeBtn = new QPushButton("关闭");
    closeBtn->setFixedWidth(100);
    closeBtn->setFixedHeight(35);
    closeBtn->setStyleSheet("QPushButton { background: #409eff; color: white; border-radius: 4px; }");
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    dialog->exec();
}


