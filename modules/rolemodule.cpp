#include "rolemodule.h"
#include "addemployeedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QtGlobal>
#include <QPushButton>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>

RoleModule::RoleModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void RoleModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    // 1. 顶部标题与搜索栏
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *title = new QLabel("员工权限与考勤管理", this);
    title->setStyleSheet("font-size: 22px; font-weight: bold; color: #303133;");

    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(10);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索员工姓名、账号、手机号...");
    searchEdit->setFixedWidth(280);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    filterLayout->addWidget(searchEdit);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addLayout(filterLayout);
    mainLayout->addLayout(headerLayout);

    // 2. 高端统计卡片区域
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(20);

    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &outValueLabel, QLabel* &outTrendLabel, const QColor &color, bool showTrend = true) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; } ");
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 20));
        shadow->setOffset(0, 4);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *l = new QHBoxLayout(card);
        l->setContentsMargins(20, 15, 20, 15);

        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(50, 50);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("font-size: 28px; color: %1; background: #f5f7fa; border-radius: 10px; border: none;").arg(color.name()));
        l->addWidget(iconLabel);
        l->addSpacing(15);

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);
        QLabel *labelTitle = new QLabel(label);
        labelTitle->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        
        outValueLabel = new QLabel("--");
        outValueLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #303133; border: none; background: transparent;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(outValueLabel);

        if (showTrend) {
            QHBoxLayout *trendLayout = new QHBoxLayout();
            outTrendLabel = new QLabel("--");
            outTrendLabel->setStyleSheet("color: #67c23a; font-size: 11px; border: none; background: transparent;");
            trendLayout->addWidget(outTrendLabel);
            trendLayout->addStretch();
            textLayout->addLayout(trendLayout);
        } else {
            outTrendLabel = nullptr;
            textLayout->addStretch();
        }

        l->addLayout(textLayout);
        l->addStretch();

        return card;
    };

    QLabel* dummy = nullptr;
    statLayout->addWidget(createStatCard("👥", "在职员工", totalEmpLabel, dummy, QColor("#409eff"), false));
    statLayout->addWidget(createStatCard("📅", "今日实到", todayAttendLabel, attendRateLabel, QColor("#67c23a")));
    mainLayout->addLayout(statLayout);

    // 3. 员工数据表格 - 定制化列结构 (拆分联系方式)
    empTable = new QTableWidget();
    empTable->setColumnCount(10); 
    empTable->setHorizontalHeaderLabels({"工号", "姓名", "职位", "性别", "年龄", "手机号", "邮箱", "状态", "底薪", "操作"});
    
    empTable->setShowGrid(false);
    empTable->setAlternatingRowColors(true);
    empTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    empTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    empTable->setWordWrap(true);
    empTable->verticalHeader()->setVisible(false);
    empTable->verticalHeader()->setDefaultSectionSize(70); 

    empTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background-color: white; color: black; outline: none; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background-color: #f5f7fa; padding: 10px; border: none; color: #606266; font-weight: bold; font-size: 13px; } "
    );

    // 精准对齐与空间分配 (优化比例)
    QHeaderView *header = empTable->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setSectionResizeMode(0, QHeaderView::Fixed); empTable->setColumnWidth(0, 50);  // 工号
    header->setSectionResizeMode(1, QHeaderView::Fixed); empTable->setColumnWidth(1, 80);  // 姓名
    header->setSectionResizeMode(2, QHeaderView::Fixed); empTable->setColumnWidth(2, 110); // 职位 (收紧)
    header->setSectionResizeMode(3, QHeaderView::Fixed); empTable->setColumnWidth(3, 40);  // 性别
    header->setSectionResizeMode(4, QHeaderView::Fixed); empTable->setColumnWidth(4, 40);  // 年龄
    header->setSectionResizeMode(7, QHeaderView::Fixed); empTable->setColumnWidth(7, 60);  // 状态
    header->setSectionResizeMode(8, QHeaderView::Fixed); empTable->setColumnWidth(8, 80);  // 底薪
    header->setSectionResizeMode(9, QHeaderView::Fixed); empTable->setColumnWidth(9, 130); // 操作

    // 注入演示数据 (包含性别、年龄、电话、邮箱、身份证、底薪、提成)
    addEmployeeRow("E001", "李四", "高级美容师", "正常", "男", 28, "13800138000", "lisi@pet.com", "440106199601011234", 3500, 15000, 2250);
    addEmployeeRow("E002", "王五", "店员", "请假", "女", 24, "13911223344", "wangwu@pet.com", "440106200005204321", 3000, 3000, 450);
    addEmployeeRow("E003", "张三", "实习生", "正常", "男", 21, "13755667788", "zhangsan@pet.com", "440106200310105566", 1200, 0, 0);
    addEmployeeRow("E004", "赵六", "宠物医生", "正常", "男", 35, "15088996677", "zhaoliu@pet.com", "440106198912128899", 6500, 40000, 5600);

    mainLayout->addWidget(empTable);

    // 4. 底部操作栏
    QPushButton *addBtn = new QPushButton("+ 录入新员工正式入职档案");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setMinimumHeight(55);
    addBtn->setStyleSheet(
        "QPushButton { background-color: #409eff; color: white; border-radius: 8px; font-weight: bold; border: none; font-size: 15px; padding: 5px 20px; } "
        "QPushButton:hover { background-color: #66b1ff; } "
    );
    connect(addBtn, &QPushButton::clicked, this, &RoleModule::onAddEmployee);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(addBtn);

    updateStats();
}

