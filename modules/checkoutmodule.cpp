#include "checkoutmodule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>
#include <QCalendarWidget>
#include <QScrollBar>

CheckoutModule::CheckoutModule(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void CheckoutModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 1. 标题
    QLabel *title = new QLabel("订单管理中心");
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #303133;");
    mainLayout->addWidget(title);

    // 2. 统计看板
    QHBoxLayout *dashLayout = new QHBoxLayout();
    dashLayout->setSpacing(20);

    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &valLabel) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; }");
        
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
        valLabel = new QLabel("0");
        valLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #303133; border: none; background: transparent;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(valLabel);
        textLayout->addStretch();
        l->addLayout(textLayout);
        l->addStretch();
        return card;
    };

    dashLayout->addWidget(createStatCard("📋", "订单总数", orderCountLabel));
    dashLayout->addWidget(createStatCard("💰", "总金额", totalAmtLabel));
    dashLayout->addWidget(createStatCard("📊", "客单价", avgAmtLabel));
    mainLayout->addLayout(dashLayout);

    // 3. 搜索/筛选栏
    QFrame *filterFrame = new QFrame();
    filterFrame->setStyleSheet("QFrame { background: white; border-radius: 8px; border: 1px solid #ebeef5; }");
    filterFrame->setFixedHeight(55);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterFrame);
    filterLayout->setContentsMargins(15, 0, 15, 0);
    filterLayout->setSpacing(10);

    QString inputStyle = "QLineEdit { border: 1px solid #dcdfe6; border-radius: 18px; padding: 6px 15px; font-size: 13px; background: white; } QLineEdit:focus { border-color: #409eff; }";

    memberSearchEdit = new QLineEdit();
    memberSearchEdit->setPlaceholderText(" 搜索订单号、会员手机号...");
    memberSearchEdit->setFixedWidth(220);
    memberSearchEdit->setFixedHeight(32);
    memberSearchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    filterLayout->addWidget(memberSearchEdit);

    QLabel *dateSep = new QLabel("日期:");
    dateSep->setStyleSheet("color: #606266; font-size: 13px; border: none; background: transparent;");
    filterLayout->addWidget(dateSep);

    // 统一美化样式
    auto setupCheckoutCombo = [&](QComboBox* cb, int w) {
        cb->setFixedWidth(w);
        cb->setStyleSheet(
            "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 4px 20px 4px 10px; font-size: 13px; background: #f5f7fa; color: #606266; } "
            "QComboBox:focus { border-color: #409eff; background: white; } "
            "QComboBox::drop-down { border: none; width: 24px; } "
            "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; }"
            "QComboBox QAbstractItemView {"
            "   border: 1px solid #e4e7ed; background-color: #ffffff; border-radius: 4px;"
            "   selection-background-color: #f5f7fa; selection-color: #409eff; outline: none;"
            "}"
            "QComboBox QAbstractItemView::item { height: 35px; padding-left: 10px; color: #606266; }"
            "QComboBox QAbstractItemView::item:selected { background-color: #f5f7fa; color: #409eff; border-left: 3px solid #409eff; }"
        );
        cb->view()->verticalScrollBar()->setStyleSheet(
            "QScrollBar:vertical { width: 0px; background: transparent; margin: 0px; } "
            "QScrollBar::handle:vertical { background: #dcdfe6; border-radius: 4px; min-height: 20px; } "
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        );
    };

    // 开始日期
    startYearCombo = new QComboBox(); setupCheckoutCombo(startYearCombo, 115);
    startMonthCombo = new QComboBox(); setupCheckoutCombo(startMonthCombo, 100);
    startDayCombo = new QComboBox(); setupCheckoutCombo(startDayCombo, 100);
    
    int currentYear = QDate::currentDate().year();
    for(int i=2024; i<=currentYear; ++i) startYearCombo->addItem(QString::number(i) + "年", i);
    for(int i=1; i<=12; ++i) startMonthCombo->addItem(QString("%1月").arg(i), i);
    startYearCombo->setCurrentText(QString::number(currentYear) + "年");
    startMonthCombo->setCurrentIndex(0);
    updateDayCombo(startYearCombo, startMonthCombo, startDayCombo);

    // 结束日期
    endYearCombo = new QComboBox(); setupCheckoutCombo(endYearCombo, 115);
    endMonthCombo = new QComboBox(); setupCheckoutCombo(endMonthCombo, 100);
    endDayCombo = new QComboBox(); setupCheckoutCombo(endDayCombo, 100);

    for(int i=2024; i<=currentYear; ++i) endYearCombo->addItem(QString::number(i) + "年", i);
    for(int i=1; i<=12; ++i) endMonthCombo->addItem(QString("%1月").arg(i), i);
    endYearCombo->setCurrentText(QString::number(currentYear) + "年");
    updateDayCombo(endYearCombo, endMonthCombo, endDayCombo);
    endDayCombo->setCurrentIndex(endDayCombo->findData(QDate::currentDate().day()));

    // 绑定联动事件
    auto bindCombo = [&](QComboBox* y, QComboBox* m, QComboBox* d) {
        connect(y, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](){ updateDayCombo(y, m, d); onFilter(); });
        connect(m, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](){ updateDayCombo(y, m, d); onFilter(); });
        connect(d, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](){ onFilter(); });
    };
    bindCombo(startYearCombo, startMonthCombo, startDayCombo);
    bindCombo(endYearCombo, endMonthCombo, endDayCombo);

    filterLayout->addWidget(startYearCombo);
    filterLayout->addWidget(startMonthCombo);
    filterLayout->addWidget(startDayCombo);

    QLabel *to = new QLabel("至"); to->setStyleSheet("color: #909399; border: none; background: transparent;");
    filterLayout->addWidget(to);

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
    connect(filterBtn, &QPushButton::clicked, this, &CheckoutModule::onFilter);
    filterLayout->addWidget(filterBtn);

    QPushButton *resetBtn = new QPushButton("重置条件");
    resetBtn->setFixedWidth(120); resetBtn->setFixedHeight(34);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setStyleSheet(
        "QPushButton { background: white; color: #606266; border-radius: 17px; border: 1px solid #dcdfe6; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { border-color: #409eff; color: #409eff; background: #fdfdfd; } "
        "QPushButton:pressed { background: #f5f7fa; }"
    );
    connect(resetBtn, &QPushButton::clicked, this, [this](){ 
        memberSearchEdit->clear(); 
        initDateGroup(startYearCombo, startMonthCombo, startDayCombo, QDate(2024, 1, 1));
        initDateGroup(endYearCombo, endMonthCombo, endDayCombo, QDate::currentDate());
        onFilter(); 
    });
    connect(memberSearchEdit, &QLineEdit::returnPressed, this, &CheckoutModule::onFilter);
    filterLayout->addWidget(resetBtn);
    filterLayout->addStretch();
    mainLayout->addWidget(filterFrame);

    // 4. 订单表格
    orderTable = new QTableWidget();
    orderTable->setColumnCount(8);
    orderTable->setHorizontalHeaderLabels({"成交日期", "订单号", "会员ID", "会员姓名", "消费项目", "单价", "数量", "折后金额"});
    orderTable->setShowGrid(false);
    orderTable->setAlternatingRowColors(true);
    orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    orderTable->verticalHeader()->setVisible(false);
    orderTable->verticalHeader()->setDefaultSectionSize(50);
    orderTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background: white; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background: #f5f7fa; padding: 10px; border: none; font-weight: bold; } "
    );
    orderTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    orderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 注入演示数据
    addOrderRow(QDate(2026, 3, 16), "ORD20260316001", "M1001", "张三", "猫咪全身洗护", 80, 1, 0.9);
    addOrderRow(QDate(2026, 3, 16), "ORD20260316002", "M1002", "李四", "皇家猫粮 2kg", 199, 2, 1.0);
    addOrderRow(QDate(2026, 3, 16), "ORD20260316003", "M1001", "张三", "宠物 SPA 精油护理", 200, 1, 0.85);
    addOrderRow(QDate(2026, 3, 15), "ORD20260315001", "M1003", "王五", "疫苗套餐", 200, 1, 1.0);
    addOrderRow(QDate(2026, 3, 15), "ORD20260315002", "M1004", "赵六", "精修造型", 200, 1, 0.9);
    addOrderRow(QDate(2026, 3, 14), "ORD20260314001", "M1002", "李四", "驱虫套餐", 150, 1, 1.0);

    mainLayout->addWidget(orderTable);

    // 5. 底部汇总
    totalLabel = new QLabel();
    totalLabel->setStyleSheet(
        "background: #f0f9eb; color: #67c23a; padding: 12px 20px; border-radius: 8px; "
        "font-size: 16px; font-weight: bold; border: 1px solid #e1f3d8;"
    );
    mainLayout->addWidget(totalLabel);

    updateTotal();
}

