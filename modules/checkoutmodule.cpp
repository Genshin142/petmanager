#include "checkoutmodule.h"
#include "petdatamanager.h"
#include "orderdetaildrawer.h"
#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateEdit>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QIntValidator>
#include <QDateTime>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QListView>

// --- 自定义 Delegate 实现全行圆角边框选中效果 ---
class RowDelegate : public QStyledItemDelegate {
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
            
            QRect rect = opt.rect.adjusted(1, 1, -1, -1);
            int radius = 8;
            QColor borderColor("#3b82f6");
            QColor bgColor("#eff6ff");

            painter->fillRect(opt.rect, bgColor);
            painter->setPen(QPen(borderColor, 2));
            
            if (isFirst) {
                QPainterPath path;
                // 从右上角（不缩进）开始画到左侧圆角处
                path.moveTo(opt.rect.right() + 1, rect.top()); 
                path.lineTo(rect.left() + radius, rect.top());
                path.arcTo(QRect(rect.left(), rect.top(), radius*2, radius*2), 90, 90);
                path.lineTo(rect.left(), rect.bottom() - radius);
                path.arcTo(QRect(rect.left(), rect.bottom() - radius*2, radius*2, radius*2), 180, 90);
                path.lineTo(opt.rect.right() + 1, rect.bottom());
                painter->drawPath(path);
            } else if (isLast) {
                QPainterPath path;
                // 从左上角（不缩进）开始画到右侧圆角处
                path.moveTo(opt.rect.left() - 1, rect.top());
                path.lineTo(rect.right() - radius, rect.top());
                path.arcTo(QRect(rect.right() - radius*2, rect.top(), radius*2, radius*2), 90, -90);
                path.lineTo(rect.right(), rect.bottom() - radius);
                path.arcTo(QRect(rect.right() - radius*2, rect.bottom() - radius*2, radius*2, radius*2), 0, -90);
                path.lineTo(opt.rect.left() - 1, rect.bottom());
                painter->drawPath(path);
            } else {
                // 中间列：画两条贯穿的水平线，不进行左右缩进
                painter->drawLine(QPoint(opt.rect.left() - 1, rect.top()), QPoint(opt.rect.right() + 1, rect.top()));
                painter->drawLine(QPoint(opt.rect.left() - 1, rect.bottom()), QPoint(opt.rect.right() + 1, rect.bottom()));
            }
        } else {
            painter->setPen(QPen(QColor("#f1f5f9"), 1));
            painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());
        }

        QRect textRect = opt.rect.adjusted(15, 0, -15, 0);
        QString text = opt.text;
        
        if (opt.state & QStyle::State_Selected) {
            painter->setPen(QColor("#1d4ed8"));
        } else {
            painter->setPen(QColor("#334155"));
        }
        
        QFont font = opt.font;
        font.setFamily("Microsoft YaHei");
        painter->setFont(font);
        painter->drawText(textRect, Qt::AlignCenter | Qt::AlignVCenter, text);
        painter->restore();
    }
};

CheckoutModule::CheckoutModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, [this](){
        updateStats();
        refreshView();
    });
    updateStats();
    refreshView();
}

