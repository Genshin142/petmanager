#include "statsmodule.h"
#include <memory>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QHeaderView>
#include <QButtonGroup>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QGraphicsLayout>
#include <QDate>
#include <QElapsedTimer>
#include <QGraphicsDropShadowEffect>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <QJsonArray>
#include <QJsonObject>
#include "custommessagedialog.h"
#include "orderdetaildrawer.h"
#include "staffdatamanager.h"
#include "servicedatamanager.h"
#include "productdatamanager.h"
#include "petdatamanager.h"
#include <QJsonDocument>
#include "petdatamanager.h"
#include "productdatamanager.h"
#include <QtCharts/QScatterSeries>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include <QEvent>

// 复刻会员界面的“全行圆角选中”效果
class StatsRowDelegate : public QStyledItemDelegate {
public:
    private:
    mutable int m_hoveredRow = -1;
public:
    StatsRowDelegate(QObject *parent) : QStyledItemDelegate(parent) {
        QAbstractItemView *view = qobject_cast<QAbstractItemView*>(parent);
        if (view) {
            view->setMouseTracking(true);
            connect(view, &QAbstractItemView::entered, this, [this, view](const QModelIndex &index) {
                m_hoveredRow = index.row();
                view->viewport()->update();
            });
            view->viewport()->installEventFilter(this);
        }
    }
    
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Leave) {
            m_hoveredRow = -1;
            QAbstractItemView *view = qobject_cast<QAbstractItemView*>(parent());
            if (view) {
                view->viewport()->update();
            }
        }
        return QStyledItemDelegate::eventFilter(watched, event);
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (index.row() == m_hoveredRow) {
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

        painter->setPen(QColor((opt.state & QStyle::State_Selected) ? "#1e40af" : "#303133"));
        QFont font = painter->font();
        font.setWeight(opt.state & QStyle::State_Selected ? QFont::Bold : QFont::Normal);
        font.setPointSize(10);
        painter->setFont(font);
        QRect textRect = opt.rect.adjusted(10, 0, -10, 0);
        painter->drawText(textRect, opt.displayAlignment | Qt::AlignVCenter, opt.text);
        
        painter->restore();
    }
};
#include <QCursor>
#include <QRandomGenerator>

StatsModule::StatsModule(QWidget *parent) : QWidget(parent), m_currentCategory(0) {
    m_currentMonth = QDate::currentDate().toString("yyyy-MM");
    setupUI();
    connect(PetDataManager::instance(), &PetDataManager::dashboardStatsReceived, this, &StatsModule::onDashboardStatsReceived);
    connect(PetDataManager::instance(), &PetDataManager::revenueTrendReceived, this, &StatsModule::onRevenueTrendReceived);
    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, &StatsModule::refreshData);
    refreshData();
}

void StatsModule::setupUI() {
    // 统一设置模块及子组件样式，特别是美化 ToolTip
    this->setStyleSheet(
        "StatsModule { background-color: #f8fafc; } "
        "QToolTip { background-color: #ffffff; color: #1e293b; border: 1px solid #e2e8f0; border-radius: 6px; padding: 8px; font-family: 'Microsoft YaHei'; }"
    );
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 内容区 (高密度布局，确保一屏显示)
    QWidget *contentArea = new QWidget();
    contentArea->setStyleSheet("background: transparent;");
    QHBoxLayout *contentLayout = new QHBoxLayout(contentArea);
    contentLayout->setContentsMargins(15, 15, 15, 15);
    contentLayout->setSpacing(15);

    // 左侧核心区
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(15);

    // 1. 统一控制面板 (全扁平化)
    QWidget *topControlPanel = new QWidget();
    topControlPanel->setStyleSheet("background: white; border: none;");
    QVBoxLayout *panelLayout = new QVBoxLayout(topControlPanel);
    panelLayout->setContentsMargins(15, 15, 15, 15);
    panelLayout->setSpacing(12);

    QLabel *title = new QLabel("数据报表统计分析中心");
    title->setStyleSheet("font-size: 18px; font-weight: 800; color: #1e293b; border: none;");
    panelLayout->addWidget(title);

    // --- 4. 灯箱预览系统 ---
    m_backdrop = new QWidget(this);
    m_backdrop->setStyleSheet("background: rgba(0,0,0,180);");
    m_backdrop->hide();
    m_backdrop->installEventFilter(this);

    QVBoxLayout *backLayout = new QVBoxLayout(m_backdrop);
    m_largePreviewLabel = new QLabel();
    m_largePreviewLabel->setFixedSize(200, 200);
    m_largePreviewLabel->setAlignment(Qt::AlignCenter);
    m_largePreviewLabel->setStyleSheet("background: #3b82f6; border-radius: 100px; color: white; font-size: 80px; font-weight: bold; border: 4px solid white;");
    backLayout->addWidget(m_largePreviewLabel, 0, Qt::AlignCenter);

    setupNavigation();
    panelLayout->addWidget(m_navBar);

    // 新增：卡片与扇形图的并列布局 (利用顶部横向空间)
    QHBoxLayout *topSplitLayout = new QHBoxLayout();
    topSplitLayout->setContentsMargins(0, 5, 0, 0);
    topSplitLayout->setSpacing(20);

    setupDashboardCards();
    topSplitLayout->addWidget(m_cardContainer, 3); // 恢复卡片区域权重到 3，确保卡片比例协调

    // 扇形图容器组 (并列显示两个占比图)
    auto createTopPie = [&](const QString &title, QWidget* &container) {
        QWidget *box = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(box);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(2);
        QLabel *l = new QLabel(title);
        l->setStyleSheet("font-size: 16px; font-weight: 800; color: #1e293b; margin-bottom: 8px;"); // 放大标题字体
        vl->addWidget(l);
        
        container = new QWidget();
        container->setFixedHeight(240); // 缩小高度
        container->setStyleSheet("background: transparent; border: none;");
        QVBoxLayout *cl = new QVBoxLayout(container);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(0);
        
        vl->addWidget(container);
        return box;
    };

    m_topPieContainer = new QWidget();
    // 设置占位策略，隐藏时仍然保留布局空间，或者通过比例控制
    m_topPieContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QHBoxLayout *tpcl = new QHBoxLayout(m_topPieContainer);
    tpcl->setContentsMargins(0, 0, 0, 0);
    tpcl->setSpacing(20);

    // 1. 财务占比组 (营收+支付)
    m_finPiesContainer = new QWidget();
    QHBoxLayout *fpl = new QHBoxLayout(m_finPiesContainer);
    fpl->setContentsMargins(0,0,0,0); fpl->setSpacing(20);
    fpl->addWidget(createTopPie("营收项目构成", m_finCompChartContainer), 1);
    fpl->addWidget(createTopPie("支付渠道分布", m_finPayChartContainer), 1);
    tpcl->addWidget(m_finPiesContainer);

    // 2. 商品占比组
    m_prodPieContainer = new QWidget();
    QHBoxLayout *ppl = new QHBoxLayout(m_prodPieContainer);
    ppl->setContentsMargins(0,0,0,0);
    ppl->addWidget(createTopPie("商品销售构成", m_productCategoryChartContainer), 1);
    tpcl->addWidget(m_prodPieContainer);

    // 3. 服务占比组
    m_svcPieContainer = new QWidget();
    QHBoxLayout *spl = new QHBoxLayout(m_svcPieContainer);
    spl->setContentsMargins(0,0,0,0);
    spl->addWidget(createTopPie("服务类目分布", m_serviceHeatmapChartContainer), 1);
    tpcl->addWidget(m_svcPieContainer);

    topSplitLayout->addWidget(m_topPieContainer, 4); // 图表区域维持 4，保持总比例 3:4

    panelLayout->addLayout(topSplitLayout);

    leftLayout->addWidget(topControlPanel);

    setupMainContent();
    leftLayout->addWidget(m_viewStack, 1);

    contentLayout->addWidget(leftPanel, 1);

    // 右侧抽屉
    m_detailDrawer = new OrderDetailDrawer(this);
    m_detailDrawer->hide();
    contentLayout->addWidget(m_detailDrawer);

    mainLayout->addWidget(contentArea, 1);
    
    // 初始化状态 (确保饼图可见性正确)
    onCategoryChanged(0);
}

void StatsModule::setupNavigation() {
    m_navBar = new QWidget();
    m_navBar->setFixedHeight(48);
    m_navBar->setStyleSheet("background: #f1f5f9; border-radius: 10px; border: none;");
    
    QHBoxLayout *navLayout = new QHBoxLayout(m_navBar);
    navLayout->setContentsMargins(10, 0, 10, 5); // 缩小下边距
    navLayout->setSpacing(8);

    QStringList categories = {"财务总览", "店员服务统计", "商品排行", "服务排行"};
    for (int i = 0; i < categories.size(); ++i) {
        QPushButton *btn = new QPushButton(categories[i]);
        btn->setCheckable(true);
        btn->setFixedSize(110, 32);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: transparent; color: #64748b; border-radius: 8px; font-weight: 600; font-size: 14px; border: none; padding: 0 10px; } "
            "QPushButton:checked { background: #3b82f6; color: white; } "
            "QPushButton:hover:!checked { background: #e2e8f0; }"
        );
        if (i == 0) btn->setChecked(true);
        
        connect(btn, &QPushButton::clicked, this, [this, i]() { onCategoryChanged(i); });
        m_navButtons.append(btn);
        navLayout->addWidget(btn);
    }
    navLayout->addStretch();
}

void StatsModule::setupDashboardCards() {
    m_cardContainer = new QWidget();
    QHBoxLayout *cardLayout = new QHBoxLayout(m_cardContainer);
    cardLayout->setContentsMargins(10, 5, 0, 5); // 缩小上下边距
    cardLayout->setSpacing(20);
    cardLayout->setAlignment(Qt::AlignLeft);

    QStringList titles = {"总营收 (REVENUE)", "毛利润 (PROFIT)", "客单价 (AVG ORDER)"};
    m_cardValues.clear();
    m_cardTitles.clear(); // 记录标题标签以便动态修改
    m_cardTrends.clear();

    for (int i = 0; i < 3; ++i) {
        QWidget *card = new QWidget();
        card->setFixedSize(190, 220);
        card->setStyleSheet("background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 12px;");
        
        QVBoxLayout *vl = new QVBoxLayout(card);
        vl->setContentsMargins(15, 15, 15, 15);
        vl->setSpacing(8);
        // 不再整体垂直居中，让内容从顶部开始排布
        vl->setAlignment(Qt::AlignTop);

        QLabel *titleLabel = new QLabel(titles[i]);
        titleLabel->setStyleSheet("color: #475569; font-size: 14px; font-weight: 800; text-transform: uppercase; border: none;");
        titleLabel->setAlignment(Qt::AlignLeft);
        vl->addWidget(titleLabel);
        m_cardTitles.append(titleLabel);

        // 添加一个弹性间距，让数值垂直居中
        vl->addStretch();

        QLabel *valLabel = new QLabel("--");
        valLabel->setStyleSheet("color: #0f172a; font-size: 28px; font-weight: 900; border: none;");
        valLabel->setAlignment(Qt::AlignLeft); // 数值也左对齐
        vl->addWidget(valLabel);
        m_cardValues.append(valLabel);

        QLabel *trendLabel = new QLabel("--");
        trendLabel->setStyleSheet("font-size: 13px; font-weight: 600; border: none; color: #94a3b8;");
        trendLabel->setAlignment(Qt::AlignLeft);
        vl->addWidget(trendLabel);
        m_cardTrends.append(trendLabel);

        vl->addStretch(); // 底部也留白
        cardLayout->addWidget(card);
    }
}

void StatsModule::setupMainContent() {
    m_viewStack = new QStackedWidget();
    m_viewStack->setStyleSheet("background: transparent;");

    m_viewStack->addWidget(createFinanceView());
    m_viewStack->addWidget(createServiceView());
    m_viewStack->addWidget(createInventoryView());
    m_viewStack->addWidget(createServiceRankView());
}

