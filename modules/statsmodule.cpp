#include "statsmodule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QDate>
#include <QGraphicsDropShadowEffect>
#include <QCalendarWidget>

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
}

void StatsModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 1. 标题
    QLabel *title = new QLabel("数据报表统计分析中心");
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #303133;");
    mainLayout->addWidget(title);

    // 2. 仪表盘卡片
    QHBoxLayout *dashLayout = new QHBoxLayout();
    dashLayout->setSpacing(15);

    auto createDash = [&](const QString &icon, const QString &label, QLabel* &valLabel, const QString &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; } ");
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 20));
        shadow->setOffset(0, 4);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *cl = new QHBoxLayout(card);
        cl->setContentsMargins(20, 15, 20, 15);

        QLabel *ic = new QLabel(icon);
        ic->setFixedSize(50, 50);
        ic->setAlignment(Qt::AlignCenter);
        ic->setStyleSheet(QString("font-size: 24px; color: %1; background: #f5f7fa; border-radius: 10px; border: none;").arg(color));

        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(2);
        QLabel *tl = new QLabel(label); tl->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        valLabel = new QLabel("0"); valLabel->setStyleSheet("color: #303133; font-size: 22px; font-weight: bold; border: none; background: transparent;");
        vl->addWidget(tl); vl->addWidget(valLabel);
        vl->addStretch();
        cl->addWidget(ic); cl->addSpacing(15); cl->addLayout(vl); cl->addStretch();
        return card;
    };

    dashLayout->addWidget(createDash("💰", "区间总营收", todayRevenueLabel, "#67c23a"));
    dashLayout->addWidget(createDash("🧾", "区间客单价", avgOrderLabel, "#409eff"));
    dashLayout->addWidget(createDash("💇", "服务单量", serviceCountLabel, "#e6a23c"));
    dashLayout->addWidget(createDash("📦", "商品单量", productCountLabel, "#f56c6c"));
    mainLayout->addLayout(dashLayout);

    // 3. 控制栏：分类切换 + 日期范围
    QHBoxLayout *controlLayout = new QHBoxLayout();

    // 分类按钮组
    serviceBtn = new QPushButton("🔥 服务类热销");
    serviceBtn->setFixedHeight(36);
    serviceBtn->setCursor(Qt::PointingHandCursor);
    serviceBtn->setCheckable(true);
    serviceBtn->setChecked(true);
    serviceBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 6px; border: none; font-weight: bold; padding: 0 18px; font-size: 13px; } "
        "QPushButton:checked { background: #409eff; } "
        "QPushButton:!checked { background: #f4f4f5; color: #606266; } "
        "QPushButton:hover:!checked { background: #e9e9eb; }"
    );
    connect(serviceBtn, &QPushButton::clicked, this, [this]() {
        isServiceMode = true;
        serviceBtn->setChecked(true); productBtn->setChecked(false);
        showServiceRank();
    });

    productBtn = new QPushButton("📦 实物类热销");
    productBtn->setFixedHeight(36);
    productBtn->setCursor(Qt::PointingHandCursor);
    productBtn->setCheckable(true);
    productBtn->setStyleSheet(serviceBtn->styleSheet());
    connect(productBtn, &QPushButton::clicked, this, [this]() {
        isServiceMode = false;
        productBtn->setChecked(true); serviceBtn->setChecked(false);
        showProductRank();
    });

    controlLayout->addWidget(serviceBtn);
    controlLayout->addWidget(productBtn);
    controlLayout->addSpacing(30);

    // 日期范围
    QLabel *dateLabel = new QLabel("统计区间:");
    dateLabel->setStyleSheet("color: #606266; font-size: 13px; border: none; background: transparent;");
    controlLayout->addWidget(dateLabel);

    startYearCombo = new QComboBox(); startYearCombo->setFixedWidth(115);
    startMonthCombo = new QComboBox(); startMonthCombo->setFixedWidth(100);
    startDayCombo = new QComboBox(); startDayCombo->setFixedWidth(100);
    
    // 统一美化样式
    auto setupStatsCombo = [&](QComboBox* cb) {
        cb->setStyleSheet(
            "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 4px 20px 4px 10px; font-size: 13px; background: #f5f7fa; color: #606266; } "
            "QComboBox:focus { border-color: #409eff; background: white; } "
            "QComboBox::drop-down { border: none; width: 24px; } "
            "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; }"
            "QComboBox QAbstractItemView {"
            "   border: 1px solid #e4e7ed;"
            "   background-color: #ffffff;"
            "   border-radius: 4px;"
            "   selection-background-color: #f5f7fa;"
            "   selection-color: #409eff;"
            "   outline: none;"
            "}"
            "QComboBox QAbstractItemView::item {"
            "   height: 35px;"
            "   padding-left: 10px;"
            "   color: #606266;"
            "}"
            "QComboBox QAbstractItemView::item:selected {"
            "   background-color: #f5f7fa;"
            "   color: #409eff;"
            "   border-left: 3px solid #409eff;"
            "}"
        );
        cb->view()->verticalScrollBar()->setStyleSheet(
            "QScrollBar:vertical { width: 0px; background: transparent; margin: 0px; } "
            "QScrollBar::handle:vertical { background: #dcdfe6; border-radius: 4px; min-height: 20px; } "
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        );
    };
    setupStatsCombo(startYearCombo); setupStatsCombo(startMonthCombo); setupStatsCombo(startDayCombo);
    
    int currentYear = QDate::currentDate().year();
    for(int i=2024; i<=currentYear; ++i) startYearCombo->addItem(QString::number(i) + "年", i);
    for(int i=1; i<=12; ++i) startMonthCombo->addItem(QString("%1月").arg(i), i);
    startYearCombo->setCurrentText(QString::number(currentYear) + "年");
    startMonthCombo->setCurrentIndex(0); // 1月
    updateDayCombo(startYearCombo, startMonthCombo, startDayCombo);

    controlLayout->addWidget(startYearCombo);
    controlLayout->addWidget(startMonthCombo);
    controlLayout->addWidget(startDayCombo);

    QLabel *toLabel = new QLabel("至"); toLabel->setStyleSheet("color: #909399; border: none; background: transparent;");
    controlLayout->addWidget(toLabel);

    // 结束日期
    endYearCombo = new QComboBox(); endYearCombo->setFixedWidth(115);
    endMonthCombo = new QComboBox(); endMonthCombo->setFixedWidth(100);
    endDayCombo = new QComboBox(); endDayCombo->setFixedWidth(100);
    setupStatsCombo(endYearCombo); setupStatsCombo(endMonthCombo); setupStatsCombo(endDayCombo);
    
    for(int i=2024; i<=currentYear; ++i) endYearCombo->addItem(QString::number(i) + "年", i);
    for(int i=1; i<=12; ++i) endMonthCombo->addItem(QString("%1月").arg(i), i);
    endYearCombo->setCurrentText(QString::number(currentYear) + "年");
    updateDayCombo(endYearCombo, endMonthCombo, endDayCombo);
    endDayCombo->setCurrentText(QString::number(QDate::currentDate().day()) + "日");

    controlLayout->addWidget(endYearCombo);
    controlLayout->addWidget(endMonthCombo);
    controlLayout->addWidget(endDayCombo);

    connect(startYearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ updateDayCombo(startYearCombo, startMonthCombo, startDayCombo); onFilter(); });
    connect(startMonthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ updateDayCombo(startYearCombo, startMonthCombo, startDayCombo); onFilter(); });
    connect(startDayCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ onFilter(); });
    connect(endYearCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ updateDayCombo(endYearCombo, endMonthCombo, endDayCombo); onFilter(); });
    connect(endMonthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ updateDayCombo(endYearCombo, endMonthCombo, endDayCombo); onFilter(); });
    connect(endDayCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ onFilter(); });

    QPushButton *filterBtn = new QPushButton("统计筛选");
    filterBtn->setFixedWidth(120); filterBtn->setFixedHeight(34);
    filterBtn->setCursor(Qt::PointingHandCursor);
    filterBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 17px; border: none; font-weight: bold; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { background: #66b1ff; } "
        "QPushButton:pressed { background: #3a8ee6; }"
    );
    connect(filterBtn, &QPushButton::clicked, this, &StatsModule::onFilter);
    controlLayout->addWidget(filterBtn);

    QPushButton *resetBtn = new QPushButton("重置条件");
    resetBtn->setFixedWidth(120); resetBtn->setFixedHeight(34);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setStyleSheet(
        "QPushButton { background: white; color: #606266; border-radius: 17px; border: 1px solid #dcdfe6; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { border-color: #409eff; color: #409eff; background: #fdfdfd; } "
        "QPushButton:pressed { background: #f5f7fa; }"
    );
    connect(resetBtn, &QPushButton::clicked, this, [this](){ /*searchEdit->clear();*/ onFilter(); }); // searchEdit is not defined, commented out
    controlLayout->addWidget(resetBtn);

    controlLayout->addStretch();
    mainLayout->addLayout(controlLayout);

    // 4. 排行表格
    rankTable = new QTableWidget();
    rankTable->setColumnCount(4);
    rankTable->setHorizontalHeaderLabels({"排名", "项目名称", "成交单量", "销售额"});
    rankTable->setShowGrid(false);
    rankTable->setAlternatingRowColors(true);
    rankTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    rankTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    rankTable->verticalHeader()->setVisible(false);
    rankTable->verticalHeader()->setDefaultSectionSize(48);
    rankTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background: white; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background: #f5f7fa; padding: 10px; border: none; font-weight: bold; } "
    );
    rankTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    rankTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(rankTable);

    // 初始显示
    showServiceRank();
    updateDashboard();
}

