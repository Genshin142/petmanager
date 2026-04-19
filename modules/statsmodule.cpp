#include "statsmodule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
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

StatsModule::StatsModule(QWidget *parent) : QWidget(parent), isServiceMode(true) {
    // 填充模拟销售数据
    QDate today = QDate::currentDate();
    
    // 服务类数据 (近一周)
    m_sales << SalesRecord{today, "猫咪全身洗护", 8, 640, false};
    m_sales << SalesRecord{today, "精修造型", 5, 1000, false};
    m_sales << SalesRecord{today.addDays(-1), "疫苗套餐", 12, 2400, false};
    m_sales << SalesRecord{today.addDays(-1), "狗狗日常代喂", 10, 1000, false};
    m_sales << SalesRecord{today.addDays(-2), "猫咪全身洗护", 15, 1200, false};
    m_sales << SalesRecord{today.addDays(-2), "宠物 SPA 精油护理", 8, 1600, false};
    m_sales << SalesRecord{today.addDays(-3), "精修造型", 12, 2400, false};
    m_sales << SalesRecord{today.addDays(-5), "指甲修剪", 20, 1000, false};

    // 商品类数据 (近两周)
    m_sales << SalesRecord{today, "皇家全价猫粮 2kg", 15, 2985, true};
    m_sales << SalesRecord{today, "混合猫砂 6L", 20, 700, true};
    m_sales << SalesRecord{today.addDays(-1), "宠物免洗手套", 18, 270, true};
    m_sales << SalesRecord{today.addDays(-2), "皇家全价猫粮 2kg", 10, 1990, true};
    m_sales << SalesRecord{today.addDays(-4), "伊丽莎白圈 M号", 12, 300, true};
    m_sales << SalesRecord{today.addDays(-7), "除臭喷剂 300ml", 8, 360, true};
    m_sales << SalesRecord{today.addDays(-10), "牵引绳 中型犬", 5, 150, true};

    setupUI();
    setupCharts();
    onFilter();
}

void StatsModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    // 1. 标题和上方按钮
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *title = new QLabel("数据报表统计分析中心");
    title->setStyleSheet("font-size: 24px; color: #303133; font-weight: bold;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    
    QPushButton *exportBtn = new QPushButton("导出报表文件");
    exportBtn->setFixedWidth(130);
    exportBtn->setFixedHeight(38);
    exportBtn->setCursor(Qt::PointingHandCursor);
    exportBtn->setStyleSheet(
        "QPushButton { background: #F59E0B; color: white; border-radius: 8px; border: none; font-size: 13px; font-weight: 600; } "
        "QPushButton:hover { background: #D97706; transform: translateY(-1px); } "
        "QPushButton:pressed { background: #B45309; }"
    );
    connect(exportBtn, &QPushButton::clicked, this, &StatsModule::onExport);
    headerLayout->addWidget(exportBtn);
    mainLayout->addLayout(headerLayout);

    // 2. 仪表盘卡片
    QHBoxLayout *dashLayout = new QHBoxLayout();
    dashLayout->setSpacing(20);

    auto createDash = [&](const QString &icon, const QString &label, QLabel* &valLabel, const QString &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(110);
        // 使用设计系统中的深邃蓝阴影和圆角
        card->setStyleSheet(
            "QFrame { background: white; border-radius: 12px; border: 1px solid #E2E8F0; } "
            "QFrame:hover { border-color: #3B82F6; }"
        );
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(20);
        shadow->setColor(QColor(30, 64, 175, 25)); // 带蓝色的深邃阴影
        shadow->setOffset(0, 4);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *cl = new QHBoxLayout(card);
        cl->setContentsMargins(22, 18, 22, 18);

        QLabel *ic = new QLabel(icon);
        ic->setFixedSize(54, 54);
        ic->setAlignment(Qt::AlignCenter);
        // 玻璃拟态背景
        ic->setStyleSheet(QString(
            "font-size: 26px; color: %1; background: #F1F5F9; border-radius: 12px; border: none;"
        ).arg(color));

        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(4);
        QLabel *tl = new QLabel(label); 
        tl->setStyleSheet("color: #64748B; font-size: 13px; font-weight: 500; border: none; background: transparent;");
        valLabel = new QLabel("0"); 
        valLabel->setStyleSheet("color: #1E3A8A; font-size: 26px; font-weight: 700; border: none; background: transparent;");
        vl->addWidget(tl); vl->addWidget(valLabel);
        vl->addStretch();
        cl->addWidget(ic); cl->addSpacing(18); cl->addLayout(vl); cl->addStretch();
        return card;
    };

    dashLayout->addWidget(createDash("💰", "区间总营收", todayRevenueLabel, "#67c23a"));
    dashLayout->addWidget(createDash("🧾", "区间客单价", avgOrderLabel, "#409eff"));
    dashLayout->addWidget(createDash("💇", "服务单量汇总", serviceCountLabel, "#e6a23c"));
    dashLayout->addWidget(createDash("📦", "商品出单量", productCountLabel, "#f56c6c"));
    mainLayout->addLayout(dashLayout);

    // 3. 图表展示区
    QHBoxLayout *chartsLayout = new QHBoxLayout();
    chartsLayout->setSpacing(20);

    pieChartView = new QChartView();
    pieChartView->setRenderHint(QPainter::Antialiasing);
    pieChartView->setStyleSheet("background: white; border-radius: 12px; border: 1px solid #f0f2f5;");
    pieChartView->setFixedHeight(280);

    barChartView = new QChartView();
    barChartView->setRenderHint(QPainter::Antialiasing);
    barChartView->setStyleSheet("background: white; border-radius: 12px; border: 1px solid #f0f2f5;");
    barChartView->setFixedHeight(280);

    chartsLayout->addWidget(pieChartView, 2);
    chartsLayout->addWidget(barChartView, 3);
    mainLayout->addLayout(chartsLayout);

    // 4. 控制栏：分类切换 + 日期范围
    QFrame *controlFrame = new QFrame();
    controlFrame->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #ebeef5; }");
    controlFrame->setFixedHeight(64);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlFrame);
    controlLayout->setContentsMargins(15, 0, 15, 0);

    // 分类切换
    QHBoxLayout *typeToggleLayout = new QHBoxLayout();
    typeToggleLayout->setSpacing(0);
    
    serviceBtn = new QPushButton("服务类分析");
    serviceBtn->setFixedWidth(120);
    serviceBtn->setFixedHeight(36);
    serviceBtn->setCheckable(true);
    serviceBtn->setChecked(true);
    serviceBtn->setStyleSheet(
        "QPushButton { background: #F8FAFC; color: #64748B; border: 1px solid #E2E8F0; border-right: none; border-radius: 8px 0 0 8px; font-size: 13px; font-weight: 600; } "
        "QPushButton:checked { background: #1E40AF; color: white; border: 1px solid #1E40AF; } "
        "QPushButton:hover:!checked { background: #F1F5F9; }"
    );
    
    productBtn = new QPushButton("商品类分析");
    productBtn->setFixedWidth(120);
    productBtn->setFixedHeight(36);
    productBtn->setCheckable(true);
    productBtn->setStyleSheet(
        "QPushButton { background: #F8FAFC; color: #64748B; border: 1px solid #E2E8F0; border-radius: 0 8px 8px 0; font-size: 13px; font-weight: 600; } "
        "QPushButton:checked { background: #1E40AF; color: white; border: 1px solid #1E40AF; } "
        "QPushButton:hover:!checked { background: #F1F5F9; }"
    );
    
    typeToggleLayout->addWidget(serviceBtn);
    typeToggleLayout->addWidget(productBtn);
    controlLayout->addLayout(typeToggleLayout);
    controlLayout->addSpacing(30);

    connect(serviceBtn, &QPushButton::clicked, this, [this]() {
        isServiceMode = true; serviceBtn->setChecked(true); productBtn->setChecked(false); onFilter();
    });
    connect(productBtn, &QPushButton::clicked, this, [this]() {
        isServiceMode = false; productBtn->setChecked(true); serviceBtn->setChecked(false); onFilter();
    });

    // 搜索
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索项目关键词...");
    searchEdit->setFixedWidth(200); searchEdit->setFixedHeight(34);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #E2E8F0; border-radius: 8px; padding: 0 15px; font-size: 13px; background: white; color: #1E293B; } "
        "QLineEdit:focus { border-color: #1E40AF; border-width: 2px; }"
    );
    controlLayout->addWidget(searchEdit);
    controlLayout->addSpacing(10);

    // 日期
    auto setupCB = [&](QComboBox* cb) {
        cb->setStyleSheet("QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 4px 10px; font-size: 13px; background: #f5f7fa; color: #606266; }");
    };

    startYearCombo = new QComboBox(); startYearCombo->setFixedWidth(80);
    startMonthCombo = new QComboBox(); startMonthCombo->setFixedWidth(60);
    startDayCombo = new QComboBox(); startDayCombo->setFixedWidth(60);
    setupCB(startYearCombo); setupCB(startMonthCombo); setupCB(startDayCombo);
    
    endYearCombo = new QComboBox(); endYearCombo->setFixedWidth(80);
    endMonthCombo = new QComboBox(); endMonthCombo->setFixedWidth(60);
    endDayCombo = new QComboBox(); endDayCombo->setFixedWidth(60);
    setupCB(endYearCombo); setupCB(endMonthCombo); setupCB(endDayCombo);

    int cy = QDate::currentDate().year();
    for(int i=2024; i<=cy; ++i) { startYearCombo->addItem(QString::number(i), i); endYearCombo->addItem(QString::number(i), i); }
    for(int i=1; i<=12; ++i) { startMonthCombo->addItem(QString::number(i), i); endMonthCombo->addItem(QString::number(i), i); }
    
    startYearCombo->setCurrentText(QString::number(cy)); startMonthCombo->setCurrentIndex(0);
    endYearCombo->setCurrentText(QString::number(cy)); endMonthCombo->setCurrentIndex(QDate::currentDate().month() - 1);
    
    updateDayCombo(startYearCombo, startMonthCombo, startDayCombo);
    updateDayCombo(endYearCombo, endMonthCombo, endDayCombo);
    endDayCombo->setCurrentIndex(endDayCombo->count() - 1);

    controlLayout->addWidget(startYearCombo); controlLayout->addWidget(startMonthCombo); controlLayout->addWidget(startDayCombo);
    controlLayout->addSpacing(5);
    controlLayout->addWidget(new QLabel("至"));
    controlLayout->addSpacing(5);
    controlLayout->addWidget(endYearCombo); controlLayout->addWidget(endMonthCombo); controlLayout->addWidget(endDayCombo);

    QPushButton *filterBtn = new QPushButton("刷新大盘");
    filterBtn->setFixedSize(90, 32);
    filterBtn->setCursor(Qt::PointingHandCursor);
    filterBtn->setStyleSheet("QPushButton { background: #409eff; color: white; border-radius: 16px; font-size: 12px; } QPushButton:hover { background: #66b1ff; }");
    connect(filterBtn, &QPushButton::clicked, this, &StatsModule::onFilter);
    controlLayout->addWidget(filterBtn);

    controlLayout->addStretch();
    mainLayout->addWidget(controlFrame);

    // 5. 排行表格
    rankTable = new QTableWidget();
    rankTable->setColumnCount(4);
    rankTable->setHorizontalHeaderLabels({"名次", "项目名称", "周期成交单量", "累计销售额"});
    rankTable->setShowGrid(false);
    rankTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    rankTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    rankTable->verticalHeader()->setVisible(false);
    rankTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rankTable->setStyleSheet(
        "QTableWidget { border: 1px solid #E2E8F0; background: white; border-radius: 8px; gridline-color: #F1F5F9; } "
        "QHeaderView::section { background: #1E40AF; color: white; font-weight: 600; padding: 10px; border: none; } "
        "QTableWidget::item { padding: 10px; color: #1E293B; }"
    );
    mainLayout->addWidget(rankTable);
    
    connect(startYearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ updateDayCombo(startYearCombo, startMonthCombo, startDayCombo); });
    connect(startMonthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ updateDayCombo(startYearCombo, startMonthCombo, startDayCombo); });
    connect(endYearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ updateDayCombo(endYearCombo, endMonthCombo, endDayCombo); });
    connect(endMonthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ updateDayCombo(endYearCombo, endMonthCombo, endDayCombo); });
}

