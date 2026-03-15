#include "petmodule.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>

PetModule::PetModule(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("宠物数字化健康档案", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    layout->addWidget(title);

    // 状态统计栏
    QHBoxLayout *statLayout = new QHBoxLayout();
    QString cardStyle = "background-color: white; border-radius: 8px; padding: 15px; border: 1px solid #ebeef5;";
    
    auto createCard = [&](QString t, QString v, QString color) {
        QWidget *w = new QWidget(); w->setStyleSheet(cardStyle);
        QVBoxLayout *l = new QVBoxLayout(w);
        QLabel *tl = new QLabel(t); tl->setStyleSheet("color: #909399;");
        QLabel *vl = new QLabel(v); vl->setStyleSheet("font-size: 24px; font-weight: bold; color: " + color + ";");
        l->addWidget(tl); l->addWidget(vl);
        return w;
    };

    statLayout->addWidget(createCard("在店寄养", "12", "#409eff"));
    statLayout->addWidget(createCard("正在美容", "5", "#67c23a"));
    statLayout->addWidget(createCard("今日离店", "8", "#e6a23c"));
    layout->addLayout(statLayout);

    QTableWidget *table = new QTableWidget(5, 7);
    table->setHorizontalHeaderLabels({"宠物ID", "名称", "品种", "健康档案", "最近疫苗", "当前状态", "主人"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    table->setItem(0, 0, new QTableWidgetItem("P1001"));
    table->setItem(0, 1, new QTableWidgetItem("二哈"));
    table->setItem(0, 2, new QTableWidgetItem("哈士奇"));
    table->setItem(0, 3, new QTableWidgetItem("接种齐, 无过敏"));
    table->setItem(0, 4, new QLine() ? new QTableWidgetItem("2025-12-01") : nullptr);
    table->setItem(0, 5, new QTableWidgetItem("寄养中"));
    table->setItem(0, 6, new QTableWidgetItem("张三"));
    
    layout->addWidget(table);
}
