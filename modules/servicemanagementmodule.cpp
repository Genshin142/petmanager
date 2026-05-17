#include "servicemanagementmodule.h"
#include "servicedialog.h"
#include "servicedatamanager.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <QSplitter>
#include <QTableWidget>
#include <QCheckBox>
#include <QScrollArea>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QRegularExpression>

// --- 复用高级行渲染委托，实现全行圆角边框选中效果 ---
class ServiceRowDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // 默认背景
        if (opt.state & QStyle::State_MouseOver) {
            painter->fillRect(opt.rect, QColor("#ecf5ff"));
        } else {
            painter->fillRect(opt.rect, Qt::white);
        }

        if (opt.state & QStyle::State_Selected) {
            bool isFirst = (index.column() == 0);
            bool isLast = (index.column() == index.model()->columnCount() - 1);
            
            QRect rect = opt.rect.adjusted(1, 4, -1, -4); // 上下留出一点空隙，增加呼吸感
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

        painter->setPen(QColor((opt.state & QStyle::State_Selected) ? "#1e40af" : "#334155"));
        QFont font = painter->font();
        font.setWeight(opt.state & QStyle::State_Selected ? QFont::Bold : QFont::Normal);
        font.setPointSize(10);
        painter->setFont(font);

        QRect textRect = opt.rect.adjusted(4, 0, -4, 0);
        painter->drawText(textRect, opt.displayAlignment | Qt::AlignVCenter, opt.text);
        
        painter->restore();
    }
};

