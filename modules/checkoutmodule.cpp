#include "checkoutmodule.h"
#include "petdatamanager.h"
#include "quickorderdialog.h"
#include "orderdetaildrawer.h"
#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QButtonGroup>
#include <QIntValidator>
#include <QDateEdit>
#include <QDateTime>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
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
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(200); // 200ms 内的多次信号只触发一次刷新
    connect(m_refreshTimer, &QTimer::timeout, this, [this](){
        updateStats();
        refreshView();
    });

    setupUI();
    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, [this](){
        m_refreshTimer->start();
    });
    m_currentModuleFilter = "全部";
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
    topLayout->setContentsMargins(25, 15, 25, 15);
    topLayout->setSpacing(12);

    QLabel *title = new QLabel("订单管理中心");
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #1a1b1e; border: none;");
    topLayout->addWidget(title);

    QHBoxLayout *dashLayout = new QHBoxLayout();
    dashLayout->setSpacing(20);
    auto createStatCard = [&](const QString &title, QLabel* &valLabel, const QString &valColor, bool isLive = false) {
        QFrame *card = new QFrame();
        card->setFixedHeight(80);
        card->setStyleSheet("QFrame { background: white; border-radius: 6px; border: 1px solid #f1f5f9; }");
        QVBoxLayout *l = new QVBoxLayout(card);
        l->setContentsMargins(15, 12, 15, 12);
        l->setSpacing(4);

        QHBoxLayout *titleLine = new QHBoxLayout();
        QLabel *t = new QLabel(title);
        t->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; border: none;");
        titleLine->addWidget(t);
        
        if (isLive) {
            QLabel *dot = new QLabel();
            dot->setFixedSize(8, 8);
            dot->setStyleSheet("background: #22c55e; border-radius: 4px;"); // 绿色指示灯
            titleLine->addSpacing(5);
            titleLine->addWidget(dot);
            QLabel *liveTxt = new QLabel("实时");
            liveTxt->setStyleSheet("color: #22c55e; font-size: 11px; font-weight: bold; border: none;");
            titleLine->addWidget(liveTxt);
        }
        titleLine->addStretch();
        l->addLayout(titleLine);

        valLabel = new QLabel("0");
        valLabel->setStyleSheet(QString("font-size: 22px; font-weight: bold; color: %1; border: none;").arg(valColor));
        l->addWidget(valLabel);
        l->addStretch();
        return card;
    };
    dashLayout->addWidget(createStatCard("今日实时营收", m_statTodayRevenue, "#3b82f6", true));
    dashLayout->addWidget(createStatCard("所选时段营收", m_statPeriodRevenue, "#1e293b"));
    dashLayout->addWidget(createStatCard("待处理订单", m_statPendingCount, "#f59e0b"));
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
        "  border-radius: 6px; "
        "  padding: 8px 12px; "
        "  "
        "  font-size: 13px; "
        "  color: #1e293b; "
        "} "
        "QLineEdit:hover { border-color: #3b82f6; } "
        "QLineEdit:focus { border-color: #3b82f6; background: white; }"
    );
    filterLayout->addWidget(m_searchEdit);

    auto createFilterLabel = [&](const QString &txt) {
        QLabel *l = new QLabel(txt);
        l->setStyleSheet("color: #64748b; font-weight: bold; font-size: 13px; border: none; ");
        return l;
    };

    // Business Module Filter
    moduleFilterContainer = new QWidget();
    QHBoxLayout *moduleLayout = new QHBoxLayout(moduleFilterContainer);
    moduleLayout->setContentsMargins(0, 0, 0, 0);
    moduleLayout->setSpacing(8);

    QList<QPair<QString, QString>> modules = {
        {"全部业务", "全部"},
        {"洗护预约", "Appointment"},
        {"宠物寄养", "Boarding"},
        {"到店服务", "Direct"},
        {"商品零售", "Product"}
    };

    for (const auto &m : modules) {
        QPushButton *btn = new QPushButton(m.first);
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setFixedHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        if (m.second == m_currentModuleFilter) btn->setChecked(true);

        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );

        connect(btn, &QPushButton::clicked, this, [=](){
            m_currentModuleFilter = m.second;
            refreshView();
        });
        moduleLayout->addWidget(btn);
    }
    filterLayout->addWidget(moduleFilterContainer);

    // Date Range Picker (Using Custom Calendar)
    filterLayout->addWidget(createFilterLabel("时段:"));
    
    QHBoxLayout *shortcuts = new QHBoxLayout();
    shortcuts->setSpacing(6);
    QStringList labels = {"今天", "昨天", "本周", "本月", "上月"};
    QButtonGroup *timeGroup = new QButtonGroup(this);
    timeGroup->setExclusive(true);

    for (const QString &txt : labels) {
        QPushButton *btn = new QPushButton(txt);
        btn->setCheckable(true);
        btn->setFixedHeight(36);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 17px; padding: 0 15px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );
        
        if (txt == "今天") btn->setChecked(true);
        timeGroup->addButton(btn);

        connect(btn, &QPushButton::clicked, this, [this, txt](){
            QDate today = QDate::currentDate();
            if (txt == "今天") setDateRange(today, today);
            else if (txt == "昨天") setDateRange(today.addDays(-1), today.addDays(-1));
            else if (txt == "本周") setDateRange(today.addDays(-(today.dayOfWeek()-1)), today);
            else if (txt == "本月") setDateRange(QDate(today.year(), today.month(), 1), QDate(today.year(), today.month(), today.daysInMonth()));
            else if (txt == "上月") {
                QDate lastMonth = today.addMonths(-1);
                setDateRange(QDate(lastMonth.year(), lastMonth.month(), 1), QDate(lastMonth.year(), lastMonth.month(), lastMonth.daysInMonth()));
            }
        });
        shortcuts->addWidget(btn);
    }
    filterLayout->addLayout(shortcuts);

    m_startDateEdit = new CustomCalendarEdit();
    m_endDateEdit = new CustomCalendarEdit();
    m_startDateEdit->setText(QDate::currentDate().addDays(-7).toString("yyyy-MM-dd"));
    m_endDateEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    
    QString dateStyle = 
        "QLineEdit { "
        "  background: #fcfcfd; "
        "  border: 1px solid #e2e8f0; "
        "  border-radius: 6px; "
        "  padding: 0 15px; "
        "  height: 40px; "
        "  color: #1e293b; "
        "  font-weight: bold; "
        "  "
        "} "
        "QLineEdit:hover { border-color: #3b82f6; } "
        "QLineEdit:focus { border-color: #3b82f6; background: white; }";

    for(auto d : {m_startDateEdit, m_endDateEdit}) {
        d->setFixedWidth(115);
        d->setStyleSheet(dateStyle);
        filterLayout->addWidget(d);
    }
    
    QLabel *toLabel = new QLabel("-");
    toLabel->setStyleSheet("color: #94a3b8; font-weight: bold; border: none;"); // 删除边框
    filterLayout->insertWidget(filterLayout->indexOf(m_endDateEdit), toLabel);

    filterLayout->addStretch();

    // 快速开单按钮
    QPushButton *quickOrderBtn = new QPushButton();
    quickOrderBtn->setFixedSize(110, 38);
    quickOrderBtn->setCursor(Qt::PointingHandCursor);
    quickOrderBtn->setStyleSheet(
        "QPushButton { background: white; border-radius: 6px; border: 1px solid #3b82f6; } "
        "QPushButton:hover { background: #eff6ff; }"
    );
    
    QHBoxLayout *btnLayout = new QHBoxLayout(quickOrderBtn);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *btnTextLabel = new QLabel("快速开单");
    btnTextLabel->setAlignment(Qt::AlignCenter);
    btnTextLabel->setStyleSheet("color: #3b82f6; font-weight: bold; font-size: 13px; background: transparent; ");
    btnLayout->addWidget(btnTextLabel);
    
    filterLayout->addWidget(quickOrderBtn);
    
    connect(quickOrderBtn, &QPushButton::clicked, this, [this](){
        QuickOrderDialog dialog(this);
        connect(&dialog, &QuickOrderDialog::orderCreated, this, &CheckoutModule::onFilter);
        dialog.exec();
    });

    contentRoot->addWidget(filterCard);

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CheckoutModule::onFilter);
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
    orderTable->setColumnWidth(1, 250); // Order ID (扩大宽度以确保长编号完整显示)
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
        
    );
    connect(orderTable, &QTableWidget::itemClicked, this, [this](QTableWidgetItem *item){
        onOrderClicked(item->row());
    });
    tableLayout->addWidget(orderTable);

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
    
    connect(prevBtn, &QPushButton::clicked, this, &CheckoutModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &CheckoutModule::onNextPage);

    // 设置拉伸系数，确保顶部卡片保持紧凑，下方内容区域占满剩余空间
    rootLayout->setStretchFactor(topCard, 0);
    rootLayout->setStretchFactor(contentArea, 1);
}

