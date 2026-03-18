#include "performancemodule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QDate>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QFrame>
#include <QCalendarWidget>

PerformanceModule::PerformanceModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void PerformanceModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    // 1. 标题
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *title = new QLabel("业绩核算与提成明细", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #303133;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // 2. 统计卡片
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(20);

    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &valLabel) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; } ");
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 25));
        shadow->setOffset(0, 4);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *l = new QHBoxLayout(card);
        l->setContentsMargins(20, 15, 20, 15);

        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(50, 50);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("font-size: 24px; background: #f5f7fa; border-radius: 10px; border: none;"));
        l->addWidget(iconLabel);
        l->addSpacing(15);

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);
        QLabel *labelTitle = new QLabel(label);
        labelTitle->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        valLabel = new QLabel("--");
        valLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #303133; border: none; background: transparent;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(valLabel);
        textLayout->addStretch();
        l->addLayout(textLayout);
        l->addStretch();
        return card;
    };

    statLayout->addWidget(createStatCard("📈", "选定区间总业绩", totalRevenueLabel));
    statLayout->addWidget(createStatCard("💇", "洗护/造型选定业绩", serviceRevenueLabel));
    statLayout->addWidget(createStatCard("📦", "商品销售选定业绩", productRevenueLabel));
    mainLayout->addLayout(statLayout);

    // 3. 筛选面板
    QFrame *filterFrame = new QFrame();
    filterFrame->setStyleSheet("QFrame { background: white; border-radius: 8px; border: 1px solid #ebeef5; }");
    filterFrame->setFixedHeight(55);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterFrame);
    filterLayout->setContentsMargins(15, 0, 15, 0);
    filterLayout->setSpacing(10);

    QString inputStyle = "QLineEdit { border: 1px solid #dcdfe6; border-radius: 18px; padding: 6px 15px; font-size: 13px; background: white; } QLineEdit:focus { border-color: #409eff; }";
    QString comboStyle = "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 10px; font-size: 13px; background: #f5f7fa; color: #606266; } "
                         "QComboBox:focus { border-color: #409eff; background: white; } "
                         "QComboBox::drop-down { border: none; width: 24px; } "
                         "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; }";

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索员工ID或姓名...");
    searchEdit->setFixedWidth(200);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    filterLayout->addWidget(searchEdit);

    QLabel *dateSep = new QLabel("日期:");
    dateSep->setStyleSheet("color: #606266; font-size: 13px; border: none; background: transparent;");
    filterLayout->addWidget(dateSep);

    // 开始日期
    startYearCombo = new QComboBox(); startYearCombo->setFixedWidth(115);
    startMonthCombo = new QComboBox(); startMonthCombo->setFixedWidth(100);
    startDayCombo = new QComboBox(); startDayCombo->setFixedWidth(100);
    
    // 结束日期
    endYearCombo = new QComboBox(); endYearCombo->setFixedWidth(115);
    endMonthCombo = new QComboBox(); endMonthCombo->setFixedWidth(100);
    endDayCombo = new QComboBox(); endDayCombo->setFixedWidth(100);

    auto initDateGroup = [&](QComboBox* y, QComboBox* m, QComboBox* d, const QDate &initDate) {
        QString style = "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 4px 20px 4px 10px; font-size: 13px; background: #f5f7fa; color: #606266; } "
                        "QComboBox:focus { border-color: #409eff; background: white; } "
                        "QComboBox::drop-down { border: none; width: 24px; } "
                        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; }"
                        "QComboBox QAbstractItemView {"
                        "   border: 1px solid #e4e7ed; background-color: #ffffff; border-radius: 4px;"
                        "   selection-background-color: #f5f7fa; selection-color: #409eff; outline: none;"
                        "}"
                        "QComboBox QAbstractItemView::item { height: 35px; padding-left: 10px; color: #606266; }"
                        "QComboBox QAbstractItemView::item:selected { background-color: #f5f7fa; color: #409eff; border-left: 3px solid #409eff; }";
        
        QString scrollStyle = "QScrollBar:vertical { width: 0px; background: transparent; margin: 0px; } "
                             "QScrollBar::handle:vertical { background: #dcdfe6; border-radius: 4px; min-height: 20px; } "
                             "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }";

        y->setStyleSheet(style);
        m->setStyleSheet(style);
        d->setStyleSheet(style);
        
        y->view()->verticalScrollBar()->setStyleSheet(scrollStyle);
        m->view()->verticalScrollBar()->setStyleSheet(scrollStyle);
        d->view()->verticalScrollBar()->setStyleSheet(scrollStyle);
        m->setStyleSheet(y->styleSheet()); 
        d->setStyleSheet(y->styleSheet());
        
        int currentYear = QDate::currentDate().year();
        for(int i=2024; i<=currentYear; ++i) y->addItem(QString::number(i) + "年", i);
        for(int i=1; i<=12; ++i) m->addItem(QString("%1月").arg(i), i);
        
        y->setCurrentText(QString::number(initDate.year()) + "年");
        m->setCurrentIndex(initDate.month()-1);
        
        updateDayCombo(y, m, d);
        d->setCurrentIndex(d->findData(initDate.day()));

        connect(y, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ updateDayCombo(y, m, d); });
        connect(m, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ updateDayCombo(y, m, d); });
    };

    initDateGroup(startYearCombo, startMonthCombo, startDayCombo, QDate(2024, 1, 1));
    initDateGroup(endYearCombo, endMonthCombo, endDayCombo, QDate::currentDate());

    filterLayout->addWidget(startYearCombo);
    filterLayout->addWidget(startMonthCombo);
    filterLayout->addWidget(startDayCombo);

    QLabel *toLabel = new QLabel("至");
    toLabel->setStyleSheet("color: #909399; border: none; background: transparent;");
    filterLayout->addWidget(toLabel);

    filterLayout->addWidget(endYearCombo);
    filterLayout->addWidget(endMonthCombo);
    filterLayout->addWidget(endDayCombo);

    QPushButton *filterBtn = new QPushButton("统计筛选");
    filterBtn->setFixedWidth(120); filterBtn->setFixedHeight(34);
    filterBtn->setCursor(Qt::PointingHandCursor);
    filterBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 17px; border: none; font-weight: bold; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { background: #66b1ff; } "
        "QPushButton:pressed { background: #3a8ee6; }"
    );
    connect(filterBtn, &QPushButton::clicked, this, &PerformanceModule::onFilter);
    filterLayout->addWidget(filterBtn);

    QPushButton *resetBtn = new QPushButton("重置条件");
    resetBtn->setFixedWidth(120); resetBtn->setFixedHeight(34);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setStyleSheet(
        "QPushButton { background: white; color: #606266; border-radius: 17px; border: 1px solid #dcdfe6; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { border-color: #409eff; color: #409eff; background: #fdfdfd; } "
        "QPushButton:pressed { background: #f5f7fa; }"
    );
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        searchEdit->clear();
        startYearCombo->setCurrentText("2024"); startMonthCombo->setCurrentIndex(0);
        endYearCombo->setCurrentText(QString::number(QDate::currentDate().year()));
        endMonthCombo->setCurrentIndex(QDate::currentDate().month()-1);
        onFilter();
    });
    filterLayout->addWidget(resetBtn);
    filterLayout->addStretch();

    mainLayout->addWidget(filterFrame);

    // 4. 表格
    perfTable = new QTableWidget();
    perfTable->setColumnCount(6);
    perfTable->setHorizontalHeaderLabels({"成交日期", "员工ID", "员工姓名", "业绩类型", "成交金额", "服务提成"});
    perfTable->setShowGrid(false);
    perfTable->setAlternatingRowColors(true);
    perfTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    perfTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    perfTable->verticalHeader()->setVisible(false);
    perfTable->verticalHeader()->setDefaultSectionSize(50);

    perfTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background-color: white; color: black; outline: none; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background-color: #f5f7fa; padding: 12px; border: none; color: #606266; font-weight: bold; font-size: 13px; } "
    );
    perfTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 注入演示数据
    addPerformanceRow("2026-03-16", "E001", "李四", "宠物洗护", 300, 45);
    addPerformanceRow("2026-03-16", "E004", "赵六", "疫苗注射", 500, 100);
    addPerformanceRow("2026-03-15", "E001", "李四", "全身剪毛", 450, 67.5);
    addPerformanceRow("2026-03-15", "E002", "王五", "商品销售", 1200, 120);
    addPerformanceRow("2026-03-14", "E004", "赵六", "体检套餐", 380, 57);
    addPerformanceRow("2026-03-14", "E001", "李四", "商品销售", 860, 86);

    mainLayout->addWidget(perfTable);

    // 5. 底部提成汇总
    totalCommLabel = new QLabel();
    totalCommLabel->setStyleSheet(
        "background: #ecf5ff; color: #409eff; padding: 12px 20px; border-radius: 8px; "
        "font-size: 15px; font-weight: bold; border: 1px solid #d9ecff;"
    );
    mainLayout->addWidget(totalCommLabel);

    updateSummary();
}