QWidget* StatsModule::createFinanceView() {
    QWidget *view = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // 1. 核心数据看板 (带图表/表格切换)
    QWidget *mainCard = new QWidget();
    mainCard->setStyleSheet("background: white; border: none; border-radius: 12px;");
    QVBoxLayout *mainVl = new QVBoxLayout(mainCard);
    mainVl->setContentsMargins(20, 15, 20, 20);
    mainVl->setSpacing(15);

    // --- 标题栏 (标题 + 筛选器 + 视图切换) ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("门店经营营收分析");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 800; color: #1e293b;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    // 视图切换按钮 (胶囊样式)
    QWidget *viewToggle = new QWidget();
    viewToggle->setStyleSheet("background: #f1f5f9; border-radius: 8px; padding: 2px;");
    QHBoxLayout *vtl = new QHBoxLayout(viewToggle);
    vtl->setContentsMargins(2, 2, 2, 2); vtl->setSpacing(0);
    
    auto createToggleBtn = [&](const QString &text, bool checked) {
        QPushButton *btn = new QPushButton(text);
        btn->setCheckable(true);
        btn->setChecked(checked);
        btn->setFixedSize(60, 28);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { border: none; border-radius: 6px; color: #64748b; font-size: 12px; font-weight: bold; background: transparent; } "
            "QPushButton:checked { background: white; color: #3b82f6; }"
        );
        return btn;
    };

    QPushButton *btnChart = createToggleBtn("趋势图", true);
    QPushButton *btnData = createToggleBtn("明细表", false);
    QButtonGroup *viewGroup = new QButtonGroup(mainCard);
    viewGroup->addButton(btnChart, 0);
    viewGroup->addButton(btnData, 1);
    viewGroup->setExclusive(true);

    vtl->addWidget(btnChart);
    vtl->addWidget(btnData);
    headerLayout->addWidget(viewToggle);
    headerLayout->addSpacing(20);

    // 筛选器
    m_trendRangeCombo = new QComboBox();
    m_trendRangeCombo->addItems({"历年对比", "年度走势", "本月走势"});
    m_trendRangeCombo->setFixedWidth(100);
    m_yearPicker = new QComboBox();
    for(int y=2026; y>=2022; --y) m_yearPicker->addItem(QString::number(y) + "年");
    m_yearPicker->setFixedWidth(85);
    m_monthPicker = new QComboBox();
    for(int m=1; m<=12; ++m) m_monthPicker->addItem(QString::number(m) + "月");
    m_monthPicker->setFixedWidth(70);

    QString comboStyle = "QComboBox { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 6px; padding: 4px 8px; color: #475569; font-size: 12px; font-weight: 600; } "
                         "QComboBox::drop-down { border: none; } QComboBox::down-arrow { image: none; border-left: 4px solid transparent; border-right: 4px solid transparent; border-top: 4px solid #64748b; margin-right: 4px; }";
    m_trendRangeCombo->setStyleSheet(comboStyle);
    m_yearPicker->setStyleSheet(comboStyle);
    m_monthPicker->setStyleSheet(comboStyle);

    headerLayout->addWidget(m_trendRangeCombo);
    headerLayout->addWidget(m_yearPicker);
    headerLayout->addWidget(m_monthPicker);
    mainVl->addLayout(headerLayout);

    // --- 内容区 (StackedWidget) ---
    m_financeMainStack = new QStackedWidget();
    
    // Page 0: Chart
    m_finTrendChartContainer = new QWidget();
    m_finTrendChartContainer->setMinimumHeight(400);
    QVBoxLayout *trendL = new QVBoxLayout(m_finTrendChartContainer);
    trendL->setContentsMargins(0, 0, 0, 0);
    m_financeMainStack->addWidget(m_finTrendChartContainer);

    // Page 1: Table Panel
    QWidget *tablePanel = new QWidget();
    QVBoxLayout *tpvl = new QVBoxLayout(tablePanel);
    tpvl->setContentsMargins(0, 0, 0, 0);

    m_dailyRevenueTable = new QTableWidget();
    m_dailyRevenueTable->setMinimumHeight(500); 
    m_dailyRevenueTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_dailyRevenueTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_dailyRevenueTable->setColumnCount(7);
    m_dailyRevenueTable->setHorizontalHeaderLabels({"日期", "总营收", "服务收入", "商品收入", "寄养收入", "订单量", "客单价"});
    m_dailyRevenueTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_dailyRevenueTable->verticalHeader()->setVisible(false);
    m_dailyRevenueTable->verticalHeader()->setDefaultSectionSize(48);
    m_dailyRevenueTable->setShowGrid(false);
    m_dailyRevenueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dailyRevenueTable->setItemDelegate(new StatsRowDelegate(m_dailyRevenueTable));
    m_dailyRevenueTable->setStyleSheet("QTableWidget { border: none; background: transparent; } "
                                      "QHeaderView::section { background: white; color: #64748b; font-weight: bold; border: none; height: 40px; }");
    tpvl->addWidget(m_dailyRevenueTable);

    // 分页器
    QWidget *pager = new QWidget();
    QHBoxLayout *phl = new QHBoxLayout(pager);
    phl->setContentsMargins(0, 10, 0, 0);
    m_dailyRevPageLabel = new QLabel("第 1 页 / 共 1 页");
    m_dailyRevPageLabel->setStyleSheet("color: #64748b; font-size: 12px; font-weight: 600; border: none;");

    auto createPageBtn = [&](const QString &t, int delta) {
        QPushButton *b = new QPushButton();
        b->setFixedSize(60, 28); b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet("QPushButton { background: #f1f5f9; border: none; border-radius: 6px; } QPushButton:hover { background: #e2e8f0; }");
        QLabel *l = new QLabel(t, b); l->setAlignment(Qt::AlignCenter); l->setAttribute(Qt::WA_TransparentForMouseEvents);
        l->setStyleSheet("color: #475569; font-size: 12px; font-weight: bold; border: none; background: transparent;");
        QVBoxLayout *bl = new QVBoxLayout(b); bl->setContentsMargins(0, 0, 0, 0); bl->addWidget(l);
        connect(b, &QPushButton::clicked, this, [=](){ goToDailyRevPage(delta); });
        return b;
    };
    phl->addStretch(); phl->addWidget(createPageBtn("上一页", -1)); phl->addWidget(m_dailyRevPageLabel); phl->addWidget(createPageBtn("下一页", 1));
    tpvl->addWidget(pager);

    m_financeMainStack->addWidget(tablePanel);
    mainVl->addWidget(m_financeMainStack, 1);

    layout->addWidget(mainCard, 1);

    // 逻辑连接
    connect(viewGroup, QOverload<int>::of(&QButtonGroup::idClicked), m_financeMainStack, &QStackedWidget::setCurrentIndex);
    
    auto updatePickerVisibility = [this]() {
        int idx = m_trendRangeCombo->currentIndex();
        m_yearPicker->setVisible(idx == 1 || idx == 2);
        m_monthPicker->setVisible(idx == 2);
    };
    connect(m_trendRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ 
        updatePickerVisibility(); 
        refreshData(); 
    });
    
    connect(PetDataManager::instance(), &PetDataManager::dashboardStatsReceived, this, &StatsModule::onDashboardStatsReceived);
    connect(PetDataManager::instance(), &PetDataManager::revenueTrendReceived, this, &StatsModule::onRevenueTrendReceived);
    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, &StatsModule::refreshData);

    connect(m_yearPicker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatsModule::refreshData);
    connect(m_monthPicker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatsModule::refreshData);
    
    updatePickerVisibility();
    return view;
}

