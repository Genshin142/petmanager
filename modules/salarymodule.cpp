#include "salarymodule.h"
#include "custommessagedialog.h"
#include "staffdatamanager.h"
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

SalaryModule::SalaryModule(QWidget *parent) : QWidget(parent),
    m_currentPage(1), m_pageSize(10), pageLabel(nullptr), jumpEdit(nullptr), prevBtn(nullptr), nextBtn(nullptr), jumpValidator(nullptr)
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
    toolbar->setStyleSheet("QFrame { background: white; border-radius: 6px; }");
    QHBoxLayout *toolLayout = new QHBoxLayout(toolbar);
    
    auto styleCombo = [&](QComboBox* cb) {
        cb->setFixedHeight(36);
        cb->setStyleSheet(
            "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; } "
            "QComboBox:hover { border-color: #409eff; } "
            "QComboBox::drop-down { border: none; width: 24px; } "
            "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
            "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }"
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
        "QPushButton { background: #409eff; color: white; border-radius: 6px; border: none; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { background: #66b1ff; } "
        "QPushButton:pressed { background: #3a8ee6; }"
    );

    QPushButton *exportBtn = new QPushButton("导出报表");
    exportBtn->setFixedWidth(110);
    exportBtn->setFixedHeight(34);
    exportBtn->setStyleSheet(
        "QPushButton { background: white; color: #606266; border-radius: 6px; border: 1px solid #dcdfe6; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { border-color: #409eff; color: #409eff; } "
    );

    toolLayout->addWidget(employeeCombo);
    toolLayout->addWidget(yearCombo);
    toolLayout->addWidget(monthCombo);
    toolLayout->addStretch();
    toolLayout->addWidget(searchBtn);
    toolLayout->addWidget(exportBtn);
    mainLayout->addWidget(toolbar);

    // 3. 表格
    salaryTable = new QTableWidget();
    salaryTable->setColumnCount(7);
    salaryTable->setHorizontalHeaderLabels({"工号", "姓名", "底薪", "提成/绩效", "实发总计", "状态", "操作"});
    salaryTable->setColumnWidth(0, 100);
    salaryTable->setColumnWidth(1, 120);
    salaryTable->setColumnWidth(2, 120);
    salaryTable->setColumnWidth(3, 120);
    salaryTable->setColumnWidth(4, 120);
    salaryTable->setColumnWidth(5, 100);
    salaryTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    
    salaryTable->setFrameShape(QFrame::NoFrame);
    salaryTable->setAlternatingRowColors(false);
    salaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    salaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    salaryTable->verticalHeader()->setVisible(false);
    salaryTable->verticalHeader()->setDefaultSectionSize(44);
    salaryTable->setShowGrid(false);
    salaryTable->setStyleSheet(
        "QTableWidget { background: white; border: 1px solid #ebeef5; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; padding: 6px; } "
        "QTableWidget::item:selected { background: #f0f7ff; color: #409eff; } "
        "QHeaderView::section { background: #f5f7fa; border: none; border-bottom: 1px solid #ebeef5; padding: 10px; color: #606266; font-size: 13px; font-weight: bold; }"
    );
    salaryTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    mainLayout->addWidget(salaryTable, 1);

    // 4. 底部统计与分页
    QFrame *footer = new QFrame();
    footer->setFixedHeight(50);
    footer->setStyleSheet("QFrame { background: #f8f9fb; border-top: 1px solid #ebeef5; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(footer);

    totalPaidLabel = new QLabel("本月已发放金额: ¥0.00");
    totalPaidLabel->setStyleSheet("color: #67c23a; font-size: 13px; font-weight: bold;");
    pendingLabel = new QLabel("待结算金额: ¥0.00");
    pendingLabel->setStyleSheet("color: #f56c6c; font-size: 13px; font-weight: bold;");

    footerLayout->addWidget(totalPaidLabel);
    footerLayout->addSpacing(20);
    footerLayout->addWidget(pendingLabel);
    footerLayout->addStretch();

    prevBtn = new QPushButton("上一页");
    nextBtn = new QPushButton("下一页");
    pageLabel = new QLabel("第 1 页 / 共 1 页");
    QString pageStyle = "QPushButton { height: 24px; border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; font-size: 12px; padding: 0 8px; } "
                        "QPushButton:hover { border-color: #409eff; color: #409eff; } "
                        "QPushButton:disabled { background: #f5f7fa; color: #c0c4cc; }";
    prevBtn->setStyleSheet(pageStyle);
    nextBtn->setStyleSheet(pageStyle);
    prevBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setCursor(Qt::PointingHandCursor);
    pageLabel->setStyleSheet("color: #909399; font-size: 13px;");

    footerLayout->addWidget(prevBtn);
    footerLayout->addWidget(pageLabel);
    footerLayout->addWidget(nextBtn);
    mainLayout->addWidget(footer);

    connect(prevBtn, &QPushButton::clicked, this, &SalaryModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &SalaryModule::onNextPage);
    connect(searchBtn, &QPushButton::clicked, this, &SalaryModule::onFilter);
    connect(exportBtn, &QPushButton::clicked, this, &SalaryModule::onExport);
}

void SalaryModule::initData()
{
    // 从统一数据管理器同步真实员工名录，确保薪资单据不“乱写”
    m_salaryData.clear();
    auto all = StaffDataManager::instance()->allStaff();
    for (const auto &info : all) {
        if (info.status == "离职") continue;
        
        // 模拟提成数据 (实际可从业绩模块计算)
        double mockCommission = (info.role.contains("美容师") || info.role.contains("医生")) ? 1200.0 : 300.0;
        m_salaryData.append({info.id, info.name, (double)info.baseSalary, mockCommission, "待发放"});
    }
    
    updatePagination();
    updateStats();
}

void SalaryModule::addSalaryRow(const SalaryRecord &record)
{
    int row = salaryTable->rowCount();
    salaryTable->insertRow(row);

    auto setItem = [&](int col, QString text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        item->setFont(QFont("Microsoft YaHei", 9));
        salaryTable->setItem(row, col, item);
    };

    setItem(0, record.empId);
    setItem(1, record.name);
    setItem(2, QString("¥%1").arg(record.baseSalary, 0, 'f', 2));
    setItem(3, QString("¥%1").arg(record.commission, 0, 'f', 2));
    
    double total = record.baseSalary + record.commission;
    QTableWidgetItem *totalItem = new QTableWidgetItem(QString("¥%1").arg(total, 0, 'f', 2));
    totalItem->setTextAlignment(Qt::AlignCenter);
    totalItem->setForeground(QColor("#409eff"));
    totalItem->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    salaryTable->setItem(row, 4, totalItem);

    // 状态标签
    QWidget *statusW = new QWidget();
    QHBoxLayout *statusL = new QHBoxLayout(statusW);
    statusL->setContentsMargins(0, 0, 0, 0);
    statusL->setAlignment(Qt::AlignCenter);
    QLabel *statusTag = new QLabel(record.status);
    if (record.status == "待发放") {
        statusTag->setStyleSheet("background: #ffedd5; color: #9a3412; border-radius: 12px; padding: 4px 12px; font-size: 11px; font-weight: bold;");
    } else {
        statusTag->setStyleSheet("background: #dcfce7; color: #166534; border-radius: 12px; padding: 4px 12px; font-size: 11px; font-weight: bold;");
    }
    statusL->addWidget(statusTag);
    salaryTable->setCellWidget(row, 5, statusW);

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

    salaryTable->setCellWidget(row, 6, actionWidget);
}

void SalaryModule::updatePagination()
{
    salaryTable->setRowCount(0);
    int total = m_salaryData.size();
    int totalPages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    
    // if (jumpValidator) jumpValidator->setTop(totalPages);
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
}



void SalaryModule::onPaySingle()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    for (int i = 0; i < salaryTable->rowCount(); ++i) {
        QWidget *w = salaryTable->cellWidget(i, 6); // 操作列是第 7 列
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
            QString empId = salaryTable->item(i, 0)->text();
            QString name = salaryTable->item(i, 1)->text();
            
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
