#include "membermodule.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include "addmemberdialog.h"
#include "custommessagedialog.h"
#include "selectpetdialog.h"
#include "addpetdialog.h"
#include <QMenu>
#include <QAction>
#include <QComboBox>
#include <QFileDialog>
#include <QTextStream>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QPixmap>
#include <QBrush>
#include <QColor>
#include <QFont>

MemberModule::MemberModule(UserRole role, QWidget *parent) : QWidget(parent), m_role(role)
{
    setupUI();
}

void MemberModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 1. 标题区域
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *title = new QLabel("会员信息管理", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #303133;");

    QPushButton *exportBtn = new QPushButton("导出 CSV");
    exportBtn->setCursor(Qt::PointingHandCursor);
    exportBtn->setStyleSheet("QPushButton { padding: 6px 16px; border-radius: 4px; border: 1px solid #dcdfe6; background: white; color: #606266; } QPushButton:hover { border-color: #409eff; color: #409eff; }");
    connect(exportBtn, &QPushButton::clicked, this, &MemberModule::exportData);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(exportBtn);
    mainLayout->addLayout(headerLayout);

    // 2. 搜索栏
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索会员姓名、手机号、ID...");
    searchEdit->setFixedWidth(280);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    searchLayout->addWidget(searchEdit);
    searchLayout->addStretch();

    QPushButton *addBtn = new QPushButton("+ 新增会员档案");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet("QPushButton { background: #67c23a; color: white; padding: 9px 20px; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: #85ce61; }");
    connect(addBtn, &QPushButton::clicked, this, &MemberModule::showAddMemberDialog);

    searchLayout->addWidget(addBtn);
    mainLayout->addLayout(searchLayout);

    // 3. 表格与空状态区域 (使用 QStackedLayout)
    stackLayout = new QStackedLayout(); // 直接作为 layout，不再包裹多余的 QWidget

    memTable = new QTableWidget();
    memTable->setColumnCount(9);
    memTable->setHorizontalHeaderLabels({"会员ID", "会员姓名", "手机号码", "会员等级", "储值余额", "累计消费金额", "可用积分", "宠物档案", "管理操作"});

    // 固定的列宽策略与居中对齐
    memTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    memTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    memTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Fixed);
    memTable->setColumnWidth(8, 290); // 彻底扩容，确保三个按钮及其间距完美显示

    memTable->setShowGrid(false);
    memTable->setAlternatingRowColors(true);
    memTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memTable->verticalHeader()->setVisible(false);
    memTable->verticalHeader()->setDefaultSectionSize(48); // 统一行高，确保按钮放得下

    // 【修复1：样式调整】强制字体为纯黑，并自定义选中后的背景色，保证选中时清晰可见
    memTable->setStyleSheet(
        "QTableWidget { "
        "   border: 1px solid #ebeef5; "
        "   background-color: white; "
        "   color: black; " /* 全局默认纯黑色文字 */
        "   selection-background-color: #e4e7ed; " /* 选中行背景变为浅灰 */
        "   selection-color: black; " /* 选中行文字依然强制为纯黑 */
        "} "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background-color: #f5f7fa; padding: 12px; border: none; border-bottom: 1px solid #ebeef5; color: #606266; font-weight: bold; font-size: 13px; } "
        "QHeaderView::section:vertical { "
        "   background-color: #f5f7fa; " 
        "   color: #909399; "
        "   border: none; "
        "   border-right: 1px solid #ebeef5; "
        "   border-bottom: 1px solid #ebeef5; "
        "   text-align: center; "
        "   font-size: 12px; "
        "} "
        );

    // 空状态提示
    emptyStateWidget = new QWidget();
    emptyStateWidget->setFixedHeight(200); // 限制高度，防止由于stretch导致的大透明边框感
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyStateWidget);
    emptyLayout->setContentsMargins(0, 40, 0, 40);
    QLabel *emptyIcon = new QLabel("📁");
    emptyIcon->setStyleSheet("font-size: 50px; color: #c0c4cc;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    QLabel *emptyText = new QLabel("暂无会员数据，快去添加吧！");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyText->setStyleSheet("color: #909399; font-size: 14px;");
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(emptyText);
    emptyStateWidget->setStyleSheet("background-color: transparent;"); // 改为透明背景，消除边框感

    stackLayout->addWidget(memTable);
    stackLayout->addWidget(emptyStateWidget);
    mainLayout->addLayout(stackLayout);

    // 4. 统计栏
    QFrame *statFrame = new QFrame();
    statFrame->setStyleSheet("QFrame { background: #f8f9fb; border-top: 1px solid #ebeef5; padding: 12px; }");
    QHBoxLayout *statLayout = new QHBoxLayout(statFrame);
    totalMemberLabel = new QLabel("总会员: 0");
    goldMemberLabel = new QLabel("黄金会员: 0");
    platinumMemberLabel = new QLabel("铂金会员: 0");
    diamondMemberLabel = new QLabel("钻石会员: 0");

    QString statStyle = "color: #606266; font-size: 13px; font-weight: bold; margin-right: 35px;";
    totalMemberLabel->setStyleSheet(statStyle);
    goldMemberLabel->setStyleSheet(statStyle + "color: #e6a23c;");
    platinumMemberLabel->setStyleSheet(statStyle + "color: #409eff;");
    diamondMemberLabel->setStyleSheet(statStyle + "color: #67c23a;");

    statLayout->addWidget(totalMemberLabel);
    statLayout->addWidget(goldMemberLabel);
    statLayout->addWidget(platinumMemberLabel);
    statLayout->addWidget(diamondMemberLabel);
    statLayout->addStretch();
    mainLayout->addWidget(statFrame);

    // 绑定事件
    connect(searchEdit, &QLineEdit::textChanged, this, &MemberModule::onSearchTextChanged);
    connect(memTable, &QTableWidget::cellClicked, this, &MemberModule::onCellClicked);

    // 禁用右键菜单
    memTable->setContextMenuPolicy(Qt::NoContextMenu);

    addSampleData();
    updateStatistics();
}

