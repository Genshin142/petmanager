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
#include <QGraphicsDropShadowEffect>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include "custommessagedialog.h"
#include "orderdetaildrawer.h"
#include "staffdatamanager.h"
#include "servicedatamanager.h"
#include "petdatamanager.h"
#include "productdatamanager.h"
#include <QtCharts/QScatterSeries>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>

// 复刻会员界面的“全行圆角选中”效果
class StatsRowDelegate : public QStyledItemDelegate {
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
    auto createTopPie = [&](const QString &title, QChartView* &chart) {
        QWidget *box = new QWidget();
        QVBoxLayout *vl = new QVBoxLayout(box);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->setSpacing(2);
        QLabel *l = new QLabel(title);
        l->setStyleSheet("font-size: 16px; font-weight: 800; color: #1e293b; margin-bottom: 8px;"); // 放大标题字体
        vl->addWidget(l);
        chart = new QChartView();
        chart->setFixedHeight(240); // 缩小高度
        chart->setRenderHint(QPainter::Antialiasing);
        chart->setStyleSheet("background: transparent; border: none;");
        vl->addWidget(chart);
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
    fpl->addWidget(createTopPie("营收项目构成", m_finCompChart), 1);
    fpl->addWidget(createTopPie("支付渠道分布", m_finPayChart), 1);
    tpcl->addWidget(m_finPiesContainer);

    // 2. 商品占比组
    m_prodPieContainer = new QWidget();
    QHBoxLayout *ppl = new QHBoxLayout(m_prodPieContainer);
    ppl->setContentsMargins(0,0,0,0);
    ppl->addWidget(createTopPie("商品销售构成", m_productCategoryChart), 1);
    tpcl->addWidget(m_prodPieContainer);

    // 3. 服务占比组
    m_svcPieContainer = new QWidget();
    QHBoxLayout *spl = new QHBoxLayout(m_svcPieContainer);
    spl->setContentsMargins(0,0,0,0);
    spl->addWidget(createTopPie("服务类目分布", m_serviceHeatmapChart), 1);
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
        card->setFixedSize(180, 180);
        card->setStyleSheet("background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 12px;");
        
        QVBoxLayout *vl = new QVBoxLayout(card);
        vl->setContentsMargins(20, 20, 20, 20);
        vl->setSpacing(15);
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
            "QPushButton:checked { background: white; color: #3b82f6; box-shadow: 0 1px 2px rgba(0,0,0,0.05); }"
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
    m_finTrendChart = new QChartView();
    m_finTrendChart->setMinimumHeight(400); 
    m_finTrendChart->setRenderHint(QPainter::Antialiasing);
    m_finTrendChart->setStyleSheet("background: transparent;");
    m_financeMainStack->addWidget(m_finTrendChart);

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
    connect(m_trendRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ updatePickerVisibility(); refreshData(); });
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

    rankLayout->addWidget(createRankPanel("商品销售单品排行", m_productRankTable, {"商品", "销量", "营收"}, true), 1);
    rankLayout->addWidget(createRankPanel("商品类目销售排行", m_categoryRankTable, {"类目", "销量", "营收"}, false), 1);
    
    layout->addLayout(rankLayout);
    return view;
}


void StatsModule::refreshData() {
    updateTopCharts();
    if (m_currentCategory == 1) updateServiceAnalysis();
    else if (m_currentCategory == 2) updateProductAnalysis();
    else if (m_currentCategory == 3) updateServiceRankAnalysis(); // 先准备服务排行数据
    
    updateCards();
    updateCharts();
    m_dailyRevPage = 0; 
    updateDailyRevTable();
}

