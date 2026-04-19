#include "salarymodule.h"
#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFrame>
#include <QTabWidget>
#include <QLineEdit>
#include <QScrollBar>
#include <QDate>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QIntValidator>

SalaryModule::SalaryModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
    initData();
}

void SalaryModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 1. 标题
    QLabel *titleLabel = new QLabel("员工薪资核算与发放记录");
    titleLabel->setStyleSheet("font-size: 20px; color: #303133;");
    mainLayout->addWidget(titleLabel);

    // 2. 筛选工具栏
    QFrame *toolbar = new QFrame();
    toolbar->setStyleSheet("QFrame { background: white; border-radius: 8px; }");
    QHBoxLayout *toolLayout = new QHBoxLayout(toolbar);
    
    auto styleCombo = [&](QComboBox* cb) {
        cb->setFixedHeight(34);
        cb->setStyleSheet(
            "QComboBox {"
            "   border: 1px solid #dcdfe6;"
            "   border-radius: 17px;"
            "   padding: 0px 25px 0px 15px;"
            "   background: white;"
            "   color: #606266;"
            "   font-size: 13px;"
            "}"
            "QComboBox:hover { border-color: #c0c4cc; }"
            "QComboBox:focus { border-color: #409eff; }"
            "QComboBox::drop-down {"
            "   subcontrol-origin: padding;"
            "   subcontrol-position: top right;"
            "   width: 30px;"
            "   border: none;"
            "}"
            "QComboBox::down-arrow {"
            "   image: url(:/images/chevron-down.svg);"
            "   width: 14px;"
            "   height: 14px;"
            "}"
            "QComboBox QAbstractItemView {"
            "   border: 1px solid #e4e7ed;"
            "   background-color: #ffffff;"
            "   border-radius: 4px;"
            "   outline: none;"
            "}"
            "QComboBox QAbstractItemView::item {"
            "   height: 35px;"
            "   padding-left: 10px;"
            "   color: #606266;"
            "}"
            "QComboBox QAbstractItemView::item:hover {"
            "   background-color: #f5f7fa;"
            "   color: #409eff;"
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

    employeeCombo = new QComboBox();
    employeeCombo->addItem("所有员工");
    employeeCombo->addItem("张三 (高级美容师)");
    employeeCombo->addItem("李四 (店员)");
    employeeCombo->setFixedWidth(160);
    styleCombo(employeeCombo);

    yearCombo = new QComboBox();
    yearCombo->setFixedWidth(115);
    int currentYear = QDate::currentDate().year();
    for(int i=2026; i<=currentYear; i++) yearCombo->addItem(QString::number(i) + "年", i);
    yearCombo->setCurrentText(QString::number(currentYear) + "年");
    styleCombo(yearCombo);

    monthCombo = new QComboBox();
    monthCombo->setFixedWidth(100);
    for(int i=1; i<=12; i++) monthCombo->addItem(QString("%1月").arg(i, 2, 10, QChar('0')), i);
    monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);
    styleCombo(monthCombo);

    QPushButton *searchBtn = new QPushButton("统计筛选");
    searchBtn->setFixedWidth(110);
    searchBtn->setFixedHeight(34);
    searchBtn->setCursor(Qt::PointingHandCursor);
    searchBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 17px; border: none; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { background: #66b1ff; } "
        "QPushButton:pressed { background: #3a8ee6; }"
    );

    QPushButton *exportBtn = new QPushButton("导出报表");
    exportBtn->setFixedWidth(110);
    exportBtn->setFixedHeight(34);
    exportBtn->setStyleSheet(
        "QPushButton { background: white; color: #606266; border-radius: 17px; border: 1px solid #dcdfe6; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { border-color: #409eff; color: #409eff; } "
    );

    QPushButton *batchPayBtn = new QPushButton("批量发放薪资");
    batchPayBtn->setFixedWidth(130);
    batchPayBtn->setFixedHeight(34);
    batchPayBtn->setCursor(Qt::PointingHandCursor);
    batchPayBtn->setStyleSheet(
        "QPushButton { background: #67c23a; color: white; border-radius: 17px; border: none; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { background: #85ce61; } "
    );

    toolLayout->addWidget(new QLabel("选择员工:"));
    toolLayout->addWidget(employeeCombo);
    toolLayout->addSpacing(10);
    
    QLineEdit *searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索姓名/工号...");
    searchEdit->setFixedWidth(180);
    searchEdit->setFixedHeight(34);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 17px; padding: 0 15px; font-size: 13px; background: white; text-align: center; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    toolLayout->addWidget(searchEdit);
    toolLayout->addSpacing(10);
    toolLayout->addWidget(new QLabel("查看日期:"));
    toolLayout->addWidget(yearCombo);
    toolLayout->addWidget(monthCombo);
    toolLayout->addSpacing(10);
    toolLayout->addWidget(searchBtn);
    toolLayout->addStretch();
    toolLayout->addWidget(batchPayBtn);
    toolLayout->addSpacing(10);
    toolLayout->addWidget(exportBtn);
    mainLayout->addWidget(toolbar);

    // 3. Tab 展示区
    QTabWidget *tabs = new QTabWidget();
    tabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #ebeef5; background: white; border-radius: 4px; } "
        "QTabBar::tab { background: #f5f7fa; padding: 12px 25px; border: 1px solid #ebeef5; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; min-width: 100px; color: #909399; } "
        "QTabBar::tab:selected { background: white; color: #409eff; border-bottom-color: white; } "
    );

    // 页1: 薪资明细
    salaryTable = new QTableWidget();
    salaryTable->setColumnCount(8);
    salaryTable->setHorizontalHeaderLabels({"选择", "工号", "姓名", "底薪", "提成/绩效", "实发总计", "发放状态", "操作"});
    salaryTable->setColumnWidth(0, 48); // 选择
    salaryTable->setColumnWidth(1, 100); // ID
    salaryTable->setColumnWidth(2, 120); // 姓名
    salaryTable->setColumnWidth(3, 120); // 底薪
    salaryTable->setColumnWidth(4, 120); // 提成
    salaryTable->setColumnWidth(5, 120); // 实发
    salaryTable->setColumnWidth(6, 100); // 状态
    salaryTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch); // 操作分配剩余宽度
    
    salaryTable->setFrameShape(QFrame::NoFrame);
    salaryTable->setAlternatingRowColors(false);
    salaryTable->verticalHeader()->setVisible(false);
    salaryTable->verticalHeader()->setDefaultSectionSize(45);
    salaryTable->setShowGrid(false);
    salaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    salaryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    salaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    salaryTable->setFocusPolicy(Qt::NoFocus);

    salaryTable->setStyleSheet(
        "QTableWidget { gridline-color: #ebeef5; outline: none; border: 1px solid #ebeef5; background-color: white; color: black; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QTableWidget::item:selected { background-color: #b3d8ff; color: black; } " 
        "QHeaderView::section { background-color: #f5f7fa; padding: 10px; border: none; color: #606266; font-size: 13px; font-weight: bold; } "
    );
    
    tabs->addTab(salaryTable, "本月薪资明细");
    tabs->addTab(new QWidget(), "历史发放记录");
    mainLayout->addWidget(tabs);

    // 4. 底部概览与分页
    QFrame *bottomBar = new QFrame();
    bottomBar->setFixedHeight(60);
    bottomBar->setStyleSheet("background: #f8f9fb; border-top: 1px solid #ebeef5;");
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    
    totalPaidLabel = new QLabel("本月已发放金额: ¥0.00");
    totalPaidLabel->setStyleSheet("color: #67c23a; font-size: 14px; font-weight: bold;");
    pendingLabel = new QLabel("待结算金额: ¥0.00");
    pendingLabel->setStyleSheet("color: #f56c6c; font-size: 14px; font-weight: bold;");
    
    bottomLayout->addWidget(totalPaidLabel);
    bottomLayout->addSpacing(30);
    bottomLayout->addWidget(pendingLabel);
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
    jumpEdit->setStyleSheet("QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; } QLineEdit:focus { border-color: #409eff; }");
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

    connect(searchBtn, &QPushButton::clicked, this, &SalaryModule::onFilter);
    connect(exportBtn, &QPushButton::clicked, this, &SalaryModule::onExport);
    connect(batchPayBtn, &QPushButton::clicked, this, &SalaryModule::onBatchPay);
    
    connect(prevBtn, &QPushButton::clicked, this, &SalaryModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &SalaryModule::onNextPage);
    connect(goBtn, &QPushButton::clicked, this, &SalaryModule::onJumpPage);
    connect(jumpEdit, &QLineEdit::returnPressed, this, &SalaryModule::onJumpPage);
}

