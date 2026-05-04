#include "performancemodule.h"
#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QDate>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QFrame>
#include <QCheckBox>
#include <QIntValidator>

PerformanceModule::PerformanceModule(QWidget *parent) : QWidget(parent),
    m_currentPage(1), m_pageSize(10), pageLabel(nullptr), jumpEdit(nullptr), prevBtn(nullptr), nextBtn(nullptr), jumpValidator(nullptr)
{
    setupUI();
    
    // 初始化模拟真实录入数据
    m_perfData.append({"2026-03-16", "E001", "李四", "宠物洗护", 300, 45, false});
    m_perfData.append({"2026-03-16", "E004", "赵六", "疫苗注射", 500, 100, false});
    m_perfData.append({"2026-03-15", "E001", "李四", "全身剪毛", 450, 67.5, true});
    m_perfData.append({"2026-03-15", "E002", "王五", "商品销售", 1200, 120, false});
    m_perfData.append({"2026-03-14", "E004", "赵六", "体检套餐", 380, 57, true});
    m_perfData.append({"2026-03-14", "E001", "李四", "商品销售", 860, 86, true});
    
    updatePagination();
    updateSummary();
}

void PerformanceModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 1. 标题
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *title = new QLabel("业绩核算与提成明细", this);
    title->setStyleSheet("font-size: 24px; color: #303133;");
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
        if (icon.isEmpty()) {
            iconLabel->hide();
        } else {
            iconLabel->setFixedSize(50, 50);
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setStyleSheet(QString("font-size: 24px; background: #f5f7fa; border-radius: 10px; border: none;"));
        }
        if (!icon.isEmpty()) {
            l->addWidget(iconLabel);
            l->addSpacing(15);
        }

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);
        QLabel *labelTitle = new QLabel(label);
        labelTitle->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        valLabel = new QLabel("--");
        valLabel->setStyleSheet("font-size: 22px; color: #303133; border: none; background: transparent;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(valLabel);
        textLayout->addStretch();
        l->addLayout(textLayout);
        l->addStretch();
        return card;
    };

    statLayout->addWidget(createStatCard("", "大盘总业绩", totalRevenueLabel));
    statLayout->addWidget(createStatCard("", "洗护/造型选定", serviceRevenueLabel));
    statLayout->addWidget(createStatCard("", "商品销售", productRevenueLabel));
    mainLayout->addLayout(statLayout);

    // 3. 筛选面板
    QFrame *filterFrame = new QFrame();
    filterFrame->setStyleSheet("QFrame { background: white; border-radius: 8px; border: 1px solid #ebeef5; }");
    filterFrame->setFixedHeight(60);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterFrame);
    filterLayout->setContentsMargins(15, 0, 15, 0);
    filterLayout->setSpacing(10);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索员工ID或姓名...");
    searchEdit->setFixedWidth(200);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    filterLayout->addWidget(searchEdit);

    QLabel *dateSep = new QLabel("日期:");
    dateSep->setStyleSheet("color: #606266; font-size: 13px; border: none; background: transparent;");
    filterLayout->addWidget(dateSep);

    startYearCombo = new QComboBox(); startYearCombo->setFixedWidth(115);
    startMonthCombo = new QComboBox(); startMonthCombo->setFixedWidth(100);
    startDayCombo = new QComboBox(); startDayCombo->setFixedWidth(100);
    
    endYearCombo = new QComboBox(); endYearCombo->setFixedWidth(115);
    endMonthCombo = new QComboBox(); endMonthCombo->setFixedWidth(100);
    endDayCombo = new QComboBox(); endDayCombo->setFixedWidth(100);

    auto initDateGroup = [&](QComboBox* y, QComboBox* m, QComboBox* d, const QDate &initDate) {
        QString style = "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; } "
                        "QComboBox:hover { border-color: #409eff; } "
                        "QComboBox::drop-down { border: none; width: 24px; } "
                        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
                        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }";
        
        QString scrollStyle = "QScrollBar:vertical { width: 0px; background: transparent; margin: 0px; } "
                             "QScrollBar::handle:vertical { background: #dcdfe6; border-radius: 6px; min-height: 20px; } "
                             "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }";

        y->setStyleSheet(style); m->setStyleSheet(style); d->setStyleSheet(style);
        y->view()->verticalScrollBar()->setStyleSheet(scrollStyle);
        m->view()->verticalScrollBar()->setStyleSheet(scrollStyle);
        d->view()->verticalScrollBar()->setStyleSheet(scrollStyle);
        
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

    initDateGroup(startYearCombo, startMonthCombo, startDayCombo, QDate(2026, 1, 1)); // 开始时间默认提前一点
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
    filterBtn->setFixedWidth(100); filterBtn->setFixedHeight(34);
    filterBtn->setCursor(Qt::PointingHandCursor);
    filterBtn->setStyleSheet("QPushButton { background: #409eff; color: white; border-radius: 17px; border: none; font-size: 13px; text-align: center; } QPushButton:hover { background: #66b1ff; }");
    connect(filterBtn, &QPushButton::clicked, this, &PerformanceModule::onFilter);
    filterLayout->addWidget(filterBtn);

    QPushButton *resetBtn = new QPushButton("重置");
    resetBtn->setFixedWidth(80); resetBtn->setFixedHeight(34);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setStyleSheet("QPushButton { background: white; color: #606266; border-radius: 17px; border: 1px solid #dcdfe6; font-size: 13px; text-align: center; } QPushButton:hover { border-color: #409eff; color: #409eff; }");
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        searchEdit->clear();
        m_currentPage = 1;
        updatePagination();
    });
    filterLayout->addWidget(resetBtn);
    
    filterLayout->addStretch();
    
    QPushButton *batchVerifyBtn = new QPushButton("批量核销入账");
    batchVerifyBtn->setFixedWidth(120); batchVerifyBtn->setFixedHeight(34);
    batchVerifyBtn->setCursor(Qt::PointingHandCursor);
    batchVerifyBtn->setStyleSheet("QPushButton { background: #67c23a; color: white; border-radius: 17px; border: none; font-size: 13px; } QPushButton:hover { background: #85ce61; }");
    connect(batchVerifyBtn, &QPushButton::clicked, this, &PerformanceModule::onBatchVerify);
    filterLayout->addWidget(batchVerifyBtn);

    mainLayout->addWidget(filterFrame);

    // 4. 表格
    perfTable = new QTableWidget();
    perfTable->setColumnCount(7);
    perfTable->setHorizontalHeaderLabels({"成交日期", "员工ID", "员工姓名", "业绩类型", "成交金额", "提成核算", "状态"});
    
     // 列宽分配
    perfTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    perfTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    perfTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    perfTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    perfTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    perfTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    perfTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);

    perfTable->setShowGrid(false);
    perfTable->setAlternatingRowColors(false);
    perfTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    perfTable->setSelectionMode(QAbstractItemView::SingleSelection);
    perfTable->setFocusPolicy(Qt::NoFocus);
    perfTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    perfTable->verticalHeader()->setVisible(false);
    perfTable->verticalHeader()->setDefaultSectionSize(45);

    perfTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background-color: white; color: black; outline: none; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QTableWidget::item:selected { background-color: #b3d8ff; } "
    );

    mainLayout->addWidget(perfTable);

    // 5. 底部概览与分页
    QFrame *bottomBar = new QFrame();
    bottomBar->setFixedHeight(60);
    bottomBar->setStyleSheet("background: #f8f9fb; border-top: 1px solid #ebeef5;");
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    
    totalCommLabel = new QLabel();
    totalCommLabel->setStyleSheet("color: #409eff; font-size: 15px; font-weight:bold;");
    bottomLayout->addWidget(totalCommLabel);
    bottomLayout->addStretch();
    
    // 分页组件
    pageLabel = new QLabel("第 1 页 / 共 1 页");
    pageLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold; margin: 0 10px;");
    
    prevBtn = new QPushButton("上一页");
    nextBtn = new QPushButton("下一页");
    QString pageStyle = "QPushButton { height: 28px; border: 1px solid #e2e8f0; border-radius: 6px; background: white; color: #64748b; font-size: 12px; padding: 0 12px; text-align: center; font-weight: bold; } "
                        "QPushButton:hover { border-color: #3b82f6; color: #3b82f6; background: #eff6ff; } "
                        "QPushButton:disabled { background: #f8fafc; color: #cbd5e1; border-color: #f1f5f9; }";
    prevBtn->setStyleSheet(pageStyle);
    nextBtn->setStyleSheet(pageStyle);
    prevBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setCursor(Qt::PointingHandCursor);

    QWidget *pageGroup = new QWidget();
    QHBoxLayout *pageLayout = new QHBoxLayout(pageGroup);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(5);
    pageLayout->addWidget(prevBtn);
    pageLayout->addWidget(pageLabel);
    pageLayout->addWidget(nextBtn);

    bottomLayout->addWidget(pageGroup);
    mainLayout->addWidget(bottomBar);

    connect(prevBtn, &QPushButton::clicked, this, &PerformanceModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &PerformanceModule::onNextPage);
}

