#include "quickorderdialog.h"
#include "productdatamanager.h"
#include "servicedatamanager.h"
#include "petdatamanager.h"
#include "memberdatamanager.h"
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
#include <functional>

// --- ItemTile 实现 ---
ItemTile::ItemTile(const QString &id, const QString &name, double price, const QString &icon, bool isService, const QString &category, QWidget *parent)
    : QFrame(parent), m_id(id), m_isService(isService), m_category(category), m_isSelected(false), m_qty(0) {
    setFixedSize(110, 130);
    setCursor(Qt::PointingHandCursor);
    setObjectName("ItemTile");
    updateStyle();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);

    QLabel *iconLabel = new QLabel(icon.isEmpty() ? (isService ? "🛁" : "📦") : icon);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 24px; background: transparent; border-radius: 8px; border: none; ");
    iconLabel->setFixedHeight(65);
    layout->addWidget(iconLabel);

    QLabel *nameLabel = new QLabel(name);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #1e293b; border: none; ");
    nameLabel->setWordWrap(true);
    layout->addWidget(nameLabel);

    QLabel *priceLabel = new QLabel(QString("¥%1").arg(price, 0, 'f', 2));
    priceLabel->setAlignment(Qt::AlignCenter);
    priceLabel->setStyleSheet("font-size: 12px; color: #3b82f6; font-weight: 800; border: none; ");
    layout->addWidget(priceLabel);

    m_qtyBadge = new QLabel(this);
    m_qtyBadge->setAlignment(Qt::AlignCenter);
    m_qtyBadge->setStyleSheet("background: #ef4444; color: white; border-radius: 9px; font-size: 10px; font-weight: bold; padding: 2px;");
    m_qtyBadge->setFixedSize(18, 18);
    m_qtyBadge->move(85, 105);
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
        QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; color: #1e293b; font-size: 13px; }
        QComboBox:hover { border-color: #409eff; }
        QComboBox::drop-down { border: none; width: 24px; }
        QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; }
        QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }
        QTableWidget { border: none; background: white; gridline-color: transparent; }
        QHeaderView::section { background: white; border-bottom: 1px solid #f1f5f9; height: 40px; color: #64748b; font-weight: bold; font-size: 13px; }
        QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; color: #1e293b; font-size: 13px; }
        QLineEdit:focus { border-color: #3b82f6; }
    )";
    setStyleSheet(globalStyle);

    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(15, 15, 15, 15);
    rootLayout->setSpacing(15);

    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(15);

    QFrame *catCard = new QFrame();
    catCard->setStyleSheet("QFrame { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    catCard->setFixedHeight(60);
    QHBoxLayout *catLayout = new QHBoxLayout(catCard);
    catLayout->setContentsMargins(15, 0, 15, 0);
    catLayout->setSpacing(10);

    m_categoryGroup = new QButtonGroup(this);
    QStringList cats = {"全部", "洗护", "美容", "寄养", "零售", "接送"};
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
    leftLayout->addWidget(catCard);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");
    
    m_tileContainer = new QWidget();
    m_tileLayout = new QGridLayout(m_tileContainer);
    m_tileLayout->setContentsMargins(5, 5, 15, 5);
    m_tileLayout->setSpacing(15);
    m_tileLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    
    scroll->setWidget(m_tileContainer);
    leftLayout->addWidget(scroll, 1);

    QFrame *listCard = new QFrame();
    listCard->setFixedHeight(300); 
    listCard->setStyleSheet("QFrame { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QVBoxLayout *listLayout = new QVBoxLayout(listCard);
    listLayout->setContentsMargins(12, 12, 12, 12);

    QLabel *listTitle = new QLabel("已选清单");
    listTitle->setStyleSheet("font-weight: 800; color: #1e293b; font-size: 14px; margin-bottom: 5px;");
    listLayout->addWidget(listTitle);

    m_cartTable = new QTableWidget();
    m_cartTable->setColumnCount(4);
    m_cartTable->setHorizontalHeaderLabels({"项目名称", "单价", "数量", "操作"});
    m_cartTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_cartTable->setColumnWidth(2, 100);
    m_cartTable->setColumnWidth(3, 80);
    m_cartTable->verticalHeader()->setVisible(false);
    m_cartTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_cartTable->setFocusPolicy(Qt::NoFocus);
    listLayout->addWidget(m_cartTable);

    leftLayout->addWidget(listCard);
    rootLayout->addWidget(leftPanel, 1);

    QWidget *sidebar = new QWidget();
    sidebar->setObjectName("QuickOrderSidebar");
    sidebar->setFixedWidth(320);
    sidebar->setStyleSheet("#QuickOrderSidebar { background: white; border: none; }"); 
    
    QVBoxLayout *mainSideLayout = new QVBoxLayout(sidebar);
    mainSideLayout->setContentsMargins(0, 0, 0, 0);
    mainSideLayout->setSpacing(0);

    QLabel *custTitle = new QLabel("客户信息");
    custTitle->setStyleSheet("font-weight: 800; color: #1e293b; font-size: 15px; margin: 25px 20px 10px 20px;");
    mainSideLayout->addWidget(custTitle);

    QScrollArea *sideScroll = new QScrollArea();
    sideScroll->setWidgetResizable(true);
    sideScroll->setFrameShape(QFrame::NoFrame);
    sideScroll->setStyleSheet("background: transparent; border: none;");

    QWidget *scrollContent = new QWidget();
    scrollContent->setObjectName("ScrollContent");
    scrollContent->setStyleSheet("#ScrollContent { background: white; }");
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(20, 0, 20, 10);
    scrollLayout->setSpacing(15);

    m_memberCombo = new QComboBox();
    m_memberCombo->setFixedHeight(42);
    m_memberCombo->setPlaceholderText("搜会员...");
    scrollLayout->addWidget(m_memberCombo);

    m_petCombo = new QComboBox();
    m_petCombo->setFixedHeight(42);
    m_petCombo->setPlaceholderText("选宠物...");
    m_petCombo->hide();
    scrollLayout->addWidget(m_petCombo);
    
    // --- 服务详情容器 (接送/寄养) ---
    m_serviceDetailContainer = new QWidget();
    m_serviceDetailLayout = new QVBoxLayout(m_serviceDetailContainer);
    m_serviceDetailLayout->setContentsMargins(0, 0, 0, 0);
    m_serviceDetailLayout->setSpacing(12);
    m_serviceDetailContainer->hide();
    scrollLayout->addWidget(m_serviceDetailContainer);

    scrollLayout->addStretch();
    sideScroll->setWidget(scrollContent);
    mainSideLayout->addWidget(sideScroll, 1);

    QFrame *totalFrame = new QFrame();
    totalFrame->setStyleSheet("border-top: 1px solid #f1f5f9; padding: 20px; background: white;");
    QVBoxLayout *totalAreaLayout = new QVBoxLayout(totalFrame);
    totalAreaLayout->setContentsMargins(0, 0, 0, 0);
    totalAreaLayout->setSpacing(20);

    QHBoxLayout *totalLine = new QHBoxLayout();
    QLabel *totalLabel = new QLabel("合计金额");
    totalLabel->setStyleSheet("color: #64748b; font-size: 15px; font-weight: bold;");
    totalLine->addWidget(totalLabel);
    totalLine->addStretch();
    m_totalLabel = new QLabel("¥ 0.00");
    m_totalLabel->setStyleSheet("color: #3b82f6; font-size: 36px; font-weight: 900;");
    totalLine->addWidget(m_totalLabel);
    totalAreaLayout->addLayout(totalLine);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("取消开单");
    cancelBtn->setFixedHeight(50);
    cancelBtn->setStyleSheet("QPushButton { background: white; color: #64748b; border: 1px solid #e2e8f0; border-radius: 12px; font-weight: bold; font-size: 14px; } QPushButton:hover { background: #f8fafc; color: #1e293b; }");
    
    QPushButton *okBtn = new QPushButton("确认下单");
    okBtn->setFixedHeight(50);
    okBtn->setStyleSheet("QPushButton { background: #3b82f6; color: white; border-radius: 12px; font-weight: bold; font-size: 14px; } QPushButton:hover { background: #2563eb; }");
    
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(okBtn);
    totalAreaLayout->addLayout(btnLayout);

    mainSideLayout->addWidget(totalFrame);
    rootLayout->addWidget(sidebar);

    connect(m_categoryGroup, &QButtonGroup::idClicked, this, &QuickOrderDialog::onCategoryChanged);
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
    if (memberId.isEmpty()) { m_petCombo->addItem("无宠物", ""); return; }
    auto pets = PetDataManager::instance()->getPetsByOwner(memberId);
    if (pets.isEmpty()) { m_petCombo->addItem("暂无关联宠物", ""); }
    else { for (const auto &p : pets) { QString display = QString("%1 (%2) - %3").arg(p.name, p.id, p.breed); m_petCombo->addItem(display, p.id); } }
}

void QuickOrderDialog::onCategoryChanged(int id)
{
    m_currentCategory = m_categoryGroup->button(id)->text();
    updateTilePanel(m_currentCategory);
}

void QuickOrderDialog::updateTilePanel(const QString &category, const QString &searchKw)
{
    QLayoutItem *child;
    while ((child = m_tileLayout->takeAt(0)) != nullptr) { if (child->widget()) child->widget()->deleteLater(); delete child; }
    int row = 0, col = 0; int maxCols = 6;
    if (category == "全部" || category == "洗护" || category == "美容" || category == "寄养" || category == "接送") {
        for (const auto &s : ServiceDataManager::instance()->activeServices()) {
            if (category != "全部" && s.category != category) continue;
            if (!searchKw.isEmpty() && !s.name.contains(searchKw)) continue;
            ItemTile *tile = new ItemTile(s.id, s.name, s.price, s.icon, true, s.category, m_tileContainer);
            tile->clickedSignal([this](QString id, bool svc) { onTileClicked(id, svc); });
            for (const auto &item : m_cart) { if (item.id == s.id) { tile->setQuantity(item.qty); break; } }
            m_tileLayout->addWidget(tile, row, col);
            if (++col >= maxCols) { col = 0; row++; }
        }
    }
    if (category == "全部" || category == "零售") {
        for (const auto &p : ProductDataManager::instance()->allProducts()) {
            if (!p.isActive) continue;
            if (!searchKw.isEmpty() && !p.name.contains(searchKw)) continue;
            ItemTile *tile = new ItemTile(p.barcode, p.name, p.price, "", false, "零售", m_tileContainer);
            tile->clickedSignal([this](QString id, bool svc) { onTileClicked(id, svc); });
            for (const auto &item : m_cart) { if (item.id == p.barcode) { tile->setQuantity(item.qty); break; } }
            m_tileLayout->addWidget(tile, row, col);
            if (++col >= maxCols) { col = 0; row++; }
        }
    }
}

void QuickOrderDialog::onTileClicked(const QString &id, bool isService)
{
    if (isService) {
        auto svc = ServiceDataManager::instance()->getService(id);
        QMap<QString, QStringList> exclusionGroups;
        exclusionGroups["洗护"] << "基础洗护" << "深度洗护" << "深度护理";
        exclusionGroups["美容"] << "整体造型" << "局部修剪";
        exclusionGroups["寄养"] << "普通寄养房间" << "豪华套房寄养" << "多宠家庭房寄养";
        exclusionGroups["接送"] << "单程接宠" << "单程送宠" << "往返接送";

        if (exclusionGroups.contains(svc.category)) {
            const QStringList &exclList = exclusionGroups[svc.category];
            if (exclList.contains(svc.name)) {
                for (int i = m_cart.size() - 1; i >= 0; --i) {
                    if (m_cart[i].isService) {
                        auto otherSvc = ServiceDataManager::instance()->getService(m_cart[i].id);
                        if (otherSvc.id != id && otherSvc.category == svc.category && exclList.contains(otherSvc.name)) { m_cart.removeAt(i); }
                    }
                }
            }
        }
    }
    for (int i = 0; i < m_cart.size(); ++i) { if (m_cart[i].id == id) { m_cart[i].qty++; updateCartUI(); return; } }
    CartItem item; item.id = id; item.isService = isService; item.qty = 1;
    if (isService) { auto s = ServiceDataManager::instance()->getService(id); item.name = s.name; item.price = s.price; item.category = s.category; }
    else { auto p = ProductDataManager::instance()->getProduct(id); item.name = p.name; item.price = p.price; item.category = "零售"; }
    m_cart.append(item);
    updateCartUI();
}

void QuickOrderDialog::updateCartUI()
{
    m_cartTable->setRowCount(0); double total = 0; bool hasService = false;
    CartItem *transportItem = nullptr;
    CartItem *boardingItem = nullptr;

    for (int i = 0; i < m_cart.size(); ++i) {
        const auto &item = m_cart[i]; int row = m_cartTable->rowCount(); m_cartTable->insertRow(row);
        QTableWidgetItem *nameItem = new QTableWidgetItem(item.name); nameItem->setTextAlignment(Qt::AlignCenter); m_cartTable->setItem(row, 0, nameItem);
        QTableWidgetItem *priceItem = new QTableWidgetItem(QString("¥%1").arg(item.price, 0, 'f', 2)); priceItem->setTextAlignment(Qt::AlignCenter); m_cartTable->setItem(row, 1, priceItem);
        QTableWidgetItem *qtyItem = new QTableWidgetItem(QString::number(item.qty)); qtyItem->setTextAlignment(Qt::AlignCenter); m_cartTable->setItem(row, 2, qtyItem);
        QPushButton *delBtn = new QPushButton("删除"); delBtn->setStyleSheet("color: #f87171; border: none; font-weight: bold; background: transparent; font-size: 13px;"); delBtn->setCursor(Qt::PointingHandCursor);
        connect(delBtn, &QPushButton::clicked, this, [this, i]() { onRemoveCartItem(i); }); m_cartTable->setCellWidget(row, 3, delBtn);
        total += item.price * item.qty; if (item.isService) hasService = true;
        if (item.category == "接送") transportItem = const_cast<CartItem*>(&m_cart[i]);
        if (item.category == "寄养") boardingItem = const_cast<CartItem*>(&m_cart[i]);
    }
    m_totalLabel->setText(QString("¥ %1").arg(total, 0, 'f', 2)); 
    m_petCombo->setVisible(hasService);

    // 更新详情容器
    QLayoutItem *child;
    while ((child = m_serviceDetailLayout->takeAt(0)) != nullptr) { if (child->widget()) child->widget()->deleteLater(); delete child; }
    
    bool showDetail = (transportItem || boardingItem);
    m_serviceDetailContainer->setVisible(showDetail);

    if (showDetail) {
        auto addCompactRow = [&](const QString &label, QWidget *edit, QWidget *date) {
            QVBoxLayout *vbox = new QVBoxLayout();
            vbox->setSpacing(4);
            vbox->setContentsMargins(0, 2, 0, 2);
            QLabel *l = new QLabel(label);
            l->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold;");
            vbox->addWidget(l);
            
            QHBoxLayout *hbox = new QHBoxLayout();
            hbox->setSpacing(8);
            hbox->addWidget(edit, 2);
            hbox->addWidget(date, 1);
            vbox->addLayout(hbox);
            m_serviceDetailLayout->addLayout(vbox);
        };

        if (transportItem) {
            if (transportItem->name == "单程接宠") {
                m_addrEdit1 = new QLineEdit(); m_addrEdit1->setPlaceholderText("接宠详细地址..."); m_addrEdit1->setFixedHeight(40);
                m_dateEdit1 = new CustomCalendarEdit(); m_dateEdit1->setFixedHeight(40); m_dateEdit1->setText(QDate::currentDate().toString("yyyy-MM-dd"));
                addCompactRow("接宠信息", m_addrEdit1, m_dateEdit1);
                m_addrEdit2 = nullptr; m_dateEdit2 = nullptr;
            } else if (transportItem->name == "单程送宠") {
                m_addrEdit1 = new QLineEdit(); m_addrEdit1->setPlaceholderText("送宠详细地址..."); m_addrEdit1->setFixedHeight(40);
                m_dateEdit1 = new CustomCalendarEdit(); m_dateEdit1->setFixedHeight(40); m_dateEdit1->setText(QDate::currentDate().toString("yyyy-MM-dd"));
                addCompactRow("送宠信息", m_addrEdit1, m_dateEdit1);
                m_addrEdit2 = nullptr; m_dateEdit2 = nullptr;
            } else if (transportItem->name == "往返接送") {
                m_addrEdit1 = new QLineEdit(); m_addrEdit1->setPlaceholderText("接宠地址..."); m_addrEdit1->setFixedHeight(40);
                m_dateEdit1 = new CustomCalendarEdit(); m_dateEdit1->setFixedHeight(40); m_dateEdit1->setText(QDate::currentDate().toString("yyyy-MM-dd"));
                addCompactRow("接宠信息", m_addrEdit1, m_dateEdit1);

                m_addrEdit2 = new QLineEdit(); m_addrEdit2->setPlaceholderText("送宠地址..."); m_addrEdit2->setFixedHeight(40);
                m_dateEdit2 = new CustomCalendarEdit(); m_dateEdit2->setFixedHeight(40); m_dateEdit2->setText(QDate::currentDate().toString("yyyy-MM-dd"));
                addCompactRow("送宠信息", m_addrEdit2, m_dateEdit2);
            }
        }

        if (boardingItem) {
            if (transportItem) {
                QFrame *line = new QFrame(); line->setFrameShape(QFrame::HLine); line->setFrameShadow(QFrame::Sunken);
                line->setStyleSheet("background-color: #f1f5f9; margin: 10px 0;"); m_serviceDetailLayout->addWidget(line);
            }
            
            QLabel *boardingLabel = new QLabel("寄养详情");
            boardingLabel->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold;");
            m_serviceDetailLayout->addWidget(boardingLabel);

            QHBoxLayout *dateRow = new QHBoxLayout();
            dateRow->setSpacing(8);
            m_checkInEdit = new CustomCalendarEdit(); m_checkInEdit->setFixedHeight(38); m_checkInEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
            m_checkOutEdit = new CustomCalendarEdit(); m_checkOutEdit->setFixedHeight(38); m_checkOutEdit->setText(QDate::currentDate().addDays(3).toString("yyyy-MM-dd"));
            
            QVBoxLayout *inCol = new QVBoxLayout(); inCol->setSpacing(2); inCol->addWidget(new QLabel("入店:")); inCol->addWidget(m_checkInEdit);
            QVBoxLayout *outCol = new QVBoxLayout(); outCol->setSpacing(2); outCol->addWidget(new QLabel("离店:")); outCol->addWidget(m_checkOutEdit);
            
            dateRow->addLayout(inCol);
            dateRow->addLayout(outCol);
            m_serviceDetailLayout->addLayout(dateRow);

            m_boardingDaysLabel = new QLabel("共 3 天");
            m_boardingDaysLabel->setStyleSheet("color: #3b82f6; font-weight: bold; font-size: 12px; margin-top: 4px;");
            m_serviceDetailLayout->addWidget(m_boardingDaysLabel);

            QLabel *roomLabel = new QLabel("分配房间");
            roomLabel->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; margin-top: 8px;");
            m_serviceDetailLayout->addWidget(roomLabel);

            m_roomCombo = new QComboBox(); m_roomCombo->setFixedHeight(40);
            m_serviceDetailLayout->addWidget(m_roomCombo);

            m_boardingStockLabel = new QLabel("正在检查房源...");
            m_boardingStockLabel->setStyleSheet("padding: 4px 8px; font-size: 11px;");
            m_serviceDetailLayout->addWidget(m_boardingStockLabel);

            auto updateBoardingInfo = [this]() {
                QDate start = QDate::fromString(m_checkInEdit->text(), "yyyy-MM-dd");
                QDate end = QDate::fromString(m_checkOutEdit->text(), "yyyy-MM-dd");
                int days = start.daysTo(end); if (days < 0) days = 0;
                m_boardingDaysLabel->setText(QString("共 %1 天").arg(days));
                m_roomCombo->clear();
                auto rooms = PetDataManager::instance()->getAvailableRooms(start, end);
                for (int r : rooms) m_roomCombo->addItem(QString("%1号房").arg(r), r);
                if (!rooms.isEmpty()) {
                    m_boardingStockLabel->setText(QString("房源充沛 (余%1)").arg(rooms.size()));
                    m_boardingStockLabel->setStyleSheet("background: #f0fdf4; color: #16a34a; border: 1px solid #bbf7d0; border-radius: 4px; padding: 4px;");
                } else {
                    m_boardingStockLabel->setText("房间已满");
                    m_boardingStockLabel->setStyleSheet("background: #fef2f2; color: #dc2626; border: 1px solid #fecaca; border-radius: 4px; padding: 4px;");
                }
            };

            connect(m_checkInEdit, &CustomCalendarEdit::textChanged, this, updateBoardingInfo);
            connect(m_checkOutEdit, &CustomCalendarEdit::textChanged, this, updateBoardingInfo);
            updateBoardingInfo();
        }
    }

    auto tiles = m_tileContainer->findChildren<ItemTile*>();
    for (auto tile : tiles) { int foundQty = 0; for (const auto &item : m_cart) { if (item.id == tile->id()) { foundQty = item.qty; break; } } tile->setQuantity(foundQty); }
}

void QuickOrderDialog::onRemoveCartItem(int row) { if (row >= 0 && row < m_cart.size()) { m_cart.removeAt(row); updateCartUI(); } }
void QuickOrderDialog::onSearchItems(const QString &text) { updateTilePanel(m_currentCategory, text); }

void QuickOrderDialog::onCreateOrder()
{
    if (m_cart.isEmpty()) { QMessageBox::warning(this, "提示", "购物车为空，请先选择项目！"); return; }
    bool hasService = false; QString detailStr; double total = 0;
    for (const auto &item : m_cart) { if (item.isService) hasService = true; detailStr += QString("%1x%2; ").arg(item.name).arg(item.qty); total += item.price * item.qty; }
    if (hasService && m_petCombo->currentData().toString().isEmpty()) { QMessageBox::warning(this, "提示", "订单包含服务项目，请先选择对应的宠物！"); return; }
    
    // 收集详情信息
    QString metaInfo;
    if (m_serviceDetailContainer->isVisible()) {
        if (m_addrEdit1) metaInfo += QString("\n[接送地址1] %1 (%2)").arg(m_addrEdit1->text(), m_dateEdit1->text());
        if (m_addrEdit2) metaInfo += QString("\n[接送地址2] %1 (%2)").arg(m_addrEdit2->text(), m_dateEdit2->text());
        if (m_checkInEdit) {
            metaInfo += QString("\n[寄养详情] 入店: %1, 离店: %2, 房间: %3").arg(m_checkInEdit->text(), m_checkOutEdit->text(), m_roomCombo->currentText());
        }
    }
    detailStr += metaInfo;

    OrderInfo order; order.id = "ORD" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss"); order.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    order.status = "Unpaid"; order.totalAmount = total; order.finalAmount = total; order.itemDetails = detailStr;
    order.memberId = m_memberCombo->currentData().toString(); order.memberName = m_memberCombo->currentText().split(" (").first();
    if (hasService) { order.petId = m_petCombo->currentData().toString(); order.petName = m_petCombo->currentText(); }
    order.sourceModule = "Direct"; PetDataManager::instance()->addOrder(order);
    emit orderCreated(order.id); QMessageBox::information(this, "成功", "订单已创建，请在主界面进行结算。"); accept();
}