void PerformanceModule::addPerformanceRow(const QString &date, const QString &empId, const QString &empName, 
                                          const QString &type, double amount, double commission)
{
    int row = perfTable->rowCount();
    perfTable->insertRow(row);

    auto setItem = [&](int col, const QString &text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        perfTable->setItem(row, col, item);
    };

    setItem(0, date);
    setItem(1, empId);
    setItem(2, empName);
    setItem(3, type);
    setItem(4, QString("￥%1").arg(amount, 0, 'f', 0));
    
    QTableWidgetItem *commItem = new QTableWidgetItem(QString("￥%1").arg(commission, 0, 'f', 1));
    commItem->setTextAlignment(Qt::AlignCenter);
    commItem->setForeground(QBrush(QColor("#409eff")));
    perfTable->setItem(row, 5, commItem);
}

void PerformanceModule::updateSummary()
{
    double totalRevenue = 0, serviceRevenue = 0, productRevenue = 0, totalComm = 0;

    for (int i = 0; i < perfTable->rowCount(); ++i) {
        if (perfTable->isRowHidden(i)) continue;

        QString amtStr = perfTable->item(i, 4)->text().remove("￥");
        QString commStr = perfTable->item(i, 5)->text().remove("￥");
        double amt = amtStr.toDouble();
        double comm = commStr.toDouble();

        totalRevenue += amt;
        totalComm += comm;

        QString type = perfTable->item(i, 3)->text();
        if (type.contains("商品")) productRevenue += amt;
        else serviceRevenue += amt;
    }

    totalRevenueLabel->setText(QString("￥%1").arg(totalRevenue, 0, 'f', 0));
    serviceRevenueLabel->setText(QString("￥%1").arg(serviceRevenue, 0, 'f', 0));
    productRevenueLabel->setText(QString("￥%1").arg(productRevenue, 0, 'f', 0));
    totalCommLabel->setText(QString("📊 当前统计周期内总提成：￥%1").arg(totalComm, 0, 'f', 1));
}