ServiceManagementModule::ServiceManagementModule(UserRole role, QWidget *parent)
    : QWidget(parent), m_role(role)
{
    setStyleSheet("background-color: #f8fafc;");
    
    // --- 1. Top Level Horizontal Layout ---
    QHBoxLayout *mainHorizontalLayout = new QHBoxLayout(this);
    mainHorizontalLayout->setContentsMargins(0, 0, 0, 0);
    mainHorizontalLayout->setSpacing(0);

    // --- 2. Left Content Container (QVBoxLayout) ---
    QWidget *leftContent = new QWidget();
    leftContent->setStyleSheet("background-color: transparent;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContent);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(20);

    // Module Title & Summary Stats
    QFrame *topCard = new QFrame();
    topCard->setFixedHeight(160);
    topCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #ebeef5; }");
    QVBoxLayout *topLayout = new QVBoxLayout(topCard);
    topLayout->setContentsMargins(25, 15, 25, 15);
    topLayout->setSpacing(12);

    QLabel *title = new QLabel("服务项目管理系统");
    title->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold; border: none; background: transparent;");
    topLayout->addWidget(title);

    QHBoxLayout *dashLayout = new QHBoxLayout();
    dashLayout->setSpacing(15);
    auto createStatCard = [&](const QString &title, const QString &val, const QString &color, QLabel* &valLabel) {
        QFrame *card = new QFrame();
        card->setFixedHeight(80);
        card->setStyleSheet("QFrame { background: #f8fafc; border-radius: 8px; border: 1px solid #f1f5f9; }");
        QVBoxLayout *l = new QVBoxLayout(card);
        l->setContentsMargins(15, 10, 15, 10);
        QLabel *t = new QLabel(title);
        t->setStyleSheet("color: #64748b; font-size: 11px; font-weight: bold; border: none;");
        valLabel = new QLabel(val);
        valLabel->setStyleSheet(QString("font-size: 20px; font-weight: bold; color: %1; border: none;").arg(color));
        l->addWidget(t);
        l->addWidget(valLabel);
        return card;
    };
    dashLayout->addWidget(createStatCard("服务项总数", "0", "#3b82f6", m_lblStatTotal));
    dashLayout->addWidget(createStatCard("本月热门", "--", "#1e293b", m_lblStatPopular));
    dashLayout->addWidget(createStatCard("营收预估", "¥ 0", "#22c55e", m_lblStatRevenue));
    dashLayout->addWidget(createStatCard("待结算提成", "¥ 0", "#f59e0b", m_lblStatComm));
    topLayout->addLayout(dashLayout);
    leftLayout->addWidget(topCard);

    // Master Table Area (with Toolbar)
    QFrame *tableCard = new QFrame();
    tableCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #ebeef5; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 5, 0, 0);
    tableLayout->setSpacing(0);

    // --- 操作中控台 (Operation Console) ---
    QFrame *toolbarWidget = new QFrame();
    toolbarWidget->setFixedHeight(64);
    toolbarWidget->setStyleSheet("QFrame { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QHBoxLayout *toolbar = new QHBoxLayout(toolbarWidget);
    toolbar->setContentsMargins(15, 0, 15, 0);
    toolbar->setSpacing(10);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(" 搜索服务项目...");
    m_searchEdit->setFixedWidth(280);
    m_searchEdit->setStyleSheet("QLineEdit { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 6px; padding: 8px 12px; }");
    toolbar->addWidget(m_searchEdit);

    m_categoryContainer = new QWidget();
    QHBoxLayout *catLayout = new QHBoxLayout(m_categoryContainer);
    catLayout->setContentsMargins(15, 0, 0, 0);
    catLayout->setSpacing(8);
    QStringList cats = {"全部", "洗护", "美容", "保健", "寄养", "接送"};
    for (const auto &c : cats) {
        QPushButton *btn = new QPushButton(c);
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setCursor(Qt::PointingHandCursor); // 设置手指样式
        btn->setFixedHeight(36);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 18px; padding: 0 15px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );
        if (c == "全部") btn->setChecked(true);
        connect(btn, &QPushButton::clicked, this, &ServiceManagementModule::onCategoryFilterChanged);
        catLayout->addWidget(btn);
    }
    toolbar->addWidget(m_categoryContainer);
    toolbar->addStretch();

    if (m_role == ADMIN) {
        QPushButton *addBtn = new QPushButton("新增服务项目");
        addBtn->setMinimumWidth(130);
        addBtn->setMinimumHeight(38);
        addBtn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #3b82f6; color: #3b82f6; border-radius: 8px; font-size: 13px; padding: 0 20px; font-weight: bold; }"
            "QPushButton:hover { background: #eff6ff; }"
        );
        addBtn->setCursor(Qt::PointingHandCursor); // 设置手指样式
        connect(addBtn, &QPushButton::clicked, this, &ServiceManagementModule::onAddService);
        toolbar->addWidget(addBtn);
    }
    leftLayout->addWidget(toolbarWidget);

    m_serviceTable = new QTableWidget();
    m_serviceTable->setColumnCount(8);
    m_serviceTable->setHorizontalHeaderLabels({"服务编码", "服务名称", "分类", "标准时长", "服务价格", "累计销量", "状态", "操作"});
    m_serviceTable->setItemDelegate(new ServiceRowDelegate(m_serviceTable));
    
    // --- 优化列宽策略：通过增大其他列的固定宽度来平衡名称列的拉伸占比 ---
    m_serviceTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_serviceTable->setColumnWidth(0, 140); // 编码 (SVR-XXXX)
    m_serviceTable->setColumnWidth(2, 140); // 分类
    m_serviceTable->setColumnWidth(3, 140); // 标准时长
    m_serviceTable->setColumnWidth(4, 140); // 服务价格
    m_serviceTable->setColumnWidth(5, 120); // 累计销量
    m_serviceTable->setColumnWidth(6, 110); // 状态
    m_serviceTable->setColumnWidth(7, 150); // 操作
    m_serviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_serviceTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // 仅名称列保持弹性拉伸


    m_serviceTable->verticalHeader()->setVisible(false);
    m_serviceTable->verticalHeader()->setDefaultSectionSize(60); // 统一行高 60px，与订单管理对齐
    m_serviceTable->setShowGrid(false);
    m_serviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_serviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serviceTable->setStyleSheet(
        "QTableWidget { border: none; background: white; outline: none; border-radius: 6px; } "
        "QHeaderView { border: none; background: transparent; border-radius: 12px 12px 0 0; }"
    );
    connect(m_serviceTable, &QTableWidget::itemSelectionChanged, this, &ServiceManagementModule::onTableSelectionChanged);
    tableLayout->addWidget(m_serviceTable);

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

    connect(prevBtn, &QPushButton::clicked, this, &ServiceManagementModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &ServiceManagementModule::onNextPage);

    leftLayout->addWidget(tableCard);

    mainHorizontalLayout->addWidget(leftContent, 1); // 左侧拉伸

    // --- 3. Right Content (Detail Panel, Fixed Width) ---
    setupDetailPanel();
    m_detailPanel->setFixedWidth(450); 
    mainHorizontalLayout->addWidget(m_detailPanel);

    connect(ServiceDataManager::instance(), &ServiceDataManager::serviceDataChanged, this, &ServiceManagementModule::updateTableData);

    QTimer::singleShot(0, this, &ServiceManagementModule::updateTableData);
}