void CheckoutModule::setupUI()
{
    setStyleSheet("background-color: #f8fafc;");

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(25, 25, 25, 25);
    rootLayout->setSpacing(25);

    // --- 1. Top Card (Title + Stats) ---
    QFrame *topCard = new QFrame();
    topCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    QVBoxLayout *topLayout = new QVBoxLayout(topCard);
    topLayout->setContentsMargins(25, 20, 25, 20);
    topLayout->setSpacing(20);

    QLabel *title = new QLabel("订单管理中心");
    title->setStyleSheet("font-size: 24px; font-weight: 800; color: #1a1b1e; font-family: 'Microsoft YaHei'; border: none;");
    topLayout->addWidget(title);

    QHBoxLayout *dashLayout = new QHBoxLayout();
    dashLayout->setSpacing(20);
    auto createStatCard = [&](const QString &title, QLabel* &valLabel, const QString &valColor = "#1e293b") {
        QFrame *card = new QFrame();
        card->setFixedHeight(80);
        card->setStyleSheet("QFrame { background: white; border-radius: 8px; border: 1px solid #f1f5f9; }");
        QVBoxLayout *l = new QVBoxLayout(card);
        l->setContentsMargins(15, 12, 15, 12);
        l->setSpacing(4);
        QLabel *t = new QLabel(title); 
        t->setStyleSheet("color: #64748b; font-size: 13px; font-weight: 700; font-family: 'Microsoft YaHei'; border: none;");
        valLabel = new QLabel("0"); 
        valLabel->setStyleSheet(QString("font-size: 22px; font-weight: 800; color: %1; font-family: 'Microsoft YaHei'; border: none;").arg(valColor));
        l->addWidget(t);
        l->addWidget(valLabel);
        return card;
    };
    dashLayout->addWidget(createStatCard("今日总营收", m_statRevenue, "#3b82f6"));
    dashLayout->addWidget(createStatCard("待结算订单", m_statPending, "#f59e0b"));
    dashLayout->addWidget(createStatCard("平均客单价", m_statAvgTicket, "#1e293b"));
    topLayout->addLayout(dashLayout);
    rootLayout->addWidget(topCard);

    // --- Bottom Content Area ---
    QWidget *contentArea = new QWidget();
    contentArea->setStyleSheet("background: transparent;");
    QVBoxLayout *contentRoot = new QVBoxLayout(contentArea);
    contentRoot->setContentsMargins(0, 0, 0, 0);
    contentRoot->setSpacing(20);

    // --- 2. Advanced Filter Card ---
    QFrame *filterCard = new QFrame();
    filterCard->setFixedHeight(65);
    filterCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: none; }");
    QHBoxLayout *filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(20, 0, 20, 0);
    filterLayout->setSpacing(15);

    // Search Box (Streamlined)
    m_searchEdit = new QLineEdit();
    m_searchEdit->setFixedWidth(200);
    m_searchEdit->setPlaceholderText("搜单号、会员、宠物...");
    m_searchEdit->setStyleSheet(
        "QLineEdit { "
        "  background: #f8fafc; "
        "  border: 1px solid #e2e8f0; "
        "  border-radius: 8px; "
        "  padding: 8px 12px; "
        "  font-family: 'Microsoft YaHei'; "
        "  font-size: 13px; "
        "  color: #1e293b; "
        "} "
        "QLineEdit:hover { border-color: #3b82f6; } "
        "QLineEdit:focus { border-color: #3b82f6; background: white; }"
    );
    filterLayout->addWidget(m_searchEdit);

    auto createFilterLabel = [&](const QString &txt) {
        QLabel *l = new QLabel(txt);
        l->setStyleSheet("color: #64748b; font-weight: 700; font-size: 13px; border: none; font-family: 'Microsoft YaHei';");
        return l;
    };

    // Business Module Filter
    filterLayout->addWidget(createFilterLabel("业务类型:"));
    m_moduleCombo = new QComboBox();
    m_moduleCombo->setFixedWidth(120);
    m_moduleCombo->setFixedHeight(36);
    m_moduleCombo->addItem("全部业务", "全部");
    m_moduleCombo->addItem("洗护预约", "Appointment");
    m_moduleCombo->addItem("宠物寄养", "Boarding");
    m_moduleCombo->addItem("到店服务", "Direct");
    m_moduleCombo->addItem("商品零售", "Product");
    
    QString comboStyle = 
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; }";
    m_moduleCombo->setStyleSheet(comboStyle);
    filterLayout->addWidget(m_moduleCombo);

    // Date Range Picker (Using Custom Calendar)
    filterLayout->addWidget(createFilterLabel("时段:"));
    m_startDateEdit = new CustomCalendarEdit();
    m_endDateEdit = new CustomCalendarEdit();
    m_startDateEdit->setText(QDate::currentDate().addDays(-7).toString("yyyy-MM-dd"));
    m_endDateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    
    QString dateStyle = 
        "QLineEdit { "
        "  background: #fcfcfd; "
        "  border: 1px solid #e2e8f0; "
        "  border-radius: 10px; "
        "  padding: 0 15px; "
        "  height: 40px; "
        "  color: #1e293b; "
        "  font-weight: 600; "
        "  font-family: 'Microsoft YaHei'; "
        "} "
        "QLineEdit:hover { border-color: #3b82f6; } "
        "QLineEdit:focus { border-color: #3b82f6; background: white; }";

    for(auto d : {m_startDateEdit, m_endDateEdit}) {
        d->setFixedWidth(115);
        d->setStyleSheet(dateStyle);
        filterLayout->addWidget(d);
    }
    
    QLabel *toLabel = new QLabel("-");
    toLabel->setStyleSheet("color: #94a3b8; font-weight: 800; border: none;"); // 删除边框
    filterLayout->insertWidget(filterLayout->indexOf(m_endDateEdit), toLabel);

    filterLayout->addStretch();
    contentRoot->addWidget(filterCard);

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CheckoutModule::onFilter);
    connect(m_moduleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CheckoutModule::onFilter);
    connect(m_startDateEdit, &CustomCalendarEdit::dateChanged, this, &CheckoutModule::onFilter);
    connect(m_endDateEdit, &CustomCalendarEdit::dateChanged, this, &CheckoutModule::onFilter);

    QHBoxLayout *mainContentLayout = new QHBoxLayout();
    mainContentLayout->setSpacing(25);

    // --- 3. Middle Card (Table) ---
    QFrame *tableCard = new QFrame();
    tableCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 5, 0, 0);

    orderTable = new QTableWidget();
    orderTable->setColumnCount(7); // Increased column count
    orderTable->setHorizontalHeaderLabels({"日期", "订单编号", "客户信息", "业务来源", "具体明细", "金额", "状态"});
    orderTable->setItemDelegate(new RowDelegate(orderTable));
    orderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    orderTable->setColumnWidth(0, 140); // Date
    orderTable->setColumnWidth(1, 150); // Order ID
    orderTable->setColumnWidth(2, 120); // Customer
    orderTable->setColumnWidth(3, 100); // Item
    orderTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch); // Details takes all remaining space
    orderTable->setColumnWidth(5, 100); // Amount
    orderTable->setColumnWidth(6, 100); // Status
    orderTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    orderTable->verticalHeader()->setDefaultSectionSize(60); 
    orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    orderTable->setSelectionMode(QAbstractItemView::SingleSelection);
    orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    orderTable->setShowGrid(false);
    orderTable->verticalHeader()->setVisible(false);
    orderTable->setStyleSheet(
        "QTableWidget { border: none; background: white; gridline-color: transparent; outline: none; } "
        "QHeaderView { background: white; border: none; } "
        "QHeaderView::section { background: white; border: none; border-bottom: 1px solid #f1f5f9; padding: 12px; font-weight: 600; color: #64748b; font-family: 'Microsoft YaHei'; font-size: 13px; }"
    );
    connect(orderTable, &QTableWidget::itemClicked, this, [this](QTableWidgetItem *item){
        onOrderClicked(item->row());
    });
    tableLayout->addWidget(orderTable);

    QHBoxLayout *pageBar = new QHBoxLayout();
    pageBar->setContentsMargins(20, 10, 20, 15);
    pageLabel = new QLabel("第 1 页 / 共 1 页");
    pageLabel->setStyleSheet("color: #64748b; font-size: 13px; font-family: 'Microsoft YaHei'; border: none;");
    prevBtn = new QPushButton("上一页");
    nextBtn = new QPushButton("下一页");
    QString pageBtnStyle = "QPushButton { background: white; border: 1px solid #e2e8f0; border-radius: 6px; padding: 6px 15px; color: #475569; font-weight: 600; font-size: 12px; font-family: 'Microsoft YaHei'; } "
                           "QPushButton:hover { background: #f8fafc; border-color: #cbd5e1; } "
                           "QPushButton:disabled { color: #cbd5e1; background: #f1f5f9; }";
    prevBtn->setStyleSheet(pageBtnStyle);
    nextBtn->setStyleSheet(pageBtnStyle);
    connect(prevBtn, &QPushButton::clicked, this, &CheckoutModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &CheckoutModule::onNextPage);
    pageBar->addStretch();
    pageBar->addWidget(prevBtn);
    pageBar->addWidget(pageLabel);
    pageBar->addWidget(nextBtn);
    tableLayout->addLayout(pageBar);

    mainContentLayout->addWidget(tableCard, 7);

    // --- 4. Right Card (Detail) ---
    QVBoxLayout *rightLayout = new QVBoxLayout();
    m_detailDrawer = new OrderDetailDrawer();
    m_detailDrawer->setFixedWidth(450);
    m_detailDrawer->setStyleSheet("OrderDetailDrawer { background: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    rightLayout->addWidget(m_detailDrawer);
    // Removed stretch to allow full height
    mainContentLayout->addLayout(rightLayout, 3);

    contentRoot->addLayout(mainContentLayout);
    rootLayout->addWidget(contentArea);
}

