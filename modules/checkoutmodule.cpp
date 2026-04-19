#include "checkoutmodule.h"
#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QCheckBox>
#include <QIntValidator>

CheckoutModule::CheckoutModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
    
    // 初始化模拟真实订单流数据
    m_orderData.append({"2026-03-16", "ORD1001", "M001", "张三", "单次小型犬洗护", 80.0, 1, 1.0, 80.0});
    m_orderData.append({"2026-03-16", "ORD1002", "M004", "赵六", "猫咪疫苗注射", 150.0, 1, 0.9, 135.0});
    m_orderData.append({"2026-03-15", "ORD1003", "M001", "张三", "商品: 猫砂", 35.0, 2, 1.0, 70.0});
    m_orderData.append({"2026-03-15", "ORD1004", "普通散客", "王女士", "商品: 狗粮", 220.0, 1, 1.0, 220.0});
    m_orderData.append({"2026-03-14", "ORD1005", "M004", "赵六", "寄养服务(1天)", 60.0, 1, 0.8, 48.0});
    m_orderData.append({"2026-03-14", "ORD1006", "M023", "李先生", "体验卡购买", 500.0, 1, 1.0, 500.0});

    updatePagination();
    updateTotal();
}

void CheckoutModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    // 1. 标题和上方筛选
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("订单管理与清结算");
    titleLabel->setStyleSheet("font-size: 24px; color: #303133;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    QPushButton *batchDeleteBtn = new QPushButton("批量清理作废");
    batchDeleteBtn->setFixedWidth(130);
    batchDeleteBtn->setFixedHeight(34);
    batchDeleteBtn->setCursor(Qt::PointingHandCursor);
    batchDeleteBtn->setStyleSheet(
        "QPushButton { background: #fef0f0; color: #f56c6c; border-radius: 17px; border: 1px solid #fbc4c4; font-size: 13px; text-align: center; } "
        "QPushButton:hover { background: #f56c6c; color: white; }"
    );
    connect(batchDeleteBtn, &QPushButton::clicked, this, &CheckoutModule::onBatchDelete);
    
    headerLayout->addWidget(batchDeleteBtn);
    mainLayout->addLayout(headerLayout);

    QFrame *filterFrame = new QFrame();
    filterFrame->setStyleSheet("QFrame { background: white; border-radius: 8px; border: 1px solid #ebeef5; }");
    filterFrame->setFixedHeight(60);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterFrame);
    filterLayout->setContentsMargins(15, 0, 15, 0);
    filterLayout->setSpacing(10);

    memberSearchEdit = new QLineEdit();
    memberSearchEdit->setPlaceholderText(" 输入会员卡号/姓名查询...");
    memberSearchEdit->setFixedWidth(200);
    memberSearchEdit->setFixedHeight(32);
    memberSearchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    filterLayout->addWidget(memberSearchEdit);

    QLabel *dateLabel = new QLabel("日期:");
    dateLabel->setStyleSheet("color: #606266; font-size: 13px; border: none; background: transparent;");
    filterLayout->addWidget(dateLabel);

    startYearCombo = new QComboBox(); startYearCombo->setFixedWidth(100);
    startMonthCombo = new QComboBox(); startMonthCombo->setFixedWidth(80);
    startDayCombo = new QComboBox(); startDayCombo->setFixedWidth(80);
    
    endYearCombo = new QComboBox(); endYearCombo->setFixedWidth(100);
    endMonthCombo = new QComboBox(); endMonthCombo->setFixedWidth(80);
    endDayCombo = new QComboBox(); endDayCombo->setFixedWidth(80);

    initDateGroup(startYearCombo, startMonthCombo, startDayCombo, QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1));
    initDateGroup(endYearCombo, endMonthCombo, endDayCombo, QDate::currentDate());

    filterLayout->addWidget(startYearCombo); filterLayout->addWidget(startMonthCombo); filterLayout->addWidget(startDayCombo);
    QLabel *toLabel = new QLabel("至");
    toLabel->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
    filterLayout->addWidget(toLabel);
    filterLayout->addWidget(endYearCombo); filterLayout->addWidget(endMonthCombo); filterLayout->addWidget(endDayCombo);

    QPushButton *filterBtn = new QPushButton("统计筛选");
    filterBtn->setFixedWidth(100);
    filterBtn->setFixedHeight(32);
    filterBtn->setCursor(Qt::PointingHandCursor);
    filterBtn->setStyleSheet("QPushButton { background: #409eff; color: white; border-radius: 16px; border: none; font-size: 13px; } QPushButton:hover { background: #66b1ff; }");
    connect(filterBtn, &QPushButton::clicked, this, &CheckoutModule::onFilter);
    filterLayout->addWidget(filterBtn);

    QPushButton *resetBtn = new QPushButton("重置");
    resetBtn->setFixedWidth(80);
    resetBtn->setFixedHeight(32);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setStyleSheet("QPushButton { background: white; color: #606266; border-radius: 16px; border: 1px solid #dcdfe6; font-size: 13px; } QPushButton:hover { border-color: #409eff; color: #409eff; }");
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        memberSearchEdit->clear();
        m_currentPage = 1;
        updatePagination();
        updateTotal();
    });
    filterLayout->addWidget(resetBtn);
    filterLayout->addStretch();
    mainLayout->addWidget(filterFrame);

    // 2. 表格展示
    orderTable = new QTableWidget();
    orderTable->setColumnCount(11); // 选择 + 9列原始数据 + 操作列
    orderTable->setHorizontalHeaderLabels({"选择", "记录日期", "流水号", "身份ID", "客户/会员姓名", "消费项目名细", "核算单价", "数量", "折扣率", "实收合计", "操作"});
    
    orderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    orderTable->setColumnWidth(0, 48); // 选择框
    orderTable->setColumnWidth(1, 100); // 日期
    orderTable->setColumnWidth(2, 100); // 订单号
    orderTable->setColumnWidth(3, 100); // ID
    orderTable->setColumnWidth(4, 100); // 姓名
    orderTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch); // 消费项目
    orderTable->setColumnWidth(6, 80); // 单价
    orderTable->setColumnWidth(7, 60); // 数量
    orderTable->setColumnWidth(8, 80); // 折扣率
    orderTable->setColumnWidth(9, 100); // 合计
    orderTable->setColumnWidth(10, 80); // 操作

    orderTable->setShowGrid(false);
    orderTable->setAlternatingRowColors(false);
    orderTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    orderTable->setSelectionMode(QAbstractItemView::SingleSelection);
    orderTable->setFocusPolicy(Qt::NoFocus);
    orderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    orderTable->verticalHeader()->setVisible(false);
    orderTable->verticalHeader()->setDefaultSectionSize(45);

    orderTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background-color: white; color: black; outline: none; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QTableWidget::item:selected { background-color: #b3d8ff; } " 
        "QHeaderView::section { background-color: #f5f7fa; padding: 12px; border: none; font-weight: bold; color: #606266; } "
        "QCheckBox::indicator { width: 16px; height: 16px; }"
        "QCheckBox::indicator:unchecked { border: 1px solid #dcdfe6; background: white; border-radius: 2px; }"
        "QCheckBox::indicator:checked { image: url(:/images/check.svg); background: #409eff; border: 1px solid #409eff; border-radius: 2px; }"
    );
    mainLayout->addWidget(orderTable);

    // 3. 底部概览与真分页栏
    QFrame *bottomBar = new QFrame();
    bottomBar->setFixedHeight(70);
    bottomBar->setStyleSheet("background: #f8f9fb; border-top: 1px solid #ebeef5; border-radius: 8px;");
    
    QGraphicsDropShadowEffect *bottomShadow = new QGraphicsDropShadowEffect();
    bottomShadow->setBlurRadius(10); bottomShadow->setColor(QColor(0,0,0,10)); bottomShadow->setOffset(0, -2);
    bottomBar->setGraphicsEffect(bottomShadow);

    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(20, 0, 20, 0);

    QVBoxLayout *statBox = new QVBoxLayout();
    statBox->setSpacing(5);
    statBox->setAlignment(Qt::AlignVCenter);
    
    orderCountLabel = new QLabel("期间总订单: 0 笔");
    orderCountLabel->setStyleSheet("color: #606266; font-size: 14px;");
    
    QHBoxLayout *amtHBox = new QHBoxLayout();
    totalAmtLabel = new QLabel("清算总金额: ￥0.00");
    totalAmtLabel->setStyleSheet("color: #f56c6c; font-size: 16px; font-weight: bold;");
    avgAmtLabel = new QLabel(" | 单笔均价: ￥0.00");
    avgAmtLabel->setStyleSheet("color: #909399; font-size: 14px;");
    amtHBox->addWidget(totalAmtLabel);
    amtHBox->addWidget(avgAmtLabel);
    amtHBox->addStretch();
    
    statBox->addWidget(orderCountLabel);
    statBox->addLayout(amtHBox);
    bottomLayout->addLayout(statBox);
    
    bottomLayout->addStretch();

    // 分页组件
    QHBoxLayout *pageLayout = new QHBoxLayout();
    pageLabel = new QLabel("第 1 页 / 共 1 页");
    pageLabel->setStyleSheet("color: #606266; font-size: 13px;");
    
    QLabel *jumpLbl1 = new QLabel("跳转到"); jumpLbl1->setStyleSheet("color: #606266; font-size: 13px;");
    jumpEdit = new QLineEdit();
    jumpEdit->setFixedSize(40, 26);
    jumpEdit->setAlignment(Qt::AlignCenter);
    jumpValidator = new QIntValidator(1, 1, this);
    jumpEdit->setValidator(jumpValidator);
    jumpEdit->setStyleSheet("QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; background: white; } QLineEdit:focus { border-color: #409eff; }");
    QLabel *jumpLbl2 = new QLabel("页"); jumpLbl2->setStyleSheet("color: #606266; font-size: 13px;");
    
    QPushButton *goBtn = new QPushButton("确认");
    goBtn->setFixedSize(40, 26);
    goBtn->setCursor(Qt::PointingHandCursor);
    goBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 4px; color: #606266; font-size: 12px; } QPushButton:hover { color: #409eff; border-color: #c6e2ff; background: #ecf5ff; }");
    
    prevBtn = new QPushButton("上一页");
    prevBtn->setFixedSize(60, 26);
    prevBtn->setCursor(Qt::PointingHandCursor);
    prevBtn->setStyleSheet(goBtn->styleSheet());
    
    nextBtn = new QPushButton("下一页");
    nextBtn->setFixedSize(60, 26);
    nextBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setStyleSheet(goBtn->styleSheet());

    pageLayout->addWidget(jumpLbl1);
    pageLayout->addWidget(jumpEdit);
    pageLayout->addWidget(jumpLbl2);
    pageLayout->addWidget(goBtn);
    pageLayout->addSpacing(15);
    pageLayout->addWidget(prevBtn);
    pageLayout->addWidget(pageLabel);
    pageLayout->addWidget(nextBtn);

    bottomLayout->addLayout(pageLayout);
    mainLayout->addWidget(bottomBar);

    connect(prevBtn, &QPushButton::clicked, this, &CheckoutModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &CheckoutModule::onNextPage);
    connect(goBtn, &QPushButton::clicked, this, &CheckoutModule::onJumpPage);
    connect(jumpEdit, &QLineEdit::returnPressed, this, &CheckoutModule::onJumpPage);
}