void StatsModule::showServiceRank() {
    rankTable->setRowCount(0);
    
    QDate sDate(startYearCombo->currentData().toInt(), startMonthCombo->currentData().toInt(), startDayCombo->currentData().toInt());
    QDate eDate(endYearCombo->currentData().toInt(), endMonthCombo->currentData().toInt(), endDayCombo->currentData().toInt());

    // 聚合数据
    QMap<QString, QPair<int, double>> aggregated;
    for (const auto &record : m_sales) {
        if (!record.isProduct && record.date >= sDate && record.date <= eDate) {
            aggregated[record.name].first += record.count;
            aggregated[record.name].second += record.amount;
        }
    }

    // 排序
    QList<QString> keys = aggregated.keys();
    std::sort(keys.begin(), keys.end(), [&](const QString &a, const QString &b) {
        return aggregated[a].second > aggregated[b].second; // 按金额降序
    });

    for (int i = 0; i < keys.size(); ++i) {
        int row = rankTable->rowCount();
        rankTable->insertRow(row);
        QString name = keys[i];

        QTableWidgetItem *rankItem = new QTableWidgetItem(QString::number(i + 1));
        rankItem->setTextAlignment(Qt::AlignCenter);
        if (i < 3) rankItem->setForeground(QColor("#e6a23c"));
        rankTable->setItem(row, 0, rankItem);

        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        nameItem->setTextAlignment(Qt::AlignCenter);
        nameItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        rankTable->setItem(row, 1, nameItem);

        QTableWidgetItem *countItem = new QTableWidgetItem(QString("%1 单").arg(aggregated[name].first));
        countItem->setTextAlignment(Qt::AlignCenter);
        rankTable->setItem(row, 2, countItem);

        QTableWidgetItem *amtItem = new QTableWidgetItem(QString("￥%1").arg(aggregated[name].second, 0, 'f', 0));
        amtItem->setTextAlignment(Qt::AlignCenter);
        amtItem->setForeground(QColor("#67c23a"));
        rankTable->setItem(row, 3, amtItem);
    }
}