QWidget* StatsModule::createServiceView() {
    QWidget *view = new QWidget();
    QVBoxLayout *mainVl = new QVBoxLayout(view);
    mainVl->setContentsMargins(0, 0, 0, 0);
    mainVl->setSpacing(15);

    // 1. 全局筛选工具栏
    QWidget *filterBar = new QWidget();
    filterBar->setFixedHeight(50);
    filterBar->setStyleSheet("background: white; border: 1px solid #e2e8f0; border-radius: 12px;");
    QHBoxLayout *fhl = new QHBoxLayout(filterBar);
    fhl->setContentsMargins(15, 0, 15, 0);
    fhl->setSpacing(10);

    QLabel *fl = new QLabel(QString::fromUtf8("统计周期："));
    fl->setStyleSheet("font-weight: bold; color: #64748b; font-size: 13px; border: none;");
    fhl->addWidget(fl);

    // 1. 左侧快捷筛选组 (复刻员工页独立圆角标签按钮)
    QWidget *timeGroup = new QWidget();
    timeGroup->setStyleSheet("border: none; background: transparent;"); // 显式删除容器边框
    QHBoxLayout *tgl = new QHBoxLayout(timeGroup);
    tgl->setContentsMargins(0, 0, 0, 0);
    tgl->setSpacing(8);

    QStringList periods = {QString::fromUtf8("今日"), QString::fromUtf8("昨日"), 
                          QString::fromUtf8("本月"), QString::fromUtf8("上月"), 
                          QString::fromUtf8("今年")};
    QButtonGroup *tg = new QButtonGroup(timeGroup);
    tg->setExclusive(true);

    // 用于刷新标题的函数指针容器 (使用 shared_ptr 避免闭包引用悬挂导致崩溃)
    auto titleUpdaters = std::make_shared<QList<std::function<void(int)>>>();

    for (int i = 0; i < periods.size(); ++i) {
        QPushButton *btn = new QPushButton(periods[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
        if (i == 2) btn->setChecked(true);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );
        tg->addButton(btn, i);
        tgl->addWidget(btn);
    }
    fhl->addWidget(timeGroup);

    // 2. 优雅分割线
    fhl->addSpacing(15);
    QFrame *sep = new QFrame();
    sep->setFixedSize(1, 20);
    sep->setStyleSheet("background-color: #e2e8f0;");
    fhl->addWidget(sep);
    fhl->addSpacing(15);

    // 3. 右侧历史选择组 (胶囊化)
    QFrame *historyGroup = new QFrame();
    historyGroup->setObjectName("HistoryGroup");
    historyGroup->setAttribute(Qt::WA_StyledBackground);
    QHBoxLayout *hgl = new QHBoxLayout(historyGroup);
    hgl->setContentsMargins(12, 4, 4, 4);
    hgl->setSpacing(8);
    historyGroup->setStyleSheet("border: none; background: transparent;"); // 显式删除容器边框

    QLabel *hl = new QLabel(QString::fromUtf8("历史查看"));
    hl->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; border: none; background: transparent;");
    hgl->addWidget(hl);

    QString comboStyle = 
        "QComboBox { "
        "  background: white; "
        "  border: 1px solid #e2e8f0; "
        "  border-radius: 6px; "
        "  padding: 4px 10px; "
        "  color: #1e293b; "
        "  font-size: 13px; "
        "  font-weight: bold; "
        "  min-width: 85px; "
        "  height: 30px; "
        "} "
        "QComboBox:hover { border-color: #3b82f6; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QAbstractItemView { border: 1px solid #e2e8f0; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; border-radius: 6px; }";

    // 年份 ComboBox
    QComboBox *yearCombo = new QComboBox();
    for(int y=2023; y<=2026; ++y) yearCombo->addItem(QString::number(y) + QString::fromUtf8("年"), y);
    yearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + QString::fromUtf8("年"));
    yearCombo->setStyleSheet(comboStyle);
    
    // 月份 ComboBox (增加"全部月份"选项)
    QComboBox *monthCombo = new QComboBox();
    monthCombo->addItem(QString::fromUtf8("全部月份"), 0);
    for(int m=1; m<=12; ++m) monthCombo->addItem(QString::number(m) + QString::fromUtf8("月"), m);
    monthCombo->setCurrentIndex(QDate::currentDate().month()); // 偏移1因为第0项是"全部月份"
    monthCombo->setStyleSheet(comboStyle);

    // 日期 ComboBox (增加"全部日期"选项 + 1~N日)
    QComboBox *dayCombo = new QComboBox();
    dayCombo->setStyleSheet(comboStyle);

    // 动态刷新日期 ComboBox 的天数
    auto refreshDayCombo = [=]() {
        int selMonth = monthCombo->currentData().toInt();
        dayCombo->blockSignals(true);
        dayCombo->clear();
        if (selMonth == 0) {
            // 全部月份 -> 隐藏日期选择
            dayCombo->setVisible(false);
        } else {
            dayCombo->setVisible(true);
            dayCombo->addItem(QString::fromUtf8("全部日期"), 0);
            int selYear = yearCombo->currentData().toInt();
            int daysInMonth = QDate(selYear, selMonth, 1).daysInMonth();
            for (int d = 1; d <= daysInMonth; ++d)
                dayCombo->addItem(QString::number(d) + QString::fromUtf8("日"), d);
            dayCombo->setCurrentIndex(0); // 默认"全部日期"
        }
        dayCombo->blockSignals(false);
    };
    refreshDayCombo(); // 初始化
    
    hgl->addWidget(yearCombo);
    hgl->addWidget(monthCombo);
    hgl->addWidget(dayCombo);
    fhl->addWidget(historyGroup);

    fhl->addStretch();

    auto refreshByHistory = [=](){
        // 如果快捷按钮有选中的，先取消它
        if(tg->checkedButton()) {
            tg->setExclusive(false);
            tg->checkedButton()->setChecked(false);
            tg->setExclusive(true);
        }
        
        // 根据选择粒度调整倍率
        int selMonth = monthCombo->currentData().toInt();
        int selDay = dayCombo->isVisible() ? dayCombo->currentData().toInt() : 0;
        if (selMonth == 0) {
            // 查看整年
            m_staffTimeRange = 2;
            m_serviceTimeRange = 2;
        } else if (selDay == 0) {
            // 查看整月
            m_staffTimeRange = 1;
            m_serviceTimeRange = 1;
        } else {
            // 查看某一天
            m_staffTimeRange = 0;
            m_serviceTimeRange = 0;
        }
        m_staffPage = 0; m_servicePage = 0;
        for(auto &updater : *titleUpdaters) updater(-1); // 特殊信号告知使用自定义标题
        updateServiceAnalysis(); 
    };

    // 年份变化 -> 刷新日期天数 -> 刷新数据
    connect(yearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){
        refreshDayCombo();
        refreshByHistory();
    });
    // 月份变化 -> 刷新日期天数 -> 刷新数据
    connect(monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){
        refreshDayCombo();
        refreshByHistory();
    });
    // 日期变化 -> 刷新数据
    connect(dayCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ refreshByHistory(); });

    // 导出按钮已按需移除
    mainVl->addWidget(filterBar);

    // 2. 排行榜主布局
    QHBoxLayout *rankLayout = new QHBoxLayout();
    rankLayout->setSpacing(15);
    mainVl->addLayout(rankLayout);

    auto createRankPanel = [&](const QString &baseTitle, QTableWidget* &table, const QStringList &headers, bool isStaff) {
        QWidget *mainWrapper = new QWidget();
        QVBoxLayout *ovl = new QVBoxLayout(mainWrapper);
        ovl->setContentsMargins(0, 0, 0, 0);
        ovl->setSpacing(10);

        QHBoxLayout *thl = new QHBoxLayout();
        thl->setContentsMargins(5, 0, 5, 0);
        QLabel *l = new QLabel(baseTitle + QString::fromUtf8(" (本月)"));
        l->setStyleSheet("font-size: 15px; font-weight: 700; color: #1e293b; border: none;");
        thl->addWidget(l);
        thl->addStretch();

        // 排序维度切换
        auto createSortToggle = [&]() {
            QFrame *toggleGroup = new QFrame();
            toggleGroup->setObjectName("SortToggle");
            toggleGroup->setFixedSize(130, 28);
            toggleGroup->setStyleSheet(
                "QFrame#SortToggle { background-color: white; border: 1px solid #e2e8f0; border-radius: 14px; }"
                "QPushButton { border: none; background: transparent; color: #64748b; font-size: 11px; font-weight: bold; padding: 0px; outline: none; } "
                "QPushButton:checked { color: #3b82f6; }"
            );
            QHBoxLayout *stgl = new QHBoxLayout(toggleGroup);
            stgl->setContentsMargins(8, 0, 8, 0);
            stgl->setSpacing(8);

            auto btnRev = new QPushButton(QString::fromUtf8("按营收"), toggleGroup);
            auto btnCount = new QPushButton(QString::fromUtf8("按单量"), toggleGroup);
            btnRev->setCheckable(true); btnCount->setCheckable(true);
            btnRev->setChecked(true); 
            btnRev->setCursor(Qt::PointingHandCursor);
            btnCount->setCursor(Qt::PointingHandCursor);

            QButtonGroup *group = new QButtonGroup(toggleGroup);
            group->addButton(btnRev); 
            group->addButton(btnCount); // group handles exclusive toggle
            
            stgl->addWidget(btnRev);
            stgl->addWidget(btnCount);

            connect(btnRev, &QPushButton::clicked, this, [=](){
                if (isStaff) m_staffSortByRev = true; else m_serviceSortByRev = true;
                if (isStaff) updateStaffTable(); else updateServiceTable();
            });
            connect(btnCount, &QPushButton::clicked, this, [=](){
                if (isStaff) m_staffSortByRev = false; else m_serviceSortByRev = false;
                if (isStaff) updateStaffTable(); else updateServiceTable();
            });
            return toggleGroup;
        };

        thl->addWidget(createSortToggle());
        ovl->addLayout(thl);

        // 注册标题更新逻辑
        titleUpdaters->append([=](int id){
            QString suffix = QString::fromUtf8(" (本月)");
            if(id == 0) suffix = QString::fromUtf8(" (今日)");
            else if(id == 1) suffix = QString::fromUtf8(" (昨日)");
            else if(id == 3) suffix = QString::fromUtf8(" (上月)");
            else if(id == 4) suffix = QString::fromUtf8(" (今年)");
            else if(id == -1) {
                int selYear = yearCombo->currentData().toInt();
                int selMonth = monthCombo->currentData().toInt();
                int selDay = dayCombo->isVisible() ? dayCombo->currentData().toInt() : 0;
                if (selMonth == 0) {
                    suffix = QString(" (%1%2)").arg(selYear).arg(QString::fromUtf8("年"));
                } else if (selDay == 0) {
                    suffix = QString(" (%1-%2)").arg(selYear).arg(selMonth, 2, 10, QChar('0'));
                } else {
                    suffix = QString(" (%1-%2-%3)").arg(selYear).arg(selMonth, 2, 10, QChar('0')).arg(selDay, 2, 10, QChar('0'));
                }
            }
            l->setText(baseTitle + suffix);
        });

        // 内部内容盒子
        QWidget *contentBox = new QWidget();
        contentBox->setObjectName("RankBox");
        contentBox->setStyleSheet("QWidget#RankBox { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
        QVBoxLayout *cvl = new QVBoxLayout(contentBox);
        cvl->setContentsMargins(0, 0, 0, 8);
        cvl->setSpacing(0);

        table = new QTableWidget();
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->verticalHeader()->setVisible(false);
        table->verticalHeader()->setDefaultSectionSize(48);
        table->setShowGrid(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setFocusPolicy(Qt::NoFocus);
        table->setItemDelegate(new StatsRowDelegate(table));
        table->setStyleSheet("QTableWidget { border: none; background: transparent; outline: none; } "
                             "QHeaderView::section { background: white; color: #475569; font-weight: bold; border: none; border-bottom: 1px solid #f1f5f9; height: 40px; text-align: center; } "
                             "QHeaderView::section:horizontal:first { border-top-left-radius: 11px; } "
                             "QHeaderView::section:horizontal:last { border-top-right-radius: 11px; } "
                             "QScrollBar:vertical { background: transparent; width: 8px; margin: 0px; } "
                             "QScrollBar::handle:vertical { background: #cbd5e1; min-height: 30px; border-radius: 4px; } "
                             "QScrollBar::handle:vertical:hover { background: #94a3b8; } "
                             "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } "
                             "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
        cvl->addWidget(table, 1);

        // 分页条
        QWidget *pager = new QWidget();
        pager->setStyleSheet("border: none; background: transparent;");
        QHBoxLayout *phl = new QHBoxLayout(pager);
        phl->setContentsMargins(0, 5, 10, 5);
        phl->setSpacing(15);
        phl->addStretch();

        auto createPageBtn = [&](const QString &txt, int delta) {
            QPushButton *b = new QPushButton();
            b->setFixedSize(60, 28);
            b->setStyleSheet("QPushButton { border: 1px solid #e2e8f0; border-radius: 6px; background: white; } QPushButton:hover { border-color: #3b82f6; background: #f8fafc; }");
            QLabel *textLbl = new QLabel(txt, b);
            textLbl->setAlignment(Qt::AlignCenter);
            textLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
            textLbl->setStyleSheet("color: #64748b; font-size: 12px; border: none; background: transparent;");
            QVBoxLayout *bl = new QVBoxLayout(b);
            bl->setContentsMargins(0, 0, 0, 0);
            bl->addWidget(textLbl);
            connect(b, &QPushButton::clicked, this, [=](){
                if (isStaff) goToStaffPage(delta); else goToServicePage(delta);
            });
            return b;
        };

        QLabel* &pLabel = isStaff ? m_staffPageLabel : m_servicePageLabel;
        pLabel = new QLabel(QString::fromUtf8("第 1 页 / 共 1 页"));
        pLabel->setStyleSheet("color: #475569; font-size: 13px; border: none;");

        phl->addWidget(createPageBtn(QString::fromUtf8("上一页"), -1));
        phl->addWidget(pLabel);
        phl->addWidget(createPageBtn(QString::fromUtf8("下一页"), 1));
        cvl->addWidget(pager);

        ovl->addWidget(contentBox);
        return mainWrapper;
    };

    rankLayout->addWidget(createRankPanel(QString::fromUtf8("店员绩效排行汇总"), m_staffRankTable, {"头像", "ID", "姓名", "岗位", "单量", "营收"}, true), 1);

    // 全局信号连接
    connect(tg, &QButtonGroup::idClicked, this, [=](int id){
        // 点击快捷按钮时，重置下拉框到当前年月（避免显示冲突）
        yearCombo->blockSignals(true);
        monthCombo->blockSignals(true);
        dayCombo->blockSignals(true);
        yearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + QString::fromUtf8("年"));
        monthCombo->setCurrentIndex(QDate::currentDate().month()); // 偏移1因为第0项是"全部月份"
        refreshDayCombo();
        yearCombo->blockSignals(false);
        monthCombo->blockSignals(false);
        dayCombo->blockSignals(false);

        // 映射 id 到 range (0:今日, 1:昨日, 2:本月, 3:上月, 4:今年)
        int rangeMap[] = {0, 0, 1, 1, 2}; 
        m_staffTimeRange = rangeMap[id];
        m_serviceTimeRange = rangeMap[id];
        m_staffPage = 0;
        m_servicePage = 0;
        
        for(auto &updater : *titleUpdaters) updater(id);
        updateServiceAnalysis();
    });

    return view;
}

void StatsModule::goToStaffPage(int delta) {
    int maxPage = (m_allStaffData.size() + 9) / 10;
    int next = m_staffPage + delta;
    if (next >= 0 && next < maxPage) {
        m_staffPage = next;
        updateStaffTable();
    }
}

void StatsModule::goToServicePage(int delta) {
    int maxPage = (m_allServiceData.size() + 9) / 10;
    int next = m_servicePage + delta;
    if (next >= 0 && next < maxPage) {
        m_servicePage = next;
        updateServiceTable();
    }
}

QWidget* StatsModule::createServiceRankView() {
    QWidget *view = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(15);

    // 1. 全局筛选工具栏 (复刻自商品统计)
    QWidget *filterBar = new QWidget();
    filterBar->setFixedHeight(50);
    filterBar->setStyleSheet("background: white; border: 1px solid #e2e8f0; border-radius: 12px;");
    QHBoxLayout *fhl = new QHBoxLayout(filterBar);
    fhl->setContentsMargins(15, 0, 15, 0);
    fhl->setSpacing(10);

    QLabel *fl = new QLabel(QString::fromUtf8("统计周期："));
    fl->setStyleSheet("font-weight: bold; color: #64748b; font-size: 13px; border: none;");
    fhl->addWidget(fl);

    QWidget *timeGroup = new QWidget();
    timeGroup->setStyleSheet("border: none; background: transparent;");
    QHBoxLayout *tgl = new QHBoxLayout(timeGroup);
    tgl->setContentsMargins(0, 0, 0, 0);
    tgl->setSpacing(8);

    QStringList periods = {QString::fromUtf8("今日"), QString::fromUtf8("昨日"), 
                          QString::fromUtf8("本月"), QString::fromUtf8("上月"), 
                          QString::fromUtf8("今年")};
    QButtonGroup *tg = new QButtonGroup(timeGroup);
    tg->setExclusive(true);

    auto titleUpdaters = std::make_shared<QList<std::function<void(int)>>>();

    for (int i = 0; i < periods.size(); ++i) {
        QPushButton *btn = new QPushButton(periods[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
        if (i == 2) btn->setChecked(true);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );
        tg->addButton(btn, i);
        tgl->addWidget(btn);
    }
    fhl->addWidget(timeGroup);

    fhl->addSpacing(15);
    QFrame *sep = new QFrame();
    sep->setFixedSize(1, 20);
    sep->setStyleSheet("background-color: #e2e8f0;");
    fhl->addWidget(sep);
    fhl->addSpacing(15);

    QFrame *historyGroup = new QFrame();
    historyGroup->setObjectName("HistoryGroup");
    historyGroup->setAttribute(Qt::WA_StyledBackground);
    QHBoxLayout *hgl = new QHBoxLayout(historyGroup);
    hgl->setContentsMargins(12, 4, 4, 4);
    hgl->setSpacing(8);
    historyGroup->setStyleSheet("border: none; background: transparent;");

    QLabel *hl = new QLabel(QString::fromUtf8("历史查看"));
    hl->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; border: none; background: transparent;");
    hgl->addWidget(hl);

    QString comboStyle = 
        "QComboBox { background: white; border: 1px solid #e2e8f0; border-radius: 6px; padding: 4px 10px; color: #1e293b; font-size: 13px; font-weight: bold; min-width: 85px; height: 30px; } "
        "QComboBox:hover { border-color: #3b82f6; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QAbstractItemView { border: 1px solid #e2e8f0; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; border-radius: 6px; }";

    QComboBox *yearCombo = new QComboBox();
    for(int y=2023; y<=2026; ++y) yearCombo->addItem(QString::number(y) + QString::fromUtf8("年"), y);
    yearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + QString::fromUtf8("年"));
    yearCombo->setStyleSheet(comboStyle);
    
    QComboBox *monthCombo = new QComboBox();
    monthCombo->addItem(QString::fromUtf8("全部月份"), 0);
    for(int m=1; m<=12; ++m) monthCombo->addItem(QString::number(m) + QString::fromUtf8("月"), m);
    monthCombo->setCurrentIndex(QDate::currentDate().month());
    monthCombo->setStyleSheet(comboStyle);

    QComboBox *dayCombo = new QComboBox();
    dayCombo->setStyleSheet(comboStyle);

    auto refreshDayCombo = [=]() {
        int selMonth = monthCombo->currentData().toInt();
        dayCombo->blockSignals(true);
        dayCombo->clear();
        if (selMonth == 0) {
            dayCombo->setVisible(false);
        } else {
            dayCombo->setVisible(true);
            dayCombo->addItem(QString::fromUtf8("全部日期"), 0);
            int selYear = yearCombo->currentData().toInt();
            int daysInMonth = QDate(selYear, selMonth, 1).daysInMonth();
            for (int d = 1; d <= daysInMonth; ++d)
                dayCombo->addItem(QString::number(d) + QString::fromUtf8("日"), d);
            dayCombo->setCurrentIndex(0);
        }
        dayCombo->blockSignals(false);
    };
    refreshDayCombo();
    
    hgl->addWidget(yearCombo);
    hgl->addWidget(monthCombo);
    hgl->addWidget(dayCombo);
    fhl->addWidget(historyGroup);
    fhl->addStretch();

    auto refreshByHistory = [=](){
        if(tg->checkedButton()) {
            tg->setExclusive(false);
            tg->checkedButton()->setChecked(false);
            tg->setExclusive(true);
        }
        int selMonth = monthCombo->currentData().toInt();
        int selDay = dayCombo->isVisible() ? dayCombo->currentData().toInt() : 0;
        if (selMonth == 0) m_serviceRankTimeRange = 2;
        else if (selDay == 0) m_serviceRankTimeRange = 1;
        else m_serviceRankTimeRange = 0;
        
        for(auto &updater : *titleUpdaters) updater(-1);
        updateServiceRankAnalysis(); 
    };

    connect(yearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ refreshDayCombo(); refreshByHistory(); });
    connect(monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ refreshDayCombo(); refreshByHistory(); });
    connect(dayCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ refreshByHistory(); });

    layout->addWidget(filterBar);

    connect(tg, &QButtonGroup::idClicked, this, [=](int id){
        yearCombo->blockSignals(true);
        monthCombo->blockSignals(true);
        dayCombo->blockSignals(true);
        yearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + QString::fromUtf8("年"));
        monthCombo->setCurrentIndex(QDate::currentDate().month());
        refreshDayCombo();
        yearCombo->blockSignals(false);
        monthCombo->blockSignals(false);
        dayCombo->blockSignals(false);

        int rangeMap[] = {0, 0, 1, 1, 2}; 
        m_serviceRankTimeRange = rangeMap[id];
        for(auto &updater : *titleUpdaters) updater(id);
        updateServiceRankAnalysis();
    });

    QHBoxLayout *rankLayout = new QHBoxLayout();
    rankLayout->setSpacing(15);

    auto createRankPanel = [&](const QString &title, QTableWidget* &table, const QStringList &headers, bool isItem) {
        QWidget *mainWrapper = new QWidget();
        QVBoxLayout *ovl = new QVBoxLayout(mainWrapper);
        ovl->setContentsMargins(0,0,0,0);

        QHBoxLayout *hl = new QHBoxLayout();
        hl->setContentsMargins(5, 0, 5, 5); 
        QLabel *tl = new QLabel(title + QString::fromUtf8(" (本月)"));
        tl->setStyleSheet("font-size: 16px; font-weight: 800; color: #1e293b; border: none;");
        hl->addWidget(tl);
        hl->addStretch();

        titleUpdaters->append([=](int id){
            QString suffix = QString::fromUtf8(" (本月)");
            if(id == 0) suffix = QString::fromUtf8(" (今日)");
            else if(id == 1) suffix = QString::fromUtf8(" (昨日)");
            else if(id == 3) suffix = QString::fromUtf8(" (上月)");
            else if(id == 4) suffix = QString::fromUtf8(" (今年)");
            else if(id == -1) {
                int selYear = yearCombo->currentData().toInt();
                int selMonth = monthCombo->currentData().toInt();
                int selDay = dayCombo->isVisible() ? dayCombo->currentData().toInt() : 0;
                if (selMonth == 0) suffix = QString(" (%1%2)").arg(selYear).arg(QString::fromUtf8("年"));
                else if (selDay == 0) suffix = QString(" (%1-%2)").arg(selYear).arg(selMonth, 2, 10, QChar('0'));
                else suffix = QString(" (%1-%2-%3)").arg(selYear).arg(selMonth, 2, 10, QChar('0')).arg(selDay, 2, 10, QChar('0'));
            }
            tl->setText(title + suffix);
        });

        auto createSortToggle = [&]() {
            QFrame *toggleGroup = new QFrame();
            toggleGroup->setObjectName("SortToggle");
            toggleGroup->setFixedSize(130, 28);
            toggleGroup->setStyleSheet(
                "QFrame#SortToggle { background-color: white; border: 1px solid #e2e8f0; border-radius: 14px; }"
                "QPushButton { border: none; background: transparent; color: #64748b; font-size: 11px; font-weight: bold; padding: 0px; outline: none; } "
                "QPushButton:checked { color: #3b82f6; }"
            );
            QHBoxLayout *stgl = new QHBoxLayout(toggleGroup);
            stgl->setContentsMargins(8, 0, 8, 0);
            stgl->setSpacing(8);
            auto btnRev = new QPushButton(QString::fromUtf8("按营收"), toggleGroup);
            auto btnCount = new QPushButton(QString::fromUtf8("按单量"), toggleGroup);
            btnRev->setCheckable(true); btnCount->setCheckable(true);
            btnRev->setChecked(true); 
            btnRev->setCursor(Qt::PointingHandCursor);
            btnCount->setCursor(Qt::PointingHandCursor);
            QButtonGroup *group = new QButtonGroup(toggleGroup);
            group->addButton(btnRev); group->addButton(btnCount);
            stgl->addWidget(btnRev); stgl->addWidget(btnCount);
            connect(btnRev, &QPushButton::clicked, this, [=](){
                if (isItem) m_serviceItemSortByRev = true; else m_serviceCategorySortByRev = true;
                if (isItem) updateServiceItemTable(); else updateServiceCategoryTable();
            });
            connect(btnCount, &QPushButton::clicked, this, [=](){
                if (isItem) m_serviceItemSortByRev = false; else m_serviceCategorySortByRev = false;
                if (isItem) updateServiceItemTable(); else updateServiceCategoryTable();
            });
            return toggleGroup;
        };
        hl->addWidget(createSortToggle());
        ovl->addLayout(hl);

        QWidget *contentBox = new QWidget();
        contentBox->setObjectName("RankBox");
        contentBox->setStyleSheet("QWidget#RankBox { background: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
        QVBoxLayout *cvl = new QVBoxLayout(contentBox);
        cvl->setContentsMargins(15, 10, 15, 15);
        table = new QTableWidget();
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->verticalHeader()->setVisible(false);
        table->verticalHeader()->setDefaultSectionSize(48);
        table->setShowGrid(false);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setFocusPolicy(Qt::NoFocus);
        table->setItemDelegate(new StatsRowDelegate(table));
        table->setStyleSheet("QTableWidget { border: none; background: transparent; } "
                             "QHeaderView::section { background: white; color: #64748b; font-weight: bold; border: none; height: 40px; }");
        cvl->addWidget(table, 1);

        QWidget *pager = new QWidget();
        QHBoxLayout *phl = new QHBoxLayout(pager);
        phl->setContentsMargins(0, 10, 0, 0);
        QLabel *pl = new QLabel("第 1 页 / 共 1 页");
        pl->setStyleSheet("color: #64748b; font-size: 12px; font-weight: 600; border: none;");
        if (isItem) m_serviceItemPageLabel = pl; else m_serviceCategoryPageLabel = pl;
        auto createPageBtn = [&](const QString &t, int delta) {
            QPushButton *b = new QPushButton();
            b->setFixedSize(60, 28);
            b->setCursor(Qt::PointingHandCursor);
            b->setStyleSheet("QPushButton { background: #f1f5f9; border: none; border-radius: 6px; } "
                             "QPushButton:hover { background: #e2e8f0; }");
            QLabel *btnLbl = new QLabel(t, b);
            btnLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
            btnLbl->setAlignment(Qt::AlignCenter);
            btnLbl->setStyleSheet("color: #475569; font-size: 12px; font-weight: bold; border: none; background: transparent;");
            QVBoxLayout *bl = new QVBoxLayout(b);
            bl->setContentsMargins(0, 0, 0, 0);
            bl->addWidget(btnLbl);
            connect(b, &QPushButton::clicked, this, [=](){ if(isItem) goToServiceItemPage(delta); else goToServiceCategoryPage(delta); });
            return b;
        };
        phl->addStretch();
        phl->addWidget(createPageBtn("上一页", -1));
        phl->addWidget(pl);
        phl->addWidget(createPageBtn("下一页", 1));
        phl->setSpacing(15);
        cvl->addWidget(pager);
        ovl->addWidget(contentBox);
        return mainWrapper;
    };

    layout->addLayout(rankLayout);
    rankLayout->addWidget(createRankPanel("服务项目单项排行", m_serviceRankTable, {"服务项目", "销量", "营收"}, true), 1);
    rankLayout->addWidget(createRankPanel("服务类目销售排行", m_serviceCategoryRankTable, {"类目", "销量", "营收"}, false), 1);
    
    return view;
}

