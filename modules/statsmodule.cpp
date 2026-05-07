#include "statsmodule.h"
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
    topSplitLayout->addWidget(m_cardContainer, 3);

    // 扇形图容器 (上移至此)
    QWidget *topPieBox = new QWidget();
    topPieBox->setStyleSheet("background: transparent; border: none;");
    QVBoxLayout *tpvl = new QVBoxLayout(topPieBox);
    tpvl->setContentsMargins(0, 0, 0, 0);
    tpvl->setSpacing(2); // 极窄间距
    
    QLabel *pt = new QLabel("服务类目分布");
    pt->setStyleSheet("font-size: 12px; font-weight: 700; color: #64748b; margin-left: 5px;");
    tpvl->addWidget(pt);

    m_serviceHeatmapChart = new QChartView();
    m_serviceHeatmapChart->setFixedHeight(160); // 顶部区域压缩高度
    m_serviceHeatmapChart->setRenderHint(QPainter::Antialiasing);
    m_serviceHeatmapChart->setStyleSheet("background: transparent; border: none;");
    tpvl->addWidget(m_serviceHeatmapChart);
    topSplitLayout->addWidget(topPieBox, 1);

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
}

void StatsModule::setupNavigation() {
    m_navBar = new QWidget();
    m_navBar->setFixedHeight(48);
    m_navBar->setStyleSheet("background: #f1f5f9; border-radius: 10px; border: none;");
    
    QHBoxLayout *navLayout = new QHBoxLayout(m_navBar);
    navLayout->setContentsMargins(10, 0, 10, 0);
    navLayout->setSpacing(8);

    QStringList categories = {"财务总览", "服务分析", "库存追踪", "会员画像"};
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
    cardLayout->setContentsMargins(0, 10, 0, 10);
    cardLayout->setSpacing(15);

    QStringList titles = {"总营收 (REVENUE)", "毛利润 (PROFIT)", "客单价 (AVG ORDER)", "服务单量 (SERVICES)"};
    m_cardValues.clear();
    m_cardTrends.clear();

    for (int i = 0; i < 4; ++i) {
        QWidget *card = new QWidget();
        card->setStyleSheet("background: #f8fafc; border: none;");
        QVBoxLayout *vl = new QVBoxLayout(card);
        vl->setContentsMargins(15, 10, 15, 10);
        vl->setSpacing(2);

        QLabel *titleLabel = new QLabel(titles[i]);
        titleLabel->setStyleSheet("color: #64748b; font-size: 11px; font-weight: 700; text-transform: uppercase; border: none;");
        vl->addWidget(titleLabel);

        QLabel *valLabel = new QLabel("--");
        valLabel->setStyleSheet("color: #0f172a; font-size: 20px; font-weight: 800; border: none;");
        vl->addWidget(valLabel);
        m_cardValues.append(valLabel);

        QLabel *trendLabel = new QLabel("--");
        trendLabel->setStyleSheet("font-size: 12px; font-weight: 600; border: none;");
        vl->addWidget(trendLabel);
        m_cardTrends.append(trendLabel);

        cardLayout->addWidget(card, 1);
    }
}

void StatsModule::setupMainContent() {
    m_viewStack = new QStackedWidget();
    m_viewStack->setStyleSheet("background: transparent;");

    m_viewStack->addWidget(createFinanceView());
    m_viewStack->addWidget(createServiceView());
    m_viewStack->addWidget(createInventoryView());
    m_viewStack->addWidget(createMemberView());
}