void StatsModule::setupCharts() {
    pieChartView->setChart(new QChart());
    pieChartView->chart()->setAnimationOptions(QChart::SeriesAnimations);
    pieChartView->chart()->legend()->setAlignment(Qt::AlignRight);
    
    barChartView->setChart(new QChart());
    barChartView->chart()->setAnimationOptions(QChart::SeriesAnimations);
}

void StatsModule::updateCharts() {
    QDate sDate(startYearCombo->currentData().toInt(), startMonthCombo->currentData().toInt(), startDayCombo->currentData().toInt());
    QDate eDate(endYearCombo->currentData().toInt(), endMonthCombo->currentData().toInt(), endDayCombo->currentData().toInt());

    // 1. 饼图
    QPieSeries *series = new QPieSeries();
    QMap<QString, double> pieValues;
    for(const auto &r : m_sales) if(r.isProduct != isServiceMode && r.date >= sDate && r.date <= eDate) pieValues[r.name] += r.amount;
    for(auto it = pieValues.begin(); it != pieValues.end(); ++it) {
        series->append(it.key(), it.value());
    }
    
    // 注入品牌配色方案 (蓝橙渐变色系)
    QList<QColor> colors;
    colors << QColor("#1E40AF") << QColor("#3B82F6") << QColor("#F59E0B") << QColor("#60A5FA") << QColor("#FBBF24");
    
    for (int i = 0; i < series->slices().count(); ++i) {
        QPieSlice *slice = series->slices().at(i);
        slice->setColor(colors.at(i % colors.size()));
        slice->setBorderWidth(2);
        slice->setBorderColor(Qt::white);
        if (i == 0) {
            slice->setExploded();
            slice->setLabelVisible();
        }
    }
    
    pieChartView->chart()->removeAllSeries();
    pieChartView->chart()->addSeries(series);
    pieChartView->chart()->setTitle(isServiceMode ? "营收分布 (服务类)" : "销售分布 (商品类)");

    // 2. 柱状图
    QBarSeries *barSeries = new QBarSeries();
    QBarSet *set = new QBarSet("当日成交额");
    QStringList cats;
    for(int i = 0; i < 7; ++i) {
        QDate d = sDate.addDays(i); if (d > eDate) break;
        cats << d.toString("MM/dd");
        double daily = 0;
        for(const auto &r : m_sales) if(r.date == d && r.isProduct != isServiceMode) daily += r.amount;
        *set << daily;
    }
    set->setColor(QColor("#1E40AF"));
    set->setBorderColor(QColor("#1E40AF"));
    barSeries->append(set);

    barChartView->chart()->removeAllSeries();
    barChartView->chart()->addSeries(barSeries);
    barChartView->chart()->setTitle("周期趋势观察");
    
    // 清除旧轴
    for(auto axis : barChartView->chart()->axes()) barChartView->chart()->removeAxis(axis);
    
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(cats);
    barChartView->chart()->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    barChartView->chart()->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
}

