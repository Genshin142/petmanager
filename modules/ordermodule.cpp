#include "ordermodule.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLineEdit>

OrderModule::OrderModule(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("业务中心：预约与收银结算", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    layout->addWidget(title);

    // 预约与收银分栏
    QHBoxLayout *topPanel = new QHBoxLayout();
    
    // --- 预约防冲突 ---
    QWidget *apptWidget = new QWidget();
    apptWidget->setStyleSheet("background-color: white; border-radius: 8px; padding: 15px;");
    QVBoxLayout *al = new QVBoxLayout(apptWidget);
    al->addWidget(new QLabel("工位预约冲突检测 (Mock)"));
    QTableWidget *apptTable = new QTableWidget(3, 3);
    apptTable->setHorizontalHeaderLabels({"时间", "工位", "宠物"});
    apptTable->setItem(0, 0, new QTableWidgetItem("14:30"));
    apptTable->setItem(0, 1, new QTableWidgetItem("美容台A"));
    apptTable->setItem(0, 2, new QTableWidgetItem("金毛-多多"));
    al->addWidget(apptTable);
    topPanel->addWidget(apptWidget);

    // --- 快速收银 ---
    QWidget *cashWidget = new QWidget();
    cashWidget->setStyleSheet("background-color: white; border-radius: 8px; padding: 15px;");
    QVBoxLayout *cl = new QVBoxLayout(cashWidget);
    cl->addWidget(new QLabel("扫码枪快速录入"));
    QLineEdit *scanEdit = new QLineEdit(); scanEdit->setPlaceholderText("扫描商品/服务码...");
    cl->addWidget(scanEdit);
    
    cl->addWidget(new QLabel("结算列表"));
    QTableWidget *billTable = new QTableWidget(2, 3);
    billTable->setHorizontalHeaderLabels({"项目", "单价", "折后"});
    billTable->setItem(0, 0, new QTableWidgetItem("洗护服务"));
    billTable->setItem(0, 1, new QTableWidgetItem("80.00"));
    billTable->setItem(0, 2, new QTableWidgetItem("72.00"));
    cl->addWidget(billTable);
    
    QLabel *totalLabel = new QLabel("<span style='font-size:18px;'>总计: </span><span style='color:red; font-size:24px; font-weight:bold;'>￥72.00</span>");
    cl->addWidget(totalLabel);
    QPushButton *payBtn = new QPushButton("一键结算 (自动更新库存)");
    payBtn->setStyleSheet("background-color: #67c23a; color: white; padding: 12px;");
    cl->addWidget(payBtn);
    
    topPanel->addWidget(cashWidget);
    layout->addLayout(topPanel);
}
