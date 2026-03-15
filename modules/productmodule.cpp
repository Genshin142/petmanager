#include "productmodule.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>

ProductModule::ProductModule(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("商品进销存监控", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    layout->addWidget(title);

    QTableWidget *table = new QTableWidget(4, 6);
    table->setHorizontalHeaderLabels({"条码", "商品名称", "规格", "售价", "当前库存", "预警状态"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    table->setItem(0, 0, new QTableWidgetItem("690123456"));
    table->setItem(0, 1, new QTableWidgetItem("皇家猫粮 2kg"));
    table->setItem(0, 2, new QTableWidgetItem("袋"));
    table->setItem(0, 3, new QTableWidgetItem("￥199.00"));
    table->setItem(0, 4, new QTableWidgetItem("5"));
    
    auto warnItem = new QTableWidgetItem("库存过低！");
    warnItem->setForeground(QBrush(Qt::red));
    table->setItem(0, 5, warnItem);

    layout->addWidget(table);
    
    QPushButton *stockInBtn = new QPushButton("商品入库登记");
    stockInBtn->setStyleSheet("background-color: #409eff; color: white; padding: 10px;");
    layout->addWidget(stockInBtn);
}