QWidget* StatsModule::createInventoryView() {
    QWidget *view = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(15);

    // 1. 全局筛选工具栏 (复刻自服务统计)
    QWidget *filterBar = new QWidget();
    filterBar->setFixedHeight(50);
    filterBar->setStyleSheet("background: white; border: 1px solid #e2e8f0; border-radius: 12px;");
    QHBoxLayout *fhl = new QHBoxLayout(filterBar);
    fhl->setContentsMargins(15, 0, 15, 0);
    fhl->setSpacing(10);

    QLabel *fl = new QLabel(QString::fromUtf8("统计周期："));
    fl->setStyleSheet("font-weight: bold; color: #64748b; font-size: 13px; border: none;");
    fhl->addWidget(fl);

    QWidget *timeGroup = new QWidget();
    timeGroup->setStyleSheet("border: none; background: transparent;");
    QHBoxLayout *tgl = new QHBoxLayout(timeGroup);
    tgl->setContentsMargins(0, 0, 0, 0);
    tgl->setSpacing(8);

    QStringList periods = {QString::fromUtf8("今日"), QString::fromUtf8("昨日"), 
                          QString::fromUtf8("本月"), QString::fromUtf8("上月"), 
                          QString::fromUtf8("今年")};
    QButtonGroup *tg = new QButtonGroup(timeGroup);
    tg->setExclusive(true);

    auto titleUpdaters = std::make_shared<QList<std::function<void(int)>>>();

    for (int i = 0; i < periods.size(); ++i) {
        QPushButton *btn = new QPushButton(periods[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
        if (i == 2) btn->setChecked(true);
        btn->setStyleSheet(
            "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; color: #606266; font-size: 13px; } "
            "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; } "
            "QPushButton:checked { background: #409eff; border-color: #409eff; color: white; font-weight: bold; } "
        );
        tg->addButton(btn, i);
        tgl->addWidget(btn);
    }
    fhl->addWidget(timeGroup);

    fhl->addSpacing(15);
    QFrame *sep = new QFrame();
    sep->setFixedSize(1, 20);
    sep->setStyleSheet("background-color: #e2e8f0;");
    fhl->addWidget(sep);
    fhl->addSpacing(15);

    QFrame *historyGroup = new QFrame();
    historyGroup->setObjectName("HistoryGroup");
    historyGroup->setAttribute(Qt::WA_StyledBackground);
    QHBoxLayout *hgl = new QHBoxLayout(historyGroup);
    hgl->setContentsMargins(12, 4, 4, 4);
    hgl->setSpacing(8);
    historyGroup->setStyleSheet("border: none; background: transparent;");

    QLabel *hl = new QLabel(QString::fromUtf8("历史查看"));
    hl->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; border: none; background: transparent;");
    hgl->addWidget(hl);

    QString comboStyle = 
        "QComboBox { background: white; border: 1px solid #e2e8f0; border-radius: 6px; padding: 4px 10px; color: #1e293b; font-size: 13px; font-weight: bold; min-width: 85px; height: 30px; } "
        "QComboBox:hover { border-color: #3b82f6; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QAbstractItemView { border: 1px solid #e2e8f0; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; border-radius: 6px; }";

    QComboBox *yearCombo = new QComboBox();
    for(int y=2023; y<=2026; ++y) yearCombo->addItem(QString::number(y) + QString::fromUtf8("年"), y);
    yearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + QString::fromUtf8("年"));
    yearCombo->setStyleSheet(comboStyle);
    
    QComboBox *monthCombo = new QComboBox();
    monthCombo->addItem(QString::fromUtf8("全部月份"), 0);
    for(int m=1; m<=12; ++m) monthCombo->addItem(QString::number(m) + QString::fromUtf8("月"), m);
    monthCombo->setCurrentIndex(QDate::currentDate().month());
    monthCombo->setStyleSheet(comboStyle);

    QComboBox *dayCombo = new QComboBox();
    dayCombo->setStyleSheet(comboStyle);

    auto refreshDayCombo = [=]() {
        int selMonth = monthCombo->currentData().toInt();
        dayCombo->blockSignals(true);
        dayCombo->clear();
        if (selMonth == 0) {
            dayCombo->setVisible(false);
        } else {
            dayCombo->setVisible(true);
            dayCombo->addItem(QString::fromUtf8("全部日期"), 0);
            int selYear = yearCombo->currentData().toInt();
            int daysInMonth = QDate(selYear, selMonth, 1).daysInMonth();
            for (int d = 1; d <= daysInMonth; ++d)
                dayCombo->addItem(QString::number(d) + QString::fromUtf8("日"), d);
            dayCombo->setCurrentIndex(0);
        }
        dayCombo->blockSignals(false);
    };
    refreshDayCombo();
    
    hgl->addWidget(yearCombo);
    hgl->addWidget(monthCombo);
    hgl->addWidget(dayCombo);
    fhl->addWidget(historyGroup);
    fhl->addStretch();

    auto refreshByHistory = [=](){
        if(tg->checkedButton()) {
            tg->setExclusive(false);
            tg->checkedButton()->setChecked(false);
            tg->setExclusive(true);
        }
        int selMonth = monthCombo->currentData().toInt();
        int selDay = dayCombo->isVisible() ? dayCombo->currentData().toInt() : 0;
        if (selMonth == 0) m_productTimeRange = 2;
        else if (selDay == 0) m_productTimeRange = 1;
        else m_productTimeRange = 0;
        
        for(auto &updater : *titleUpdaters) updater(-1);
        updateProductAnalysis(); 
    };

    connect(yearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ refreshDayCombo(); refreshByHistory(); });
    connect(monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ refreshDayCombo(); refreshByHistory(); });
    connect(dayCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ refreshByHistory(); });

    layout->addWidget(filterBar);

    connect(tg, &QButtonGroup::idClicked, this, [=](int id){
        yearCombo->blockSignals(true);
        monthCombo->blockSignals(true);
        dayCombo->blockSignals(true);
        yearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + QString::fromUtf8("年"));
        monthCombo->setCurrentIndex(QDate::currentDate().month());
        refreshDayCombo();
        yearCombo->blockSignals(false);
        monthCombo->blockSignals(false);
        dayCombo->blockSignals(false);

        int rangeMap[] = {0, 0, 1, 1, 2}; 
        m_productTimeRange = rangeMap[id];
        for(auto &updater : *titleUpdaters) updater(id);
        updateProductAnalysis();
    });

    QHBoxLayout *rankLayout = new QHBoxLayout();
    rankLayout->setSpacing(15);

    auto createRankPanel = [&](const QString &title, QTableWidget* &table, const QStringList &headers, bool isProduct) {
        QWidget *mainWrapper = new QWidget();
        QVBoxLayout *ovl = new QVBoxLayout(mainWrapper);
        ovl->setContentsMargins(0,0,0,0);

        QHBoxLayout *hl = new QHBoxLayout();
        hl->setContentsMargins(5, 0, 5, 5); // 底部留点间距给下方的盒子
        QLabel *tl = new QLabel(title + QString::fromUtf8(" (本月)"));
        tl->setStyleSheet("font-size: 16px; font-weight: 800; color: #1e293b; border: none;");
        hl->addWidget(tl);
        hl->addStretch();

        // 注册标题更新逻辑
        titleUpdaters->append([=](int id){
            QString suffix = QString::fromUtf8(" (本月)");
            if(id == 0) suffix = QString::fromUtf8(" (今日)");
            else if(id == 1) suffix = QString::fromUtf8(" (昨日)");
            else if(id == 3) suffix = QString::fromUtf8(" (上月)");
            else if(id == 4) suffix = QString::fromUtf8(" (今年)");
            else if(id == -1) {
                int selYear = yearCombo->currentData().toInt();
                int selMonth = monthCombo->currentData().toInt();
                int selDay = dayCombo->isVisible() ? dayCombo->currentData().toInt() : 0;
                if (selMonth == 0) {
                    suffix = QString(" (%1%2)").arg(selYear).arg(QString::fromUtf8("年"));
                } else if (selDay == 0) {
                    suffix = QString(" (%1-%2)").arg(selYear).arg(selMonth, 2, 10, QChar('0'));
                } else {
                    suffix = QString(" (%1-%2-%3)").arg(selYear).arg(selMonth, 2, 10, QChar('0')).arg(selDay, 2, 10, QChar('0'));
                }
            }
            tl->setText(title + suffix);
        });

        auto createSortToggle = [&]() {
            QFrame *toggleGroup = new QFrame();
            toggleGroup->setObjectName("SortToggle");
            toggleGroup->setFixedSize(130, 28);
            toggleGroup->setStyleSheet(
                "QFrame#SortToggle { background-color: white; border: 1px solid #e2e8f0; border-radius: 14px; }"
                "QPushButton { border: none; background: transparent; color: #64748b; font-size: 11px; font-weight: bold; padding: 0px; outline: none; } "
                "QPushButton:checked { color: #3b82f6; }"
            );
            QHBoxLayout *stgl = new QHBoxLayout(toggleGroup);
            stgl->setContentsMargins(8, 0, 8, 0);
            stgl->setSpacing(8);

            auto btnRev = new QPushButton(QString::fromUtf8("按营收"), toggleGroup);
            auto btnCount = new QPushButton(QString::fromUtf8("按单量"), toggleGroup);
            btnRev->setCheckable(true); btnCount->setCheckable(true);
            btnRev->setChecked(true); 
            btnRev->setCursor(Qt::PointingHandCursor);
            btnCount->setCursor(Qt::PointingHandCursor);

            QButtonGroup *group = new QButtonGroup(toggleGroup);
            group->addButton(btnRev); group->addButton(btnCount);
            
            stgl->addWidget(btnRev);
            stgl->addWidget(btnCount);

            connect(btnRev, &QPushButton::clicked, this, [=](){
                if (isProduct) m_productSortByRev = true; else m_categorySortByRev = true;
                if (isProduct) updateProductTable(); else updateCategoryTable();
            });
            connect(btnCount, &QPushButton::clicked, this, [=](){
                if (isProduct) m_productSortByRev = false; else m_categorySortByRev = false;
                if (isProduct) updateProductTable(); else updateCategoryTable();
            });
            return toggleGroup;
        };
        hl->addWidget(createSortToggle());
        
        ovl->addLayout(hl); // 将标题和按钮加入外部布局，位于盒子上方

        QWidget *contentBox = new QWidget();
        contentBox->setObjectName("RankBox");
        contentBox->setStyleSheet("QWidget#RankBox { background: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
        QVBoxLayout *cvl = new QVBoxLayout(contentBox);
        cvl->setContentsMargins(15, 10, 15, 15);

        table = new QTableWidget();
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        if (headers.size() > 0 && headers[0] == QString::fromUtf8("图片")) {
            table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
            table->setColumnWidth(0, 50); // 图片列宽 50px
            for (int k = 1; k < headers.size(); ++k) {
                table->horizontalHeader()->setSectionResizeMode(k, QHeaderView::Stretch);
            }
        } else {
            table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        }
        table->verticalHeader()->setVisible(false);
        table->verticalHeader()->setDefaultSectionSize(48);
        table->setShowGrid(false);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setFocusPolicy(Qt::NoFocus);
        table->setItemDelegate(new StatsRowDelegate(table));
        table->setStyleSheet("QTableWidget { border: none; background: transparent; } "
                             "QHeaderView::section { background: white; color: #64748b; font-weight: bold; border: none; height: 40px; }");
        cvl->addWidget(table, 1);

        QWidget *pager = new QWidget();
        QHBoxLayout *phl = new QHBoxLayout(pager);
        phl->setContentsMargins(0, 10, 0, 0);
        QLabel *pl = new QLabel("第 1 页 / 共 1 页");
        pl->setStyleSheet("color: #64748b; font-size: 12px; font-weight: 600; border: none;");
        if (isProduct) m_productPageLabel = pl; else m_categoryPageLabel = pl;

        auto createPageBtn = [&](const QString &t, int delta) {
            QPushButton *b = new QPushButton();
            b->setFixedSize(60, 28);
            b->setCursor(Qt::PointingHandCursor);
            b->setStyleSheet("QPushButton { background: #f1f5f9; border: none; border-radius: 6px; } "
                             "QPushButton:hover { background: #e2e8f0; }");
            QLabel *btnLbl = new QLabel(t, b);
            btnLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
            btnLbl->setAlignment(Qt::AlignCenter);
            btnLbl->setStyleSheet("color: #475569; font-size: 12px; font-weight: bold; border: none; background: transparent;");
            QVBoxLayout *bl = new QVBoxLayout(b);
            bl->setContentsMargins(0, 0, 0, 0);
            bl->addWidget(btnLbl);
            connect(b, &QPushButton::clicked, this, [=](){ if(isProduct) goToProductPage(delta); else goToCategoryPage(delta); });
            return b;
        };
        phl->addStretch();
        phl->addWidget(createPageBtn("上一页", -1));
        phl->addWidget(pl);
        phl->addWidget(createPageBtn("下一页", 1));
        phl->setSpacing(15);
        cvl->addWidget(pager);
        ovl->addWidget(contentBox);
        return mainWrapper;
    };

    rankLayout->addWidget(createRankPanel("商品销售单品排行", m_productRankTable, {"图片", "商品", "销量", "营收"}, true), 1);
    rankLayout->addWidget(createRankPanel("商品类目销售排行", m_categoryRankTable, {"类目", "销量", "营收"}, false), 1);
    
    layout->addLayout(rankLayout);
    return view;
}