void CheckoutModule::initDateGroup(QComboBox* y, QComboBox* m, QComboBox* d, const QDate &initDate) {
    y->blockSignals(true);
    m->blockSignals(true);
    d->blockSignals(true);

    QString comboStyle = "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 5px 10px; font-size: 13px; background: #f5f7fa; color: #606266; } "
                         "QComboBox:focus { border-color: #409eff; background: white; } "
                         "QComboBox::drop-down { border: none; width: 24px; } "
                         "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; }";
    
    y->setStyleSheet(comboStyle); 
    m->setStyleSheet(comboStyle); 
    d->setStyleSheet(comboStyle);
    
    if (y->count() == 0) {
        for(int i=2024; i<=2030; ++i) y->addItem(QString::number(i), i);
    }
    if (m->count() == 0) {
        for(int i=1; i<=12; ++i) m->addItem(QString("%1月").arg(i), i);
    }
    
    y->setCurrentText(QString::number(initDate.year()));
    m->setCurrentIndex(initDate.month()-1);
    
    updateDayCombo(y, m, d);
    d->setCurrentText(QString::number(initDate.day()));

    y->blockSignals(false);
    m->blockSignals(false);
    d->blockSignals(false);
}

void CheckoutModule::addOrderRow(const QDate &date, const QString &orderId, const QString &memberId, const QString &memberName,
                                 const QString &item, double unitPrice, int qty, double discount)
{
    int row = orderTable->rowCount();
    orderTable->insertRow(row);

    auto setItem = [&](int col, const QString &text) {
        QTableWidgetItem *it = new QTableWidgetItem(text);
        it->setTextAlignment(Qt::AlignCenter);
        orderTable->setItem(row, col, it);
    };

    double finalAmt = unitPrice * qty * discount;

    setItem(0, date.toString("yyyy-MM-dd"));
    QTableWidgetItem *dateItem = orderTable->item(row, 0);
    dateItem->setData(Qt::UserRole, date); // 存储原始日期便于过滤

    setItem(1, orderId);
    setItem(2, memberId);
    setItem(3, memberName);
    QTableWidgetItem *itemName = new QTableWidgetItem(item);
    itemName->setTextAlignment(Qt::AlignCenter);
    itemName->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    orderTable->setItem(row, 4, itemName);
    setItem(5, QString("￥%1").arg(unitPrice, 0, 'f', 2));
    setItem(6, QString::number(qty));

    QTableWidgetItem *amtItem = new QTableWidgetItem(QString("￥%1").arg(finalAmt, 0, 'f', 2));
    amtItem->setTextAlignment(Qt::AlignCenter);
    amtItem->setForeground(QColor("#67c23a"));
    amtItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    orderTable->setItem(row, 7, amtItem);
}