void ServiceManagementModule::setupDetailPanel()
{
    m_detailPanel = new QWidget();
    m_detailPanel->setObjectName("ServiceDetailPanel");
    m_detailPanel->setStyleSheet("QWidget#ServiceDetailPanel { background-color: transparent; }"); 
    
    QVBoxLayout *outerLayout = new QVBoxLayout(m_detailPanel);
    outerLayout->setContentsMargins(20, 20, 20, 20);
    outerLayout->setSpacing(0);

    // 圆角容器
    QFrame *container = new QFrame();
    container->setObjectName("DetailContainer");
    container->setStyleSheet("#DetailContainer { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    container->setAttribute(Qt::WA_StyledBackground);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    outerLayout->addWidget(container);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // 关键修正：确保 ScrollArea 的视口（viewport）也是白色且拥有正确的圆角裁切
    scroll->setStyleSheet(
        "QScrollArea { background: transparent; border: none; border-radius: 12px; } "
        "QScrollArea > QWidget > QWidget { background: white; border: none; border-radius: 12px; } "
        "QScrollBar:vertical { width: 8px; background: transparent; margin: 4px 2px 4px 2px; } "
        "QScrollBar::handle:vertical { background: #e2e8f0; border-radius: 4px; min-height: 20px; } "
        "QScrollBar::handle:vertical:hover { background: #cbd5e1; } "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );

    QWidget *scrollContent = new QWidget();
    scrollContent->setObjectName("ScrollContent");
    scrollContent->setStyleSheet("QWidget#ScrollContent { background: white; border-radius: 12px; }");
    QVBoxLayout *detailLayout = new QVBoxLayout(scrollContent);
    detailLayout->setContentsMargins(20, 15, 20, 15); 
    detailLayout->setSpacing(15);

    // Title
    QHBoxLayout *header = new QHBoxLayout();
    m_lblDetailName = new QLabel("精油SPA洗浴 (小型犬)");
    m_lblDetailName->setWordWrap(true); 
    m_lblDetailName->setStyleSheet("font-size: 24px; font-weight: bold; color: #0f172a; background: transparent;"); 
    header->addWidget(m_lblDetailName, 1); 
    
    QPushButton *editBtn = new QPushButton("编辑服务");
    editBtn->setFixedSize(90, 32);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #409eff; border-radius: 6px; } "
        "QPushButton:hover { background: #ecf5ff; }"
    );
    
    QHBoxLayout *editBtnLayout = new QHBoxLayout(editBtn);
    editBtnLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *editBtnText = new QLabel("编辑服务");
    editBtnText->setAlignment(Qt::AlignCenter);
    editBtnText->setAttribute(Qt::WA_TransparentForMouseEvents);
    editBtnText->setStyleSheet("color: #409eff; font-size: 12px; font-weight: bold; background: transparent; border: none;");
    editBtnLayout->addWidget(editBtnText);
    
    connect(editBtn, &QPushButton::clicked, this, &ServiceManagementModule::onEditService);
    
    detailLayout->addLayout(header);

    // Stats Grid
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(20);
    auto createStatCard = [&](const QString &title, QLabel* &valLabel) {
        QFrame *card = new QFrame();
        card->setStyleSheet("background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; padding: 8px;"); // 缩小 padding
        QVBoxLayout *l = new QVBoxLayout(card);
        l->setSpacing(2);
        QLabel *t = new QLabel(title);
        t->setStyleSheet("color: #475569; font-size: 12px; font-weight: bold; border: none;"); // 缩小标题
        valLabel = new QLabel("0");
        valLabel->setStyleSheet("color: #0f172a; font-size: 20px; font-weight: bold; margin: 2px 0; border: none;"); // 24px -> 20px
        l->addWidget(t);
        l->addWidget(valLabel);
        return card;
    };
    statsLayout->addWidget(createStatCard("累计订单量", m_lblDetailSales));
    statsLayout->addWidget(createStatCard("本月营收", m_lblDetailRevenue));
    detailLayout->addLayout(statsLayout);

    // Base Info Section
    auto createSectionHeader = [&](const QString &text) {
        QWidget *w = new QWidget();
        w->setStyleSheet("background: transparent; border: none;");
        QHBoxLayout *l = new QHBoxLayout(w);
        l->setContentsMargins(0, 5, 0, 2);
        l->setSpacing(0);
        
        QLabel *lbl = new QLabel(text);
        lbl->setStyleSheet("color: #334155; font-size: 15px; font-weight: bold; margin-left: 4px; border: none; background: transparent;");
        
        l->addWidget(lbl);
        l->addStretch();
        return w;
    };

    detailLayout->addWidget(createSectionHeader("基础参数"));

    QFrame *baseCard = new QFrame();
    baseCard->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; } QLabel { background: transparent; border: none; }");
    QGridLayout *baseGrid = new QGridLayout(baseCard);
    baseGrid->setContentsMargins(16, 16, 16, 16);
    baseGrid->setSpacing(12);
    
    m_editPrice = new QLineEdit(); 
    m_editDuration = new QLineEdit();
    m_editCategory = new QLineEdit();
    m_editId = new QLineEdit();
    
    auto addBaseField = [&](const QString &label, QLineEdit* &edit, int r, int c) {
        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(4);
        QLabel *l = new QLabel(label);
        l->setStyleSheet("color: #94a3b8; font-size: 12px; font-weight: bold;"); 
        edit = new QLineEdit();
        edit->setReadOnly(true);
        edit->setStyleSheet("background: transparent; border: none; color: #1e293b; font-weight: bold; font-size: 15px;");
        vl->addWidget(l);
        vl->addWidget(edit);
        baseGrid->addLayout(vl, r, c);
    };

    addBaseField("服务价格", m_editPrice, 0, 0);
    addBaseField("预计时长", m_editDuration, 0, 1);
    addBaseField("服务分类", m_editCategory, 1, 0);
    addBaseField("服务编码", m_editId, 1, 1);
    detailLayout->addWidget(baseCard);

    // Commission Section
    detailLayout->addWidget(createSectionHeader("提成配置"));

    QFrame *commCard = new QFrame();
    commCard->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; }");
    QVBoxLayout *commL = new QVBoxLayout(commCard);
    commL->setContentsMargins(16, 16, 16, 16);
    commL->setSpacing(6);

    QLabel *commLbl = new QLabel("固定提成金额 (元)");
    commLbl->setStyleSheet("color: #94a3b8; font-size: 12px; font-weight: bold; border: none; background: transparent;");
    m_editCommFixed = new QLineEdit();
    m_editCommFixed->setReadOnly(true);
    m_editCommFixed->setStyleSheet("background: transparent; border: none; color: #1e293b; font-weight: bold; font-size: 15px;");
    commL->addWidget(commLbl);
    commL->addWidget(m_editCommFixed);
    detailLayout->addWidget(commCard);

    // Description Section
    detailLayout->addWidget(createSectionHeader("项目描述"));
    m_lblDetailDesc = new QLabel();
    m_lblDetailDesc->setWordWrap(true);
    m_lblDetailDesc->setStyleSheet("color: #475569; line-height: 1.4; font-size: 13px; background: #f8fafc; border-radius: 8px; padding: 10px;");
    detailLayout->addWidget(m_lblDetailDesc);

    // History Section
    detailLayout->addWidget(createSectionHeader("最近操作记录"));
    QFrame *historyCard = new QFrame();
    historyCard->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; }");
    QVBoxLayout *histL = new QVBoxLayout(historyCard);
    histL->setContentsMargins(16, 16, 16, 16);
    
    m_lblHistory = new QLabel();
    m_lblHistory->setWordWrap(true);
    m_lblHistory->setStyleSheet("color: #94a3b8; font-size: 13px; border: none; background: transparent;");
    histL->addWidget(m_lblHistory);
    detailLayout->addWidget(historyCard);

    detailLayout->addStretch();
    
    // 将滚动区域加入主面板
    mainLayout->addWidget(scroll);
    scroll->setWidget(scrollContent);

    // 采用绝对定位固定位置 (297, 21) 确保跨模块视觉一致
    editBtn->setParent(container);
    editBtn->move(297, 21);
    editBtn->raise();
    editBtn->show();
}