void StatsModule::updateCharts() {
    if (!m_finTrendChart || m_currentCategory != 0) return;

    // 1. 营收趋势 (带历史穿透的核心逻辑)
    {
        QDateTime now = QDateTime::currentDateTime();
        QDateTime start, end;
        int rangeIdx = m_trendRangeCombo->currentIndex();
        
        // 解析选择的时间锚点
        int selYear = m_yearPicker->currentText().left(4).toInt();
        int selMonth = m_monthPicker->currentIndex() + 1;

        bool groupByYear = (rangeIdx == 0);
        bool groupByMonth = (rangeIdx == 1);
        
        if (groupByYear) { // 历年对比 (显示近5年)
            start = QDateTime(QDate(now.date().year() - 4, 1, 1), QTime(0, 0));
            end = now;
        }
        else if (groupByMonth) { // 年度走势 (显示选定年份的12个月)
            start = QDateTime(QDate(selYear, 1, 1), QTime(0, 0));
            end = QDateTime(QDate(selYear, 12, 31), QTime(23, 59, 59));
        }
        else { // 月度走势 (显示选定年月的具体日期)
            start = QDateTime(QDate(selYear, selMonth, 1), QTime(0, 0));
            end = start.addMonths(1).addDays(-1);
        }

        auto orders = PetDataManager::instance()->getOrders(start.date(), end.date());
        QMap<QString, double> trendData;
        QString format = groupByYear ? "yyyy年" : (groupByMonth ? "M月" : "MM-dd");

        // 1. 预填充补零
        QDateTime temp = start;
        while (temp <= end) {
            trendData[temp.toString(format)] = 0.0;
            if (groupByYear) temp = temp.addYears(1);
            else if (groupByMonth) temp = temp.addMonths(1);
            else temp = temp.addDays(1);
        }

        // 2. 填充真实数据
        for (const auto &o : orders) {
            QDateTime dt = QDateTime::fromString(o.createTime, "yyyy-MM-dd HH:mm:ss");
            if (!dt.isValid()) dt = QDateTime::fromString(o.createTime.left(10), "yyyy-MM-dd");
            if (dt.isValid()) {
                QString key = dt.toString(format);
                if (trendData.contains(key)) trendData[key] += o.totalAmount;
            }
        }

        QChart *chart = new QChart();
        chart->setBackgroundVisible(false);
        chart->layout()->setContentsMargins(0, 0, 0, 0);
        
        QLineSeries *series = new QLineSeries();
        series->setColor(QColor("#3b82f6"));
        series->setPen(QPen(QColor("#3b82f6"), 3));
        series->setPointsVisible(true);
        
        // 使用透明柱状图作为“标签载体”，实现完美的顶部悬浮
        QBarSeries *labelSeries = new QBarSeries();
        QBarSet *set = new QBarSet("");
        set->setBrush(Qt::transparent);
        set->setPen(QPen(Qt::transparent)); // 彻底透明
        set->setLabelColor(QColor("#475569"));
        set->setLabelFont(QFont("Microsoft YaHei", 8, QFont::Bold));

        QStringList categories;
        double maxRev = 100;
        for (auto it = trendData.begin(); it != trendData.end(); ++it) {
            series->append(categories.size(), it.value());
            *set << it.value();
            categories << it.key();
            if (it.value() > maxRev) maxRev = it.value();
        }

        labelSeries->append(set);
        labelSeries->setLabelsVisible(true);
        labelSeries->setLabelsFormat("¥@value"); // 加上货币符号
        labelSeries->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd); 
        labelSeries->setBarWidth(0.01); // 进一步收缩宽度

        chart->addSeries(series);
        chart->addSeries(labelSeries);
        
        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(categories);
        axisX->setGridLineVisible(false);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis();
        axisY->setLabelFormat("%d");
        axisY->setGridLineColor(QColor("#f1f5f9"));
        axisY->setRange(0, maxRev * 1.2); // 留出 20% 的顶部空间
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);

        chart->legend()->hide();
        m_finTrendChart->setChart(chart);
    }

    // 2. 营收构成 & 支付方式 (按近30天展示)
    {
        double svc = 0, prod = 0;
        QMap<QString, double> payMap;
        auto orders = PetDataManager::instance()->getOrders(QDate::currentDate().addDays(-30), QDate::currentDate());
        for(const auto &o : orders) {
            if(o.status != "Paid") continue;
            if(o.sourceModule == "Product") prod += o.finalAmount; else svc += o.finalAmount;
            payMap[o.payMethod] += o.finalAmount;
        }

        // 营收构成饼图
        QChart *compChart = new QChart();
        compChart->setBackgroundVisible(false);
        // 禁用动画
        QPieSeries *compSeries = new QPieSeries();
        compSeries->setHoleSize(0.35); 
        compSeries->setPieSize(0.7); // 大幅放大尺寸
        
        QPieSlice *s1 = compSeries->append("服务营收", svc);
        QPieSlice *s2 = compSeries->append("商品销售", prod);
        s1->setBrush(QColor("#3b82f6")); s2->setBrush(QColor("#0ea5e9"));
        
        for (auto slice : compSeries->slices()) {
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setLabelArmLengthFactor(0.02); // 极短引线
            slice->setLabelFont(QFont("Microsoft YaHei", 10, QFont::Bold)); // 增大并加粗
            slice->setLabel(QString("%1 %2%").arg(slice->label()).arg(QString::number(slice->percentage() * 100, 'f', 1)));
        }
        compChart->addSeries(compSeries);
        compChart->setMargins(QMargins(0, 0, 0, 0));
        compChart->legend()->hide();
        m_finCompChart->setChart(compChart);

        // 支付方式饼图
        QChart *payChart = new QChart();
        payChart->setBackgroundVisible(false);
        // 禁用动画
        QPieSeries *paySeries = new QPieSeries();
        paySeries->setHoleSize(0.35);
        paySeries->setPieSize(0.7); 
        
        QMap<QString, QColor> payColors = {{"会员卡", QColor("#6366f1")}, {"支付宝", QColor("#06b6d4")}, {"现金", QColor("#94a3b8")}};
        for(auto it = payMap.begin(); it != payMap.end(); ++it) {
            QString label = it.key() == "MemberCard" ? "会员卡" : (it.key() == "Alipay" ? "支付宝" : "现金");
            QPieSlice *slice = paySeries->append(label, it.value());
            if (payColors.contains(label)) slice->setBrush(payColors[label]);
        }
        
        for (auto slice : paySeries->slices()) {
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setLabelArmLengthFactor(0.02);
            slice->setLabelFont(QFont("Microsoft YaHei", 10, QFont::Bold));
            slice->setLabel(QString("%1 %2%").arg(slice->label()).arg(QString::number(slice->percentage() * 100, 'f', 1)));
        }
        payChart->addSeries(paySeries);
        payChart->setMargins(QMargins(0, 0, 0, 0));
        payChart->legend()->hide();
        m_finPayChart->setChart(payChart);
    }
}