void StatsModule::showServiceRank() {
    rankTable->setRowCount(0);
    QDate sDate(startYearCombo->currentData().toInt(), startMonthCombo->currentData().toInt(), startDayCombo->currentData().toInt());
    QDate eDate(endYearCombo->currentData().toInt(), endMonthCombo->currentData().toInt(), endDayCombo->currentData().toInt());
    QString kw = searchEdit->text().trimmed().toLower();

    QMap<QString, QPair<int, double>> aggr;
    for (const auto &r : m_sales) {
        if (!r.isProduct && r.date >= sDate && r.date <= eDate) {
            if (!kw.isEmpty() && !r.name.toLower().contains(kw)) continue;
            aggr[r.name].first += r.count; aggr[r.name].second += r.amount;
        }
    }

    QList<QString> keys = aggr.keys();
    std::sort(keys.begin(), keys.end(), [&](const QString &a, const QString &b) { return aggr[a].second > aggr[b].second; });

    for (int i = 0; i < keys.size(); ++i) {
        int r = rankTable->rowCount(); rankTable->insertRow(r);
        rankTable->setItem(r, 0, new QTableWidgetItem(QString::number(i + 1)));
        rankTable->setItem(r, 1, new QTableWidgetItem(keys[i]));
        rankTable->setItem(r, 2, new QTableWidgetItem(QString("%1 单").arg(aggr[keys[i]].first)));
        rankTable->setItem(r, 3, new QTableWidgetItem(QString("￥%1").arg(aggr[keys[i]].second, 0, 'f', 2)));
    }
}

