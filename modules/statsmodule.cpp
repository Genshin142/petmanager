#include "statsmodule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QHeaderView>
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
#include <QToolTip>
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

    // 2. 内容区 (带边距)
    QWidget *contentArea = new QWidget();
    QHBoxLayout *contentLayout = new QHBoxLayout(contentArea);
    contentLayout->setContentsMargins(30, 30, 30, 30);
    contentLayout->setSpacing(30);

    // 左侧核心区
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(30);

    // 1. 统一控制面板 (标题 + 导航 + 数据磁贴)
    QWidget *topControlPanel = new QWidget();
    topControlPanel->setStyleSheet("background: white; border: 1px solid #e2e8f0; border-radius: 16px;");
    QVBoxLayout *panelLayout = new QVBoxLayout(topControlPanel);
    panelLayout->setContentsMargins(25, 25, 25, 25);
    panelLayout->setSpacing(24);

    // A. 标题
    QLabel *title = new QLabel("数据报表统计分析中心");
    title->setStyleSheet("font-size: 20px; font-weight: 800; color: #1e293b; border: none;");
    panelLayout->addWidget(title);

    // B. 导航条
    setupNavigation();
    panelLayout->addWidget(m_navBar);

    // C. 数据磁贴
    setupDashboardCards();
    panelLayout->addWidget(m_cardContainer);

    leftLayout->addWidget(topControlPanel);

    setupMainContent();
    leftLayout->addWidget(m_viewStack, 1);

    contentLayout->addWidget(leftPanel, 1);

    // 右侧抽屉 (Detail)
    m_detailDrawer = new OrderDetailDrawer(this);
    m_detailDrawer->hide();
    contentLayout->addWidget(m_detailDrawer);

    mainLayout->addWidget(contentArea, 1);
}

