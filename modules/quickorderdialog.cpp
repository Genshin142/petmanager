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

// --- 自定义商品/服务磁贴组件 ---
class ItemTile : public QFrame {
public:
    ItemTile(const QString &id, const QString &name, double price, const QString &icon, bool isService, QWidget *parent = nullptr)
        : QFrame(parent), m_id(id), m_isService(isService) {
        setFixedSize(110, 130);
        setCursor(Qt::PointingHandCursor);
        setObjectName("ItemTile");
        setStyleSheet(
            "#ItemTile { background: white; border: 1px solid #e2e8f0; border-radius: 10px; } "
            "#ItemTile:hover { border-color: #409eff; background: #f0f7ff; }"
        );

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(4);

        QLabel *iconLabel = new QLabel(icon.isEmpty() ? (isService ? "🛁" : "📦") : icon);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("font-size: 24px; background: #f1f5f9; border-radius: 8px; border: none; ");
        iconLabel->setFixedHeight(65);
        layout->addWidget(iconLabel);

        QLabel *nameLabel = new QLabel(name);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #1e293b; border: none; ");
        nameLabel->setWordWrap(true);
        layout->addWidget(nameLabel);

        QLabel *priceLabel = new QLabel(QString("¥%1").arg(price, 0, 'f', 2));
        priceLabel->setAlignment(Qt::AlignCenter);
        priceLabel->setStyleSheet("font-size: 12px; color: #409eff; font-weight: 800; border: none; ");
        layout->addWidget(priceLabel);
    }

    void clickedSignal(std::function<void(QString, bool)> callback) { m_callback = callback; }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        QFrame::mousePressEvent(event);
        if (m_callback) m_callback(m_id, m_isService);
    }

private:
    QString m_id;
    bool m_isService;
    std::function<void(QString, bool)> m_callback;
};

QuickOrderDialog::QuickOrderDialog(QWidget *parent) : QDialog(parent)
{
    setupUI();
    m_currentCategory = "全部";
    updateTilePanel(m_currentCategory);
}