void CheckoutModule::initDateGroup(QComboBox* y, QComboBox* m, QComboBox* d, const QDate &initDate)
{
    QString style = "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 4px 10px; font-size: 13px; background: white; color: #606266; } "
                    "QComboBox:focus { border-color: #409eff; } "
                    "QComboBox::drop-down { border: none; width: 24px; } "
                    "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; }";
    
    y->setStyleSheet(style); m->setStyleSheet(style); d->setStyleSheet(style);
    
    int currentYear = QDate::currentDate().year();
    for(int i=2024; i<=currentYear; ++i) y->addItem(QString::number(i) + "年", i);
    for(int i=1; i<=12; ++i) m->addItem(QString("%1月").arg(i, 2, 10, QChar('0')), i);
    
    y->setCurrentText(QString::number(initDate.year()) + "年");
    m->setCurrentIndex(initDate.month()-1);
    
    updateDayCombo(y, m, d);
    d->setCurrentIndex(d->findData(initDate.day()));

    connect(y, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ updateDayCombo(y, m, d); });
    connect(m, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){ updateDayCombo(y, m, d); });
}

void CheckoutModule::updateDayCombo(QComboBox* y, QComboBox* m, QComboBox* d) {
    if(!d) return;
    d->blockSignals(true);
    int year = y->currentText().remove("年").toInt();
    int month = m->currentIndex() + 1;
    int oldDay = d->currentData().toInt();
    if(oldDay <= 0) oldDay = d->currentText().remove("日").toInt();
    
    QDate date(year, month, 1);
    int days = date.daysInMonth();
    d->clear();
    for(int i=1; i<=days; ++i) d->addItem(QString("%1日").arg(i, 2, 10, QChar('0')), i);
    
    if(d->findData(oldDay) != -1) d->setCurrentIndex(d->findData(oldDay));
    else d->setCurrentIndex(0);
    d->blockSignals(false);
}