void RoleModule::updateStats()
{
    int totalEmp = empTable->rowCount();
    int todayAttend = 0;
    double totalSalary = 0;

    for (int i = 0; i < totalEmp; ++i) {
        // 状态列索引是 7 (工号, 姓名, 职位, 性别, 年龄, 手机, 邮箱, 状态...)
        QWidget* statusWidget = empTable->cellWidget(i, 7);
        if (statusWidget) {
            QLabel* tag = statusWidget->findChild<QLabel*>();
            if (tag && tag->text() == "正常") {
                todayAttend++;
            }
        }

        // 薪酬列索引是 8
        QTableWidgetItem* salaryItem = empTable->item(i, 8);
        if (salaryItem) {
            QString text = salaryItem->text();
            int index = text.lastIndexOf("￥");
            if (index != -1) {
                totalSalary += text.mid(index + 1).toDouble();
            }
        }
    }

    totalEmpLabel->setText(QString("%1人").arg(totalEmp));
    todayAttendLabel->setText(QString("%1人").arg(todayAttend));
    double rate = (totalEmp > 0) ? (double)todayAttend / totalEmp * 100 : 0;
    attendRateLabel->setText(QString("当日出勤率: %1%").arg(rate, 0, 'f', 1));
}

void RoleModule::addEmployeeRow(const QString &id, const QString &name, const QString &role, const QString &status, 
                                const QString &gender, int age, const QString &phone, const QString &email, const QString &idCard,
                                double baseSalary, double /*performance*/, double commission)
{
    Q_UNUSED(idCard);
    int row = empTable->rowCount();
    empTable->insertRow(row);

    // 辅助函数：设置居中文字项
    auto setItem = [&](int col, const QString &text, bool bold = false, const QColor &color = QColor()) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        if (bold) item->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        if (color.isValid()) item->setForeground(QBrush(color));
        empTable->setItem(row, col, item);
    };

    setItem(0, id);
    setItem(1, name, true);
    setItem(2, role);
    setItem(3, gender);
    setItem(4, QString("%1岁").arg(age));
    setItem(5, phone);
    setItem(6, email);

    // 考勤状态
    QWidget *statusContainer = new QWidget();
    QHBoxLayout *statusLayout = new QHBoxLayout(statusContainer);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setAlignment(Qt::AlignCenter);
    QLabel *statusTag = new QLabel(status);
    QString tagStyle = "padding: 2px 8px; border-radius: 10px; font-size: 11px; font-weight: bold; ";
    if (status == "正常") tagStyle += "background-color: #f0f9eb; color: #67c23a; border: 1px solid #e1f3d8;";
    else if (status == "请假") tagStyle += "background-color: #fdf6ec; color: #e6a23c; border: 1px solid #faecd8;";
    else tagStyle += "background-color: #fef0f0; color: #f56c6c; border: 1px solid #fde2e2;";
    statusTag->setStyleSheet(tagStyle);
    statusLayout->addWidget(statusTag);
    empTable->setCellWidget(row, 7, statusContainer);

    // 底薪显示
    setItem(8, QString("￥%1").arg(baseSalary, 0, 'f', 0), true, QColor("#303133"));

    // 管理按钮 - 修改 & 删除 (双按钮设计)
    QWidget *btnContainer = new QWidget();
    QHBoxLayout *btnLayout = new QHBoxLayout(btnContainer);
    btnLayout->setContentsMargins(0, 0, 0, 0); // 移除边距，给按钮留空间
    btnLayout->setSpacing(6);
    btnLayout->setAlignment(Qt::AlignCenter);

    QPushButton *editBtn = new QPushButton("修改");
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setFixedSize(56, 28);
    editBtn->setStyleSheet(
        "QPushButton { border: 1px solid #409eff; border-radius: 4px; background: white; color: #409eff; font-size: 11px; font-weight: bold; padding: 0 5px; } "
        "QPushButton:hover { background: #409eff; color: white; }"
    );
    connect(editBtn, &QPushButton::clicked, this, &RoleModule::onEditEmployee);

    QPushButton *delBtn = new QPushButton("删除");
    delBtn->setCursor(Qt::PointingHandCursor);
    delBtn->setFixedSize(56, 28);
    delBtn->setStyleSheet(
        "QPushButton { border: 1px solid #f56c6c; border-radius: 4px; background: white; color: #f56c6c; font-size: 11px; font-weight: bold; padding: 0 5px; } "
        "QPushButton:hover { background: #f56c6c; color: white; }"
    );
    connect(delBtn, &QPushButton::clicked, this, &RoleModule::onDeleteEmployee);

    btnLayout->addWidget(editBtn);
    btnLayout->addWidget(delBtn);
    empTable->setCellWidget(row, 9, btnContainer);
}