void PerformanceModule::addPerformanceRow(const QString &date, const QString &empId, const QString &empName, 
                                          const QString &type, double amount, double commission)
{
    // 这个方法在此次重构留作向下兼容或动态插入单条使用
    m_perfData.append({date, empId, empName, type, amount, commission, false});
    updatePagination();
    updateSummary();
}

void PerformanceModule::updatePagination()
{
    perfTable->setRowCount(0);
    
    // 过滤数据 (实现真分页过滤)
    QString kw = searchEdit->text().trimmed().toLower();
    QDate sDate(startYearCombo->currentText().toInt(), startMonthCombo->currentIndex()+1, startDayCombo->currentText().toInt());
    QDate eDate(endYearCombo->currentText().toInt(), endMonthCombo->currentIndex()+1, endDayCombo->currentText().toInt());
    
    QList<PerfRecord> filteredData;
    for (const auto &record : m_perfData) {
        bool match = true;
        if (!kw.isEmpty() && !record.empId.toLower().contains(kw) && !record.empName.toLower().contains(kw)) {
             match = false;
        }
        QDate rowDate = QDate::fromString(record.date, "yyyy-MM-dd");
        if (rowDate.isValid() && (rowDate < sDate || rowDate > eDate)) {
             match = false;
        }
        if (match) filteredData.append(record);
    }
    
    int total = filteredData.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    
    // if (jumpValidator) jumpValidator->setTop(totalPages);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, total);

    for (int i = start; i < end; ++i) {
        const auto &record = filteredData[i];
        int row = perfTable->rowCount();
        perfTable->insertRow(row);
        
        auto setItem = [&](int col, QString text) {
            QTableWidgetItem *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            item->setFont(QFont("Microsoft YaHei", 9));
            perfTable->setItem(row, col, item);
        };
        
        setItem(0, record.date);
        setItem(1, record.empId);
        setItem(2, record.empName);
        setItem(3, record.type);
        setItem(4, QString("￥%1").arg(record.amount, 0, 'f', 0));
        
        QTableWidgetItem *commItem = new QTableWidgetItem(QString("￥%1").arg(record.commission, 0, 'f', 1));
        commItem->setTextAlignment(Qt::AlignCenter);
        commItem->setForeground(QColor("#e6a23c"));
        commItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        perfTable->setItem(row, 5, commItem);
        
        // Col 7: Action / Status
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setAlignment(Qt::AlignCenter);
        
        if (!record.isVerified) {
            QPushButton *okBtn = new QPushButton("核销");
            okBtn->setFixedSize(50, 26);
            okBtn->setCursor(Qt::PointingHandCursor);
            okBtn->setStyleSheet("QPushButton { background: #f0f9eb; color: #67c23a; border: 1px solid #c2e7b0; border-radius: 3px; font-size: 11px; padding: 0; }"
                                 "QPushButton:hover { background: #67c23a; color: white; }");
            connect(okBtn, &QPushButton::clicked, this, &PerformanceModule::onVerifySingle);
            // 绑定数据到按钮
            okBtn->setProperty("date", record.date);
            okBtn->setProperty("empId", record.empId);
            actionLayout->addWidget(okBtn);
        } else {
            QLabel *lbl = new QLabel("✔ 已入账");
            lbl->setStyleSheet("color: #909399; font-size: 12px;");
            actionLayout->addWidget(lbl);
        }
        perfTable->setCellWidget(row, 6, actionWidget);
    }
    
    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);
}