void CheckoutModule::updateStats()
{
    qDebug() << "[CHECKOUT] updateStats started.";
    QLocale locale(QLocale::Chinese, QLocale::China);
    
    // 1. 获取今日实时营收 (锁定)
    auto todayStats = PetDataManager::instance()->getOrderStats(QDate::currentDate(), QDate::currentDate());
    m_statTodayRevenue->setText(QString("¥ %1").arg(locale.toString((long long)todayStats.totalRevenue)));
    
    // 2. 获取所选时段统计 (动态)
    auto periodStats = PetDataManager::instance()->getOrderStats(m_startDateEdit->date(), m_endDateEdit->date());
    m_statPeriodRevenue->setText(QString("¥ %1").arg(locale.toString((long long)periodStats.totalRevenue)));
    
    // 3. 待处理订单与客单价
    m_statAvgTicket->setText(QString("¥ %1").arg(locale.toString((long long)periodStats.avgTicket)));
    
    // 动态颜色反馈
    if (periodStats.pendingCount > 0) {
        m_statPendingCount->setStyleSheet("font-size: 22px; font-weight: bold; color: #f59e0b; border: none;");
        m_statPendingCount->setText(locale.toString(periodStats.pendingCount));
    } else {
        m_statPendingCount->setStyleSheet("font-size: 18px; font-weight: bold; color: #22c55e; border: none;");
        m_statPendingCount->setText("✓ 已清空");
    }
    qDebug() << "[CHECKOUT] updateStats finished.";
}