void StatsModule::setupNavigation() {
    m_navBar = new QWidget();
    m_navBar->setFixedHeight(64);
    m_navBar->setStyleSheet("background: #f1f5f9; border-radius: 10px; border: none;");
    
    QHBoxLayout *navLayout = new QHBoxLayout(m_navBar);
    navLayout->setContentsMargins(10, 0, 10, 0);
    navLayout->setSpacing(8);

    QStringList categories = {"财务总览", "服务分析", "库存追踪", "会员画像"};
    for (int i = 0; i < categories.size(); ++i) {
        QPushButton *btn = new QPushButton(categories[i]);
        btn->setCheckable(true);
        btn->setFixedSize(115, 42);
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
        card->setStyleSheet("background: #f8fafc; border: 1px solid #f1f5f9; border-radius: 12px;");
        QVBoxLayout *vl = new QVBoxLayout(card);
        vl->setContentsMargins(20, 15, 20, 15);
        vl->setSpacing(4);

        QLabel *titleLabel = new QLabel(titles[i]);
        titleLabel->setStyleSheet("color: #64748b; font-size: 11px; font-weight: 700; text-transform: uppercase; border: none;");
        vl->addWidget(titleLabel);

        QLabel *valLabel = new QLabel("--");
        valLabel->setStyleSheet("color: #0f172a; font-size: 24px; font-weight: 800; border: none;");
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
    m_finTrendChart->setFixedHeight(420);
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
        chartView->setFixedHeight(300); // 放大高度
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
    QHBoxLayout *layout = new QHBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(25);

    // 理容师排行
    QWidget *rankPanel = new QWidget();
    rankPanel->setStyleSheet("background: white; border: 1px solid #e2e8f0; border-radius: 12px;");
    QVBoxLayout *rvl = new QVBoxLayout(rankPanel);
    rvl->setContentsMargins(20, 20, 20, 20);
    QLabel *rt = new QLabel("理容师绩效排行榜 (本月)");
    rt->setStyleSheet("font-size: 16px; font-weight: 700; color: #1e293b;");
    rvl->addWidget(rt);

    m_staffRankTable = new QTableWidget();
    m_staffRankTable->setColumnCount(3);
    m_staffRankTable->setHorizontalHeaderLabels({"姓名", "成单量", "营收额"});
    m_staffRankTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_staffRankTable->verticalHeader()->setVisible(false);
    m_staffRankTable->setStyleSheet("border: none;");
    rvl->addWidget(m_staffRankTable);
    layout->addWidget(rankPanel, 3);

    // 服务热力图
    QWidget *heatPanel = new QWidget();
    heatPanel->setStyleSheet("background: white; border: 1px solid #e2e8f0; border-radius: 12px;");
    QVBoxLayout *hvl = new QVBoxLayout(heatPanel);
    hvl->setContentsMargins(20, 20, 20, 20);
    QLabel *ht = new QLabel("服务类目热力分布");
    ht->setStyleSheet("font-size: 16px; font-weight: 700; color: #1e293b;");
    hvl->addWidget(ht);

    m_serviceHeatmapChart = new QChartView();
    hvl->addWidget(m_serviceHeatmapChart);
    layout->addWidget(heatPanel, 2);

    return view;
}

QWidget* StatsModule::createInventoryView() {
    QWidget *view = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(25);

    // 低库存预警
    QWidget *alertBox = new QWidget();
    alertBox->setStyleSheet("background: #fff1f2; border: 1px solid #fecaca; border-radius: 12px;");
    alertBox->setFixedHeight(220);
    QVBoxLayout *avl = new QVBoxLayout(alertBox);
    QLabel *at = new QLabel("⚠️ 低库存预警");
    at->setStyleSheet("color: #991b1b; font-weight: 700; font-size: 14px;");
    avl->addWidget(at);

    m_invAlertTable = new QTableWidget();
    m_invAlertTable->setColumnCount(3);
    m_invAlertTable->setHorizontalHeaderLabels({"商品名称", "库存", "预警值"});
    m_invAlertTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_invAlertTable->setStyleSheet("background: transparent; border: none;");
    avl->addWidget(m_invAlertTable);
    layout->addWidget(alertBox);

    // 销售排行
    QWidget *rankBox = new QWidget();
    rankBox->setStyleSheet("background: white; border: 1px solid #e2e8f0; border-radius: 12px;");
    QVBoxLayout *rvl = new QVBoxLayout(rankBox);
    QLabel *rt = new QLabel("商品动销 Top 排行");
    rt->setStyleSheet("font-weight: 700; font-size: 16px;");
    rvl->addWidget(rt);

    m_productRankTable = new QTableWidget();
    m_productRankTable->setColumnCount(3);
    m_productRankTable->setHorizontalHeaderLabels({"商品", "销量", "营收"});
    m_productRankTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_productRankTable->setStyleSheet("border: none;");
    rvl->addWidget(m_productRankTable);
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
            QString l = it.key() == "MemberCard" ? "会员卡" : (it.key() == "Alipay" ? "支付宝" : "现金");
            QPieSlice *slice = paySeries->append(l, it.value());
            if (payColors.contains(l)) slice->setBrush(payColors[l]);
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
    if (!m_staffRankTable) return;
    m_staffRankTable->setRowCount(0);
    auto staff = StaffDataManager::instance()->activeStaffNames();
    for (const auto &name : staff) {
        int r = m_staffRankTable->rowCount(); m_staffRankTable->insertRow(r);
        m_staffRankTable->setItem(r, 0, new QTableWidgetItem(name));
        m_staffRankTable->setItem(r, 1, new QTableWidgetItem(QString::number(QRandomGenerator::global()->bounded(10, 40))));
        m_staffRankTable->setItem(r, 2, new QTableWidgetItem(QString("¥ %1").arg(QRandomGenerator::global()->bounded(2000, 6000))));
    }
    QChart *heatChart = new QChart();
    heatChart->setBackgroundVisible(false);
    QPieSeries *hs = new QPieSeries();
    hs->append("洗护", 60); hs->append("寄养", 25); hs->append("医疗", 15);
    heatChart->addSeries(hs);
    m_serviceHeatmapChart->setChart(heatChart);
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