void SalaryModule::initData()
{
    // 注入初始模拟数据结构
    m_salaryData.append({"E001", "张三", 5000, 1200, "已发放"});
    m_salaryData.append({"E002", "李四", 4500, 800, "待发放"});
    m_salaryData.append({"E003", "王五", 4000, 1500, "待发放"});
    m_salaryData.append({"E004", "赵六", 4200, 900, "已发放"});
    m_salaryData.append({"E005", "孙七", 5000, 2000, "待发放"});
    
    updatePagination();
    updateStats();
}

void SalaryModule::addSalaryRow(const SalaryRecord &record)
{
    int row = salaryTable->rowCount();
    salaryTable->insertRow(row);
    
    // 第 0 列: 勾选框
    QWidget *cbWidget = new QWidget();
    QHBoxLayout *cbLayout = new QHBoxLayout(cbWidget);
    cbLayout->setContentsMargins(0, 0, 0, 0);
    cbLayout->setAlignment(Qt::AlignCenter);
    QCheckBox *cb = new QCheckBox();
    cb->setStyleSheet("QCheckBox::indicator { width: 16px; height: 16px; }"
                      "QCheckBox::indicator:unchecked { border: 1px solid #dcdfe6; background: white; border-radius: 2px; }"
                      "QCheckBox::indicator:checked { image: url(:/images/check.svg); background: #409eff; border: 1px solid #409eff; border-radius: 2px; }");
    // 只有待发放可被勾选
    if (record.status != "待发放") cb->setEnabled(false);
    cbLayout->addWidget(cb);
    salaryTable->setCellWidget(row, 0, cbWidget);

    auto setItem = [&](int col, QString text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        item->setFont(QFont("Microsoft YaHei", 9));
        salaryTable->setItem(row, col, item);
    };

    setItem(1, record.empId);
    setItem(2, record.name);
    setItem(3, QString("¥%1").arg(record.baseSalary, 0, 'f', 2));
    setItem(4, QString("¥%1").arg(record.commission, 0, 'f', 2));
    
    double total = record.baseSalary + record.commission;
    QTableWidgetItem *totalItem = new QTableWidgetItem(QString("¥%1").arg(total, 0, 'f', 2));
    totalItem->setTextAlignment(Qt::AlignCenter);
    totalItem->setForeground(QColor("#409eff"));
    totalItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    salaryTable->setItem(row, 5, totalItem);

    QTableWidgetItem *statusItem = new QTableWidgetItem(record.status);
    statusItem->setTextAlignment(Qt::AlignCenter);
    if (record.status == "待发放") {
        statusItem->setForeground(QColor("#f56c6c"));
    } else {
        statusItem->setForeground(QColor("#67c23a"));
    }
    salaryTable->setItem(row, 6, statusItem);

    // 操作列
    QWidget *actionWidget = new QWidget();
    QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(5);
    actionLayout->setAlignment(Qt::AlignCenter);

    auto createActBtn = [&](const QString &text, const QString &style) {
        QPushButton *b = new QPushButton(text);
        b->setFixedSize(50, 26);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(style);
        return b;
    };

    if (record.status == "待发放") {
        QPushButton *payBtn = createActBtn("发放", "QPushButton { background: #f0f7ff; color: #409eff; border: 1px solid #b3d8ff; border-radius: 3px; font-size: 11px; padding: 0; }"
                                                 "QPushButton:hover { background: #409eff; color: white; }");
        QPushButton *calcBtn = createActBtn("重算", "QPushButton { background: #fdf6ec; color: #e6a23c; border: 1px solid #f5dab1; border-radius: 3px; font-size: 11px; padding: 0; }"
                                                  "QPushButton:hover { background: #e6a23c; color: white; }");
        connect(payBtn, &QPushButton::clicked, this, &SalaryModule::onPaySingle);
        connect(calcBtn, &QPushButton::clicked, this, &SalaryModule::onRecalculateSingle);
        actionLayout->addWidget(payBtn);
        actionLayout->addWidget(calcBtn);
    } else {
        QLabel *doneLbl = new QLabel("已归档");
        doneLbl->setStyleSheet("color: #c0c4cc; font-size: 12px;");
        actionLayout->addWidget(doneLbl);
    }

    salaryTable->setCellWidget(row, 7, actionWidget);
}