void CheckoutModule::updatePagination()
{
    orderTable->setRowCount(0);
    
    QString kw = memberSearchEdit->text().trimmed().toLower();
    QDate sDate(startYearCombo->currentText().toInt(), startMonthCombo->currentIndex()+1, startDayCombo->currentText().toInt());
    QDate eDate(endYearCombo->currentText().toInt(), endMonthCombo->currentIndex()+1, endDayCombo->currentText().toInt());
    
    QList<OrderRecord> filteredData;
    for (const auto &r : m_orderData) {
        bool match = true;
        if (!kw.isEmpty() && !r.memberId.toLower().contains(kw) && !r.memberName.toLower().contains(kw) && !r.orderId.toLower().contains(kw)) match = false;
        QDate rowDate = QDate::fromString(r.date, "yyyy-MM-dd");
        if (rowDate.isValid() && (rowDate < sDate || rowDate > eDate)) match = false;
        
        if (match) filteredData.append(r);
    }
    
    int total = filteredData.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    
    if (jumpValidator) jumpValidator->setTop(totalPages);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, total);

    for (int i = start; i < end; ++i) {
        const auto &r = filteredData[i];
        int row = orderTable->rowCount();
        orderTable->insertRow(row);
        
        // 0: CheckBox
        QWidget *cbWidget = new QWidget();
        QHBoxLayout *cbLayout = new QHBoxLayout(cbWidget);
        cbLayout->setContentsMargins(0, 0, 0, 0); cbLayout->setAlignment(Qt::AlignCenter);
        QCheckBox *cb = new QCheckBox();
        cbLayout->addWidget(cb);
        orderTable->setCellWidget(row, 0, cbWidget);
        
        auto setItem = [&](int col, QString text) {
            QTableWidgetItem *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            item->setFont(QFont("Microsoft YaHei", 9));
            orderTable->setItem(row, col, item);
            return item;
        };
        
        setItem(1, r.date);
        setItem(2, r.orderId);
        setItem(3, r.memberId);
        setItem(4, r.memberName);
        setItem(5, r.item);
        setItem(6, QString("¥%1").arg(r.unitPrice, 0, 'f', 2));
        setItem(7, QString::number(r.qty));
        setItem(8, QString("%1 折").arg(r.discount * 10, 0, 'f', 1));
        
        QTableWidgetItem *tot = setItem(9, QString("¥%1").arg(r.total, 0, 'f', 2));
        tot->setForeground(QColor("#f56c6c")); tot->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        
        // 10: Action
        QWidget *actW = new QWidget();
        QHBoxLayout *actL = new QHBoxLayout(actW);
        actL->setContentsMargins(0,0,0,0); actL->setAlignment(Qt::AlignCenter);
        
        QPushButton *delBtn = new QPushButton("删作废");
        delBtn->setFixedSize(50, 24); delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setStyleSheet("QPushButton { background: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 3px; font-size: 11px; padding: 0; } QPushButton:hover { background: #f56c6c; color: white; }");
        delBtn->setProperty("orderId", r.orderId);
        connect(delBtn, &QPushButton::clicked, this, &CheckoutModule::onDeleteSingle);
        actL->addWidget(delBtn);
        orderTable->setCellWidget(row, 10, actW);
    }
    
    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);
}