void StatsModule::refreshData() {
    // 1. 请求核心统计指标 (总营收、订单量、饼图分布)
    PetDataManager::instance()->requestDashboardStats();
    
    // 2. 请求趋势图数据
    int rangeIdx = m_trendRangeCombo->currentIndex();
    QString range = (rangeIdx == 0) ? "all_years" : (rangeIdx == 1 ? "year" : "month");
    int selYear = m_yearPicker->currentText().left(4).toInt();
    int selMonth = m_monthPicker->currentIndex() + 1;
    PetDataManager::instance()->requestRevenueTrend(range, selYear, selMonth);

    // 3. 局部列表更新 (暂保持本地过滤)
    if (m_currentCategory == 1) updateServiceAnalysis();
    else if (m_currentCategory == 2) updateProductAnalysis();
    else if (m_currentCategory == 3) updateServiceRankAnalysis();
    
    m_dailyRevPage = 0; 
    updateDailyRevTable();
    updateCards();
}

void StatsModule::updateCharts() {
    if (!m_finTrendChart || m_currentCategory != 0) return;

    // 营收趋势图数据刷新由 onRevenueTrendReceived 驱动
}

void StatsModule::updateServiceAnalysis() {
    if (!m_staffRankTable) return;
    
    // 1. 初始化全量员工列表 (默认值为 0)
    m_allStaffData.clear();
    QList<EmployeeInfo> employees = StaffDataManager::instance()->allStaff();
    QStringList colors = {"#3b82f6", "#8b5cf6", "#10b981", "#f59e0b", "#6366f1", "#ec4899", "#94a3b8"};
    int colorIdx = 0;

    for (const auto &e : employees) {
        if (e.status == "离职") continue;
        m_allStaffData.append({e.name, e.id, e.role, colors[colorIdx % colors.size()], 0, 0.0});
        colorIdx++;
    }

    // 2. 遍历真实订单进行员工统计
    QList<OrderInfo> orders = PetDataManager::instance()->allOrders();
    QDate today = QDate::currentDate();

    for (const auto &order : orders) {
        // 时间筛选
        bool inRange = false;
        QDate orderDate = QDateTime::fromString(order.createTime, "yyyy-MM-dd HH:mm:ss").date();
        if (m_staffTimeRange == 0) inRange = (orderDate == today);
        else if (m_staffTimeRange == 1) inRange = (orderDate.year() == today.year() && orderDate.month() == today.month());
        else if (m_staffTimeRange == 2) inRange = (orderDate.year() == today.year());
        
        if (!inRange || order.status != "Paid") continue;

        // 订单通常会记录执行人或创建人 ID
        // 在本项目中，如果是服务类项目，业绩通常归属于执行人
        // 为了演示，我们假设订单的执行人 ID 存储在某个字段或通过关联查询，
        // 这里我们优先匹配订单关联的员工。
        // 由于 OrderInfo 可能没有直接执行人 ID，我们假设从 itemDetails 中解析或使用创建人。
        
        for (auto &sd : m_allStaffData) {
            // 这里根据业务逻辑匹配：例如订单的创建者或执行者
            // 假设订单 ID 包含某种员工 ID，或者我们在 OrderInfo 中有执行者字段
            // 此处简化处理：统计该员工在时间段内的总成单
            // (实际生产中应根据 sys_performance_records 进行更精确统计)
        }
    }

    // 3. 执行首次分页渲染
    m_staffPage = 0;
    updateStaffTable();
}

void StatsModule::updateStaffTable() {
    // 动态重排全量数据
    std::sort(m_allStaffData.begin(), m_allStaffData.end(), [this](const StaffRankData &a, const StaffRankData &b) {
        if (m_staffSortByRev) return a.rev > b.rev;
        return a.count > b.count;
    });

    m_staffRankTable->setRowCount(0);
    int start = m_staffPage * 10;
    int end = qMin(start + 10, (int)m_allStaffData.size());
    
    auto createCenterItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    for (int i = start; i < end; ++i) {
        const auto &s = m_allStaffData[i];
        int r = m_staffRankTable->rowCount(); m_staffRankTable->insertRow(r);
        
        QLabel *avatar = new QLabel(s.name.left(1));
        avatar->setFixedSize(32, 32);
        avatar->setAlignment(Qt::AlignCenter);
        avatar->setStyleSheet(QString("background: %1; color: white; border-radius: 16px; font-weight: bold; font-size: 13px; border: none;").arg(s.color));
        avatar->setCursor(Qt::PointingHandCursor);
        avatar->setProperty("isAvatar", true);
        avatar->installEventFilter(this);

        QWidget *aw = new QWidget();
        aw->setStyleSheet("background: transparent;");
        QHBoxLayout *al = new QHBoxLayout(aw);
        al->setContentsMargins(0, 0, 0, 0); al->addWidget(avatar, 0, Qt::AlignCenter);
        m_staffRankTable->setCellWidget(r, 0, aw);

        m_staffRankTable->setItem(r, 1, createCenterItem(s.id));
        m_staffRankTable->setItem(r, 2, createCenterItem(s.name));
        m_staffRankTable->setItem(r, 3, createCenterItem(s.pos));
        m_staffRankTable->setItem(r, 4, createCenterItem(QString::number(s.count)));
        m_staffRankTable->setItem(r, 5, createCenterItem(QString("¥ %1").arg(QString::number(s.rev, 'f', 2))));
    }
    
    int maxPage = (m_allStaffData.size() + 9) / 10;
    m_staffPageLabel->setText(QString::fromUtf8("第 %1 页 / 共 %2 页").arg(m_staffPage + 1).arg(qMax(1, maxPage)));
}

void StatsModule::updateServiceTable() {
    // 动态重排全量数据
    std::sort(m_allServiceData.begin(), m_allServiceData.end(), [this](const SvcRankData &a, const SvcRankData &b) {
        if (m_serviceSortByRev) return a.rev > b.rev;
        return a.count > b.count;
    });

    m_serviceRankTable->setRowCount(0);
    int start = m_servicePage * 10;
    int end = qMin(start + 10, (int)m_allServiceData.size());

    auto createCenterItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    for (int i = start; i < end; ++i) {
        const auto &s = m_allServiceData[i];
        int r = m_serviceRankTable->rowCount(); m_serviceRankTable->insertRow(r);
        m_serviceRankTable->setItem(r, 0, createCenterItem(s.name));
        m_serviceRankTable->setItem(r, 1, createCenterItem(QString::number(s.count)));
        m_serviceRankTable->setItem(r, 2, createCenterItem(QString("¥ %1").arg(QString::number(s.rev, 'f', 2))));
    }

    int maxPage = (m_allServiceData.size() + 9) / 10;
    m_servicePageLabel->setText(QString::fromUtf8("第 %1 页 / 共 %2 页").arg(m_servicePage + 1).arg(qMax(1, maxPage)));
}

