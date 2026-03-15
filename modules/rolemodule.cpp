#include "rolemodule.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>

RoleModule::RoleModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void RoleModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("员工权限与考勤管理 (管理员专享)", this);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #303133;");
    mainLayout->addWidget(title);

    // 快捷统计
    QHBoxLayout *statLayout = new QHBoxLayout();
    auto createStat = [&](QString label, QString value) {
        QWidget *w = new QWidget();
        w->setStyleSheet("background-color: white; border-radius: 8px; border: 1px solid #ebeef5; padding: 10px;");
        QVBoxLayout *l = new QVBoxLayout(w);
        l->addWidget(new QLabel(label));
        QLabel *v = new QLabel(value); v->setStyleSheet("font-size: 20px; font-weight: bold; color: #409eff;");
        l->addWidget(v);
        return w;
    };
    statLayout->addWidget(createStat("在职员工", "8人"));
    statLayout->addWidget(createStat("今日排班", "5人"));
    statLayout->addWidget(createStat("待核提成", "￥12,500.00"));
    mainLayout->addLayout(statLayout);

    QTableWidget *empTable = new QTableWidget(4, 5);
    empTable->setHorizontalHeaderLabels({"工号", "姓名", "角色", "考勤状态", "本月提成"});
    empTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    empTable->setItem(0, 0, new QTableWidgetItem("E001"));
    empTable->setItem(0, 1, new QTableWidgetItem("李四"));
    empTable->setItem(0, 2, new QTableWidgetItem("高级美容师"));
    empTable->setItem(0, 3, new QTableWidgetItem("正常"));
    empTable->setItem(0, 4, new QTableWidgetItem("￥1,250.00"));

    empTable->setItem(1, 0, new QTableWidgetItem("E002"));
    empTable->setItem(1, 1, new QTableWidgetItem("王五"));
    empTable->setItem(1, 2, new QTableWidgetItem("店员"));
    empTable->setItem(1, 3, new QTableWidgetItem("请假"));
    empTable->setItem(1, 4, new QTableWidgetItem("￥450.00"));

    mainLayout->addWidget(empTable);

    QPushButton *addBtn = new QPushButton("+ 新增员工并分配角色");
    addBtn->setStyleSheet("background-color: #409eff; color: white; padding: 10px; border-radius: 4px;");
    mainLayout->addWidget(addBtn);
}
