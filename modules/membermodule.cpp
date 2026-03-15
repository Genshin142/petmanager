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

MemberModule::MemberModule(QWidget *parent) : QWidget(parent)
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
    searchEdit->setPlaceholderText(" 搜索姓名或联系方式...");
    searchEdit->setFixedWidth(320);
    searchEdit->setStyleSheet("QLineEdit { border: 1px solid #dcdfe6; border-radius: 18px; padding: 8px 15px; font-size: 13px; background: white; } QLineEdit:focus { border-color: #409eff; }");

    QPushButton *addBtn = new QPushButton("+ 新增会员档案");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet("QPushButton { background: #67c23a; color: white; padding: 9px 20px; border-radius: 6px; font-weight: bold; } QPushButton:hover { background: #85ce61; }");
    connect(addBtn, &QPushButton::clicked, this, &MemberModule::showAddMemberDialog);

    searchLayout->addWidget(searchEdit);
    searchLayout->addStretch();
    searchLayout->addWidget(addBtn);
    mainLayout->addLayout(searchLayout);

    // 3. 表格与空状态区域 (使用 QStackedLayout)
    stackLayout = new QStackedLayout(); // 直接作为 layout，不再包裹多余的 QWidget

    memTable = new QTableWidget();
    memTable->setColumnCount(6);
    memTable->setHorizontalHeaderLabels({"会员姓名", "手机号码", "会员等级", "累计积分", "宠物档案", "管理操作"});

    // 固定的列宽策略
    memTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    memTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    memTable->setColumnWidth(5, 290); // 彻底扩容，确保三个按钮及其间距完美显示

    memTable->setShowGrid(false);
    memTable->setAlternatingRowColors(true);
    memTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memTable->verticalHeader()->setVisible(true);
    memTable->verticalHeader()->setFixedWidth(50);
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
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; padding-left: 5px; } "
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
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyStateWidget);
    emptyLayout->addStretch();
    QLabel *emptyIcon = new QLabel("📁"); // 换成Emoji防止你本地找不到图片文件导致空白
    emptyIcon->setStyleSheet("font-size: 50px; color: #c0c4cc;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    QLabel *emptyText = new QLabel("暂无会员数据，快去添加吧！");
    emptyText->setAlignment(Qt::AlignCenter);
    emptyText->setStyleSheet("color: #909399; font-size: 14px;");
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(emptyText);
    emptyLayout->addStretch();
    emptyStateWidget->setStyleSheet("background-color: white; border: 1px solid #ebeef5;");

    stackLayout->addWidget(memTable);
    stackLayout->addWidget(emptyStateWidget);
    mainLayout->addLayout(stackLayout);

    // 4. 统计栏
    QFrame *statFrame = new QFrame();
    statFrame->setStyleSheet("QFrame { background: #f8f9fb; border-top: 1px solid #ebeef5; padding: 12px; }");
    QHBoxLayout *statLayout = new QHBoxLayout(statFrame);
    totalMemberLabel = new QLabel("总会员: 0");
    goldMemberLabel = new QLabel("黄金会员: 0");
    totalMemberLabel->setStyleSheet("color: #606266; font-size: 13px; font-weight: bold; margin-right: 30px;");
    goldMemberLabel->setStyleSheet("color: #606266; font-size: 13px; font-weight: bold;");
    statLayout->addWidget(totalMemberLabel);
    statLayout->addWidget(goldMemberLabel);
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

void MemberModule::addRow(const QString &name, const QString &phone, const QString &level, int pts, const QString &pets)
{
    int r = memTable->rowCount();
    memTable->insertRow(r);

    // 【修复1：移除了 setForeground】让它自然继承 StyleSheet 中的黑色
    auto createItem = [&](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFont(QFont("Microsoft YaHei", 9));
        return item;
    };

    memTable->setItem(r, 0, createItem(name));
    memTable->setItem(r, 1, createItem(phone));
    memTable->setItem(r, 2, createItem(level));
    memTable->setItem(r, 3, createItem(QString::number(pts)));
    
    // 【美化：使用 Element UI 风格的下拉框】
    QComboBox *petCombo = new QComboBox();
    petCombo->setCursor(Qt::PointingHandCursor);
    petCombo->setFixedHeight(32);
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
    memTable->setCellWidget(r, 4, comboWrapper);
    memTable->setItem(r, 5, new QTableWidgetItem()); // 必须占位

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

    // 【按钮逻辑】
    connect(editBtn, &QPushButton::clicked, this, [=](){
        // 查找当前行
        int currentRow = -1;
        for (int i = 0; i < memTable->rowCount(); ++i) {
            if (memTable->cellWidget(i, 5) == actionWidget) {
                currentRow = i;
                break;
            }
        }
        if (currentRow == -1) return;

        // 获取当前行数据
        MemberInfo info;
        info.name = memTable->item(currentRow, 0)->text();
        info.phone = memTable->item(currentRow, 1)->text();
        info.level = memTable->item(currentRow, 2)->text();
        info.points = memTable->item(currentRow, 3)->text().toInt();

        AddMemberDialog dialog(this);
        dialog.setInitialData(info); // 进入编辑模式
        if (dialog.exec() == QDialog::Accepted) {
            MemberInfo newInfo = dialog.getMemberInfo();
            // 更新 UI
            memTable->item(currentRow, 0)->setText(newInfo.name);
            memTable->item(currentRow, 1)->setText(newInfo.phone);
            memTable->item(currentRow, 2)->setText(newInfo.level);
            memTable->item(currentRow, 3)->setText(QString::number(newInfo.points));
            
            updateStatistics();
            CustomMessageDialog::showWarning(this, "修改成功", "会员信息已成功更新");
        }
    });

    connect(petBtn, &QPushButton::clicked, this, [=](){
        // 查找当前行
        int currentRow = -1;
        for (int i = 0; i < memTable->rowCount(); ++i) {
            if (memTable->cellWidget(i, 5) == actionWidget) {
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
            QWidget *wrapper = memTable->cellWidget(currentRow, 4);
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
                QString("已为会员 %1 成功添加新宠物档案：%2").arg(memTable->item(currentRow, 0)->text(), pet.name));
        }
    });

    connect(deleteBtn, &QPushButton::clicked, this, [=](){
        // 动态定位被点击按钮所在的行
        int currentRow = -1;
        for (int i = 0; i < memTable->rowCount(); ++i) {
            if (memTable->cellWidget(i, 5) == actionWidget) {
                currentRow = i;
                break;
            }
        }

        if (currentRow != -1) {
            QString memberName = memTable->item(currentRow, 0)->text();
            if(CustomMessageDialog::confirm(this, "业务确认", "确定移除会员 [" + memberName + "] 的档案吗？")) {
                memTable->removeRow(currentRow);
                updateStatistics();
            }
        }
    });

    memTable->setCellWidget(r, 5, actionWidget);

    if (stackLayout->currentWidget() == emptyStateWidget) {
        stackLayout->setCurrentWidget(memTable);
    }
}

void MemberModule::addSampleData()
{
    addRow("张三", "13800138000", "黄金会员", 1250, "无");
    addRow("李芳", "13912345678", "普通会员", 100, "无");
    addRow("赵四", "18189294306", "普通会员", 100, "无");
}

void MemberModule::updateStatistics()
{
    int total = memTable->rowCount();
    int gold = 0;
    for(int i=0; i<total; ++i) {
        if(memTable->item(i, 2) && memTable->item(i, 2)->text().contains("黄金")) gold++;
    }
    totalMemberLabel->setText(QString("总会员: %1").arg(total));
    goldMemberLabel->setText(QString("黄金会员: %1").arg(gold));

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
                      memTable->item(i, 1)->text().contains(text));
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
        addRow(info.name, info.phone, info.level, info.points, "无");
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
        ts << "姓名,手机,等级,积分,宠物\n";
        for(int r=0; r<memTable->rowCount(); ++r) {
            if(!memTable->isRowHidden(r)) {
                ts << QString("%1,%2,%3,%4,%5\n").arg(memTable->item(r,0)->text(), memTable->item(r,1)->text(), memTable->item(r,2)->text(), memTable->item(r,3)->text(), memTable->item(r,4)->text());
            }
        }
        file.close();
        CustomMessageDialog::showWarning(this, "成功", "数据已成功导出");
    }
}

void MemberModule::onCellClicked(int row, int column)
{
    // 已由 QComboBox 接管，此处保留空实现或未来用于其它点击逻辑
}