void StatsModule::updateProductAnalysis() {
    if (!m_productRankTable || !m_categoryRankTable) return;

    // 1. 初始化全量商品列表 (默认值为 0)
    m_allProductData.clear();
    m_allCategoryData.clear();
    
    QList<ProductInfo> products = ProductDataManager::instance()->allProducts();
    QMap<QString, int> categoryIdxMap;
    
    for (const auto &p : products) {
        m_allProductData.append({p.name, 0, 0.0, p.imgData});
        if (!p.category.isEmpty() && !categoryIdxMap.contains(p.category)) {
            categoryIdxMap[p.category] = m_allCategoryData.size();
            m_allCategoryData.append({p.category, 0, 0.0, ""});
        }
    }

    // 2. 遍历真实订单进行统计
    QList<OrderInfo> orders = PetDataManager::instance()->allOrders();
    QDate today = QDate::currentDate();
    
    for (const auto &order : orders) {
        // 时间筛选逻辑
        bool inRange = false;
        QDate orderDate = QDateTime::fromString(order.createTime, "yyyy-MM-dd HH:mm:ss").date();
        if (m_productTimeRange == 0) inRange = (orderDate == today); // 今日
        else if (m_productTimeRange == 1) inRange = (orderDate.year() == today.year() && orderDate.month() == today.month()); // 本月
        else if (m_productTimeRange == 2) inRange = (orderDate.year() == today.year()); // 今年
        
        if (!inRange) continue;
        if (order.status != "Paid") continue;

        // 解析商品详情 (JSON 格式)
        QJsonDocument doc = QJsonDocument::fromJson(order.itemDetails.toUtf8());
        if (doc.isArray()) {
            QJsonArray items = doc.array();
            for (const auto &v : items) {
                QJsonObject item = v.toObject();
                QString name = item["name"].toString();
                int count = item["count"].toInt();
                double price = item["price"].toDouble();
                
                // 累加到商品排行榜
                for (auto &pd : m_allProductData) {
                    if (pd.name == name) {
                        pd.count += count;
                        pd.rev += (double)count * price;
                        break;
                    }
                }
                
                // 累加到类目排行榜
                ProductInfo pInfo = ProductDataManager::instance()->getProductByName(name);
                if (!pInfo.category.isEmpty() && categoryIdxMap.contains(pInfo.category)) {
                    int idx = categoryIdxMap[pInfo.category];
                    m_allCategoryData[idx].count += count;
                    m_allCategoryData[idx].rev += (double)count * price;
                }
            }
        }
    }

    m_productPage = 0;
    m_categoryPage = 0;
    updateProductTable();
    updateCategoryTable();
}

void StatsModule::updateProductTable() {
    std::sort(m_allProductData.begin(), m_allProductData.end(), [this](const ProductRankData &a, const ProductRankData &b) {
        return m_productSortByRev ? (a.rev > b.rev) : (a.count > b.count);
    });

    m_productRankTable->setRowCount(0);
    int start = m_productPage * 9;
    int end = qMin(start + 9, (int)m_allProductData.size());
    for (int i = start; i < end; ++i) {
        const auto &d = m_allProductData[i];
        int r = m_productRankTable->rowCount(); m_productRankTable->insertRow(r);
        
        // 1. 生成并设置可点击缩放的图片列
        QLabel *imgLabel = new QLabel();
        imgLabel->setFixedSize(32, 32);
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setCursor(Qt::PointingHandCursor);
        imgLabel->setProperty("isAvatar", true);
        imgLabel->installEventFilter(this);

        if (!d.imgData.isEmpty()) {
            QByteArray decoded = QByteArray::fromBase64(d.imgData.toUtf8());
            QPixmap pix;
            if (pix.loadFromData(decoded)) {
                QPixmap scaled = pix.scaled(32, 32, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                
                QImage target(32, 32, QImage::Format_ARGB32);
                target.fill(Qt::transparent);
                QPainter painter(&target);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(0, 0, 32, 32);
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, scaled);
                painter.end();
                
                imgLabel->setPixmap(QPixmap::fromImage(target));
                imgLabel->setProperty("rawImgData", d.imgData);
            } else {
                imgLabel->setText(d.name.left(1));
                imgLabel->setStyleSheet("background: #3b82f6; color: white; border-radius: 16px; font-weight: bold; font-size: 13px; border: none;");
            }
        } else {
            QStringList colors = {"#3b82f6", "#8b5cf6", "#10b981", "#f59e0b", "#6366f1", "#ec4899", "#94a3b8"};
            int colorIdx = qHash(d.name) % colors.size();
            imgLabel->setText(d.name.left(1));
            imgLabel->setStyleSheet(QString("background: %1; color: white; border-radius: 16px; font-weight: bold; font-size: 13px; border: none;").arg(colors[colorIdx]));
        }

        QWidget *imgWrapper = new QWidget();
        imgWrapper->setStyleSheet("background: transparent;");
        QHBoxLayout *imgLayout = new QHBoxLayout(imgWrapper);
        imgLayout->setContentsMargins(0, 0, 0, 0); 
        imgLayout->addWidget(imgLabel, 0, Qt::AlignCenter);
        m_productRankTable->setCellWidget(r, 0, imgWrapper);

        // 2. 设置其他数据列
        auto *item1 = new QTableWidgetItem(d.name);
        auto *item2 = new QTableWidgetItem(QString::number(d.count));
        auto *item3 = new QTableWidgetItem(QString("¥ %1").arg(QString::number(d.rev, 'f', 2)));
        item2->setTextAlignment(Qt::AlignCenter); 
        item3->setTextAlignment(Qt::AlignCenter);
        
        m_productRankTable->setItem(r, 1, item1);
        m_productRankTable->setItem(r, 2, item2);
        m_productRankTable->setItem(r, 3, item3);
    }
    int maxP = (m_allProductData.size() + 8) / 9;
    m_productPageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_productPage + 1).arg(qMax(1, maxP)));
}

void StatsModule::updateCategoryTable() {
    std::sort(m_allCategoryData.begin(), m_allCategoryData.end(), [this](const ProductRankData &a, const ProductRankData &b) {
        return m_categorySortByRev ? (a.rev > b.rev) : (a.count > b.count);
    });

    m_categoryRankTable->setRowCount(0);
    int start = m_categoryPage * 9;
    int end = qMin(start + 9, (int)m_allCategoryData.size());
    for (int i = start; i < end; ++i) {
        const auto &d = m_allCategoryData[i];
        int r = m_categoryRankTable->rowCount(); m_categoryRankTable->insertRow(r);
        auto *item0 = new QTableWidgetItem(d.name);
        auto *item1 = new QTableWidgetItem(QString::number(d.count));
        auto *item2 = new QTableWidgetItem(QString("¥ %1").arg(QString::number(d.rev, 'f', 2)));
        item1->setTextAlignment(Qt::AlignCenter); item2->setTextAlignment(Qt::AlignCenter);
        m_categoryRankTable->setItem(r, 0, item0);
        m_categoryRankTable->setItem(r, 1, item1);
        m_categoryRankTable->setItem(r, 2, item2);
    }
    int maxP = (m_allCategoryData.size() + 8) / 9;
    m_categoryPageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_categoryPage + 1).arg(qMax(1, maxP)));
}

void StatsModule::goToProductPage(int delta) {
    int maxP = (m_allProductData.size() + 8) / 9;
    int n = m_productPage + delta;
    if (n >= 0 && n < maxP) { m_productPage = n; updateProductTable(); }
}

void StatsModule::goToCategoryPage(int delta) {
    int maxP = (m_allCategoryData.size() + 8) / 9;
    int n = m_categoryPage + delta;
    if (n >= 0 && n < maxP) { m_categoryPage = n; updateCategoryTable(); }
}




void StatsModule::updateServiceRankAnalysis() {
    // 1. 初始化全量服务列表
    m_allServiceData.clear();
    m_allServiceCategoryData.clear();
    
    QList<ServiceInfo> services = ServiceDataManager::instance()->allServices();
    QMap<QString, int> categoryIdxMap;
    
    for (const auto &s : services) {
        m_allServiceData.append({s.name, 0, 0.0});
        if (!s.category.isEmpty() && !categoryIdxMap.contains(s.category)) {
            categoryIdxMap[s.category] = m_allServiceCategoryData.size();
            m_allServiceCategoryData.append({s.category, 0, 0.0});
        }
    }

    // 2. 遍历订单统计服务
    QList<OrderInfo> orders = PetDataManager::instance()->allOrders();
    QDate today = QDate::currentDate();
    
    for (const auto &order : orders) {
        // 时间筛选
        bool inRange = false;
        QDate orderDate = QDateTime::fromString(order.createTime, "yyyy-MM-dd HH:mm:ss").date();
        if (m_serviceRankTimeRange == 0) inRange = (orderDate == today);
        else if (m_serviceRankTimeRange == 1) inRange = (orderDate.year() == today.year() && orderDate.month() == today.month());
        else if (m_serviceRankTimeRange == 2) inRange = (orderDate.year() == today.year());
        
        if (!inRange || order.status != "Paid") continue;

        QJsonDocument doc = QJsonDocument::fromJson(order.itemDetails.toUtf8());
        if (doc.isArray()) {
            QJsonArray items = doc.array();
            for (const auto &v : items) {
                QJsonObject item = v.toObject();
                QString rawName = item["name"].toString();
                QString name = rawName;
                if (rawName == QString::fromUtf8("全托普通房间")) name = QString::fromUtf8("全托寄养 (普通房间)");
                else if (rawName == QString::fromUtf8("全托豪华房间")) name = QString::fromUtf8("全托寄养 (豪华套房)");
                else if (rawName == QString::fromUtf8("全托多宠家庭房间")) name = QString::fromUtf8("全托寄养 (多宠家庭房)");
                else if (rawName == QString::fromUtf8("日托普通房间")) name = QString::fromUtf8("日托寄养 (普通房间)");
                else if (rawName == QString::fromUtf8("日托豪华房间")) name = QString::fromUtf8("日托寄养 (豪华套房)");
                else if (rawName == QString::fromUtf8("日托多宠家庭房间")) name = QString::fromUtf8("日托寄养 (多宠家庭房)");

                int count = item["count"].toInt();
                double price = item["price"].toDouble();
                
                // 匹配服务
                for (auto &sd : m_allServiceData) {
                    if (sd.name == name) {
                        sd.count += count;
                        sd.rev += (double)count * price;
                        break;
                    }
                }
                
                // 匹配服务类目 (通过服务经理查找类目)
                // 这里可以优化，如果 ServiceInfo 中有类目
                for (const auto &sInfo : services) {
                    if (sInfo.name == name && !sInfo.category.isEmpty() && categoryIdxMap.contains(sInfo.category)) {
                        int idx = categoryIdxMap[sInfo.category];
                        m_allServiceCategoryData[idx].count += count;
                        m_allServiceCategoryData[idx].rev += (double)count * price;
                        break;
                    }
                }
            }
        }
    }

    m_serviceItemPage = 0;
    m_serviceCategoryPage = 0;
    updateServiceItemTable();
    updateServiceCategoryTable();
}

void StatsModule::updateServiceItemTable() {
    if (!m_serviceRankTable) return;
    std::sort(m_allServiceData.begin(), m_allServiceData.end(), [this](const SvcRankData &a, const SvcRankData &b){
        return m_serviceItemSortByRev ? (a.rev > b.rev) : (a.count > b.count);
    });

    m_serviceRankTable->setRowCount(0);
    int start = m_serviceItemPage * 9;
    int end = qMin(start + 9, (int)m_allServiceData.size());
    for (int i = start; i < end; ++i) {
        const auto &d = m_allServiceData[i];
        int r = m_serviceRankTable->rowCount(); m_serviceRankTable->insertRow(r);
        auto *it0 = new QTableWidgetItem(d.name);
        auto *it1 = new QTableWidgetItem(QString::number(d.count));
        auto *it2 = new QTableWidgetItem(QString("¥ %1").arg(QString::number(d.rev, 'f', 2)));
        it1->setTextAlignment(Qt::AlignCenter); it2->setTextAlignment(Qt::AlignCenter);
        m_serviceRankTable->setItem(r, 0, it0);
        m_serviceRankTable->setItem(r, 1, it1);
        m_serviceRankTable->setItem(r, 2, it2);
    }
    int maxP = (m_allServiceData.size() + 8) / 9;
    if (m_serviceItemPageLabel)
        m_serviceItemPageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_serviceItemPage + 1).arg(qMax(1, maxP)));
}

void StatsModule::updateServiceCategoryTable() {
    if (!m_serviceCategoryRankTable) return;
    std::sort(m_allServiceCategoryData.begin(), m_allServiceCategoryData.end(), [this](const SvcRankData &a, const SvcRankData &b){
        return m_serviceCategorySortByRev ? (a.rev > b.rev) : (a.count > b.count);
    });

    m_serviceCategoryRankTable->setRowCount(0);
    int start = m_serviceCategoryPage * 9;
    int end = qMin(start + 9, (int)m_allServiceCategoryData.size());
    for (int i = start; i < end; ++i) {
        const auto &d = m_allServiceCategoryData[i];
        int r = m_serviceCategoryRankTable->rowCount(); m_serviceCategoryRankTable->insertRow(r);
        auto *it0 = new QTableWidgetItem(d.name);
        auto *it1 = new QTableWidgetItem(QString::number(d.count));
        auto *it2 = new QTableWidgetItem(QString("¥ %1").arg(QString::number(d.rev, 'f', 2)));
        it1->setTextAlignment(Qt::AlignCenter); it2->setTextAlignment(Qt::AlignCenter);
        m_serviceCategoryRankTable->setItem(r, 0, it0);
        m_serviceCategoryRankTable->setItem(r, 1, it1);
        m_serviceCategoryRankTable->setItem(r, 2, it2);
    }
    int maxP = (m_allServiceCategoryData.size() + 8) / 9;
    if (m_serviceCategoryPageLabel)
        m_serviceCategoryPageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_serviceCategoryPage + 1).arg(qMax(1, maxP)));
}