void StatsModule::updateServiceAnalysis() {
    if (!m_staffRankTable) return;
    
    // 1. 预备全量数据 (理容师)
    m_allStaffData.clear();
    struct MockStaff { QString id; QString name; QString pos; QString color; };
    QList<MockStaff> staffTemplate = {
        {"S001", "李四", "高级理容师", "#3b82f6"}, {"S002", "王五", "首席美容师", "#8b5cf6"},
        {"S003", "张三", "理容助理", "#10b981"}, {"S004", "赵六", "高级理容师", "#f59e0b"},
        {"S005", "孙梅", "理容助理", "#6366f1"}, {"S006", "周莉", "资深美容师", "#ec4899"},
        {"S007", "韩梅", "高级理容师", "#94a3b8"}, {"S008", "李雷", "理容助理", "#94a3b8"}
    };
    // 根据时间维度调整倍率
    int sMult = (m_staffTimeRange == 0) ? 1 : (m_staffTimeRange == 1 ? 25 : 300);

    for (const auto &s : staffTemplate) {
        int count = QRandomGenerator::global()->bounded(1, 10) * sMult;
        double rev = count * QRandomGenerator::global()->bounded(80, 200);
        m_allStaffData.append({s.name, s.id, s.pos, s.color, count, rev});
    }
    // 2. 服务排行数据已移除

    // 3. 执行首次分页渲染
    m_staffPage = 0;
    m_staffPage = 0;
    updateStaffTable();

    // 4. 顶部图表已在 refreshData 中全局更新
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

    // 根据时间维度调整倍率
    int pMult = (m_productTimeRange == 0) ? 1 : (m_productTimeRange == 1 ? 30 : 365);

    // 1. 商品数据 (Mock)
    m_allProductData.clear();
    QStringList prodNames = {"皇家猫粮 2kg", "混合猫砂 10L", "体内驱虫药", "皮脂护理喷雾", "磨牙棒 5支装", "猫薄荷球", "冻干鸡肉粒", "自动喂食机", "宠物牵引绳", "洗澡按摩梳"};
    for (const auto &name : prodNames) {
        int count = QRandomGenerator::global()->bounded(1, 10) * pMult;
        m_allProductData.append({name, count, (double)count * QRandomGenerator::global()->bounded(20, 150)});
    }

    // 2. 类目数据 (Mock)
    m_allCategoryData.clear();
    QStringList catNames = {"食品零食", "清洁用品", "日常用品", "医疗保健", "智能硬件", "户外出行"};
    for (const auto &name : catNames) {
        int count = QRandomGenerator::global()->bounded(5, 50) * pMult;
        m_allCategoryData.append({name, count, (double)count * QRandomGenerator::global()->bounded(15, 80)});
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
        auto *item0 = new QTableWidgetItem(d.name);
        auto *item1 = new QTableWidgetItem(QString::number(d.count));
        auto *item2 = new QTableWidgetItem(QString("¥ %1").arg(QString::number(d.rev, 'f', 2)));
        item1->setTextAlignment(Qt::AlignCenter); item2->setTextAlignment(Qt::AlignCenter);
        m_productRankTable->setItem(r, 0, item0);
        m_productRankTable->setItem(r, 1, item1);
        m_productRankTable->setItem(r, 2, item2);
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
    double pMult = 1.0;
    if (m_serviceRankTimeRange == 0) pMult = 0.05;
    else if (m_serviceRankTimeRange == 1) pMult = 1.0;
    else if (m_serviceRankTimeRange == 2) pMult = 12.0;

    m_allServiceData.clear();
    QStringList svcNames = {"精细洗澡", "全身造型剪毛", "洁耳洁齿", "精修指甲", "SPA水疗", "局部除毛", "精油开结", "驱虫护理"};
    for (const auto &name : svcNames) {
        int count = QRandomGenerator::global()->bounded(10, 50) * pMult;
        m_allServiceData.append({name, count, (double)count * QRandomGenerator::global()->bounded(50, 300)});
    }

    m_allServiceCategoryData.clear();
    QStringList catNames = {"基础护理", "洗浴造型", "高级SPA", "专项护理"};
    for (const auto &name : catNames) {
        int count = QRandomGenerator::global()->bounded(30, 100) * pMult;
        m_allServiceCategoryData.append({name, count, (double)count * QRandomGenerator::global()->bounded(80, 500)});
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
    auto updatePie = [&](QChartView* view, const QStringList &names, const QList<double> &vals, const QList<QColor> &colors) {
        QChart *chart = new QChart();
        chart->setBackgroundVisible(false);
        QPieSeries *series = new QPieSeries();
        series->setHoleSize(0.4); 
        series->setPieSize(0.7); 
        for (int i = 0; i < names.size(); ++i) {
            QPieSlice *slice = series->append(names[i], vals[i]);
            slice->setBrush(colors[i]);
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setLabelArmLengthFactor(0.02);
            slice->setLabelFont(QFont("Microsoft YaHei", 10, QFont::Bold));
            slice->setLabelColor(QColor("#475569")); 
        }
        for (auto slice : series->slices()) {
            slice->setLabel(QString("%1 %2%").arg(slice->label()).arg(QString::number(slice->percentage() * 100, 'f', 1)));
        }
        chart->addSeries(series);
        chart->setMargins(QMargins(0, 0, 0, 0)); // 移除图表内部边距
        chart->legend()->hide();
        view->setChart(chart);
    };

    updatePie(m_serviceHeatmapChart, 
        {"洗护", "寄养", "医疗", "其他"}, 
        {55.0, 25.0, 15.0, 5.0}, 
        {QColor("#3b82f6"), QColor("#10b981"), QColor("#f59e0b"), QColor("#94a3b8")});

    updatePie(m_productCategoryChart, 
        {"主粮", "零食", "用品", "玩具"}, 
        {45.0, 30.0, 15.0, 10.0}, 
        {QColor("#8b5cf6"), QColor("#ec4899"), QColor("#06b6d4"), QColor("#64748b")});

    updatePie(m_finCompChart,
        {"洗护营收", "商品营收", "寄养营收", "会员充值", "其他"},
        {40.0, 30.0, 15.0, 10.0, 5.0},
        {QColor("#3b82f6"), QColor("#8b5cf6"), QColor("#10b981"), QColor("#f59e0b"), QColor("#94a3b8")});

    updatePie(m_finPayChart,
        {"微信支付", "支付宝", "会员卡", "现金"},
        {50.0, 30.0, 15.0, 5.0},
        {QColor("#22c55e"), QColor("#3b82f6"), QColor("#f59e0b"), QColor("#94a3b8")});
}

void StatsModule::updateCards() {
    if (m_cardValues.size() < 3) return;

    int selYear = m_yearPicker->currentText().left(4).toInt();
    int selMonth = m_monthPicker->currentIndex() + 1;
    int rangeIdx = m_trendRangeCombo->currentIndex();

    QDate curStart, curEnd, prevStart, prevEnd;

    if (rangeIdx == 1) { // 年度
        curStart = QDate(selYear, 1, 1); curEnd = QDate(selYear, 12, 31);
        prevStart = QDate(selYear - 1, 1, 1); prevEnd = QDate(selYear - 1, 12, 31);
    } else if (rangeIdx == 2) { // 月度
        curStart = QDate(selYear, selMonth, 1); curEnd = curStart.addMonths(1).addDays(-1);
        prevStart = curStart.addMonths(-1); prevEnd = curStart.addDays(-1);
    } else { // 历年
        curStart = QDate(selYear - 4, 1, 1); curEnd = QDate(selYear, 12, 31);
        prevStart = QDate(selYear - 9, 1, 1); prevEnd = QDate(selYear - 5, 12, 31);
    }

    auto updateCard = [&](int idx, const QString &val, double diff) {
        m_cardValues[idx]->setText(val);
        if (diff != 0) {
            QString sign = diff >= 0 ? "↑" : "↓";
            QString color = diff >= 0 ? "#22c55e" : "#ef4444";
            m_cardTrends[idx]->setText(QString("%1 %2%").arg(sign).arg(QString::number(qAbs(diff), 'f', 1)));
            m_cardTrends[idx]->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600; border: none;").arg(color));
        } else {
            m_cardTrends[idx]->setText("--");
            m_cardTrends[idx]->setStyleSheet("color: #94a3b8; font-size: 12px; border: none;");
        }
    };

    if (m_currentCategory == 1) { // 店员统计卡片
        // 从已准备好的 m_allStaffData 中提取冠军
        if (m_allStaffData.isEmpty()) {
            updateCard(0, "无", 0);
            updateCard(1, "无", 0);
            updateCard(2, "¥0", 0);
            return;
        }

        // 营收冠军 (m_allStaffData 已经按营收或单量排序，但为了保险我们重新找一次)
        auto topRevIt = std::max_element(m_allStaffData.begin(), m_allStaffData.end(), [](const StaffRankData &a, const StaffRankData &b){
            return a.rev < b.rev;
        });
        
        // 单量冠军
        auto topOrderIt = std::max_element(m_allStaffData.begin(), m_allStaffData.end(), [](const StaffRankData &a, const StaffRankData &b){
            return a.count < b.count;
        });

        double totalRev = 0;
        int totalOrders = 0;
        for(const auto &s : m_allStaffData) {
            totalRev += s.rev;
            totalOrders += s.count;
        }
        double avgEff = totalOrders > 0 ? totalRev / totalOrders : 0;

        updateCard(0, topRevIt->name.left(6), 0);
        updateCard(1, topOrderIt->name.left(6), 0);
        updateCard(2, QString("¥%1").arg(QString::number(avgEff, 'f', 0)), 0);
        
        m_cardTrends[0]->setText(QString("¥%1").arg(QString::number(topRevIt->rev, 'f', 0)));
        m_cardTrends[1]->setText(QString("%1 单").arg(topOrderIt->count));
        m_cardTrends[2]->setText("全店客单均值");
        m_cardTrends[0]->setStyleSheet("color: #64748b; font-size: 13px; font-weight: 600; border: none;");
        m_cardTrends[1]->setStyleSheet("color: #64748b; font-size: 13px; font-weight: 600; border: none;");
        m_cardTrends[2]->setStyleSheet("color: #94a3b8; font-size: 13px; font-weight: 600; border: none;");
    } else if (m_currentCategory == 2) { // 商品统计卡片
        if (m_allProductData.isEmpty()) {
            updateCard(0, "无", 0); updateCard(1, "0", 0); updateCard(2, "¥0", 0);
            return;
        }
        
        // 销量之王 (m_allProductData 通常已经排好序)
        auto topQtyIt = std::max_element(m_allProductData.begin(), m_allProductData.end(), [](const ProductRankData &a, const ProductRankData &b){
            return a.count < b.count;
        });
        
        // 营收之王
        auto topRevIt = std::max_element(m_allProductData.begin(), m_allProductData.end(), [](const ProductRankData &a, const ProductRankData &b){
            return a.rev < b.rev;
        });

        updateCard(0, topQtyIt->name.left(6), 0); // 热销榜首 (按销量)
        updateCard(1, QString::number(topQtyIt->count), 0); // 冠军销量
        updateCard(2, QString("¥%1").arg(QString::number(topRevIt->rev, 'f', 0)), 0); // 冠军营收
        
        m_cardTrends[0]->setText("最受宠爱单品");
        m_cardTrends[1]->setText("本期累计卖出");
        m_cardTrends[2]->setText(topRevIt->name.left(6) + " 贡献");
        m_cardTrends[0]->setStyleSheet("color: #94a3b8; font-size: 13px; font-weight: 600; border: none;");
        m_cardTrends[1]->setStyleSheet("color: #94a3b8; font-size: 13px; font-weight: 600; border: none;");
        m_cardTrends[2]->setStyleSheet("color: #64748b; font-size: 13px; font-weight: 600; border: none;");
    } else if (m_currentCategory == 3) { // 服务排行卡片
        if (m_allServiceData.isEmpty()) {
            updateCard(0, "无", 0); updateCard(1, "0", 0); updateCard(2, "¥0", 0);
            return;
        }
        
        auto topQtyIt = std::max_element(m_allServiceData.begin(), m_allServiceData.end(), [](const SvcRankData &a, const SvcRankData &b){
            return a.count < b.count;
        });
        auto topRevIt = std::max_element(m_allServiceData.begin(), m_allServiceData.end(), [](const SvcRankData &a, const SvcRankData &b){
            return a.rev < b.rev;
        });

        updateCard(0, topQtyIt->name.left(6), 0); // 最受欢迎
        updateCard(1, QString::number(topQtyIt->count), 0); // 服务单量
        updateCard(2, QString("¥%1").arg(QString::number(topRevIt->rev, 'f', 0)), 0); // 服务营收
        
        m_cardTrends[0]->setText("金牌服务项目");
        m_cardTrends[1]->setText("本期服务人次");
        m_cardTrends[2]->setText(topRevIt->name.left(6) + " 贡献");
        m_cardTrends[0]->setStyleSheet("color: #94a3b8; font-size: 13px; font-weight: 600; border: none;");
        m_cardTrends[1]->setStyleSheet("color: #94a3b8; font-size: 13px; font-weight: 600; border: none;");
        m_cardTrends[2]->setStyleSheet("color: #64748b; font-size: 13px; font-weight: 600; border: none;");
    } else {
        // 财务总览卡片 (原有逻辑)
        auto getStats = [&](const QDate &s, const QDate &e) {
            auto orders = PetDataManager::instance()->getOrders(s, e);
            double rev = 0, profit = 0;
            int count = 0;
            for (const auto &o : orders) {
                if (o.status != "Paid") continue;
                rev += o.finalAmount;
                profit += o.finalAmount * 0.65;
                count++;
            }
            double avg = count > 0 ? rev / count : 0;
            return QVector<double>{rev, profit, avg};
        };

        QVector<double> cur = getStats(curStart, curEnd);
        QVector<double> prev = getStats(prevStart, prevEnd);

        auto calcDiff = [](double c, double p) { return p > 0 ? ((c - p) / p) * 100 : 0; };

        updateCard(0, QString("¥%1").arg(QString::number(cur[0], 'f', 0)), calcDiff(cur[0], prev[0]));
        updateCard(1, QString("¥%1").arg(QString::number(cur[1], 'f', 0)), calcDiff(cur[1], prev[1]));
        updateCard(2, QString("¥%1").arg(QString::number(cur[2], 'f', 0)), calcDiff(cur[2], prev[2]));
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
            m_cardTitles[0]->setText("总营收 (REVENUE)");
            m_cardTitles[1]->setText("毛利润 (PROFIT)");
            m_cardTitles[2]->setText("客单价 (AVG ORDER)");
        } else if (index == 1) {
            m_cardTitles[0]->setText("营收冠军 (TOP REVENUE)");
            m_cardTitles[1]->setText("单量冠军 (TOP ORDERS)");
            m_cardTitles[2]->setText("人效均值 (EFFICIENCY)");
        } else if (index == 2) {
            m_cardTitles[0]->setText("热销榜首 (BEST SELLER)");
            m_cardTitles[1]->setText("冠军销量 (TOP SALES)");
            m_cardTitles[2]->setText("冠军营收 (TOP REVENUE)");
        } else if (index == 3) {
            m_cardTitles[0]->setText("最受欢迎 (MOST POPULAR)");
            m_cardTitles[1]->setText("服务单量 (SVC VOLUME)");
            m_cardTitles[2]->setText("服务营收 (SVC REVENUE)");
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
            m_largePreviewLabel->setText(small->text());
            m_largePreviewLabel->setStyleSheet(small->styleSheet() + "border-radius: 100px; font-size: 80px; border: 4px solid white;");
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
        
        // 补偿模拟数据，确保演示效果
        if (total < 100) { 
            svc = QRandomGenerator::global()->bounded(1000, 5000);
            prod = QRandomGenerator::global()->bounded(500, 3000);
            foster = QRandomGenerator::global()->bounded(300, 2000);
            total = svc + prod + foster;
            count = QRandomGenerator::global()->bounded(10, 50);
        }
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