void CheckoutModule::updateTotal()
{
    QString kw = memberSearchEdit->text().trimmed().toLower();
    QDate sDate(startYearCombo->currentText().toInt(), startMonthCombo->currentIndex()+1, startDayCombo->currentText().toInt());
    QDate eDate(endYearCombo->currentText().toInt(), endMonthCombo->currentIndex()+1, endDayCombo->currentText().toInt());
    
    int count = 0;
    double sum = 0.0;
    
    for (const auto &r : m_orderData) {
        bool match = true;
        if (!kw.isEmpty() && !r.memberId.toLower().contains(kw) && !r.memberName.toLower().contains(kw) && !r.orderId.toLower().contains(kw)) match = false;
        QDate rowDate = QDate::fromString(r.date, "yyyy-MM-dd");
        if (rowDate.isValid() && (rowDate < sDate || rowDate > eDate)) match = false;
        
        if (match) {
            count++;
            sum += r.total;
        }
    }

    orderCountLabel->setText(QString("期间总订单: %1 笔").arg(count));
    totalAmtLabel->setText(QString("清算总金额: ￥%1").arg(sum, 0, 'f', 2));
    if (count > 0) avgAmtLabel->setText(QString(" | 单笔均价: ￥%1").arg(sum / count, 0, 'f', 2));
    else avgAmtLabel->setText(" | 单笔均价: ￥0.00");
}

