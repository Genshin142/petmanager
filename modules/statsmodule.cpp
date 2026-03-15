#include "statsmodule.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>

StatsModule::StatsModule(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("系统统计分析报表", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    layout->addWidget(title);

    QLabel *flow = new QLabel("今日营收概况：￥4,280.00 | 洗护占比：45% | 商品占比：30% | 寄养占比：25%");
    flow->setStyleSheet("background-color: #304156; color: #409eff; padding: 15px; border-radius: 4px; font-size: 16px;");
    layout->addWidget(flow);

    QLabel *rankTitle = new QLabel("服务热销排行 Top 5", this);
    rankTitle->setStyleSheet("margin-top: 10px; font-weight: bold;");
    layout->addWidget(rankTitle);

    QTableWidget *table = new QTableWidget(5, 3);
    table->setHorizontalHeaderLabels({"排名", "项目名称", "成交单量"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    QStringList items = {"猫咪全身洗护", "精修造型", "皇家全价粮", "疫苗套餐", "狗狗日常代喂"};
    QStringList counts = {"42", "28", "15", "10", "8"};
    for(int i=0; i<5; ++i) {
        table->setItem(i, 0, new QTableWidgetItem(QString::number(i+1)));
        table->setItem(i, 1, new QTableWidgetItem(items[i]));
        table->setItem(i, 2, new QTableWidgetItem(counts[i]));
    }
    layout->addWidget(table);
}