void ServiceManagementModule::updateTableData()
{
    m_serviceTable->setRowCount(0);
    QList<ServiceInfo> allServices = ServiceDataManager::instance()->allServices();
    
    // --- 实时统计逻辑 ---
    int totalCount = allServices.size();
    double totalRevenue = 0;
    double totalCommission = 0;
    QString popularName = "--";
    int maxSales = -1;

    for (const auto &info : allServices) {
        totalRevenue += (info.price * info.salesCount);
        totalCommission += (info.commissionFixed * info.salesCount);
        if (info.salesCount > maxSales && info.salesCount > 0) {
            maxSales = info.salesCount;
            popularName = info.name;
        }
    }

    m_lblStatTotal->setText(QString::number(totalCount));
    m_lblStatPopular->setText(popularName);
    m_lblStatRevenue->setText(QString("¥ %1").arg(totalRevenue, 0, 'f', 0).replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,"));
    m_lblStatComm->setText(QString("¥ %1").arg(totalCommission, 0, 'f', 0).replace(QRegularExpression("(\\d)(?=(\\d{3})+(?!\\d))"), "\\1,"));

    QString activeCat = "全部";
    for (auto btn : m_categoryContainer->findChildren<QPushButton*>()) {
        if (btn->isChecked()) { activeCat = btn->text(); break; }
    }

    // 1. 过滤
    QList<ServiceInfo> filtered;
    QString kw = m_searchEdit->text().trimmed().toLower();
    for (const auto &info : allServices) {
        if (activeCat != "全部" && info.category != activeCat) continue;
        if (!kw.isEmpty() && !info.name.toLower().contains(kw)) continue;
        filtered.append(info);
    }

    // 2. 分页计算
    int total = filtered.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, total);

    m_serviceTable->setUpdatesEnabled(false);
    m_serviceTable->setRowCount(0);
    m_serviceTable->setRowCount(end - start);

    auto addItem = [&](int row, int col, const QString &text, bool center = true) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        if (center) item->setTextAlignment(Qt::AlignCenter);
        else item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_serviceTable->setItem(row, col, item);
    };

    for (int i = start; i < end; ++i) {
        const auto &info = filtered[i];
        int row = i - start;
        m_serviceTable->setRowHeight(row, 60);


        // 1. 服务编码 (统一格式: S001)
        QString formattedId = QString("S%1").arg(info.id.toInt(), 3, 10, QChar('0'));
        QTableWidgetItem *idItem = new QTableWidgetItem(formattedId);
        idItem->setData(Qt::UserRole, info.id);
        idItem->setTextAlignment(Qt::AlignCenter);
        m_serviceTable->setItem(row, 0, idItem);

        // 2. 服务名称
        addItem(row, 1, info.name, true);

        // 3. 所属分类
        addItem(row, 2, info.category);

        // 4. 标准时长 (根据时长自动选择单位)
        QString durationText;
        if (info.durationMinutes >= 1440) {
            durationText = QString::number(info.durationMinutes / 1440.0, 'f', 0) + " 天";
        } else if (info.durationMinutes >= 60) {
            durationText = QString::number(info.durationMinutes / 60.0, 'f', 0) + " 小时";
        } else {
            durationText = QString::number(info.durationMinutes) + " 分钟";
        }
        addItem(row, 3, durationText);

        // 5. 服务价格
        addItem(row, 4, QString("￥ %1").arg(info.price, 0, 'f', 2));

        // 6. 累计销量
        addItem(row, 5, QString::number(info.salesCount));

        // 7. 当前状态
        QWidget *stW = new QWidget();
        QHBoxLayout *stL = new QHBoxLayout(stW);
        stL->setContentsMargins(0, 0, 0, 0);
        stL->setAlignment(Qt::AlignCenter);
        QLabel *stLh = new QLabel(info.isActive ? "启用中" : "已停用");
        stLh->setStyleSheet(info.isActive ? 
            "background: #dcfce7; color: #166534; padding: 4px 12px; border-radius: 12px; font-weight: bold; font-size: 11px;" :
            "background: #f1f5f9; color: #64748b; padding: 4px 12px; border-radius: 12px; font-weight: bold; font-size: 11px;");
        stL->addWidget(stLh);
        m_serviceTable->setCellWidget(row, 6, stW);

        // 8. 操作
        QWidget *opW = new QWidget();
        QHBoxLayout *opL = new QHBoxLayout(opW);
        opL->setContentsMargins(10, 0, 10, 0);
        opL->setAlignment(Qt::AlignCenter);
        
        QPushButton *downBtn = new QPushButton(info.isActive ? "下架" : "上架");
        downBtn->setProperty("serviceId", info.id);
        downBtn->setMinimumWidth(70);
        downBtn->setMinimumHeight(28);
        QString downBtnStyle = info.isActive ? 
            "QPushButton { color: #ef4444; background-color: #fff1f2; border: 1px solid #fecaca; border-radius: 4px; font-weight: bold; font-size: 12px; padding: 2px 10px; text-align: center; } QPushButton:hover { background-color: #ef4444; color: white; }" :
            "QPushButton { color: #3b82f6; background-color: #eff6ff; border: 1px solid #dbeafe; border-radius: 4px; font-weight: bold; font-size: 12px; padding: 2px 10px; text-align: center; } QPushButton:hover { background-color: #3b82f6; color: white; }";
        downBtn->setStyleSheet(downBtnStyle);
        downBtn->setCursor(Qt::PointingHandCursor);
        connect(downBtn, &QPushButton::clicked, this, &ServiceManagementModule::onToggleServiceStatus);
        opL->addWidget(downBtn);
        m_serviceTable->setCellWidget(row, 7, opW);
    }
    m_serviceTable->setUpdatesEnabled(true);

    // 更新分页控件状态
    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);
    
    // 默认选中第一行
    if (m_serviceTable->rowCount() > 0) {
        m_serviceTable->selectRow(0);
    }
}