void PerformanceModule::updateSummary()
{
    // 全库基于真过滤结果计算，不仅仅是当前页
    QString kw = searchEdit->text().trimmed().toLower();
    QDate sDate(startYearCombo->currentText().toInt(), startMonthCombo->currentIndex()+1, startDayCombo->currentText().toInt());
    QDate eDate(endYearCombo->currentText().toInt(), endMonthCombo->currentIndex()+1, endDayCombo->currentText().toInt());
    
    double totalRevenue = 0, serviceRevenue = 0, productRevenue = 0, totalComm = 0;

    for (const auto &record : m_perfData) {
        bool match = true;
        if (!kw.isEmpty() && !record.empId.toLower().contains(kw) && !record.empName.toLower().contains(kw)) match = false;
        QDate rowDate = QDate::fromString(record.date, "yyyy-MM-dd");
        if (rowDate.isValid() && (rowDate < sDate || rowDate > eDate)) match = false;
        
        if (match) {
            totalRevenue += record.amount;
            if (record.isVerified) {
                totalComm += record.commission; // 仅统计已核销入账的提成
            }
            if (record.type.contains("商品")) productRevenue += record.amount;
            else serviceRevenue += record.amount;
        }
    }

    totalRevenueLabel->setText(QString("￥%1").arg(totalRevenue, 0, 'f', 0));
    serviceRevenueLabel->setText(QString("￥%1").arg(serviceRevenue, 0, 'f', 0));
    productRevenueLabel->setText(QString("￥%1").arg(productRevenue, 0, 'f', 0));
    // 大屏文字告知
    totalCommLabel->setText(QString("当前统计期内，已安全核销的有效提成总额：￥%1").arg(totalComm, 0, 'f', 1));
}