void CheckoutModule::updateDayCombo(QComboBox* y, QComboBox* m, QComboBox* d) {
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

void CheckoutModule::onFilter() {
    QString kw = memberSearchEdit->text().trimmed().toLower();
    QDate sDate(startYearCombo->currentData().toInt(), startMonthCombo->currentData().toInt(), startDayCombo->currentData().toInt());
    QDate eDate(endYearCombo->currentData().toInt(), endMonthCombo->currentData().toInt(), endDayCombo->currentData().toInt());

    int visibleCount = 0;
    double totalAmt = 0;

    for(int i=0; i<orderTable->rowCount(); ++i) {
        QDate oDate = orderTable->item(i, 0)->data(Qt::UserRole).toDate();
        QString orderId = orderTable->item(i, 1)->text().toLower();
        QString memberId = orderTable->item(i, 2)->text().toLower();
        QString memberName = orderTable->item(i, 3)->text().toLower();

        bool dateMatch = (oDate >= sDate && oDate <= eDate);
        bool kwMatch = kw.isEmpty() || orderId.contains(kw) || memberId.contains(kw) || memberName.contains(kw);

        bool visible = dateMatch && kwMatch;
        orderTable->setRowHidden(i, !visible);

        if (visible) {
            visibleCount++;
            QString amtStr = orderTable->item(i, 7)->text().remove("￥");
            totalAmt += amtStr.toDouble();
        }
    }

    // 更新统计卡片
    orderCountLabel->setText(QString("%1 笔").arg(visibleCount));
    totalAmtLabel->setText(QString("￥%1").arg(totalAmt, 0, 'f', 0));
    double avg = (visibleCount > 0) ? totalAmt / visibleCount : 0;
    avgAmtLabel->setText(QString("￥%1").arg(avg, 0, 'f', 0));

    // 更新底部汇总
    totalLabel->setText(QString(" 📊 合计应收：￥%1 | 共 %2 笔订单")
                           .arg(totalAmt, 0, 'f', 2)
                           .arg(visibleCount));
}

void CheckoutModule::updateTotal()
{
    onFilter(); 
}