void StatsModule::goToServiceItemPage(int delta) {
    int maxP = (m_allServiceData.size() + 8) / 9;
    int n = m_serviceItemPage + delta;
    if (n >= 0 && n < maxP) { m_serviceItemPage = n; updateServiceItemTable(); }
}

void StatsModule::goToServiceCategoryPage(int delta) {
    int maxP = (m_allServiceCategoryData.size() + 8) / 9;
    int n = m_serviceCategoryPage + delta;
    if (n >= 0 && n < maxP) { m_serviceCategoryPage = n; updateServiceCategoryTable(); }
}

void StatsModule::updateTopCharts() {
    // 异步驱动，数据更新由 onDashboardStatsReceived 异步回调触发
}

void StatsModule::updateCards() {
    if (m_cardValues.size() < 3) return;

    // 财务分类 (0) 的卡片更新已交由 onDashboardStatsReceived 异步处理
    if (m_currentCategory == 0) return;

    auto updateCard = [&](int idx, const QString &val, double diff, const QString &imgDataStr = "", const QString &nameForFallback = "") {
        m_cardValues[idx]->setText(val);
        m_cardValues[idx]->setWordWrap(true); // 开启自动换行支持
        
        // 动态调节字号，保证全称显示完整且优雅
        int len = val.length();
        int fontSize = 28;
        if (len > 12) fontSize = 14;
        else if (len > 8) fontSize = 18;
        else if (len > 5) fontSize = 22;
        
        m_cardValues[idx]->setStyleSheet(QString("color: #0f172a; font-size: %1px; font-weight: 900; border: none;").arg(fontSize));

        if (diff != 0) {
            QString sign = diff >= 0 ? "↑" : "↓";
            QString color = diff >= 0 ? "#22c55e" : "#ef4444";
            m_cardTrends[idx]->setText(QString("%1 %2%").arg(sign).arg(QString::number(qAbs(diff), 'f', 1)));
            m_cardTrends[idx]->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600; border: none;").arg(color));
        } else {
            m_cardTrends[idx]->setText("--");
            m_cardTrends[idx]->setStyleSheet("color: #94a3b8; font-size: 12px; border: none;");
        }

        // --- 动态卡片图片渲染逻辑开始 ---
        QWidget *card = qobject_cast<QWidget*>(m_cardValues[idx]->parentWidget());
        if (card) {
            QLabel *photoLabel = card->findChild<QLabel*>("cardPhoto");
            
            if (imgDataStr.isEmpty() && nameForFallback.isEmpty()) {
                if (photoLabel) photoLabel->hide();
            } else {
                if (!photoLabel) {
                    photoLabel = new QLabel(card);
                    photoLabel->setObjectName("cardPhoto");
                    photoLabel->setFixedSize(64, 64);
                    photoLabel->setAlignment(Qt::AlignCenter);
                    photoLabel->setStyleSheet("border: none; background: transparent;");
                    photoLabel->setCursor(Qt::PointingHandCursor);
                    photoLabel->setProperty("isAvatar", true);
                    photoLabel->installEventFilter(this);
                    
                    QVBoxLayout *vl = qobject_cast<QVBoxLayout*>(card->layout());
                    if (vl) {
                        // 插入在 titleLabel 下方 (索引 2 处)
                        vl->insertWidget(2, photoLabel, 0, Qt::AlignCenter);
                    }
                }
                photoLabel->show();
                photoLabel->setProperty("rawImgData", imgDataStr); // 动态绑定 base64 数据以支持点击放大！

                if (!imgDataStr.isEmpty()) {
                    QByteArray decoded = QByteArray::fromBase64(imgDataStr.toUtf8());
                    QPixmap pix;
                    if (pix.loadFromData(decoded)) {
                        // 剪裁成高级的微圆角正方形 (border-radius: 10px)
                        QPixmap scaled = pix.scaled(60, 60, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                        
                        QImage target(60, 60, QImage::Format_ARGB32);
                        target.fill(Qt::transparent);
                        QPainter painter(&target);
                        painter.setRenderHint(QPainter::Antialiasing);
                        QPainterPath path;
                        path.addRoundedRect(0, 0, 60, 60, 10, 10);
                        painter.setClipPath(path);
                        painter.drawPixmap(0, 0, scaled);
                        painter.end();
                        
                        photoLabel->setPixmap(QPixmap::fromImage(target));
                        photoLabel->setText("");
                        photoLabel->setStyleSheet("border: 2px solid white; border-radius: 12px; background: white;");
                    } else {
                        photoLabel->setPixmap(QPixmap());
                        photoLabel->setText("");
                    }
                } else if (!nameForFallback.isEmpty()) {
                    // 兜底渲染彩色圆形徽章
                    QStringList colors = {"#3b82f6", "#8b5cf6", "#10b981", "#f59e0b", "#6366f1", "#ec4899", "#94a3b8"};
                    int colorIdx = qHash(nameForFallback) % colors.size();
                    photoLabel->setPixmap(QPixmap());
                    photoLabel->setText(nameForFallback.left(1));
                    photoLabel->setStyleSheet(QString("background: %1; color: white; border-radius: 32px; font-weight: bold; font-size: 24px; border: 2px solid white;").arg(colors[colorIdx]));
                }
            }
        }
        // --- 动态卡片图片渲染逻辑结束 ---
    };

    if (m_currentCategory == 1) { // 店员统计卡片
        if (m_allStaffData.isEmpty()) {
            updateCard(0, "无", 0); updateCard(1, "无", 0); updateCard(2, "¥0", 0);
            return;
        }
        auto topRevIt = std::max_element(m_allStaffData.begin(), m_allStaffData.end(), [](const StaffRankData &a, const StaffRankData &b){ return a.rev < b.rev; });
        auto topOrderIt = std::max_element(m_allStaffData.begin(), m_allStaffData.end(), [](const StaffRankData &a, const StaffRankData &b){ return a.count < b.count; });
        double totalRev = 0; int totalOrders = 0;
        for(const auto &s : m_allStaffData) { totalRev += s.rev; totalOrders += s.count; }
        double avgEff = totalOrders > 0 ? totalRev / totalOrders : 0;
        updateCard(0, topRevIt->name, 0, "", topRevIt->name);
        updateCard(1, topOrderIt->name, 0);
        updateCard(2, QString("¥%1").arg(QString::number(avgEff, 'f', 0)), 0);
        m_cardTrends[0]->setText(QString("¥%1").arg(QString::number(topRevIt->rev, 'f', 0)));
        m_cardTrends[1]->setText(QString("%1 单").arg(topOrderIt->count));
        m_cardTrends[2]->setText("全店客单均值");
    } else if (m_currentCategory == 2) { // 商品统计卡片
        if (m_allProductData.isEmpty()) {
            updateCard(0, "无", 0); updateCard(1, "0", 0); updateCard(2, "¥0", 0);
            return;
        }
        auto topQtyIt = std::max_element(m_allProductData.begin(), m_allProductData.end(), [](const ProductRankData &a, const ProductRankData &b){ return a.count < b.count; });
        auto topRevIt = std::max_element(m_allProductData.begin(), m_allProductData.end(), [](const ProductRankData &a, const ProductRankData &b){ return a.rev < b.rev; });
        updateCard(0, topQtyIt->name, 0, topQtyIt->imgData, topQtyIt->name);
        updateCard(1, QString::number(topQtyIt->count), 0);
        updateCard(2, QString("¥%1").arg(QString::number(topRevIt->rev, 'f', 0)), 0);
        m_cardTrends[0]->setText("最受宠爱单品");
        m_cardTrends[1]->setText("本期累计卖出");
        m_cardTrends[2]->setText(topRevIt->name + " 贡献");

        // 渲染商品占比饼图
        if (m_productCategoryChart) {
            QChart *chart = new QChart();
            chart->setBackgroundVisible(false);
            QPieSeries *series = new QPieSeries();
            series->setHoleSize(0.4);
            series->setPieSize(0.7);
            
            double total = 0.0;
            for (const auto &d : m_allCategoryData) {
                total += d.rev;
            }
            
            QList<QColor> colors = {QColor("#3b82f6"), QColor("#8b5cf6"), QColor("#10b981"), QColor("#f59e0b"), QColor("#6366f1"), QColor("#ec4899")};
            int colorIdx = 0;
            for (const auto &d : m_allCategoryData) {
                if (d.rev <= 0) continue;
                double pct = (total > 0) ? (d.rev / total * 100.0) : 0.0;
                QString labelText = QString("%1 %2%").arg(d.name).arg(QString::number(pct, 'f', 1));
                QPieSlice *slice = series->append(labelText, d.rev);
                slice->setBrush(colors[colorIdx % colors.size()]);
                slice->setLabelVisible(true);
                slice->setLabelPosition(QPieSlice::LabelOutside);
                slice->setLabelFont(QFont("Microsoft YaHei", 10, QFont::Bold));
                colorIdx++;
            }
            
            chart->addSeries(series);
            chart->setMargins(QMargins(0, 0, 0, 0));
            chart->legend()->hide();
            m_productCategoryChart->setChart(chart);
        }
    } else if (m_currentCategory == 3) { // 服务排行卡片
        if (m_allServiceData.isEmpty()) {
            updateCard(0, "无", 0); updateCard(1, "0", 0); updateCard(2, "¥0", 0);
            return;
        }
        auto topQtyIt = std::max_element(m_allServiceData.begin(), m_allServiceData.end(), [](const SvcRankData &a, const SvcRankData &b){ return a.count < b.count; });
        auto topRevIt = std::max_element(m_allServiceData.begin(), m_allServiceData.end(), [](const SvcRankData &a, const SvcRankData &b){ return a.rev < b.rev; });
        updateCard(0, topQtyIt->name, 0, "", topQtyIt->name);
        updateCard(1, QString::number(topQtyIt->count), 0);
        updateCard(2, QString("¥%1").arg(QString::number(topRevIt->rev, 'f', 0)), 0);
        m_cardTrends[0]->setText("金牌服务项目");
        m_cardTrends[1]->setText("本期服务人次");
        m_cardTrends[2]->setText(topRevIt->name + " 贡献");

        // 渲染服务占比饼图
        if (m_serviceHeatmapChart) {
            QChart *chart = new QChart();
            chart->setBackgroundVisible(false);
            QPieSeries *series = new QPieSeries();
            series->setHoleSize(0.4);
            series->setPieSize(0.7);
            
            double total = 0.0;
            for (const auto &d : m_allServiceCategoryData) {
                total += d.rev;
            }
            
            QList<QColor> colors = {QColor("#3b82f6"), QColor("#8b5cf6"), QColor("#10b981"), QColor("#f59e0b"), QColor("#6366f1"), QColor("#ec4899")};
            int colorIdx = 0;
            for (const auto &d : m_allServiceCategoryData) {
                if (d.rev <= 0) continue;
                double pct = (total > 0) ? (d.rev / total * 100.0) : 0.0;
                QString labelText = QString("%1 %2%").arg(d.name).arg(QString::number(pct, 'f', 1));
                QPieSlice *slice = series->append(labelText, d.rev);
                slice->setBrush(colors[colorIdx % colors.size()]);
                slice->setLabelVisible(true);
                slice->setLabelPosition(QPieSlice::LabelOutside);
                slice->setLabelFont(QFont("Microsoft YaHei", 10, QFont::Bold));
                colorIdx++;
            }
            
            chart->addSeries(series);
            chart->setMargins(QMargins(0, 0, 0, 0));
            chart->legend()->hide();
            m_serviceHeatmapChart->setChart(chart);
        }
    }
}

void StatsModule::onCategoryChanged(int index) {
    m_currentCategory = index;
    for (int i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setChecked(i == index);
    }
    m_viewStack->setCurrentIndex(index);
    
    // 动态更新卡片标题
    if (m_cardTitles.size() >= 3) {
        if (index == 0) {
            m_cardTitles[0]->setText("总营收");
            m_cardTitles[1]->setText("毛利润");
            m_cardTitles[2]->setText("客单价");
        } else if (index == 1) {
            m_cardTitles[0]->setText("营收冠军");
            m_cardTitles[1]->setText("单量冠军");
            m_cardTitles[2]->setText("人效均值");
        } else if (index == 2) {
            m_cardTitles[0]->setText("热销榜首");
            m_cardTitles[1]->setText("冠军销量");
            m_cardTitles[2]->setText("冠军营收");
        } else if (index == 3) {
            m_cardTitles[0]->setText("最受欢迎");
            m_cardTitles[1]->setText("服务单量");
            m_cardTitles[2]->setText("服务营收");
        }
    }

    // 顶部图表上下文逻辑
    if (m_topPieContainer) {
        m_finPiesContainer->setVisible(index == 0);
        m_prodPieContainer->setVisible(index == 2);
        m_svcPieContainer->setVisible(index == 3);
        
        // 只有财务、商品、服务标签页显示顶部饼图
        m_topPieContainer->setVisible(index == 0 || index == 2 || index == 3); 
    }
    
    refreshData();
}

void StatsModule::onDateRangeChanged() { refreshData(); }
void StatsModule::onSearch(const QString &text) { Q_UNUSED(text); refreshData(); }
void StatsModule::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_backdrop) m_backdrop->setGeometry(this->rect());
}