void PerformanceModule::updateDayCombo(QComboBox* y, QComboBox* m, QComboBox* d)
{
    if(!y || !m || !d) return;
    d->blockSignals(true);
    
    int year = y->currentText().remove("年").toInt();
    int month = m->currentIndex() + 1;
    int oldDay = d->currentData().toInt();
    if (oldDay <= 0) oldDay = d->currentText().remove("日").toInt();
    
    QDate date(year, month, 1);
    int daysInMonth = date.daysInMonth();
    
    d->clear();
    for(int i=1; i<=daysInMonth; ++i) d->addItem(QString("%1日").arg(i), i);
    
    int index = d->findData(oldDay);
    if(index != -1) d->setCurrentIndex(index);
    else d->setCurrentIndex(0); // 默认指向 1 日而非最后一天
    
    d->blockSignals(false);
}

void PerformanceModule::onFilter()
{
    m_currentPage = 1;
    updatePagination();
    updateSummary();
}

void PerformanceModule::onPrevPage()
{
    if (m_currentPage > 1) {
        m_currentPage--;
        updatePagination();
    }
}

void PerformanceModule::onNextPage()
{
    // 因不需要在此计算真实总数，直接+1调用updatePagination会有边界拦截
    m_currentPage++;
    updatePagination();
}

void PerformanceModule::onJumpPage()
{
}

void PerformanceModule::onBatchVerify()
{
    int count = 0;
    for (int i = 0; i < perfTable->rowCount(); ++i) {
        QString rowDate = perfTable->item(i, 0)->text();
        QString rowEmpId = perfTable->item(i, 1)->text();
        for (auto &r : m_perfData) {
            if (r.date == rowDate && r.empId == rowEmpId && !r.isVerified) {
                r.isVerified = true;
                count++;
                break;
            }
        }
    }
    
    if (count == 0) {
        CustomMessageDialog::showWarning(this, "批量操作", "当前页没有待核销的记录！");
        return;
    }
    
    CustomMessageDialog::showSuccess(this, "入账成功", QString("共计 %1 条核销记录已被稳妥入账执行！").arg(count));
    updatePagination();
    updateSummary();
}

void PerformanceModule::onVerifySingle()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    QString rowDate = btn->property("date").toString();
    QString rowEmpId = btn->property("empId").toString();
    
    if (CustomMessageDialog::confirm(this, "操作确定", "您确定这笔交易提成已经可以安全核销入账了吗？")) {
        for (auto &r : m_perfData) {
            if (r.date == rowDate && r.empId == rowEmpId && !r.isVerified) {
                r.isVerified = true;
                break;
            }
        }
        updatePagination();
        updateSummary();
    }
}