void CheckoutModule::refreshView()
{
    QString modFilter = m_currentModuleFilter;
    m_displayData = PetDataManager::instance()->getOrders(m_startDateEdit->date(), m_endDateEdit->date(), m_searchEdit->text(), modFilter);
    updatePagination();
}

void CheckoutModule::updatePagination()
{
    int total = m_displayData.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, total);
    
    orderTable->setUpdatesEnabled(false); // 停止重绘，大幅提升填充速度
    orderTable->setRowCount(end - start); // 预分配行数
    
    for (int i = start; i < end; ++i) {
        int row = i - start;
        const auto &order = m_displayData[i];
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
        
        // 具体明细：解析 JSON 格式或处理旧版 + 分隔格式
        QString displayDetails = order.itemDetails.trimmed();
        if (displayDetails.startsWith("[") && displayDetails.endsWith("]")) {
            // 尝试作为 JSON 解析
            QJsonDocument doc = QJsonDocument::fromJson(displayDetails.toUtf8());
            if (!doc.isNull() && doc.isArray()) {
                QJsonArray arr = doc.array();
                if (!arr.isEmpty()) {
                    QJsonObject firstObj = arr[0].toObject();
                    QString name = firstObj["name"].toString();
                    if (name.isEmpty()) name = "未知服务";
                    int count = firstObj["count"].toInt();
                    
                    displayDetails = name;
                    if (count > 1) displayDetails += QString(" x%1").arg(count);
                    if (arr.size() > 1) displayDetails += "...";
                }
            }
        } else if (displayDetails.contains("+")) {
            displayDetails = displayDetails.split("+").first() + "...";
        }
        
        // 限制长度
        if (displayDetails.length() > 40) displayDetails = displayDetails.left(37) + "...";
        setItem(4, displayDetails);
        
        QTableWidgetItem *amtIt = new QTableWidgetItem(QString("¥ %1").arg(order.totalAmount, 0, 'f', 2));
        amtIt->setForeground(QColor("#334155"));
        amtIt->setTextAlignment(Qt::AlignCenter);
        orderTable->setItem(row, 5, amtIt);

        QTableWidgetItem *statusIt = new QTableWidgetItem(order.status == "Paid" ? "已支付" : (order.status == "Unpaid" ? "待结算" : "已取消"));
        statusIt->setTextAlignment(Qt::AlignCenter);
        statusIt->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        
        if (order.status == "Paid") {
            statusIt->setForeground(QColor("#166534"));
            statusIt->setBackground(QColor("#dcfce7"));
        } else if (order.status == "Unpaid") {
            statusIt->setForeground(QColor("#9a3412"));
            statusIt->setBackground(QColor("#ffedd5"));
        } else {
            statusIt->setForeground(QColor("#64748b"));
            statusIt->setBackground(QColor("#f1f5f9"));
        }
        orderTable->setItem(row, 6, statusIt);
    }
    orderTable->setUpdatesEnabled(true);
    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);

    if (total == 0) {
        m_detailDrawer->showEmptyState();
        prevBtn->setEnabled(false);
        nextBtn->setEnabled(false);
        pageLabel->setText("第 0 页 / 共 0 页");
    } else {
        // 自动选中当前页第一行并更新详情
        int startIdx = (m_currentPage - 1) * m_pageSize;
        if (startIdx < m_displayData.size()) {
            m_detailDrawer->setOrder(m_displayData[startIdx]);
            orderTable->selectRow(0);
        }
    }
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