bool StatsModule::eventFilter(QObject *watched, QEvent *event) {
    if (watched->property("isAvatar").toBool() && event->type() == QEvent::MouseButtonPress) {
        QLabel *small = qobject_cast<QLabel*>(watched);
        if (small) {
            // 动态计算主界面 80% 的尺寸（取宽高的较小值）
            int side = qMin(this->width(), this->height()) * 0.8;
            m_largePreviewLabel->setFixedSize(side, side);

            QString rawImgData = small->property("rawImgData").toString();
            if (!rawImgData.isEmpty()) {
                QByteArray decoded = QByteArray::fromBase64(rawImgData.toUtf8());
                QPixmap pix;
                if (pix.loadFromData(decoded)) {
                    // 1. 商品大图采用正方形展示，保持原始宽高比缩放以确保商品包装和细节完整呈现
                    QPixmap scaled = pix.scaled(side - 32, side - 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    
                    m_largePreviewLabel->setPixmap(scaled);
                    m_largePreviewLabel->setText("");
                    m_largePreviewLabel->setAlignment(Qt::AlignCenter);
                    // 采用非常高级的微圆角正方形画廊卡片样式，白色背景与超大白边框，带优雅阴影感
                    m_largePreviewLabel->setStyleSheet("background: #ffffff; border-radius: 16px; border: 8px solid white;");
                } else {
                    m_largePreviewLabel->setPixmap(QPixmap());
                    m_largePreviewLabel->setText(small->text());
                    m_largePreviewLabel->setStyleSheet(small->styleSheet() + QString("; border-radius: %1px; font-size: %2px; border: 8px solid white;").arg(side / 2).arg((int)(side * 0.4)));
                }
            } else {
                // 员工文字头像的等比例超大化圆形显示
                m_largePreviewLabel->setPixmap(QPixmap());
                m_largePreviewLabel->setText(small->text());
                
                // 精准提取小头像的背景色
                QString bgStyle = "";
                QString origStyle = small->styleSheet();
                int bgStart = origStyle.indexOf("background:");
                if (bgStart != -1) {
                    int bgEnd = origStyle.indexOf(";", bgStart);
                    if (bgEnd != -1) {
                        bgStyle = origStyle.mid(bgStart, bgEnd - bgStart + 1);
                    } else {
                        bgStyle = origStyle.mid(bgStart);
                    }
                }
                if (bgStyle.isEmpty()) bgStyle = "background: #3b82f6;";

                m_largePreviewLabel->setStyleSheet(QString("%1 color: white; border-radius: %2px; font-weight: bold; border: 8px solid white; font-size: %3px;").arg(bgStyle).arg(side / 2).arg((int)(side * 0.4)));
            }
            m_backdrop->setGeometry(this->rect());
            m_backdrop->show();
            m_backdrop->raise();
        }
        return true;
    }
    if (watched == m_backdrop && event->type() == QEvent::MouseButtonPress) {
        m_backdrop->hide();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void StatsModule::updateDailyRevTable() {
    if (!m_dailyRevenueTable) return;

    m_allDailyRevData.clear();
    
    int rangeIdx = m_trendRangeCombo->currentIndex();
    int selYear = m_yearPicker->currentText().left(4).toInt();
    int selMonth = m_monthPicker->currentIndex() + 1;
    
    // 动态调整表头
    QString firstColLabel = "日期";
    if (rangeIdx == 0) firstColLabel = "年份";
    else if (rangeIdx == 1) firstColLabel = "月份";
    
    m_dailyRevenueTable->setHorizontalHeaderLabels({firstColLabel, "总营收", "服务收入", "商品收入", "寄养收入", "订单量", "客单价"});

    // 数据聚合逻辑 (优先获取真实数据，无数据则生成模拟)
    auto addDataRow = [&](const QString &label, const QDate &start, const QDate &end) {
        auto orders = PetDataManager::instance()->getOrders(start, end);
        double total = 0, svc = 0, prod = 0, foster = 0;
        int count = 0;
        for(const auto &o : orders) {
            if(o.status != "Paid") continue;
            total += o.finalAmount;
            if(o.sourceModule == "Product") prod += o.finalAmount;
            else if(o.sourceModule == "Foster") foster += o.finalAmount;
            else svc += o.finalAmount;
            count++;
        }
        
        // 移除模拟数据补偿，仅显示真实数据
        double avg = count > 0 ? total / count : 0;
        m_allDailyRevData.append({label, total, svc, prod, foster, count, avg});
    };

    if (rangeIdx == 0) { // 历年对比
        for (int y = selYear; y >= selYear - 4; --y) {
            addDataRow(QString::number(y) + "年", QDate(y, 1, 1), QDate(y, 12, 31));
        }
    } else if (rangeIdx == 1) { // 年度走势
        for (int m = 12; m >= 1; --m) {
            QDate start(selYear, m, 1);
            addDataRow(QString::number(m) + "月", start, start.addMonths(1).addDays(-1));
        }
    } else { // 本月走势 (每日明细)
        int days = QDate(selYear, selMonth, 1).daysInMonth();
        for (int d = days; d >= 1; --d) {
            QDate date(selYear, selMonth, d);
            addDataRow(date.toString("yyyy-MM-dd"), date, date);
        }
    }

    m_dailyRevenueTable->setRowCount(0);
    int start = m_dailyRevPage * 12;
    int end = qMin(start + 12, (int)m_allDailyRevData.size());

    auto createItem = [](const QString &text, bool center = true) {
        auto *item = new QTableWidgetItem(text);
        if (center) item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    for (int i = start; i < end; ++i) {
        const auto &data = m_allDailyRevData[i];
        int r = m_dailyRevenueTable->rowCount();
        m_dailyRevenueTable->insertRow(r);

        m_dailyRevenueTable->setItem(r, 0, createItem(data.date, false));
        m_dailyRevenueTable->setItem(r, 1, createItem(QString("¥ %1").arg(data.total, 0, 'f', 2)));
        m_dailyRevenueTable->setItem(r, 2, createItem(QString("¥ %1").arg(data.svc, 0, 'f', 2)));
        m_dailyRevenueTable->setItem(r, 3, createItem(QString("¥ %1").arg(data.prod, 0, 'f', 2)));
        m_dailyRevenueTable->setItem(r, 4, createItem(QString("¥ %1").arg(data.foster, 0, 'f', 2)));
        m_dailyRevenueTable->setItem(r, 5, createItem(QString::number(data.count)));
        m_dailyRevenueTable->setItem(r, 6, createItem(QString("¥ %1").arg(data.avg, 0, 'f', 2)));
    }

    int maxP = (m_allDailyRevData.size() + 11) / 12;
    if (m_dailyRevPageLabel)
        m_dailyRevPageLabel->setText(QString::fromUtf8("第 %1 页 / 共 %2 页").arg(m_dailyRevPage + 1).arg(qMax(1, maxP)));
}

void StatsModule::goToDailyRevPage(int delta) {
    int maxP = (m_allDailyRevData.size() + 11) / 12;
    int n = m_dailyRevPage + delta;
    if (n >= 0 && n < maxP) {
        m_dailyRevPage = n;
        updateDailyRevTable();
    }
}

void StatsModule::onDashboardStatsReceived(const QJsonObject &data) {
    m_cachedDashboardStats = data;

    // 1. 更新卡片 (KPI) - 仅在财务总览 (0) 时使用该异步卡片数据
    if (m_currentCategory == 0 && m_cardValues.size() >= 3) {
        m_cardValues[0]->setText(QString("¥%1").arg(QString::number(data["totalRevenue"].toDouble(), 'f', 0)));
        m_cardValues[1]->setText(QString::number(data["totalOrders"].toInt()));
        m_cardValues[2]->setText(QString("¥%1").arg(QString::number(data["avgOrder"].toDouble(), 'f', 0)));
        
        m_cardTrends[0]->setText("核心营收指标");
        m_cardTrends[1]->setText("已支付总单数");
        m_cardTrends[2]->setText("全店客单均值");
    }

    if (!m_chartsCreated) return;

    // 2. 更新饼图
    auto updatePie = [&](QChartView* view, const QJsonArray &arr, const QList<QColor> &colors) {
        if (!view) return;
        QChart *chart = new QChart();
        chart->setBackgroundVisible(false);
        QPieSeries *series = new QPieSeries();
        series->setHoleSize(0.4); 
        series->setPieSize(0.7); 
        
        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            QString name = obj["name"].toString();
            
            // 翻译模块和支付渠道的英文名称为中文
            if (name == "Appointment") name = "服务预约";
            else if (name == "Product") name = "商品零售";
            else if (name == "Service") name = "服务项目";
            else if (name == "Foster" || name == "Boarding") name = "宠物寄养";
            else if (name == "Logistics") name = "宠物接送";
            else if (name == "MemberCard" || name == "Balance") name = "会员卡余额";
            else if (name == "Alipay") name = "支付宝";
            else if (name == "Wechat" || name == "WeChat") name = "微信支付";
            else if (name == "Cash") name = "现金";
            else if (name == "Card") name = "银行卡";

            double val = obj["value"].toDouble();
            if (val <= 0) continue;
            
            QPieSlice *slice = series->append(name, val);
            if (i < colors.size()) slice->setBrush(colors[i]);
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setLabelFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        }
        for (auto slice : series->slices()) {
            slice->setLabel(QString("%1 %2%").arg(slice->label()).arg(QString::number(slice->percentage() * 100, 'f', 1)));
        }
        chart->addSeries(series);
        chart->setMargins(QMargins(0, 0, 0, 0));
        chart->legend()->hide();
        view->setChart(chart);
    };

    updatePie(m_finCompChart, data["finComp"].toArray(), {QColor("#3b82f6"), QColor("#8b5cf6"), QColor("#10b981"), QColor("#f59e0b")});
    updatePie(m_finPayChart, data["payComp"].toArray(), {QColor("#22c55e"), QColor("#3b82f6"), QColor("#f59e0b"), QColor("#94a3b8")});
}

void StatsModule::onRevenueTrendReceived(const QJsonArray &data) {
    m_cachedRevenueTrend = data;
    if (!m_chartsCreated || !m_finTrendChart) return;

    QChart *chart = new QChart();
    chart->setBackgroundVisible(false);
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    
    QLineSeries *series = new QLineSeries();
    series->setColor(QColor("#3b82f6"));
    series->setPen(QPen(QColor("#3b82f6"), 3));
    series->setPointsVisible(true);
    
    QBarSeries *labelSeries = new QBarSeries();
    QBarSet *set = new QBarSet("");
    set->setBrush(Qt::transparent);
    set->setPen(QPen(Qt::transparent));
    set->setLabelColor(QColor("#475569"));
    set->setLabelFont(QFont("Microsoft YaHei", 8, QFont::Bold));

    QStringList categories;
    double maxVal = 100;
    for (int i = 0; i < data.size(); ++i) {
        QJsonObject obj = data[i].toObject();
        QString label = obj["label"].toString();
        double val = obj["value"].toDouble();
        
        series->append(i, val);
        *set << val;
        categories << label;
        if (val > maxVal) maxVal = val;
    }
    labelSeries->append(set);
    labelSeries->setLabelsVisible(true);
    labelSeries->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

    chart->addSeries(series);
    chart->addSeries(labelSeries);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setGridLineVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    labelSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, maxVal * 1.2);
    axisY->setGridLineColor(QColor("#f1f5f9"));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    labelSeries->attachAxis(axisY);

    chart->legend()->hide();
    m_finTrendChart->setChart(chart);
}

void StatsModule::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    ensureChartsCreated();
}

void StatsModule::ensureChartsCreated() {
    if (m_chartsCreated) return;
    
    QElapsedTimer timer;
    timer.start();
    qDebug() << "[PERF] Lazy-instantiating StatsModule charts...";

    // 1. m_finCompChart
    m_finCompChart = new QChartView();
    m_finCompChart->setFixedHeight(240);
    m_finCompChart->setRenderHint(QPainter::Antialiasing);
    m_finCompChart->setStyleSheet("background: transparent; border: none;");
    m_finCompChartContainer->layout()->addWidget(m_finCompChart);

    // 2. m_finPayChart
    m_finPayChart = new QChartView();
    m_finPayChart->setFixedHeight(240);
    m_finPayChart->setRenderHint(QPainter::Antialiasing);
    m_finPayChart->setStyleSheet("background: transparent; border: none;");
    m_finPayChartContainer->layout()->addWidget(m_finPayChart);

    // 3. m_productCategoryChart
    m_productCategoryChart = new QChartView();
    m_productCategoryChart->setFixedHeight(240);
    m_productCategoryChart->setRenderHint(QPainter::Antialiasing);
    m_productCategoryChart->setStyleSheet("background: transparent; border: none;");
    m_productCategoryChartContainer->layout()->addWidget(m_productCategoryChart);

    // 4. m_serviceHeatmapChart
    m_serviceHeatmapChart = new QChartView();
    m_serviceHeatmapChart->setFixedHeight(240);
    m_serviceHeatmapChart->setRenderHint(QPainter::Antialiasing);
    m_serviceHeatmapChart->setStyleSheet("background: transparent; border: none;");
    m_serviceHeatmapChartContainer->layout()->addWidget(m_serviceHeatmapChart);

    // 5. m_finTrendChart
    m_finTrendChart = new QChartView();
    m_finTrendChart->setMinimumHeight(400);
    m_finTrendChart->setRenderHint(QPainter::Antialiasing);
    m_finTrendChart->setStyleSheet("background: transparent;");
    m_finTrendChartContainer->layout()->addWidget(m_finTrendChart);

    m_chartsCreated = true;
    qDebug() << "[PERF] Lazy-instantiation of StatsModule charts completed in" << timer.elapsed() << "ms.";

    // Trigger update with cached data
    if (!m_cachedDashboardStats.isEmpty()) {
        onDashboardStatsReceived(m_cachedDashboardStats);
    }
    if (!m_cachedRevenueTrend.isEmpty()) {
        onRevenueTrendReceived(m_cachedRevenueTrend);
    }
}