void SalaryModule::updatePagination()
{
    salaryTable->setRowCount(0);
    int total = m_salaryData.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    
    if (jumpValidator) jumpValidator->setTop(totalPages);
    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, total);

    for (int i = start; i < end; ++i) {
        addSalaryRow(m_salaryData[i]);
    }

    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);
}

void SalaryModule::updateStats()
{
    double paidAmt = 0;
    double pendingAmt = 0;
    
    for (const auto &record : m_salaryData) {
        double total = record.baseSalary + record.commission;
        if (record.status == "已发放") paidAmt += total;
        else pendingAmt += total;
    }
    
    totalPaidLabel->setText(QString("本月已发放金额: ¥%1").arg(paidAmt, 0, 'f', 2));
    pendingLabel->setText(QString("待结算金额: ¥%1").arg(pendingAmt, 0, 'f', 2));
}

void SalaryModule::onPrevPage()
{
    if (m_currentPage > 1) {
        m_currentPage--;
        updatePagination();
    }
}

void SalaryModule::onNextPage()
{
    int total = m_salaryData.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    if (m_currentPage < totalPages) {
        m_currentPage++;
        updatePagination();
    }
}

void SalaryModule::onJumpPage()
{
    int page = jumpEdit->text().toInt();
    if (page >= 1) {
        m_currentPage = page;
        updatePagination();
    }
    jumpEdit->clear();
    jumpEdit->clearFocus();
}