void MemberModule::addRow(const QString &id, const QString &name, const QString &phone, const QString &level, double balance, double consume_amt, int pts, const QString &pets)
{
    int r = memTable->rowCount();
    memTable->insertRow(r);

    // 【修复1：移除了 setForeground】让它自然继承 StyleSheet 中的黑色
    auto createItem = [&](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFont(QFont("Microsoft YaHei", 9));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    memTable->setItem(r, 0, createItem(id));
    memTable->setItem(r, 1, createItem(name));
    memTable->setItem(r, 2, createItem(phone));
    memTable->setItem(r, 3, createItem(level));
    memTable->setItem(r, 4, createItem(QString::number(balance, 'f', 2)));
    memTable->setItem(r, 5, createItem(QString::number(consume_amt, 'f', 2)));
    memTable->setItem(r, 6, createItem(QString::number(pts)));
    
    // 【美化：使用 Element UI 风格的下拉框】
    QComboBox *petCombo = new QComboBox();
    petCombo->setCursor(Qt::PointingHandCursor);
    petCombo->setFixedHeight(32);
    petCombo->setStyleSheet(
        "QComboBox { "
        "   border: 1px solid #dcdfe6; "
        "   border-radius: 4px; " // 统一为 4px
        "   padding: 2px 12px; "
        "   background: #f5f7fa; "
        "   color: #606266; "
        "   font-size: 13px; "
        "} "
        "QComboBox:hover { border-color: #c0c4cc; } "
        "QComboBox:focus { border-color: #409eff; background: white; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; height: 14px; } "
        "QComboBox QAbstractItemView { "
        "   border: 1px solid #ebeef5; "
        "   border-radius: 4px; "
        "   background-color: white; "
        "   outline: none; "
        "   padding: 4px 0px; "
        "} "
        "QComboBox QAbstractItemView::item { "
        "   height: 35px; "
        "   padding-left: 12px; "
        "   color: #606266; "
        "   background-color: white; "
        "} "
        "QComboBox QAbstractItemView::item:selected { "
        "   background-color: #f0f7ff; "
        "   color: #409eff; "
        "   border-left: 3px solid #409eff; "
        "} "
    );

    QStringList petNames;
    if (pets != "无" && !pets.isEmpty()) {
        petNames = pets.split(", ", Qt::SkipEmptyParts);
    }

    if (petNames.isEmpty()) {
        petCombo->addItem("无");
        petCombo->setEnabled(false);
        petCombo->setStyleSheet(petCombo->styleSheet() + "QComboBox { color: #c0c4cc; background: #fafafa; }");
    } else {
        petCombo->addItems(petNames);
    }

    connect(petCombo, QOverload<int>::of(&QComboBox::activated), this, [=](int index){
        QString petName = petCombo->itemText(index);
        if (petName == "无") return;

        if (CustomMessageDialog::confirm(this, "页面跳转确认", 
            QString("检测到您选择了宠物档案：[%1]\n是否立即跳转到“宠物健康档案中心”查看详情？").arg(petName))) {
            CustomMessageDialog::showWarning(this, "提示", "正在为您跳转至宠物管理模块...");
        }
    });

    // 为 ComboBox 增加一个容器来控制垂直偏移 (向下移动一点点)
    QWidget *comboWrapper = new QWidget();
    QVBoxLayout *comboLayout = new QVBoxLayout(comboWrapper);
    comboLayout->setContentsMargins(5, 4, 5, 0); // 上边距 4px
    comboLayout->addWidget(petCombo);
    memTable->setCellWidget(r, 7, comboWrapper);
    memTable->setItem(r, 8, new QTableWidgetItem()); // 必须占位

    // 【修复2：使用标准 QWidget 渲染操作列，并加上明显的底色保证可见性】
    QWidget *actionWidget = new QWidget();
    actionWidget->setStyleSheet("background: transparent;"); // 让容器透明，防止遮挡表格选中时的整行高亮

    QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
    actionLayout->setContentsMargins(10, 5, 10, 5);
    actionLayout->setSpacing(8);

    auto createBtn = [&](const QString &text, const QString &bgColor, const QString &textColor, const QString &borderColor) {
        QPushButton *btn = new QPushButton(text);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(30); 
        btn->setFixedWidth(85); // 确保 4 个中文字符 + 边距绝对完整显示
        // 赋予按钮背景色和边框，避免在某些主题下全透明不可见
        btn->setStyleSheet(QString(
                               "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; font-size: 12px; padding: 0 10px; }"
                               "QPushButton:hover { opacity: 0.8; background-color: %2; color: white; }"
                               ).arg(bgColor, textColor, borderColor));
        return btn;
    };

    // 生成显眼的按钮 (按照用户最新要求：1.修改信息 2.添加宠物 3.删除会员)
    QPushButton *editBtn = createBtn("修改信息", "#ecf5ff", "#409eff", "#b3d8ff");
    QPushButton *petBtn = createBtn("添加宠物", "#f0f9eb", "#67c23a", "#c2e7b0");
    QPushButton *deleteBtn = createBtn("删除会员", "#fef0f0", "#f56c6c", "#fbc4c4");

    actionLayout->addWidget(editBtn);
    actionLayout->addWidget(petBtn);
    actionLayout->addWidget(deleteBtn);
    actionLayout->addStretch();

    if (m_role == STAFF) {
        deleteBtn->setVisible(false);
    }

    // 【按钮逻辑】
    connect(editBtn, &QPushButton::clicked, this, [=](){
        // 查找当前行
        int currentRow = -1;
        for (int i = 0; i < memTable->rowCount(); ++i) {
            if (memTable->cellWidget(i, 8) == actionWidget) {
                currentRow = i;
                break;
            }
        }
        if (currentRow == -1) return;

        // 获取当前行数据
        MemberInfo info;
        info.id = memTable->item(currentRow, 0)->text();
        info.name = memTable->item(currentRow, 1)->text();
        info.phone = memTable->item(currentRow, 2)->text();
        info.level = memTable->item(currentRow, 3)->text();
        info.balance = memTable->item(currentRow, 4)->text().toDouble();
        info.consume_amt = memTable->item(currentRow, 5)->text().toDouble();
        info.points = memTable->item(currentRow, 6)->text().toInt();

        AddMemberDialog dialog(this);
        dialog.setInitialData(info); // 进入编辑模式
        if (dialog.exec() == QDialog::Accepted) {
            MemberInfo newInfo = dialog.getMemberInfo();
            // 更新 UI
            memTable->item(currentRow, 0)->setText(newInfo.id);
            memTable->item(currentRow, 1)->setText(newInfo.name);
            memTable->item(currentRow, 2)->setText(newInfo.phone);
            memTable->item(currentRow, 3)->setText(newInfo.level);
            memTable->item(currentRow, 4)->setText(QString::number(newInfo.balance, 'f', 2));
            memTable->item(currentRow, 5)->setText(QString::number(newInfo.consume_amt, 'f', 2));
            memTable->item(currentRow, 6)->setText(QString::number(newInfo.points));
            
            updateStatistics();
            CustomMessageDialog::showWarning(this, "修改成功", "会员信息已成功更新");
        }
    });

    connect(petBtn, &QPushButton::clicked, this, [=](){
        // 查找当前行
        int currentRow = -1;
        for (int i = 0; i < memTable->rowCount(); ++i) {
            if (memTable->cellWidget(i, 8) == actionWidget) {
                currentRow = i;
                break;
            }
        }
        if (currentRow == -1) return;

        // 根据用户要求，使用“原本的添加宠物窗口” (AddPetDialog)
        AddPetDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            PetInfo pet = dialog.getPetInfo();
            
            // 获取当前行的下拉框并更新
            // 注意这里现在外面包裹了一层 QWidget
            QWidget *wrapper = memTable->cellWidget(currentRow, 7);
            QComboBox *petCombo = nullptr;
            if (wrapper) petCombo = wrapper->findChild<QComboBox*>();

            if (petCombo) {
                if (!petCombo->isEnabled() || petCombo->itemText(0) == "无") {
                    petCombo->clear();
                    petCombo->setEnabled(true);
                    petCombo->setStyleSheet(
                        "QComboBox { "
                        "   border: 1px solid #dcdfe6; "
                        "   border-radius: 16px; "
                        "   padding: 2px 12px; "
                        "   background: #f5f7fa; "
                        "   color: #606266; "
                        "   font-size: 13px; "
                        "} "
                        "QComboBox:hover { border-color: #c0c4cc; } "
                        "QComboBox:focus { border-color: #409eff; background: white; } "
                        "QComboBox::drop-down { border: none; width: 24px; } "
                        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; height: 14px; } "
                        "QComboBox QAbstractItemView { "
                        "   border: 1px solid #ebeef5; "
                        "   border-radius: 10px; "
                        "   background-color: white; "
                        "   outline: none; "
                        "} "
                        "QComboBox QAbstractItemView::item { "
                        "   height: 35px; "
                        "   padding-left: 10px; "
                        "   color: #606266; "
                        "   background-color: white; "
                        "   border-radius: 4px; "
                        "   margin: 2px 5px; "
                        "} "
                        "QComboBox QAbstractItemView::item:hover, QComboBox QAbstractItemView::item:selected { "
                        "   background-color: #f0f7ff; "
                        "   color: #409eff; "
                        "} "
                    );
                }
                petCombo->addItem(pet.name);
                petCombo->setCurrentText(pet.name);
            }
            
            CustomMessageDialog::showWarning(this, "添加成功", 
                QString("已为会员 %1 成功添加新宠物档案：%2").arg(memTable->item(currentRow, 1)->text(), pet.name));
        }
    });

    connect(deleteBtn, &QPushButton::clicked, this, [=](){
        // 动态定位被点击按钮所在的行
        int currentRow = -1;
        for (int i = 0; i < memTable->rowCount(); ++i) {
            if (memTable->cellWidget(i, 8) == actionWidget) {
                currentRow = i;
                break;
            }
        }

        if (currentRow != -1) {
            QString memberName = memTable->item(currentRow, 1)->text();
            if(CustomMessageDialog::confirm(this, "业务确认", "确定移除会员 [" + memberName + "] 的档案吗？")) {
                memTable->removeRow(currentRow);
                updateStatistics();
            }
        }
    });

    memTable->setCellWidget(r, 8, actionWidget);

    if (stackLayout->currentWidget() == emptyStateWidget) {
        stackLayout->setCurrentWidget(memTable);
    }
}

