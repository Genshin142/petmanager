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
        painter->fillRect(opt.rect, Qt::white);

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

        // 文本绘制
        painter->setPen(QColor((opt.state & QStyle::State_Selected) ? "#1e40af" : "#334155"));
        QFont font = painter->font();
        font.setWeight(QFont::Bold);
        font.setPointSize(10);
        painter->setFont(font);

        QRect textRect = opt.rect.adjusted(10, 0, -10, 0);
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
    leftLayout->setSpacing(25);

    // Module Title & Summary Stats
    QFrame *topCard = new QFrame();
    topCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    QVBoxLayout *topLayout = new QVBoxLayout(topCard);
    topLayout->setContentsMargins(25, 15, 25, 15);
    topLayout->setSpacing(12);

    QLabel *title = new QLabel("服务项目管理系统");
    title->setStyleSheet("font-size: 22px; font-weight: bold; color: #1a1b1e; border: none;");
    topLayout->addWidget(title);

    QHBoxLayout *dashLayout = new QHBoxLayout();
    dashLayout->setSpacing(15);
    auto createStatCard = [&](const QString &title, const QString &val, const QString &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(75);
        card->setStyleSheet("QFrame { background: #fcfcfd; border-radius: 8px; border: 1px solid #f1f5f9; }");
        QVBoxLayout *l = new QVBoxLayout(card);
        l->setContentsMargins(15, 10, 15, 10);
        QLabel *t = new QLabel(title);
        t->setStyleSheet("color: #64748b; font-size: 11px; font-weight: bold; border: none;");
        QLabel *v = new QLabel(val);
        v->setStyleSheet(QString("font-size: 20px; font-weight: bold; color: %1; border: none;").arg(color));
        l->addWidget(t);
        l->addWidget(v);
        return card;
    };
    dashLayout->addWidget(createStatCard("服务项总数", "24", "#3b82f6"));
    dashLayout->addWidget(createStatCard("本月热门", "精油SPA", "#1e293b"));
    dashLayout->addWidget(createStatCard("营收预估", "¥ 4,280", "#22c55e"));
    dashLayout->addWidget(createStatCard("待结算提成", "¥ 1,250", "#f59e0b"));
    topLayout->addLayout(dashLayout);
    leftLayout->addWidget(topCard);

    // Master Table Area (with Toolbar)
    QFrame *tableCard = new QFrame();
    tableCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);
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
            "QPushButton { background: #3b82f6; color: white; border-radius: 8px; font-size: 13px; padding: 0 20px; border: none; }"
            "QPushButton:hover { background: #2563eb; }"
        );
        addBtn->setCursor(Qt::PointingHandCursor); // 设置手指样式
        connect(addBtn, &QPushButton::clicked, this, &ServiceManagementModule::onAddService);
        toolbar->addWidget(addBtn);
    }
    tableLayout->addWidget(toolbarWidget);

    m_serviceTable = new QTableWidget();
    m_serviceTable->setColumnCount(8);
    m_serviceTable->setHorizontalHeaderLabels({"服务编码", "服务名称", "分类", "标准时长", "服务价格", "累计销量", "状态", "操作"});
    m_serviceTable->setItemDelegate(new ServiceRowDelegate(m_serviceTable));
    
    // 精确控制列宽对齐
    m_serviceTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_serviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_serviceTable->setColumnWidth(0, 130); // 编码
    m_serviceTable->setColumnWidth(1, 220); // 名称
    m_serviceTable->setColumnWidth(2, 100); // 分类
    m_serviceTable->setColumnWidth(3, 100); // 时长
    m_serviceTable->setColumnWidth(4, 110); // 价格
    m_serviceTable->setColumnWidth(5, 100); // 销量
    m_serviceTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents); // 状态
    m_serviceTable->horizontalHeader()->setStretchLastSection(true); // 操作列自动填充

    m_serviceTable->verticalHeader()->setVisible(false);
    m_serviceTable->verticalHeader()->setDefaultSectionSize(60); // 统一行高 60px，与订单管理对齐
    m_serviceTable->setShowGrid(false);
    m_serviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_serviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_serviceTable->setStyleSheet(
        "QTableWidget { border: none; background: white; outline: none; } "
        
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

    connect(prevBtn, &QPushButton::clicked, this, &ServiceManagementModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &ServiceManagementModule::onNextPage);

    leftLayout->addWidget(tableCard);

    mainHorizontalLayout->addWidget(leftContent, 1); // 左侧拉伸

    // --- 3. Right Content (Detail Panel, Fixed Width) ---
    setupDetailPanel();
    m_detailPanel->setFixedWidth(450); // 统一宽度为 450px
    mainHorizontalLayout->addWidget(m_detailPanel);

    QTimer::singleShot(0, this, &ServiceManagementModule::updateTableData);
}