void RoleModule::onAddEmployee()
{
    AddEmployeeDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        EmployeeInfo info = dlg.employeeInfo();
        // 业绩演示为 0
        addEmployeeRow(info.id, info.name, info.role, info.status, info.gender, info.age, 
                       info.phone, info.email, info.idCard, info.baseSalary, 0, 0);
        updateStats();
    }
}

void RoleModule::onEditEmployee()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    // 找到按钮所在的行
    for (int i = 0; i < empTable->rowCount(); ++i) {
        QWidget *w = empTable->cellWidget(i, 9);
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
             // 提取当前行信息
             EmployeeInfo info;
             info.id = empTable->item(i, 0)->text();
             info.name = empTable->item(i, 1)->text();
             info.role = empTable->item(i, 2)->text();
             info.gender = empTable->item(i, 3)->text();
             QString ageStr = empTable->item(i, 4)->text();
             info.age = ageStr.left(ageStr.length() - 1).toInt();
             info.phone = empTable->item(i, 5)->text();
             info.email = empTable->item(i, 6)->text();

             info.status = empTable->cellWidget(i, 7)->findChild<QLabel*>()->text();
             info.idCard = ""; 
             
             // 底薪提取
             QString salary = empTable->item(i, 8)->text();
             info.baseSalary = salary.remove("￥").toDouble();

             AddEmployeeDialog dlg(this);
             dlg.setEmployeeInfo(info); 
             if (dlg.exec() == QDialog::Accepted) {
                 EmployeeInfo newInfo = dlg.employeeInfo();
                 // 由于是内存演示版，我们直接删除旧行添加新行，或者原地修改
                 empTable->removeRow(i);
                 addEmployeeRow(newInfo.id, newInfo.name, newInfo.role, newInfo.status, newInfo.gender, 
                                newInfo.age, newInfo.phone, newInfo.email, newInfo.idCard, 
                                newInfo.baseSalary, 0, 0);
                 updateStats();
             }
             break;
        }
    }
}

void RoleModule::onDeleteEmployee()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    for (int i = 0; i < empTable->rowCount(); ++i) {
        QWidget *w = empTable->cellWidget(i, 9);
        if (w && w->layout() && w->layout()->indexOf(btn) != -1) {
            empTable->removeRow(i);
            updateStats();
            break;
        }
    }
}