QWidget* StatsModule::createFinanceView() {
    QWidget *view = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // 1. 顶部：营收趋势大图 (带切换选择)
    QWidget *topCard = new QWidget();
    topCard->setStyleSheet("background: white; border: none;");
    QVBoxLayout *topVl = new QVBoxLayout(topCard);
    topVl->setContentsMargins(0, 10, 0, 10);

    QHBoxLayout *chartHeader = new QHBoxLayout();
    QLabel *tl = new QLabel("门店经营营收趋势");
    tl->setStyleSheet("font-size: 15px; font-weight: 700; color: #1e293b; border: none;");
    chartHeader->addWidget(tl);
    chartHeader->addStretch();

    m_trendRangeCombo = new QComboBox();
    m_trendRangeCombo->addItems({"历年对比", "年度走势", "本月走势"});
    m_trendRangeCombo->setFixedWidth(110);
    
    m_yearPicker = new QComboBox();
    for(int y=2026; y>=2022; --y) m_yearPicker->addItem(QString::number(y) + "年");
    m_yearPicker->setFixedWidth(90);
    m_yearPicker->setVisible(false);

    m_monthPicker = new QComboBox();
    for(int m=1; m<=12; ++m) m_monthPicker->addItem(QString::number(m) + "月");
    m_monthPicker->setFixedWidth(75);
    m_monthPicker->setCurrentIndex(QDate::currentDate().month() - 1);
    m_monthPicker->setVisible(false);

    QString comboStyle = 
        "QComboBox { background: #f1f5f9; border: 1px solid #e2e8f0; border-radius: 6px; padding: 4px 8px; color: #475569; font-weight: 600; } "
        "QComboBox::drop-down { border: none; } "
        "QComboBox::down-arrow { image: none; border-left: 4px solid transparent; border-right: 4px solid transparent; border-top: 4px solid #64748b; margin-right: 4px; }";
    
    m_trendRangeCombo->setStyleSheet(comboStyle);
    m_yearPicker->setStyleSheet(comboStyle);
    m_monthPicker->setStyleSheet(comboStyle);

    auto updatePickerVisibility = [this]() {
        int idx = m_trendRangeCombo->currentIndex();
        m_yearPicker->setVisible(idx == 1 || idx == 2);
        m_monthPicker->setVisible(idx == 2);
    };

    connect(m_trendRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, updatePickerVisibility](){
        updatePickerVisibility();
        refreshData();
    });
    connect(m_yearPicker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatsModule::refreshData);
    connect(m_monthPicker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatsModule::refreshData);

    chartHeader->addWidget(m_trendRangeCombo);
    chartHeader->addWidget(m_yearPicker);
    chartHeader->addWidget(m_monthPicker);
    topVl->addLayout(chartHeader);

    m_finTrendChart = new QChartView();
    m_finTrendChart->setFixedHeight(280); // 压缩高度以适应一屏
    m_finTrendChart->setRenderHint(QPainter::Antialiasing);
    m_finTrendChart->setStyleSheet("background: transparent; border: none;");
    
    m_customTooltip = new QLabel(m_finTrendChart);
    m_customTooltip->setWindowFlags(Qt::ToolTip);
    m_customTooltip->setStyleSheet(
        "background: white; border: 1px solid #3b82f6; border-radius: 8px; "
        "padding: 10px; color: #1e293b; font-size: 13px; font-weight: 600; "
        "box-shadow: 0 4px 6px -1px rgb(0 0 0 / 0.1);"
    );
    m_customTooltip->hide();

    topVl->addWidget(m_finTrendChart);
    layout->addWidget(topCard);

    // 2. 底部：并排分布图
    QWidget *bottomArea = new QWidget();
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomArea);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(20);

    auto createChartCard = [&](const QString &title, QChartView* &chartView) {
        QWidget *card = new QWidget();
        card->setStyleSheet("background: white; border: none;"); // 移除内部边框
        QVBoxLayout *vl = new QVBoxLayout(card);
        vl->setContentsMargins(0, 10, 0, 10);
        QLabel *l = new QLabel(title);
        l->setStyleSheet("font-size: 14px; font-weight: 700; color: #1e293b; border: none;");
        vl->addWidget(l);
        chartView = new QChartView();
        chartView->setFixedHeight(240); // 压缩高度
        chartView->setStyleSheet("background: transparent; border: none;");
        vl->addWidget(chartView);
        return card;
    };

    bottomLayout->addWidget(createChartCard("营收项目构成", m_finCompChart), 1);
    bottomLayout->addWidget(createChartCard("支付渠道分布", m_finPayChart), 1);
    layout->addWidget(bottomArea);

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

    // 1. 左侧快捷筛选组 (胶囊化)
    QFrame *timeGroup = new QFrame();
    timeGroup->setObjectName("GlobalTimeToggle");
    timeGroup->setAttribute(Qt::WA_StyledBackground);
    QHBoxLayout *tgl = new QHBoxLayout(timeGroup);
    tgl->setContentsMargins(4, 4, 4, 4);
    tgl->setSpacing(2);
    timeGroup->setStyleSheet(
        "QFrame#GlobalTimeToggle { background-color: #f1f5f9; border-radius: 18px; }"
        "QPushButton { border: none; border-radius: 14px; padding: 6px 18px; font-size: 12px; color: #64748b; background: transparent; font-weight: 500; } "
        "QPushButton:checked { background: #3b82f6; color: white; font-weight: bold; }"
        "QPushButton:hover:!checked { background: #e2e8f0; }"
    );

    QStringList periods = {QString::fromUtf8("今日"), QString::fromUtf8("昨日"), 
                          QString::fromUtf8("本月"), QString::fromUtf8("上月"), 
                          QString::fromUtf8("今年")};
    QButtonGroup *tg = new QButtonGroup(timeGroup);
    tg->setExclusive(true);

    // 用于刷新标题的函数指针容器
    QList<std::function<void(int)>> titleUpdaters;

    for (int i = 0; i < periods.size(); ++i) {
        QPushButton *btn = new QPushButton(periods[i]);
        btn->setCheckable(true);
        if (i == 2) btn->setChecked(true);
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
    historyGroup->setStyleSheet("QFrame#HistoryGroup { background-color: #f1f5f9; border-radius: 18px; }");

    QLabel *hl = new QLabel(QString::fromUtf8("按月查看"));
    hl->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; border: none; background: transparent;");
    hgl->addWidget(hl);

    QString comboStyle = 
        "QComboBox { background: white; border: none; border-radius: 14px; padding: 4px 12px; color: #475569; font-size: 12px; font-weight: 600; min-width: 75px; } "
        "QComboBox:hover { color: #3b82f6; } "
        "QComboBox::drop-down { border: none; width: 15px; } "
        "QComboBox::down-arrow { image: none; border-left: 3px solid transparent; border-right: 3px solid transparent; border-top: 4px solid #94a3b8; margin-right: 5px; } "
        "QAbstractItemView { border: 1px solid #e2e8f0; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; border-radius: 8px; }";

    QComboBox *yearCombo = new QComboBox();
    for(int y=2023; y<=2026; ++y) yearCombo->addItem(QString::number(y) + QString::fromUtf8("年"), y);
    yearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + QString::fromUtf8("年"));
    yearCombo->setStyleSheet(comboStyle);
    
    QComboBox *monthCombo = new QComboBox();
    for(int m=1; m<=12; ++m) monthCombo->addItem(QString::number(m) + QString::fromUtf8("月"), m);
    monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);
    monthCombo->setStyleSheet(comboStyle);
    
    hgl->addWidget(yearCombo);
    hgl->addWidget(monthCombo);
    fhl->addWidget(historyGroup);

    fhl->addStretch();

    auto refreshByHistory = [=, &titleUpdaters](){
        // 如果快捷按钮有选中的，先取消它
        if(tg->checkedButton()) {
            tg->setExclusive(false);
            tg->checkedButton()->setChecked(false);
            tg->setExclusive(true);
        }
        
        m_staffTimeRange = 1; // 始终按月模拟
        m_serviceTimeRange = 1;
        m_staffPage = 0; m_servicePage = 0;
        for(auto &updater : titleUpdaters) updater(-1); // 特殊信号告知使用自定义标题
        updateServiceAnalysis(); 
    };

    connect(yearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, refreshByHistory);
    connect(monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, refreshByHistory);

    // 导出按钮
    QPushButton *exportBtn = new QPushButton(QString::fromUtf8(" 导出报表 "));
    exportBtn->setStyleSheet("QPushButton { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 6px; color: #64748b; font-size: 12px; padding: 5px 12px; } QPushButton:hover { background: #f1f5f9; }");
    fhl->addWidget(exportBtn);

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
            toggleGroup->setAttribute(Qt::WA_StyledBackground); 
            QHBoxLayout *stgl = new QHBoxLayout(toggleGroup);
            stgl->setContentsMargins(4, 2, 4, 2);
            stgl->setSpacing(5);
            toggleGroup->setStyleSheet(
                "QFrame#SortToggle { background-color: white; border: 1px solid #e2e8f0; border-radius: 14px; }"
                "QPushButton { border: none; background: transparent; color: #64748b; font-size: 11px; padding: 4px 10px; outline: none; }"
                "QPushButton:checked { color: #3b82f6; font-weight: bold; }"
            );
            auto btnRev = new QPushButton(QString::fromUtf8("按营收"), toggleGroup);
            auto btnCount = new QPushButton(QString::fromUtf8("按单量"), toggleGroup);
            btnRev->setCheckable(true); btnCount->setCheckable(true);
            btnRev->setChecked(true); 
            QButtonGroup *group = new QButtonGroup(toggleGroup);
            group->addButton(btnRev); group->addButton(btnCount);
            stgl->addWidget(btnRev); stgl->addWidget(btnCount);

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
        titleUpdaters.append([=](int id){
            QString suffix = QString::fromUtf8(" (本月)");
            if(id == 0) suffix = QString::fromUtf8(" (今日)");
            else if(id == 1) suffix = QString::fromUtf8(" (昨日)");
            else if(id == 3) suffix = QString::fromUtf8(" (上月)");
            else if(id == 4) suffix = QString::fromUtf8(" (今年)");
            else if(id == -1) suffix = QString(" (%1-%2)").arg(yearCombo->currentData().toInt()).arg(monthCombo->currentData().toInt(), 2, 10, QChar('0'));
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

    rankLayout->addWidget(createRankPanel(QString::fromUtf8("理容师绩效排行"), m_staffRankTable, {"头像", "ID", "姓名", "岗位", "单量", "营收"}, true), 1);
    rankLayout->addWidget(createRankPanel(QString::fromUtf8("服务项目排行"), m_serviceRankTable, {"项目", "单量", "营收"}, false), 1);

    // 全局信号连接
    connect(tg, &QButtonGroup::idClicked, this, [=, titleUpdaters](int id){
        // 点击快捷按钮时，重置下拉框到当前年月（避免显示冲突）
        yearCombo->blockSignals(true);
        monthCombo->blockSignals(true);
        yearCombo->setCurrentText(QString::number(QDate::currentDate().year()) + QString::fromUtf8("年"));
        monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);
        yearCombo->blockSignals(false);
        monthCombo->blockSignals(false);

        // 映射 id 到 range (0:今日, 1:昨日, 2:本月, 3:上月, 4:今年)
        int rangeMap[] = {0, 0, 1, 1, 2}; 
        m_staffTimeRange = rangeMap[id];
        m_serviceTimeRange = rangeMap[id];
        m_staffPage = 0;
        m_servicePage = 0;
        
        for(auto &updater : titleUpdaters) updater(id);
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

QWidget* StatsModule::createInventoryView() {
    QWidget *view = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(25);

    // 低库存预警
    QWidget *alertBox = new QWidget();
    alertBox->setStyleSheet("background: #fff1f2; border: none;"); // 移除圆角边框
    alertBox->setFixedHeight(220);
    QVBoxLayout *avl = new QVBoxLayout(alertBox);
    QLabel *at = new QLabel("⚠️ 低库存预警");
    at->setStyleSheet("color: #991b1b; font-weight: 700; font-size: 14px;");
    avl->addWidget(at);

    m_invAlertTable = new QTableWidget();
    m_invAlertTable->setColumnCount(3);
    m_invAlertTable->setHorizontalHeaderLabels({"商品名称", "库存", "预警值"});
    m_invAlertTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_invAlertTable->verticalHeader()->setVisible(false);
    m_invAlertTable->verticalHeader()->setDefaultSectionSize(48);
    m_invAlertTable->setShowGrid(false);
    m_invAlertTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_invAlertTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_invAlertTable->setFocusPolicy(Qt::NoFocus);
    m_invAlertTable->setItemDelegate(new StatsRowDelegate(m_invAlertTable));
    m_invAlertTable->setStyleSheet("QTableWidget { border: none; background: transparent; outline: none; } "
                                   "QHeaderView::section { background: #fee2e2; color: #991b1b; font-weight: bold; border: none; height: 36px; } "
                                   "QScrollBar:vertical { background: transparent; width: 8px; margin: 0px; } "
                                   "QScrollBar::handle:vertical { background: #fecaca; min-height: 30px; border-radius: 4px; } "
                                   "QScrollBar::handle:vertical:hover { background: #f87171; } "
                                   "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } "
                                   "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
    avl->addWidget(m_invAlertTable);
    layout->addWidget(alertBox);

    // 销售排行
    QWidget *rankBox = new QWidget();
    rankBox->setStyleSheet("background: white; border: none;"); 
    QVBoxLayout *rivl = new QVBoxLayout(rankBox);
    QLabel *rit = new QLabel("商品动销 Top 排行");
    rit->setStyleSheet("font-weight: 700; font-size: 16px; color: #1e293b;");
    rivl->addWidget(rit);

    m_productRankTable = new QTableWidget();
    m_productRankTable->setColumnCount(3);
    m_productRankTable->setHorizontalHeaderLabels({"商品", "销量", "营收"});
    m_productRankTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_productRankTable->verticalHeader()->setVisible(false);
    m_productRankTable->verticalHeader()->setDefaultSectionSize(48);
    m_productRankTable->setShowGrid(false);
    m_productRankTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_productRankTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_productRankTable->setFocusPolicy(Qt::NoFocus);
    m_productRankTable->setItemDelegate(new StatsRowDelegate(m_productRankTable));
    m_productRankTable->setStyleSheet("QTableWidget { border: none; background: transparent; outline: none; } "
                                     "QHeaderView::section { background: #f8fafc; color: #64748b; font-weight: bold; border: none; height: 36px; } "
                                     "QScrollBar:vertical { background: transparent; width: 8px; margin: 0px; } "
                                     "QScrollBar::handle:vertical { background: #cbd5e1; min-height: 30px; border-radius: 4px; } "
                                     "QScrollBar::handle:vertical:hover { background: #94a3b8; } "
                                     "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } "
                                     "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
    rivl->addWidget(m_productRankTable);
    layout->addWidget(rankBox, 1);

    return view;
}

QWidget* StatsModule::createMemberView() {
    QWidget *view = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    QLabel *p = new QLabel("会员增长与画像分析中心 (开发中)");
    p->setAlignment(Qt::AlignCenter);
    p->setStyleSheet("color: #94a3b8; font-size: 16px; font-weight: 600;");
    layout->addWidget(p);
    return view;
}

void StatsModule::refreshData() {
    updateCards();
    updateCharts();
    if (m_currentCategory == 1) updateServiceAnalysis();
    else if (m_currentCategory == 2) updateInventoryAnalysis();
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
        compChart->setAnimationOptions(QChart::SeriesAnimations);
        QPieSeries *compSeries = new QPieSeries();
        compSeries->setHoleSize(0.35); // 变成环形图更有现代感
        
        QPieSlice *s1 = compSeries->append("服务营收", svc);
        QPieSlice *s2 = compSeries->append("商品销售", prod);
        s1->setBrush(QColor("#3b82f6")); s2->setBrush(QColor("#0ea5e9"));
        
        for (auto slice : compSeries->slices()) {
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setLabel(QString("%1\n%2%").arg(slice->label()).arg(QString::number(slice->percentage() * 100, 'f', 1)));
        }
        compChart->addSeries(compSeries);
        compChart->legend()->hide();
        m_finCompChart->setChart(compChart);

        // 支付方式饼图
        QChart *payChart = new QChart();
        payChart->setBackgroundVisible(false);
        payChart->setAnimationOptions(QChart::SeriesAnimations);
        QPieSeries *paySeries = new QPieSeries();
        paySeries->setHoleSize(0.35);
        
        QMap<QString, QColor> payColors = {{"会员卡", QColor("#6366f1")}, {"支付宝", QColor("#06b6d4")}, {"现金", QColor("#94a3b8")}};
        for(auto it = payMap.begin(); it != payMap.end(); ++it) {
            QString label = it.key() == "MemberCard" ? "会员卡" : (it.key() == "Alipay" ? "支付宝" : "现金");
            QPieSlice *slice = paySeries->append(label, it.value());
            if (payColors.contains(label)) slice->setBrush(payColors[label]);
        }
        
        for (auto slice : paySeries->slices()) {
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setLabel(QString("%1\n%2%").arg(slice->label()).arg(QString::number(slice->percentage() * 100, 'f', 1)));
        }
        payChart->addSeries(paySeries);
        payChart->legend()->hide();
        m_finPayChart->setChart(payChart);
    }
}

void StatsModule::updateServiceAnalysis() {
    if (!m_staffRankTable || !m_serviceRankTable) return;
    
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
    // 2. 预备全量数据 (服务)
    m_allServiceData.clear();
    auto allSvc = ServiceDataManager::instance()->activeServices();
    int vMult = (m_serviceTimeRange == 0) ? 1 : (m_serviceTimeRange == 1 ? 20 : 250);

    for (const auto &svc : allSvc) {
        bool hasSales = QRandomGenerator::global()->bounded(100) > 20;
        int count = hasSales ? QRandomGenerator::global()->bounded(2, 15) * vMult : 0;
        m_allServiceData.append({svc.name, count, count * svc.price});
    }

    // 3. 执行首次分页渲染
    m_staffPage = 0;
    m_servicePage = 0;
    updateStaffTable();
    updateServiceTable();

    // 4. 服务类目占比 (带百分比与引线)
    QChart *heatChart = new QChart();
    heatChart->setBackgroundVisible(false);
    heatChart->setAnimationOptions(QChart::SeriesAnimations);
    
    QPieSeries *hs = new QPieSeries();
    hs->setHoleSize(0.35); 
    
    struct CatData { QString name; double val; QColor color; };
    QList<CatData> data = {
        {"洗护", 55.0, QColor("#3b82f6")},
        {"寄养", 25.0, QColor("#10b981")},
        {"医疗", 15.0, QColor("#f59e0b")},
        {"其他", 5.0, QColor("#94a3b8")}
    };

    for (const auto &d : data) {
        QPieSlice *slice = hs->append(d.name, d.val);
        slice->setBrush(d.color);
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelFont(QFont("Microsoft YaHei", 8, QFont::Bold));
    }
    
    for (auto slice : hs->slices()) {
        slice->setLabel(QString("%1\n%2%").arg(slice->label()).arg(QString::number(slice->percentage() * 100, 'f', 1)));
    }

    heatChart->addSeries(hs);
    heatChart->legend()->hide();
    m_serviceHeatmapChart->setChart(heatChart);
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

void StatsModule::updateInventoryAnalysis() {
    if (!m_invAlertTable) return;
    m_invAlertTable->setRowCount(0);
    auto low = ProductDataManager::instance()->getLowStockItems();
    for (const auto &it : low) {
        int r = m_invAlertTable->rowCount(); m_invAlertTable->insertRow(r);
        m_invAlertTable->setItem(r, 0, new QTableWidgetItem(it.name));
        m_invAlertTable->setItem(r, 1, new QTableWidgetItem(QString::number(ProductDataManager::instance()->calculateTotalStock(it.barcode))));
        m_invAlertTable->setItem(r, 2, new QTableWidgetItem("10"));
    }
    m_productRankTable->setRowCount(0);
    QStringList top = {"皇家猫粮", "混合猫砂", "驱虫药"};
    for (const auto &n : top) {
        int r = m_productRankTable->rowCount(); m_productRankTable->insertRow(r);
        m_productRankTable->setItem(r, 0, new QTableWidgetItem(n));
    m_productRankTable->setItem(r, 1, new QTableWidgetItem(QString::number(QRandomGenerator::global()->bounded(20, 100))));
        m_productRankTable->setItem(r, 2, new QTableWidgetItem(QString("¥ %1").arg(QRandomGenerator::global()->bounded(1000, 3000))));
    }
}

void StatsModule::updateCards() {
    if (m_cardValues.size() < 4) return;

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
    } else { // 历年 (对比近5年 vs 之前5年)
        curStart = QDate(selYear - 4, 1, 1); curEnd = QDate(selYear, 12, 31);
        prevStart = QDate(selYear - 9, 1, 1); prevEnd = QDate(selYear - 5, 12, 31);
    }

    auto getStats = [&](const QDate &s, const QDate &e) {
        auto orders = PetDataManager::instance()->getOrders(s, e);
        double rev = 0, profit = 0;
        int count = 0;
        for (const auto &o : orders) {
            if (o.status != "Paid") continue;
            rev += o.finalAmount;
            profit += o.finalAmount * 0.65; // 假设综合毛利 65%
            count++;
        }
        double avg = count > 0 ? rev / count : 0;
        return QVector<double>{rev, profit, avg, (double)count};
    };

    QVector<double> cur = getStats(curStart, curEnd);
    QVector<double> prev = getStats(prevStart, prevEnd);

    auto updateCard = [&](int idx, double current, double previous, bool isCurrency) {
        // 更新主数值
        if (isCurrency) m_cardValues[idx]->setText(QString("¥ %1").arg(QString::number(current, 'f', 2)));
        else m_cardValues[idx]->setText(QString::number((int)current));

        // 更新趋势
        if (previous > 0) {
            double diff = ((current - previous) / previous) * 100;
            QString sign = diff >= 0 ? "↑" : "↓";
            QString color = diff >= 0 ? "#22c55e" : "#ef4444";
            m_cardTrends[idx]->setText(QString("%1 %2%").arg(sign).arg(QString::number(qAbs(diff), 'f', 1)));
            m_cardTrends[idx]->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600; border: none;").arg(color));
        } else {
            m_cardTrends[idx]->setText("--");
            m_cardTrends[idx]->setStyleSheet("color: #94a3b8; font-size: 12px; border: none;");
        }
    };

    updateCard(0, cur[0], prev[0], true);  // 营收
    updateCard(1, cur[1], prev[1], true);  // 毛利
    updateCard(2, cur[2], prev[2], true);  // 客单价
    updateCard(3, cur[3], prev[3], false); // 单量
}

void StatsModule::onCategoryChanged(int index) {
    m_currentCategory = index;
    for (int i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setChecked(i == index);
    }
    m_viewStack->setCurrentIndex(index);
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