void ServiceManagementModule::onPrevPage() {
    if (m_currentPage > 1) {
        m_currentPage--;
        updateTableData();
    }
}

void ServiceManagementModule::onNextPage() {
    m_currentPage++;
    updateTableData();
}

void ServiceManagementModule::updatePagination() {
    updateTableData();
}

void ServiceManagementModule::onTableSelectionChanged()
{
    auto items = m_serviceTable->selectedItems();
    if (items.isEmpty()) return;
    int row = items.first()->row();
    // ID 现在存储在第 1 列 (服务编码列)
    QString id = m_serviceTable->item(row, 0)->data(Qt::UserRole).toString();
    updateDetailPanel(ServiceDataManager::instance()->getService(id));
}

void ServiceManagementModule::updateDetailPanel(const ServiceInfo &info)
{
    m_currentService = info;
    m_lblDetailName->setText(info.name);
    m_lblDetailSales->setText(QString::number(info.salesCount));
    m_lblDetailRevenue->setText("¥ " + QString::number(info.salesCount * info.price, 'f', 2));
    
    m_editPrice->setText("¥ " + QString::number(info.price, 'f', 2));
    
    // 详情面板时长转换
    QString durationText;
    if (info.durationMinutes >= 1440) {
        durationText = QString::number(info.durationMinutes / 1440.0, 'f', 0) + " 天";
    } else if (info.durationMinutes >= 60) {
        durationText = QString::number(info.durationMinutes / 60.0, 'f', 0) + " 小时";
    } else {
        durationText = QString::number(info.durationMinutes) + " 分钟";
    }
    m_editDuration->setText(durationText);
    m_editCategory->setText(info.category);
    m_editId->setText(QString("S%1").arg(info.id.toInt(), 3, 10, QChar('0')));
    m_editCommFixed->setText(QString("￥ %1").arg(info.commissionFixed, 0, 'f', 2));
    
    // 恢复固定的 Label 文本
    if (m_editCommFixed->parentWidget()) {
        QLabel *l = m_editCommFixed->parentWidget()->findChild<QLabel*>();
        if (l) l->setText("服务提成金额 (元)");
    }
    
    m_lblDetailDesc->setText(info.description.isEmpty() ? "<font color='#94a3b8'>暂无项目描述</font>" : info.description);
    
    // 清空旧记录，显示默认状态（如果以后需要持久化日志，可以从数据库读取）
    if (m_lblHistory->text().isEmpty()) {
        m_lblHistory->setText("<font color='#94a3b8'>暂无最近操作记录</font>");
    }
}