void CheckoutModule::updateStats()
{
    auto stats = PetDataManager::instance()->getOrderStats(m_startDateEdit->date(), m_endDateEdit->date());
    QLocale locale(QLocale::Chinese, QLocale::China);
    m_statRevenue->setText(QString("¥ %1").arg(locale.toString((long long)stats.totalRevenue)));
    m_statPending->setText(locale.toString(stats.pendingCount));
    m_statAvgTicket->setText(QString("¥ %1").arg(locale.toString((long long)stats.avgTicket)));
}

void CheckoutModule::refreshView()
{
    QString moduleFilter = m_moduleCombo->currentData().toString();
    m_displayData = PetDataManager::instance()->getOrders(m_startDateEdit->date(), m_endDateEdit->date(), m_searchEdit->text(), moduleFilter);
    updatePagination();
    if (m_displayData.isEmpty()) {
        m_detailDrawer->showEmptyState();
    } else {
        int start = (m_currentPage - 1) * m_pageSize;
        if (start < m_displayData.size()) {
            m_detailDrawer->setOrder(m_displayData[start]);
            orderTable->selectRow(0);
        }
    }
}

void CheckoutModule::updatePagination()
{
    orderTable->setRowCount(0);
    int total = m_displayData.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, total);
    for (int i = start; i < end; ++i) {
        const auto &order = m_displayData[i];
        int row = orderTable->rowCount();
        orderTable->insertRow(row);
        auto setItem = [&](int col, const QString &text, const QString &color = "#475569") {
            QTableWidgetItem *it = new QTableWidgetItem(text);
            it->setForeground(QColor(color));
            it->setTextAlignment(Qt::AlignCenter);
            orderTable->setItem(row, col, it);
        };

        setItem(0, order.createTime.left(16));
        setItem(1, order.id, "#64748b");
        // 客户信息：显示姓名 (ID)，若为空则显示“临时客”
        QString memberStr = order.memberName.isEmpty() ? "临时客" : order.memberName;
        if (!order.memberId.isEmpty()) memberStr += QString(" %1").arg(order.memberId);
        setItem(2, memberStr, "#1e293b");
        
        // 业务来源：采用更专业的业务分类命名
        QString sourceZh = (order.sourceModule == "Product" ? "商品零售" : 
                           (order.sourceModule == "Boarding" ? "宠物寄养" : 
                           (order.sourceModule == "Appointment" ? "洗护预约" : 
                           (order.sourceModule == "Direct" ? "到店服务" : "其他业务"))));
        setItem(3, sourceZh, "#3b82f6");
        
        setItem(4, order.itemDetails);
        
        QTableWidgetItem *amtIt = new QTableWidgetItem(QString("¥ %1").arg(order.totalAmount, 0, 'f', 2));
        amtIt->setForeground(QColor("#334155"));
        amtIt->setTextAlignment(Qt::AlignCenter);
        orderTable->setItem(row, 5, amtIt);

        QWidget *tagContainer = new QWidget();
        QHBoxLayout *tagLayout = new QHBoxLayout(tagContainer);
        tagLayout->setContentsMargins(0, 0, 0, 0);
        tagLayout->setAlignment(Qt::AlignCenter);
        QLabel *tag = new QLabel(order.status == "Paid" ? "已支付" : (order.status == "Unpaid" ? "待结算" : "已作废"));
        QString tagStyle = "padding: 4px 12px; border-radius: 6px; font-size: 11px; font-weight: 600; font-family: 'Microsoft YaHei';";
        if (order.status == "Paid") tagStyle += "background: #dcfce7; color: #166534;";
        else if (order.status == "Unpaid") tagStyle += "background: #ffedd5; color: #9a3412;";
        else tagStyle += "background: #f1f5f9; color: #475569;";
        tag->setStyleSheet(tagStyle);
        tagLayout->addWidget(tag);
        orderTable->setCellWidget(row, 6, tagContainer);
    }
    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);
}

void CheckoutModule::onFilter() { m_currentPage = 1; refreshView(); }
void CheckoutModule::onPrevPage() { if (m_currentPage > 1) { m_currentPage--; updatePagination(); } }
void CheckoutModule::onNextPage() { if (m_currentPage < (m_displayData.size() + m_pageSize - 1) / m_pageSize) { m_currentPage++; updatePagination(); } }
void CheckoutModule::onJumpPage() {}
void CheckoutModule::onOrderClicked(int row)
{
    int dataIdx = (m_currentPage - 1) * m_pageSize + row;
    if (dataIdx < m_displayData.size()) {
        m_detailDrawer->setOrder(m_displayData[dataIdx]);
    }
}
void CheckoutModule::setDateRange(const QDate &start, const QDate &end) {
    m_startDateEdit->setText(start.toString("yyyy-MM-dd"));
    m_endDateEdit->setText(end.toString("yyyy-MM-dd"));
    onFilter(); // 触发刷新
}

void CheckoutModule::resizeEvent(QResizeEvent *event) { QWidget::resizeEvent(event); }
