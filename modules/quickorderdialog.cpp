#include "quickorderdialog.h"
#include "productdatamanager.h"
#include "servicedatamanager.h"
#include "petdatamanager.h"
#include "memberdatamanager.h"
#include "logisticsmanager.h"
#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QMessageBox>
#include <QHeaderView>
#include <QButtonGroup>
#include <QComboBox>
#include <QGridLayout>
#include <QTableWidget>
#include <QScrollArea>
#include <QPainter>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <functional>
#include <QShowEvent>
#include <QRandomGenerator>
#include <QDebug>

// --- ItemTile 实现 ---
ItemTile::ItemTile(const QString &id, const QString &name, double price, const QString &imgData, bool isService, const QString &category, QWidget *parent)
    : QFrame(parent), m_id(id), m_category(category), m_isService(isService), m_isSelected(false), m_qty(0) {
    setFixedSize(145, 190);
    setCursor(Qt::PointingHandCursor);
    setObjectName("ItemTile");
    updateStyle();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    QLabel *imgLabel = new QLabel();
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setFixedHeight(110);
    imgLabel->setStyleSheet("background: #f8fafc; border-radius: 8px; border: none;");
    
    if (!imgData.isEmpty()) {
        QPixmap pix;
        if (pix.loadFromData(QByteArray::fromBase64(imgData.toUtf8()))) {
            imgLabel->setPixmap(pix.scaled(130, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            imgLabel->setText(isService ? "🛁" : "📦");
            imgLabel->setStyleSheet(imgLabel->styleSheet() + "font-size: 32px;");
        }
    } else {
        imgLabel->setText(isService ? "🛁" : "📦");
        imgLabel->setStyleSheet(imgLabel->styleSheet() + "font-size: 32px;");
    }
    layout->addWidget(imgLabel);

    QLabel *nameLabel = new QLabel(name);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #1e293b; border: none; ");
    nameLabel->setWordWrap(true);
    nameLabel->setMaximumHeight(35);
    layout->addWidget(nameLabel);

    QLabel *priceLabel = new QLabel(QString("¥%1").arg(price, 0, 'f', 2));
    priceLabel->setAlignment(Qt::AlignCenter);
    priceLabel->setStyleSheet("font-size: 13px; color: #3b82f6; font-weight: 800; border: none; ");
    layout->addWidget(priceLabel);

    m_qtyBadge = new QLabel(this);
    m_qtyBadge->setAlignment(Qt::AlignCenter);
    m_qtyBadge->setStyleSheet("background: #ef4444; color: white; border-radius: 9px; font-size: 10px; font-weight: bold; padding: 2px;");
    m_qtyBadge->setFixedSize(18, 18);
    m_qtyBadge->move(120, 165);
    m_qtyBadge->hide();
}

void ItemTile::setQuantity(int qty) {
    m_qty = qty;
    m_isSelected = (qty > 0);
    if (m_qty > 1 && m_category != "寄养" && m_category != "接送") {
        m_qtyBadge->setText(QString("×%1").arg(m_qty));
        m_qtyBadge->show();
    } else {
        m_qtyBadge->hide();
    }
    updateStyle();
}

void ItemTile::mousePressEvent(QMouseEvent *event) {
    QFrame::mousePressEvent(event);
    if (m_callback) m_callback(m_id, m_isService);
}

void ItemTile::updateStyle() {
    if (m_isSelected) {
        setStyleSheet("#ItemTile { background: #eff6ff; border: 2px solid #3b82f6; border-radius: 10px; } ");
    } else {
        setStyleSheet("#ItemTile { background: white; border: 1px solid #e2e8f0; border-radius: 10px; } #ItemTile:hover { border-color: #3b82f6; background: #f0f7ff; }");
    }
}

// --- QuickOrderDialog 实现 ---
QuickOrderDialog::QuickOrderDialog(QWidget *parent) : QDialog(parent)
{
    setupUI();
    m_currentCategory = "全部";

    // 1. 监听数据变化，自动刷新界面
    connect(ProductDataManager::instance(), &ProductDataManager::productDataChanged, this, [this](){
        updateTilePanel(m_currentCategory, this->m_searchEdit->text());
    });

    // 2. 初始请求数据
    ProductDataManager::instance()->requestProductList();

    updateTilePanel(m_currentCategory);
}

void QuickOrderDialog::setupUI()
{
    setWindowTitle("快速开单 (POS)");
    resize(1150, 850); 
    
    QString globalStyle = R"(
        QDialog { background-color: white; }
        QLabel { border: none; background: transparent; }
        QScrollBar:vertical { border: none; background: #fafafa; width: 8px; margin: 0px; border-radius: 4px; }
        QScrollBar::handle:vertical { background: #e2e8f0; min-height: 20px; border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: #cbd5e1; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        
        QLineEdit, QComboBox, CustomCalendarEdit, #ScrollContent QLineEdit, #ScrollContent QComboBox { 
            border: 1px solid #94a3b8; 
            border-radius: 6px; 
            padding: 0 8px; 
            background: white; 
            color: #1e293b; 
            min-height: 32px;
        }
        QLineEdit:focus, QComboBox:hover, CustomCalendarEdit:focus { 
            border: 1px solid #3b82f6; 
        }
        
        QComboBox::drop-down { border: none; width: 20px; }
        QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 10px; height: 10px; }
        QComboBox QAbstractItemView { 
            border: 1px solid #cbd5e1; 
            border-radius: 8px; 
            background-color: white; 
            selection-background-color: #f1f5f9; 
            selection-color: #3b82f6; 
            outline: none; 
            padding: 4px;
        }
        
        QTableWidget { border: none; gridline-color: #f1f5f9; background: white; outline: none; }
        QTableWidget::item { border-bottom: 1px solid #f1f5f9; padding: 5px; }
        QHeaderView::section { background: #f8fafc; border: none; border-bottom: 1px solid #e2e8f0; color: #64748b; font-weight: bold; padding: 6px; }

        #BottomCard { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }
    )";
    setStyleSheet(globalStyle);
    
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(15);

    // --- 1. 顶部：分类导航 ---
    QFrame *catCard = new QFrame();
    catCard->setStyleSheet("QFrame { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    catCard->setFixedHeight(60);
    QHBoxLayout *catLayout = new QHBoxLayout(catCard);
    catLayout->setContentsMargins(15, 0, 15, 0);
    catLayout->setSpacing(10);

    m_categoryGroup = new QButtonGroup(this);
    QStringList cats = {"全部", "零食", "用品", "主粮"};
    for (int i = 0; i < cats.size(); ++i) {
        QPushButton *btn = new QPushButton(cats[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: #f8fafc; border: none; border-radius: 10px; padding: 0 18px; font-size: 13px; color: #64748b; font-weight: 500; } "
            "QPushButton:hover { background: #e2e8f0; color: #1e293b; } "
            "QPushButton:checked { background: #3b82f6; color: white; font-weight: bold; } "
        );
        if (i == 0) btn->setChecked(true);
        m_categoryGroup->addButton(btn, i);
        catLayout->addWidget(btn);
    }
    catLayout->addStretch();

    // -- 搜索项 --
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(" 🔍 搜索名称或条码...");
    m_searchEdit->setFixedWidth(240);
    m_searchEdit->setFixedHeight(36);
    m_searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 18px; padding: 0 15px; font-size: 13px; background: #f8fafc; } "
        "QLineEdit:focus { border-color: #3b82f6; background: white; }"
    );
    catLayout->addWidget(m_searchEdit);
    rootLayout->addWidget(catCard);

    // --- 2. 中间：商品网格 (滚动区) ---
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");
    
    m_tileContainer = new QWidget();
    m_tileLayout = new QGridLayout(m_tileContainer);
    m_tileLayout->setContentsMargins(0, 10, 0, 10);
    m_tileLayout->setSpacing(12);
    m_tileLayout->setAlignment(Qt::AlignTop);
    
    scroll->setWidget(m_tileContainer);
    rootLayout->addWidget(scroll, 1);

    // --- 3. 底部：清单与结算区 ---
    QFrame *bottomCard = new QFrame();
    bottomCard->setObjectName("BottomCard");
    bottomCard->setFixedHeight(360);
    bottomCard->setStyleSheet("QFrame#BottomCard { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomCard);
    bottomLayout->setContentsMargins(20, 20, 20, 20);
    bottomLayout->setSpacing(25);

    // 3.1 底部左侧：已选清单表格
    QVBoxLayout *listContainer = new QVBoxLayout();
    QLabel *listTitle = new QLabel("已选清单");
    listTitle->setStyleSheet("font-weight: bold; color: #1e293b; font-size: 15px; border: none; margin-bottom: 5px;");
    listContainer->addWidget(listTitle);

    m_cartTable = new QTableWidget();
    m_cartTable->setColumnCount(4);
    m_cartTable->setHorizontalHeaderLabels({"项目名称", "单价", "数量", "操作"});
    m_cartTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_cartTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_cartTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_cartTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_cartTable->setColumnWidth(1, 110);
    m_cartTable->setColumnWidth(2, 90);
    m_cartTable->setColumnWidth(3, 110);
    m_cartTable->verticalHeader()->setVisible(false);
    m_cartTable->verticalHeader()->setDefaultSectionSize(48);
    m_cartTable->setShowGrid(false);
    m_cartTable->setFocusPolicy(Qt::NoFocus);
    listContainer->addWidget(m_cartTable);
    bottomLayout->addLayout(listContainer, 3);

    // 3.2 底部右侧：结算区
    QVBoxLayout *settleContainer = new QVBoxLayout();
    settleContainer->setSpacing(15);

    QLabel *memberTitle = new QLabel("会员信息");
    memberTitle->setStyleSheet("font-weight: bold; color: #1e293b; font-size: 14px; border: none;");
    settleContainer->addWidget(memberTitle);

    m_memberCombo = new QComboBox();
    m_memberCombo->setFixedHeight(42);
    settleContainer->addWidget(m_memberCombo);

    m_petCombo = new QComboBox();
    m_petCombo->setFixedHeight(42);
    m_petCombo->hide(); 
    settleContainer->addWidget(m_petCombo);

    m_serviceDetailContainer = new QWidget();
    m_serviceDetailContainer->hide();
    settleContainer->addWidget(m_serviceDetailContainer);

    settleContainer->addStretch();

    QHBoxLayout *totalLine = new QHBoxLayout();
    QLabel *totalLabel = new QLabel("合计金额");
    totalLabel->setStyleSheet("color: #64748b; font-size: 14px; font-weight: bold; border: none;");
    totalLine->addWidget(totalLabel);
    m_totalLabel = new QLabel("¥ 0.00");
    m_totalLabel->setStyleSheet("color: #3b82f6; font-size: 28px; font-weight: 900; border: none;");
    totalLine->addStretch();
    totalLine->addWidget(m_totalLabel);
    settleContainer->addLayout(totalLine);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setFixedHeight(44);
    cancelBtn->setStyleSheet("QPushButton { background: white; color: #64748b; border: 1px solid #e2e8f0; border-radius: 8px; font-weight: bold; } QPushButton:hover { background: #f8fafc; }");
    
    QPushButton *okBtn = new QPushButton("确认下单");
    okBtn->setFixedHeight(44);
    okBtn->setStyleSheet("QPushButton { background: #3b82f6; color: white; border: none; border-radius: 8px; font-weight: bold; } QPushButton:hover { background: #2563eb; }");
    
    btnLayout->addWidget(cancelBtn, 1);
    btnLayout->addWidget(okBtn, 2);
    settleContainer->addLayout(btnLayout);

    bottomLayout->addLayout(settleContainer, 2);
    rootLayout->addWidget(bottomCard);

    connect(m_categoryGroup, &QButtonGroup::idClicked, this, &QuickOrderDialog::onCategoryChanged);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &QuickOrderDialog::onSearchItems);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, this, &QuickOrderDialog::onCreateOrder);
    connect(m_memberCombo, &QComboBox::currentIndexChanged, this, &QuickOrderDialog::onMemberChanged);

    initMemberData();
}

void QuickOrderDialog::initMemberData()
{
    m_memberCombo->clear();
    m_memberCombo->addItem("临时客 (散客)", "");
    for (const auto &m : MemberDataManager::instance()->activeMembers()) {
        m_memberCombo->addItem(QString("%1 (%2)").arg(m.name, m.id), m.id);
    }
}

void QuickOrderDialog::onMemberChanged(int index)
{
    QString memberId = m_memberCombo->itemData(index).toString();
    m_petCombo->clear();
    if (memberId.isEmpty()) { m_petCombo->addItem("无宠物", ""); m_petCombo->hide(); return; }
    auto pets = PetDataManager::instance()->getPetsByOwner(memberId);
    if (pets.isEmpty()) { m_petCombo->addItem("暂无关联宠物", ""); }
    else { 
        for (const auto &p : pets) { 
            QString display = QString("%1 (%2) - %3").arg(p.name, p.id, p.breed); 
            m_petCombo->addItem(display, p.id); 
        } 
    }
    m_petCombo->show();
}

void QuickOrderDialog::onCategoryChanged(int id)
{
    m_currentCategory = m_categoryGroup->button(id)->text();
    updateTilePanel(m_currentCategory, m_searchEdit->text());
}

void QuickOrderDialog::updateTilePanel(const QString &category, const QString &searchKw)
{
    QLayoutItem *child;
    while ((child = m_tileLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    
    int row = 0, col = 0;
    int maxCols = 7;

    // 1. 商品零售 (快速开单功能)
    for (const auto &p : ProductDataManager::instance()->allProducts()) {
        if (!p.isActive) continue;
        if (category != "全部" && p.category != category) continue;
        if (!searchKw.isEmpty() && !p.name.contains(searchKw) && !p.barcode.contains(searchKw)) continue;
        
        ItemTile *tile = new ItemTile(p.barcode, p.name, p.price, p.imgData, false, p.category, m_tileContainer);
        tile->clickedSignal([this](QString id, bool svc) { onTileClicked(id, svc); });
        
        for (const auto &item : m_cart) { 
            if (item.id == p.barcode && !item.isService) { 
                tile->setQuantity(item.qty); 
                break; 
            } 
        }
        m_tileLayout->addWidget(tile, row, col);
        col++; if (col >= maxCols) { col = 0; row++; }
    }
}

void QuickOrderDialog::onTileClicked(const QString &id, bool isService)
{
    for (int i = 0; i < m_cart.size(); ++i) { 
        if (m_cart[i].id == id && m_cart[i].isService == isService) { 
            m_cart[i].qty++; 
            updateCartUI(); 
            return; 
        } 
    }

    CartItem item; 
    item.id = id; 
    item.isService = isService; 
    item.qty = 1;

    if (isService) {
        auto s = ServiceDataManager::instance()->getService(id);
        if (s.id.isEmpty()) return;
        item.name = s.name; item.price = s.price; item.category = s.category;
    } else {
        auto p = ProductDataManager::instance()->getProduct(id);
        if (p.barcode.isEmpty()) return;
        item.name = p.name; item.price = p.price; item.category = p.category;
    }

    m_cart.append(item);
    updateCartUI();
}

void QuickOrderDialog::updateCartUI()
{
    m_cartTable->setRowCount(0); 
    double total = 0; 

    for (int i = 0; i < m_cart.size(); ++i) {
        const auto &item = m_cart[i]; 
        int row = m_cartTable->rowCount(); 
        m_cartTable->insertRow(row);
        
        QTableWidgetItem *nameItem = new QTableWidgetItem(item.name); 
        nameItem->setTextAlignment(Qt::AlignCenter); 
        m_cartTable->setItem(row, 0, nameItem);
        
        QTableWidgetItem *priceItem = new QTableWidgetItem(QString("¥%1").arg(item.price, 0, 'f', 2)); 
        priceItem->setTextAlignment(Qt::AlignCenter); 
        m_cartTable->setItem(row, 1, priceItem);
        
        QTableWidgetItem *qtyItem = new QTableWidgetItem(QString::number(item.qty)); 
        qtyItem->setTextAlignment(Qt::AlignCenter); 
        m_cartTable->setItem(row, 2, qtyItem);
        
        QPushButton *delBtn = new QPushButton("移除");
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setFixedSize(70, 30);
        delBtn->setStyleSheet("QPushButton { "
                             "  background: #fef2f2; "
                             "  color: #ef4444; "
                             "  border: 1px solid #fee2e2; "
                             "  border-radius: 6px; "
                             "  font-size: 11px; "
                             "  font-weight: bold; "
                             "  padding: 0px; "
                             "  text-align: center; "
                             "} "
                             "QPushButton:hover { "
                             "  background: #ef4444; "
                             "  color: white; "
                             "}");
        
        connect(delBtn, &QPushButton::clicked, this, [this, i]() { onRemoveCartItem(i); });
        
        QWidget *container = new QWidget();
        container->setStyleSheet("background: transparent; border: none;");
        QHBoxLayout *cl = new QHBoxLayout(container);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(0);
        cl->setAlignment(Qt::AlignCenter);
        cl->addWidget(delBtn);
        m_cartTable->setCellWidget(row, 3, container);
        
        total += item.price * item.qty; 
    }
    m_totalLabel->setText(QString("¥ %1").arg(total, 0, 'f', 2)); 

    // 更新磁贴上的数字角标
    auto tiles = m_tileContainer->findChildren<ItemTile*>();
    for (auto tile : tiles) { 
        int foundQty = 0; 
        for (const auto &item : m_cart) { 
            if (item.id == tile->id()) { foundQty = item.qty; break; } 
        } 
        tile->setQuantity(foundQty); 
    }
}

void QuickOrderDialog::onRemoveCartItem(int row) { 
    if (row >= 0 && row < m_cart.size()) { 
        m_cart.removeAt(row); 
        updateCartUI(); 
    } 
}

void QuickOrderDialog::onSearchItems(const QString &text) { 
    updateTilePanel(m_currentCategory, text); 
}

void QuickOrderDialog::onCreateOrder()
{
    if (m_cart.isEmpty()) { CustomMessageDialog::showWarning(this, "提示", "购物车为空，请先选择项目！"); return; }
    
    QString memberId = m_memberCombo->currentData().toString();
    QString memberName = m_memberCombo->currentText().split(" (").first();
    
    double total = 0;
    QJsonArray detailsArr;
    for (const auto &item : m_cart) {
        total += item.price * item.qty;
        
        QJsonObject obj;
        obj["name"] = item.name;
        obj["price"] = item.price;
        obj["count"] = item.qty;
        obj["barcode"] = item.id;
        
        // 如果是商品，注入照片路径（避免存入base64导致超过数据库TEXT列65KB限制）
        if (!item.isService) {
            ProductInfo p = ProductDataManager::instance()->getProduct(item.id);
            if (!p.images.isEmpty()) {
                QString photoPath = p.images[0];
                if (!photoPath.startsWith("/9j/") && photoPath.length() < 512) {
                    obj["photo"] = photoPath;
                }
            }
        }
        
        detailsArr.append(obj);
    }
    QString detailStr = QString::fromUtf8(QJsonDocument(detailsArr).toJson(QJsonDocument::Compact));
    
    QString orderId = "ORD" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
    qDebug() << "[POS] Creating order:" << orderId << "Total:" << total;
    
    OrderInfo order;
    order.id = orderId;
    order.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    order.status = "Unpaid"; 
    order.totalAmount = total;
    order.finalAmount = total;
    order.itemDetails = detailStr;
    order.memberId = memberId;
    order.memberName = memberName;
    order.sourceModule = "Product"; 
    order.payMethod = "";
    order.relatedId = "";
    
    PetDataManager::instance()->addOrder(order);

    CustomMessageDialog::showSuccess(this, "下单成功", 
        QString("订单 %1 已成功创建！\n\n"
                "请在【订单管理中心】完成支付结算。").arg(orderId));
    
    emit orderCreated(orderId);
    accept();
}

void QuickOrderDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    
    QWidget *topLevel = parentWidget();
    while (topLevel && topLevel->parentWidget()) {
        topLevel = topLevel->parentWidget();
    }
    
    if (topLevel) {
        QPoint center = topLevel->mapToGlobal(topLevel->rect().center());
        move(center.x() - width() / 2, center.y() - height() / 2);
    }
}