void ServiceManagementModule::setupDetailPanel()
{
    m_detailPanel = new QWidget();
    m_detailPanel->setStyleSheet("background-color: white; border-left: 1px solid #f1f5f9;");
    QVBoxLayout *outerLayout = new QVBoxLayout(m_detailPanel);
    outerLayout->setContentsMargins(0, 20, 20, 20); // 顶部保持 20px 对齐

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: white; border: none;");

    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: white;");
    QVBoxLayout *detailLayout = new QVBoxLayout(scrollContent);
    detailLayout->setContentsMargins(25, 25, 25, 25); // 缩小边距
    detailLayout->setSpacing(30);

    // Title
    QHBoxLayout *header = new QHBoxLayout();
    m_lblDetailName = new QLabel("精油SPA洗浴 (大型犬)");
    m_lblDetailName->setStyleSheet("font-size: 28px; font-weight: bold; color: #0f172a;");
    header->addWidget(m_lblDetailName);
    header->addStretch();
    QPushButton *editBtn = new QPushButton("编辑服务");
    editBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border-radius: 8px; padding: 8px 20px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    editBtn->setCursor(Qt::PointingHandCursor); // 设置手指样式
    connect(editBtn, &QPushButton::clicked, this, &ServiceManagementModule::onEditService);
    header->addWidget(editBtn);
    detailLayout->addLayout(header);

    // Stats Grid
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(20);
    auto createStatCard = [&](const QString &title, QLabel* &valLabel) {
        QFrame *card = new QFrame();
        // 增加灰色边框，缩小内边距
        card->setStyleSheet("background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; padding: 12px;");
        QVBoxLayout *l = new QVBoxLayout(card);
        l->setSpacing(4);
        QLabel *t = new QLabel(title);
        // 标题加深，增加对比度
        t->setStyleSheet("color: #334155; font-size: 14px; font-weight: bold; border: none;");
        valLabel = new QLabel("0");
        valLabel->setStyleSheet("color: #0f172a; font-size: 32px; font-weight: bold; margin: 4px 0; border: none;");
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
        QHBoxLayout *l = new QHBoxLayout(w);
        l->setContentsMargins(0, 15, 0, 5);
        l->setSpacing(8);
        
        // 蓝色视觉锚点条
        QFrame *bar = new QFrame();
        bar->setFixedSize(4, 18);
        bar->setStyleSheet("background: #3b82f6; border-radius: 2px;");
        
        QLabel *lbl = new QLabel(text);
        lbl->setStyleSheet("font-size: 17px; font-weight: bold; color: #0f172a; border: none;");
        
        l->addWidget(bar);
        l->addWidget(lbl);
        l->addStretch();
        return w;
    };

    detailLayout->addWidget(createSectionHeader("基础参数"));

    QFrame *baseCard = new QFrame();
    baseCard->setStyleSheet("background: #f8fafc; border-radius: 8px; padding: 10px;");
    QGridLayout *baseGrid = new QGridLayout(baseCard);
    
    m_editPrice = new QLineEdit(); 
    m_editDuration = new QLineEdit();
    m_editCategory = new QLineEdit();
    m_editId = new QLineEdit();
    
    auto addBaseField = [&](const QString &label, QLineEdit* &edit, int r, int c) {
        QVBoxLayout *vl = new QVBoxLayout();
        QLabel *l = new QLabel(label);
        l->setStyleSheet("color: #475569; font-size: 12px; font-weight: bold;"); // 调深标签颜色
        edit = new QLineEdit();
        edit->setReadOnly(true);
        edit->setStyleSheet("background: transparent; border: none; color: #0f172a; font-weight: bold; font-size: 14px;");
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
    commCard->setStyleSheet("background: white; border: none;"); // 去掉边框
    QGridLayout *commGrid = new QGridLayout(commCard);
    commGrid->setContentsMargins(0, 10, 0, 10);
    commGrid->setSpacing(15);
    auto addField = [&](const QString &label, QLineEdit* &edit, int r, int c) {
        QVBoxLayout *l = new QVBoxLayout();
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("color: #475569; font-size: 12px; font-weight: bold; border: none;");
        edit = new QLineEdit();
        edit->setReadOnly(true);
        // 去掉边框，仅保留圆角背景
        edit->setStyleSheet("background: #f8fafc; border: none; border-radius: 8px; padding: 10px; color: #0f172a; font-weight: bold;");
        l->addWidget(lbl);
        l->addWidget(edit);
        commGrid->addLayout(l, r, c);
    };
    addField("固定提成金额 (元)", m_editCommFixed, 0, 0);
    detailLayout->addWidget(commCard);

    // History Section
    detailLayout->addWidget(createSectionHeader("最近操作记录"));
    m_lblHistory = new QLabel();
    m_lblHistory->setWordWrap(true);
    m_lblHistory->setStyleSheet("color: #475569; line-height: 1.8; font-size: 13px;");
    detailLayout->addWidget(m_lblHistory);

    detailLayout->addStretch();
    
    // 将滚动区域加入主面板
    outerLayout->addWidget(scroll);
    scroll->setWidget(scrollContent);
}

void ServiceManagementModule::updateTableData()
{
    m_serviceTable->setRowCount(0);
    QList<ServiceInfo> allServices = ServiceDataManager::instance()->allServices();
    
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

    auto addItem = [&](int row, int col, const QString &text, bool center = true) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        if (center) item->setTextAlignment(Qt::AlignCenter);
        else item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_serviceTable->setItem(row, col, item);
    };

    for (int i = start; i < end; ++i) {
        const auto &info = filtered[i];
        int row = m_serviceTable->rowCount();
        m_serviceTable->insertRow(row);
        m_serviceTable->setRowHeight(row, 50);


        // 1. 服务编码
        QTableWidgetItem *idItem = new QTableWidgetItem("SVR-100" + QString::number(row + 1));
        idItem->setData(Qt::UserRole, info.id);
        idItem->setTextAlignment(Qt::AlignCenter);
        m_serviceTable->setItem(row, 0, idItem);

        // 2. 服务名称
        addItem(row, 1, info.name, true);

        // 3. 所属分类
        addItem(row, 2, info.category);

        // 4. 标准时长
        addItem(row, 3, QString("%1 分钟").arg(info.durationMinutes));

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
    m_editDuration->setText(QString::number(info.durationMinutes) + " 分钟");
    m_editCategory->setText(info.category);
    m_editId->setText(info.id);

    m_editCommFixed->setText(QString::number(info.commissionFixed, 'f', 2));
    
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