void StatsModule::showProductRank() {
    rankTable->setRowCount(0);
    
    QDate sDate(startYearCombo->currentData().toInt(), startMonthCombo->currentData().toInt(), startDayCombo->currentData().toInt());
    QDate eDate(endYearCombo->currentData().toInt(), endMonthCombo->currentData().toInt(), endDayCombo->currentData().toInt());

    QMap<QString, QPair<int, double>> aggregated;
    for (const auto &record : m_sales) {
        if (record.isProduct && record.date >= sDate && record.date <= eDate) {
            aggregated[record.name].first += record.count;
            aggregated[record.name].second += record.amount;
        }
    }

    QList<QString> keys = aggregated.keys();
    std::sort(keys.begin(), keys.end(), [&](const QString &a, const QString &b) {
        return aggregated[a].second > aggregated[b].second;
    });

    for (int i = 0; i < keys.size(); ++i) {
        int row = rankTable->rowCount();
        rankTable->insertRow(row);
        QString name = keys[i];

        QTableWidgetItem *rankItem = new QTableWidgetItem(QString::number(i + 1));
        rankItem->setTextAlignment(Qt::AlignCenter);
        if (i < 3) rankItem->setForeground(QColor("#e6a23c"));
        rankTable->setItem(row, 0, rankItem);

        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        nameItem->setTextAlignment(Qt::AlignCenter);
        nameItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        rankTable->setItem(row, 1, nameItem);

        QTableWidgetItem *countItem = new QTableWidgetItem(QString("%1 件").arg(aggregated[name].first));
        countItem->setTextAlignment(Qt::AlignCenter);
        rankTable->setItem(row, 2, countItem);

        QTableWidgetItem *amtItem = new QTableWidgetItem(QString("￥%1").arg(aggregated[name].second, 0, 'f', 0));
        amtItem->setTextAlignment(Qt::AlignCenter);
        amtItem->setForeground(QColor("#67c23a"));
        rankTable->setItem(row, 3, amtItem);
    }
}

