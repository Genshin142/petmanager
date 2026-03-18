#include "salarymodule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFrame>
#include <QTabWidget>
#include <QLineEdit>
#include <QScrollBar>
#include <QDate>
#include <QAbstractItemView>

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
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #303133;");
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
            "   selection-background-color: #f5f7fa;"
            "   selection-color: #409eff;"
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
    for(int i=2024; i<=currentYear; i++) yearCombo->addItem(QString::number(i) + "年", i);
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
        "QPushButton { background: #409eff; color: white; border-radius: 17px; border: none; font-weight: bold; font-size: 13px; text-align: center; padding: 0 5px; } "
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
    toolLayout->addWidget(exportBtn);
    mainLayout->addWidget(toolbar);

    // 3. Tab 展示区
    QTabWidget *tabs = new QTabWidget();
    tabs->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #ebeef5; background: white; border-radius: 4px; } "
        "QTabBar::tab { background: #f5f7fa; padding: 12px 25px; border: 1px solid #ebeef5; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; min-width: 100px; color: #909399; } "
        "QTabBar::tab:selected { background: white; color: #409eff; font-weight: bold; border-bottom-color: white; } "
    );

    // 页1: 薪资明细
    salaryTable = new QTableWidget();
    salaryTable->setColumnCount(6);
    salaryTable->setHorizontalHeaderLabels({"工号", "姓名", "底薪", "提成/绩效", "实发总计", "发放状态"});
    salaryTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    salaryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    salaryTable->setFrameShape(QFrame::NoFrame);
    salaryTable->setAlternatingRowColors(true);
    salaryTable->verticalHeader()->setVisible(false);
    salaryTable->verticalHeader()->setDefaultSectionSize(45);
    salaryTable->setStyleSheet(
        "QTableWidget { gridline-color: #f0f2f5; outline: none; } "
        "QHeaderView::section { background-color: #f5f7fa; padding: 10px; border: none; font-weight: bold; color: #606266; } "
    );
    
    tabs->addTab(salaryTable, "本月薪资明细");
    tabs->addTab(new QWidget(), "历史发放记录");
    mainLayout->addWidget(tabs);

    // 4. 底部概览
    QFrame *bottomBar = new QFrame();
    bottomBar->setFixedHeight(60);
    bottomBar->setStyleSheet("background: #f8f9fb; border-top: 1px solid #ebeef5;");
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    
    totalPaidLabel = new QLabel("本月已发放金额: ¥0.00");
    totalPaidLabel->setStyleSheet("color: #67c23a; font-weight: bold; font-size: 14px;");
    pendingLabel = new QLabel("待结算金额: ¥0.00");
    pendingLabel->setStyleSheet("color: #f56c6c; font-weight: bold; font-size: 14px;");
    
    bottomLayout->addWidget(totalPaidLabel);
    bottomLayout->addSpacing(30);
    bottomLayout->addWidget(pendingLabel);
    bottomLayout->addStretch();
    mainLayout->addWidget(bottomBar);

    connect(searchBtn, &QPushButton::clicked, this, &SalaryModule::onFilter);
    connect(exportBtn, &QPushButton::clicked, this, &SalaryModule::onExport);
}

void SalaryModule::initData()
{
    salaryTable->setRowCount(2);
    
    auto setItem = [&](int row, int col, QString text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        salaryTable->setItem(row, col, item);
    };

    setItem(0, 0, "E001"); setItem(0, 1, "张三"); setItem(0, 2, "¥5000"); setItem(0, 3, "¥1200"); setItem(0, 4, "¥6200"); setItem(0, 5, "已发放");
    setItem(1, 0, "E002"); setItem(1, 1, "李四"); setItem(1, 2, "¥4500"); setItem(1, 3, "¥800"); setItem(1, 4, "¥5300"); setItem(1, 5, "待发放");

    totalPaidLabel->setText("本月已发放金额: ¥6,200.00");
    pendingLabel->setText("待结算金额: ¥5,300.00");
}

void SalaryModule::onFilter() {}
void SalaryModule::onExport() {}