void CheckoutModule::onFilter()
{
    m_currentPage = 1;
    updatePagination();
    updateTotal();
}

void CheckoutModule::onPrevPage() {
    if (m_currentPage > 1) { m_currentPage--; updatePagination(); }
}

void CheckoutModule::onNextPage() {
    m_currentPage++; updatePagination(); 
}

void CheckoutModule::onJumpPage() {
    int page = jumpEdit->text().toInt();
    if (page >= 1) { m_currentPage = page; updatePagination(); }
    jumpEdit->clear(); jumpEdit->clearFocus();
}

void CheckoutModule::onBatchDelete() {
    QStringList toDeleteIds;
    for (int r = 0; r < orderTable->rowCount(); ++r) {
        QWidget *w = orderTable->cellWidget(r, 0);
        if (w) {
            QCheckBox *cb = w->findChild<QCheckBox*>();
            if (cb && cb->isChecked()) {
                toDeleteIds.append(orderTable->item(r, 2)->text());
            }
        }
    }
    
    if (toDeleteIds.isEmpty()) {
        CustomMessageDialog::showWarning(this, "批量操作", "请先在前方的方框中勾选需要作废的流水记录！");
        return;
    }
    
    if (CustomMessageDialog::confirm(this, "核准清理", QString("是否明确要彻底冲正作废这批 %1 笔订单流水？\n该操作无法撤销。").arg(toDeleteIds.size()))) {
        m_orderData.erase(std::remove_if(m_orderData.begin(), m_orderData.end(), [&](const OrderRecord& rec){
            return toDeleteIds.contains(rec.orderId);
        }), m_orderData.end());
        updatePagination();
        updateTotal();
    }
}

void CheckoutModule::onDeleteSingle() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString oid = btn->property("orderId").toString();
    if (CustomMessageDialog::confirm(this, "严重操作", QString("确认立刻抹除并废弃单号为 [%1] 的流水吗？").arg(oid))) {
        for (int i=0; i<m_orderData.size(); ++i) {
            if (m_orderData[i].orderId == oid) {
                m_orderData.removeAt(i);
                break;
            }
        }
        updatePagination();
        updateTotal();
    }
}