void ServiceManagementModule::onSearchChanged(const QString &) { updateTableData(); }
void ServiceManagementModule::onCategoryFilterChanged() { updateTableData(); }
void ServiceManagementModule::onAddService()
{
    ServiceDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        ServiceInfo info = dialog.getServiceInfo();
        ServiceDataManager::instance()->addService(info);
        updateTableData();
    }
}

void ServiceManagementModule::onEditService()
{
    if (m_currentService.id.isEmpty()) return;
    
    ServiceDialog dialog(this);
    dialog.setWindowTitle("编辑服务项目");
    dialog.setServiceInfo(m_currentService);
    
    if (dialog.exec() == QDialog::Accepted) {
        ServiceInfo oldInfo = m_currentService;
        ServiceInfo newInfo = dialog.getServiceInfo();
        
        // 关键修复：继承原有统计数据，防止编辑后销量清零
        newInfo.salesCount = oldInfo.salesCount;
        newInfo.isActive = oldInfo.isActive;
        
        ServiceDataManager::instance()->updateService(newInfo);
        
        // 1. 自动差分逻辑
        QStringList changes;
        if (oldInfo.name != newInfo.name) 
            changes << QString("名称由 <font color='#3b82f6'>%1</font> 改为 <font color='#3b82f6'>%2</font>").arg(oldInfo.name, newInfo.name);
        if (oldInfo.category != newInfo.category)
            changes << QString("分类由 <font color='#3b82f6'>%1</font> 改为 <font color='#3b82f6'>%2</font>").arg(oldInfo.category, newInfo.category);
        if (oldInfo.price != newInfo.price)
            changes << QString("价格由 <font color='#3b82f6'>¥%1</font> 改为 <font color='#3b82f6'>¥%2</font>").arg(QString::number(oldInfo.price, 'f', 2), QString::number(newInfo.price, 'f', 2));
        if (oldInfo.durationMinutes != newInfo.durationMinutes)
            changes << QString("时长由 <font color='#3b82f6'>%1分钟</font> 改为 <font color='#3b82f6'>%2分钟</font>").arg(QString::number(oldInfo.durationMinutes), QString::number(newInfo.durationMinutes));
        if (oldInfo.commissionFixed != newInfo.commissionFixed)
            changes << QString("提成由 <font color='#3b82f6'>¥%1</font> 改为 <font color='#3b82f6'>¥%2</font>").arg(QString::number(oldInfo.commissionFixed, 'f', 2), QString::number(newInfo.commissionFixed, 'f', 2));
        if (oldInfo.description != newInfo.description)
            changes << QString("描述已修改");

        if (!changes.isEmpty()) {
            // 2. 格式化日志
            QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            QString logMsg = QString(
                "<div style='margin-bottom: 15px;'>"
                "  <font color='#94a3b8' style='font-size: 13px; font-weight: bold;'>[%1]</font><br>"
                "  <font color='#475569'><b>管理员 任坤</b> %2</font>"
                "</div>"
            ).arg(time, changes.join("，"));

            QString currentLogs = m_lblHistory->text();
            if (currentLogs.contains("暂无最近操作记录")) currentLogs = "";
            m_lblHistory->setText(logMsg + currentLogs);
        }

        updateTableData();
    }
}
void ServiceManagementModule::onToggleServiceStatus()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString id = btn->property("serviceId").toString();
    ServiceInfo info = ServiceDataManager::instance()->getService(id);
    
    // 切换状态
    info.isActive = !info.isActive;
    
    // 保存并刷新
    ServiceDataManager::instance()->updateService(info);
    updateTableData();
}
