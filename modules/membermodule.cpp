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


/**
 * 专门的操作按钮容器件，确保布局和尺寸稳定
 */
class ActionButtonsWidget : public QWidget {
public:
    ActionButtonsWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(15, 0, 15, 0);
        layout->setSpacing(10);
        
        auto createBtn = [&](const QString &text, const QString &color) {
            QPushButton *btn = new QPushButton(text);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFixedWidth(50);
            btn->setStyleSheet(QString(
                "QPushButton { border: none; color: %1; background: transparent; font-size: 13px; font-weight: bold; } "
                "QPushButton:hover { text-decoration: underline; }"
            ).arg(color));
            return btn;
        };

        rechargeBtn = createBtn("充值", "#409eff");
        detailBtn = createBtn("详情", "#67c23a");
        deleteBtn = createBtn("删除", "#f56c6c");

        layout->addWidget(rechargeBtn);
        layout->addWidget(detailBtn);
        layout->addWidget(deleteBtn);
        layout->addStretch();
    }

    QPushButton *rechargeBtn;
    QPushButton *detailBtn;
    QPushButton *deleteBtn;
};

MemberModule::MemberModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void MemberModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 标题区域
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *title = new QLabel("会员信息管理", this);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #303133;");
    
    QPushButton *exportBtn = new QPushButton("导出 CSV");
    exportBtn->setStyleSheet("QPushButton { padding: 6px 16px; border-radius: 4px; border: 1px solid #dcdfe6; background: white; color: #606266; } QPushButton:hover { border-color: #409eff; color: #409eff; }");
    connect(exportBtn, &QPushButton::clicked, this, &MemberModule::exportData);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(exportBtn);
    mainLayout->addLayout(headerLayout);

    // 搜索栏
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

    // 3. 表格区域 (含空状态堆叠)
    QWidget *tableContainer = new QWidget();
    stackLayout = new QStackedLayout(tableContainer);

    // 表格设计
    memTable = new QTableWidget();
    memTable->setColumnCount(6);
    memTable->setHorizontalHeaderLabels({"会员姓名", "手机号码", "会员等级", "累计积分", "宠物档案", "管理操作"});
    
    // 固定的列宽策略
    memTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    memTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    memTable->setColumnWidth(5, 230); // 确保操作列足够宽
    
    memTable->setShowGrid(false);
    memTable->setAlternatingRowColors(true);
    memTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memTable->verticalHeader()->setVisible(true);
    memTable->verticalHeader()->setFixedWidth(60); 
    memTable->verticalHeader()->setDefaultSectionSize(50); // 彻底解决高度太小导致按钮消失的问题
    
    // 强制全局表格样式，移除所有特殊着色
    memTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background-color: white; color: #606266; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background-color: #f5f7fa; padding: 12px; border: none; border-bottom: 1px solid #ebeef5; color: #909399; font-weight: bold; font-size: 13px; } "
        "QHeaderView::section:vertical { background: #ffffff; color: #c0c4cc; border-right: 1px solid #f0f2f5; text-align: center; font-size: 12px; } "
    );

    // 空状态提示
    emptyStateWidget = new QWidget();
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyStateWidget);
    emptyLayout->addStretch();
    QLabel *emptyIcon = new QLabel();
    emptyIcon->setPixmap(QPixmap(":/icons/empty_state.png").scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)); // 假设有一个空状态图标
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
    mainLayout->addWidget(tableContainer);

    // 统计栏
    QFrame *statFrame = new QFrame();
    statFrame->setStyleSheet("QFrame { background: #f8f9fbc; border-top: 1px solid #ebeef5; padding: 12px; }");
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

    // 禁用右键菜单，防止方案混淆
    memTable->setContextMenuPolicy(Qt::NoContextMenu);

    addSampleData();
    updateStatistics();
}

void MemberModule::addRow(const QString &name, const QString &phone, const QString &level, int pts, const QString &pets)
{
    int r = memTable->rowCount();
    memTable->insertRow(r);
    
    // 设置通用 Item (强制灰黑色)
    auto createItem = [&](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setForeground(QBrush(QColor("#606266")));
        item->setFont(QFont("Microsoft YaHei", 9));
        return item;
    };

    memTable->setItem(r, 0, createItem(name));
    memTable->setItem(r, 1, createItem(phone));
    memTable->setItem(r, 2, createItem(level));
    memTable->setItem(r, 3, createItem(QString::number(pts)));
    memTable->setItem(r, 4, createItem(pets));
    memTable->setItem(r, 5, new QTableWidgetItem());
    // 创建并设置操作组件
    ActionButtonsWidget *actions = new ActionButtonsWidget();
    
    // 绑定按钮事件 (示例：删除功能)
    connect(actions->deleteBtn, &QPushButton::clicked, this, [=](){
        if(CustomMessageDialog::confirm(this, "业务确认", "确定移除该会员档案吗？")) {
            for(int i=0; i<memTable->rowCount(); ++i) {
                if(memTable->item(i, 1) && memTable->item(i, 1)->text() == phone) {
                    memTable->removeRow(i);
                    break;
                }
            }
            updateStatistics();
            if (memTable->rowCount() == 0) {
                stackLayout->setCurrentWidget(emptyStateWidget);
            }
        }
    });

    memTable->setCellWidget(r, 5, actions);
    
    // 更新行高，双重保险
    memTable->setRowHeight(r, 50);

    // 如果之前是空状态，现在有数据了，切换回表格
    if (stackLayout->currentWidget() == emptyStateWidget) {
        stackLayout->setCurrentWidget(memTable);
    }
}

void MemberModule::addSampleData()
{
    addRow("张三", "13800138000", "黄金会员(9.0折)", 1250, "哈士奇-二哈");
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
    // 修正：关联宠物是第 4 列
    if (column == 4) { 
        QTableWidgetItem *item = memTable->item(row, column);
        if (!item) return; // 增加空指针保护
        
        // 准备模拟的宠物档案列表（实际应从 PetModule 获取）
        QStringList allPets = {"哈士奇-二哈", "布偶-咪咪", "金毛-多多", "拉布拉多-豆豆", "狸花猫-球球"};
        
        QString currentPetsStr = item->text();
        QStringList selectedPets = currentPetsStr == "无" ? QStringList() : currentPetsStr.split(", ");

        SelectPetDialog dialog(allPets, selectedPets, this);
        if (dialog.exec() == QDialog::Accepted) {
            QStringList newSelected = dialog.getSelectedPets();
            QString result = newSelected.isEmpty() ? "无" : newSelected.join(", ");
            
            item->setText(result);
            CustomMessageDialog::showWarning(this, "操作成功", 
                QString("已为会员 %1 成功关联宠物档案。").arg(memTable->item(row, 0)->text()));
        }
    }
}