void PerformanceModule::updateDayCombo(QComboBox* y, QComboBox* m, QComboBox* d)
{
    int year = y->currentData().toInt();
    int month = m->currentIndex() + 1;
    int oldDay = d->currentData().toInt();
    if (oldDay <= 0) oldDay = d->currentText().remove("日").toInt();
    
    QDate date(year, month, 1);
    int daysInMonth = date.daysInMonth();
    
    d->clear();
    for(int i=1; i<=daysInMonth; ++i) d->addItem(QString("%1日").arg(i), i);
    
    int index = d->findData(oldDay);
    if(index != -1) d->setCurrentIndex(index);
    else d->setCurrentIndex(d->count()-1);
}


void PerformanceModule::onFilter()
{
    QString kw = searchEdit->text().trimmed().toLower();
    
    QDate startDate(startYearCombo->currentText().toInt(), startMonthCombo->currentIndex()+1, startDayCombo->currentText().toInt());
    QDate endDate(endYearCombo->currentText().toInt(), endMonthCombo->currentIndex()+1, endDayCombo->currentText().toInt());

    for (int i = 0; i < perfTable->rowCount(); ++i) {
        bool visible = true;

        // 搜索过滤 (ID或姓名)
        if (!kw.isEmpty()) {
            QString empId = perfTable->item(i, 1)->text().toLower();
            QString empName = perfTable->item(i, 2)->text().toLower();
            if (!empId.contains(kw) && !empName.contains(kw)) visible = false;
        }

        // 日期过滤
        if (visible) {
            QDate rowDate = QDate::fromString(perfTable->item(i, 0)->text(), "yyyy-MM-dd");
            if (rowDate.isValid() && (rowDate < startDate || rowDate > endDate)) {
                visible = false;
            }
        }

        perfTable->setRowHidden(i, !visible);
    }

    updateSummary();
}