void MemberModule::addSampleData()
{
    addRow("M001", "张三", "13800138000", "黄金会员", 500.00, 1250.00, 125, "无");
    addRow("M002", "李芳", "13912345678", "普通会员", 0.00, 100.00, 10, "无");
    addRow("M003", "王五", "13777777777", "铂金会员", 1200.00, 3500.00, 350, "无");
    addRow("M004", "赵六", "13666666666", "钻石会员", 2500.00, 8800.00, 880, "无");
    addRow("M005", "孙七", "18189294306", "普通会员", 50.00, 100.00, 10, "无");
}

void MemberModule::updateStatistics()
{
    int total = memTable->rowCount();
    int gold = 0, platinum = 0, diamond = 0;
    for(int i=0; i<total; ++i) {
        if(memTable->item(i, 3)) {
            QString level = memTable->item(i, 3)->text();
            if(level.contains("黄金")) gold++;
            else if(level.contains("铂金")) platinum++;
            else if(level.contains("钻石")) diamond++;
        }
    }
    totalMemberLabel->setText(QString("总会员: %1").arg(total));
    goldMemberLabel->setText(QString("黄金会员: %1").arg(gold));
    platinumMemberLabel->setText(QString("铂金会员: %1").arg(platinum));
    diamondMemberLabel->setText(QString("钻石会员: %1").arg(diamond));

    if (total == 0) {
        stackLayout->setCurrentWidget(emptyStateWidget);
    } else {
        stackLayout->setCurrentWidget(memTable);
    }
}