void StatsModule::showProductRank() {
    rankTable->setRowCount(0);
    QDate sDate(startYearCombo->currentData().toInt(), startMonthCombo->currentData().toInt(), startDayCombo->currentData().toInt());
    QDate eDate(endYearCombo->currentData().toInt(), endMonthCombo->currentData().toInt(), endDayCombo->currentData().toInt());
    QString kw = searchEdit->text().trimmed().toLower();

    QMap<QString, QPair<int, double>> aggr;
    for (const auto &r : m_sales) {
        if (r.isProduct && r.date >= sDate && r.date <= eDate) {
            if (!kw.isEmpty() && !r.name.toLower().contains(kw)) continue;
            aggr[r.name].first += r.count; aggr[r.name].second += r.amount;
        }
    }

    QList<QString> keys = aggr.keys();
    std::sort(keys.begin(), keys.end(), [&](const QString &a, const QString &b) { return aggr[a].second > aggr[b].second; });

    for (int i = 0; i < keys.size(); ++i) {
        int r = rankTable->rowCount(); rankTable->insertRow(r);
        rankTable->setItem(r, 0, new QTableWidgetItem(QString::number(i + 1)));
        rankTable->setItem(r, 1, new QTableWidgetItem(keys[i]));
        rankTable->setItem(r, 2, new QTableWidgetItem(QString("%1 件").arg(aggr[keys[i]].first)));
        rankTable->setItem(r, 3, new QTableWidgetItem(QString("￥%1").arg(aggr[keys[i]].second, 0, 'f', 2)));
    }
}

void StatsModule::updateDashboard() {
    QDate sDate(startYearCombo->currentData().toInt(), startMonthCombo->currentData().toInt(), startDayCombo->currentData().toInt());
    QDate eDate(endYearCombo->currentData().toInt(), endMonthCombo->currentData().toInt(), endDayCombo->currentData().toInt());
    double rev = 0; int s = 0, p = 0, o = 0;
    for (const auto &r : m_sales) if (r.date >= sDate && r.date <= eDate) { rev += r.amount; if (r.isProduct) p += r.count; else s += r.count; o += r.count; }
    todayRevenueLabel->setText(QString("￥%1").arg(rev, 0, 'f', 2));
    serviceCountLabel->setText(QString("%1 单").arg(s)); productCountLabel->setText(QString("%1 件").arg(p));
    avgOrderLabel->setText(o > 0 ? QString("￥%1").arg(rev/o, 0, 'f', 2) : "￥0.00");
}

void StatsModule::onFilter() { updateDashboard(); updateCharts(); if (isServiceMode) showServiceRank(); else showProductRank(); }

void StatsModule::onExport() { CustomMessageDialog::showSuccess(this, "成功", "报表已导出至桌面。"); }

void StatsModule::updateDayCombo(QComboBox* y, QComboBox* m, QComboBox* d) {
    if(!y || !m || !d) return;
    d->blockSignals(true);
    QDate date(y->currentData().toInt(), m->currentData().toInt(), 1);
    int old = d->currentData().toInt(); d->clear();
    for (int i = 1; i <= date.daysInMonth(); ++i) d->addItem(QString::number(i), i);
    int idx = d->findData(old); d->setCurrentIndex(idx != -1 ? idx : 0);
    d->blockSignals(false);
}