void SalaryModule::onBatchPay()
{
    int count = 0;
    
    for (int i = 0; i < salaryTable->rowCount(); ++i) {
        QWidget *w = salaryTable->cellWidget(i, 0);
        if (w) {
            QCheckBox *cb = w->findChild<QCheckBox*>();
            if (cb && cb->isChecked()) {
                QString empId = salaryTable->item(i, 1)->text();
                // 找到并更新数据集
                for (auto &record : m_salaryData) {
                    if (record.empId == empId && record.status == "待发放") {
                        record.status = "已发放";
                        count++;
                        break;
                    }
                }
            }
        }
    }
    
    if (count == 0) {
        CustomMessageDialog::showWarning(this, "批量发放", "请先勾选需要发放且状态为「待发放」的记录！");
        return;
    }
    
    CustomMessageDialog::showSuccess(this, "系统提示", QString("批量发放成功，共计影响 %1 条薪资记录！").arg(count));
    updatePagination();
    updateStats();
}

void SalaryModule::onPaySingle()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    for (int i = 0; i < salaryTable->rowCount(); ++i) {
        QWidget *w = salaryTable->cellWidget(i, 7); // 操作列是第 7 列
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
            QString empId = salaryTable->item(i, 1)->text();
            QString name = salaryTable->item(i, 2)->text();
            
            if (CustomMessageDialog::confirm(this, "发放确认", QString("是否确认发放【%1】的当月薪资？记录之后不可逆。").arg(name))) {
                for (auto &record : m_salaryData) {
                    if (record.empId == empId) {
                        record.status = "已发放";
                        break;
                    }
                }
                updatePagination();
                updateStats();
            }
            break;
        }
    }
}

void SalaryModule::onRecalculateSingle()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    CustomMessageDialog::showSuccess(this, "重新核算", "已触发系统后台重新调取该员工本月业绩及考勤，生成了最新提成记录。");
    // 此处可编写触发重新请求对应 empId 的提成接口逻辑。
}

void SalaryModule::onFilter() 
{
    // 实现假筛选，只是演示：你可以根据 combo 的数据去重建 m_salaryData
    m_currentPage = 1;
    updatePagination();
}

void SalaryModule::onExport() 
{
    CustomMessageDialog::showSuccess(this, "导出报表", "报表 PDF/Excel 文件已生成并存放在当前桌面上。");
}