void MemberModule::onSearchTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        for (int i = 0; i < memTable->rowCount(); ++i) memTable->setRowHidden(i, false);
        stackLayout->setCurrentWidget(memTable);
        return;
    }

    int visibleCount = 0;
    for (int i = 0; i < memTable->rowCount(); ++i) {
        bool match = (memTable->item(i, 0)->text().contains(text, Qt::CaseInsensitive) ||
                      memTable->item(i, 1)->text().contains(text, Qt::CaseInsensitive) ||
                      memTable->item(i, 2)->text().contains(text));
        memTable->setRowHidden(i, !match);
        if (match) visibleCount++;
    }

    if (visibleCount > 0) {
        stackLayout->setCurrentWidget(memTable);
    } else {
        stackLayout->setCurrentWidget(emptyStateWidget);
    }
}

void MemberModule::showAddMemberDialog()
{
    AddMemberDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        MemberInfo info = dialog.getMemberInfo();
        addRow(info.id, info.name, info.phone, info.level, info.balance, info.consume_amt, info.points, "无");
        updateStatistics();
    }
}

void MemberModule::exportData()
{
    QString path = QFileDialog::getSaveFileName(this, "导出", "members.csv", "CSV (*.csv)");
    if(path.isEmpty()) return;
    QFile file(path);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&file);
        ts.setGenerateByteOrderMark(true);
        ts << "会员ID,姓名,手机,等级,储值余额,累计消费,积分,宠物\n";
        for(int r=0; r<memTable->rowCount(); ++r) {
            if(!memTable->isRowHidden(r)) {
                ts << QString("%1,%2,%3,%4,%5,%6,%7,%8\n").arg(
                          memTable->item(r,0)->text(), 
                          memTable->item(r,1)->text(), 
                          memTable->item(r,2)->text(), 
                          memTable->item(r,3)->text(), 
                          memTable->item(r,4)->text(), 
                          memTable->item(r,5)->text(),
                          memTable->item(r,6)->text(),
                          memTable->item(r,7)->text());
            }
        }
        file.close();
        CustomMessageDialog::showWarning(this, "成功", "数据已成功导出");
    }
}

void MemberModule::onCellClicked(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    // 已由 QComboBox 接管，此处保留空实现或未来用于其它点击逻辑
}