void QuickOrderDialog::setupUI()
{
    setWindowTitle("快速开单 (POS)");
    resize(1100, 750);
    setStyleSheet("QDialog { background-color: #f8fafc; }");

    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(12);

    // --- 左侧主区域 (分类 + 网格 + 列表) ---
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    // 1. 分类栏
    QFrame *catCard = new QFrame();
    catCard->setStyleSheet("QFrame { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    catCard->setFixedHeight(55);
    QHBoxLayout *catLayout = new QHBoxLayout(catCard);
    catLayout->setContentsMargins(10, 0, 10, 0);
    catLayout->setSpacing(8);

    m_categoryGroup = new QButtonGroup(this);
    QStringList cats = {"全部", "洗护", "美容", "寄养", "零售", "其他"};
    for (int i = 0; i < cats.size(); ++i) {
        QPushButton *btn = new QPushButton(cats[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 8px; padding: 0 15px; font-size: 13px; color: #606266; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );
        if (i == 0) btn->setChecked(true);
        m_categoryGroup->addButton(btn, i);
        catLayout->addWidget(btn);
    }
    catLayout->addStretch();
    leftLayout->addWidget(catCard);

    // 2. 商品网格 (带滚动条)
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");
    
    m_tileContainer = new QWidget();
    m_tileLayout = new QGridLayout(m_tileContainer);
    m_tileLayout->setContentsMargins(0, 0, 10, 0);
    m_tileLayout->setSpacing(12);
    m_tileLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    
    scroll->setWidget(m_tileContainer);
    leftLayout->addWidget(scroll, 1);

    // 3. 底部已选列表
    QFrame *listCard = new QFrame();
    listCard->setFixedHeight(220);
    listCard->setStyleSheet("QFrame { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QVBoxLayout *listLayout = new QVBoxLayout(listCard);
    listLayout->setContentsMargins(0, 10, 0, 0);

    QLabel *listTitle = new QLabel(" 已选清单");
    listTitle->setStyleSheet("font-weight: bold; color: #64748b; font-size: 13px; border: none; ");
    listLayout->addWidget(listTitle);

    m_cartTable = new QTableWidget();
    m_cartTable->setColumnCount(4);
    m_cartTable->setHorizontalHeaderLabels({"项目名称", "单价", "数量", "操作"});
    m_cartTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_cartTable->setColumnWidth(2, 80);
    m_cartTable->setColumnWidth(3, 60);
    m_cartTable->setShowGrid(false);
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cartTable->verticalHeader()->setVisible(false);
    m_cartTable->setStyleSheet("QTableWidget { border: none; font-size: 13px; } QHeaderView::section { background: #f8fafc; border: none; height: 35px; color: #64748b; font-weight: bold; }");
    listLayout->addWidget(m_cartTable);

    leftLayout->addWidget(listCard);
    rootLayout->addWidget(leftPanel, 1);

    // --- 右侧侧边栏 (会员/宠物 + 合计 + 按钮) ---
    QWidget *sidebar = new QWidget();
    sidebar->setFixedWidth(320);
    sidebar->setStyleSheet("QWidget { background: white; border-left: 1px solid #e2e8f0; }");
    QVBoxLayout *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(16, 20, 16, 20);
    sideLayout->setSpacing(20);

    // 1. 客户信息
    QLabel *custTitle = new QLabel("客户信息");
    custTitle->setStyleSheet("font-weight: 800; color: #1e293b; font-size: 14px; border: none;");
    sideLayout->addWidget(custTitle);

    QHBoxLayout *custSelectLayout = new QHBoxLayout();
    m_memberCombo = new QComboBox();
    m_memberCombo->setEditable(true);
    m_memberCombo->setPlaceholderText("搜会员...");
    m_memberCombo->setFixedHeight(36);
    
    m_petCombo = new QComboBox();
    m_petCombo->setFixedHeight(36);
    m_petCombo->setPlaceholderText("选宠物...");
    
    custSelectLayout->addWidget(m_memberCombo, 2);
    custSelectLayout->addWidget(m_petCombo, 1);
    sideLayout->addLayout(custSelectLayout);
    
    sideLayout->addStretch();

    // 2. 合计金额
    QFrame *totalFrame = new QFrame();
    totalFrame->setObjectName("TotalFrame");
    totalFrame->setStyleSheet("#TotalFrame { border-top: 1px solid #f1f5f9; padding-top: 20px; }");
    QVBoxLayout *totalLayout = new QVBoxLayout(totalFrame);
    totalLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *totalLine = new QHBoxLayout();
    QLabel *totalLabel = new QLabel("合计金额");
    totalLabel->setStyleSheet("color: #64748b; font-size: 14px; font-weight: bold; border: none;");
    totalLine->addWidget(totalLabel);
    totalLine->addStretch();
    m_totalLabel = new QLabel("¥ 0.00");
    m_totalLabel->setStyleSheet("color: #409eff; font-size: 32px; font-weight: 800; border: none;");
    totalLine->addWidget(m_totalLabel);
    totalLayout->addLayout(totalLine);

    sideLayout->addWidget(totalFrame);

    // 3. 按钮组
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("取消开单");
    cancelBtn->setFixedHeight(45);
    cancelBtn->setStyleSheet("QPushButton { background: #f1f5f9; color: #64748b; border-radius: 10px; font-weight: bold; } QPushButton:hover { background: #e2e8f0; }");
    
    QPushButton *okBtn = new QPushButton("确认下单");
    okBtn->setFixedHeight(45);
    okBtn->setStyleSheet("QPushButton { background: #409eff; color: white; border-radius: 10px; font-weight: bold; } QPushButton:hover { background: #3388ff; }");
    
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(okBtn);
    sideLayout->addLayout(btnLayout);

    rootLayout->addWidget(sidebar);

    // 连接信号
    connect(m_categoryGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &QuickOrderDialog::onCategoryChanged);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, this, &QuickOrderDialog::onCreateOrder);
    connect(m_memberCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QuickOrderDialog::onMemberChanged);

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
    
    if (memberId.isEmpty()) {
        m_petCombo->addItem("无宠物", "");
        return;
    }

    auto pets = PetDataManager::instance()->getPetsByOwner(memberId);
    if (pets.isEmpty()) {
        m_petCombo->addItem("暂无关联宠物", "");
    } else {
        for (const auto &p : pets) {
            m_petCombo->addItem(p.name, p.id);
        }
    }
}

void QuickOrderDialog::onCategoryChanged(int id)
{
    m_currentCategory = m_categoryGroup->button(id)->text();
    updateTilePanel(m_currentCategory);
}

void QuickOrderDialog::updateTilePanel(const QString &category, const QString &searchKw)
{
    // 清理旧磁贴
    QLayoutItem *child;
    while ((child = m_tileLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    int row = 0, col = 0;
    int maxCols = 5;

    // 加载服务
    if (category == "全部" || category == "洗护" || category == "美容" || category == "寄养" || category == "其他") {
        for (const auto &s : ServiceDataManager::instance()->activeServices()) {
            if (category != "全部" && s.category != category) continue;
            if (!searchKw.isEmpty() && !s.name.contains(searchKw)) continue;

            ItemTile *tile = new ItemTile(s.id, s.name, s.price, s.icon, true, m_tileContainer);
            tile->clickedSignal([this](QString id, bool svc) { onTileClicked(id, svc); });
            m_tileLayout->addWidget(tile, row, col);
            if (++col >= maxCols) { col = 0; row++; }
        }
    }

    // 加载商品
    if (category == "全部" || category == "零售") {
        for (const auto &p : ProductDataManager::instance()->allProducts()) {
            if (!p.isActive) continue;
            if (!searchKw.isEmpty() && !p.name.contains(searchKw)) continue;

            ItemTile *tile = new ItemTile(p.barcode, p.name, p.price, "", false, m_tileContainer);
            tile->clickedSignal([this](QString id, bool svc) { onTileClicked(id, svc); });
            m_tileLayout->addWidget(tile, row, col);
            if (++col >= maxCols) { col = 0; row++; }
        }
    }
}

void QuickOrderDialog::onTileClicked(const QString &id, bool isService)
{
    // 检查是否已在购物车
    for (int i = 0; i < m_cart.size(); ++i) {
        if (m_cart[i].id == id) {
            m_cart[i].qty++;
            updateCartUI();
            return;
        }
    }

    // 新增
    CartItem item;
    item.id = id;
    item.isService = isService;
    item.qty = 1;
    if (isService) {
        auto s = ServiceDataManager::instance()->getService(id);
        item.name = s.name; item.price = s.price;
    } else {
        auto p = ProductDataManager::instance()->getProduct(id);
        item.name = p.name; item.price = p.price;
    }
    m_cart.append(item);
    updateCartUI();
}

void QuickOrderDialog::updateCartUI()
{
    m_cartTable->setRowCount(0);
    double total = 0;
    bool hasService = false;

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
        
        QPushButton *delBtn = new QPushButton("删除");
        delBtn->setStyleSheet("color: #f56c6c; border: none; font-weight: bold; background: transparent;");
        delBtn->setCursor(Qt::PointingHandCursor);
        connect(delBtn, &QPushButton::clicked, this, [this, i]() { onRemoveCartItem(i); });
        m_cartTable->setCellWidget(row, 3, delBtn);

        total += item.price * item.qty;
        if (item.isService) hasService = true;
    }

    m_totalLabel->setText(QString("¥ %1").arg(total, 0, 'f', 2));
    m_petCombo->setEnabled(hasService);
}

void QuickOrderDialog::onRemoveCartItem(int row)
{
    if (row >= 0 && row < m_cart.size()) {
        m_cart.removeAt(row);
        updateCartUI();
    }
}

void QuickOrderDialog::onSearchItems(const QString &text) { updateTilePanel(m_currentCategory, text); }

void QuickOrderDialog::onCreateOrder()
{
    if (m_cart.isEmpty()) {
        QMessageBox::warning(this, "提示", "购物车为空，请先选择项目！");
        return;
    }

    // 验证服务项目是否关联了宠物
    bool hasService = false;
    QString detailStr;
    double total = 0;
    for (const auto &item : m_cart) {
        if (item.isService) hasService = true;
        detailStr += QString("%1x%2; ").arg(item.name).arg(item.qty);
        total += item.price * item.qty;
    }

    if (hasService && m_petCombo->currentData().toString().isEmpty()) {
        QMessageBox::warning(this, "提示", "订单包含服务项目，请先选择对应的宠物！");
        return;
    }

    // 构造订单信息
    OrderInfo order;
    order.id = "ORD" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
    order.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    order.status = "Unpaid"; // 待结算
    order.totalAmount = total;
    order.finalAmount = total;
    order.itemDetails = detailStr;
    
    // 会员信息
    order.memberId = m_memberCombo->currentData().toString();
    order.memberName = m_memberCombo->currentText().split(" (").first();
    
    // 宠物信息 (如果有)
    if (hasService) {
        order.petId = m_petCombo->currentData().toString();
        order.petName = m_petCombo->currentText();
    }
    
    order.sourceModule = "Direct"; // 直到开单/到店服务
    
    // 保存到数据中心
    PetDataManager::instance()->addOrder(order);
    
    emit orderCreated(order.id);
    QMessageBox::information(this, "成功", "订单已创建，请在主界面进行结算。");
    accept();
}