void StatsModule::updateDashboard() {
    QDate sDate(startYearCombo->currentData().toInt(), startMonthCombo->currentData().toInt(), startDayCombo->currentData().toInt());
    QDate eDate(endYearCombo->currentData().toInt(), endMonthCombo->currentData().toInt(), endDayCombo->currentData().toInt());

    double totalRevenue = 0;
    int serviceOrders = 0;
    int productOrders = 0;
    int totalCount = 0;

    for (const auto &record : m_sales) {
        if (record.date >= sDate && record.date <= eDate) {
            // 只统计当前模式对应的数据
            if (isServiceMode && !record.isProduct) {
                totalRevenue += record.amount;
                totalCount += record.count;
            } else if (!isServiceMode && record.isProduct) {
                totalRevenue += record.amount;
                totalCount += record.count;
            }
            
            // 模式无关的计数（用于底部或隐藏参考）
            if (record.isProduct) productOrders += record.count;
            else serviceOrders += record.count;
        }
    }

    double avgOrder = (totalCount > 0) ? totalRevenue / totalCount : 0;

    todayRevenueLabel->setText(QString("￥%1").arg(totalRevenue, 0, 'f', 0));
    avgOrderLabel->setText(QString("￥%1").arg(avgOrder, 0, 'f', 0));
    serviceCountLabel->setText(QString("%1").arg(serviceOrders));
    productCountLabel->setText(QString("%1").arg(productOrders));
}

void StatsModule::updateDayCombo(QComboBox* y, QComboBox* m, QComboBox* d) {
    int year = y->currentData().toInt();
    int month = m->currentData().toInt();
    int oldDay = d->currentData().toInt();
    if (oldDay <= 0) oldDay = d->currentText().remove("日").toInt();

    QDate date(year, month, 1);
    int daysInMonth = date.daysInMonth();

    d->clear();
    for (int i = 1; i <= daysInMonth; ++i) d->addItem(QString("%1日").arg(i), i);

    int index = d->findData(oldDay);
    if (index != -1) d->setCurrentIndex(index);
    else d->setCurrentIndex(d->count() - 1);
}

void StatsModule::onFilter() {
    updateDashboard();
    if (isServiceMode) showServiceRank();
    else showProductRank();
}
