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
#include <QGraphicsDropShadowEffect>
#include <QIntValidator>

MemberModule::MemberModule(UserRole role, QWidget *parent) : QWidget(parent), m_role(role), m_currentPage(1), m_pageSize(30)
{
    setupUI();
}

void MemberModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 1. 顶部标题栏
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("会员信息管理", this);
    titleLabel->setStyleSheet("font-size: 22px; color: #303133; font-weight: bold;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // 2. 统计卡片行
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(20);

    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &outValueLabel, const QColor &color) {
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
        outValueLabel->setStyleSheet("font-size: 22px; color: #303133; border: none; background: transparent; font-weight: bold;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(outValueLabel);
        textLayout->addStretch();

        l->addLayout(textLayout);
        l->addStretch();

        return card;
    };

    statLayout->addWidget(createStatCard("👥", "总计会员", totalMemberLabel, QColor("#409eff")));
    statLayout->addWidget(createStatCard("🌟", "普通会员", regularMemberLabel, QColor("#909399")));
    statLayout->addWidget(createStatCard("👑", "黄金会员", goldMemberLabel, QColor("#e6a23c")));
    statLayout->addWidget(createStatCard("💎", "铂金会员", platinumMemberLabel, QColor("#409eff")));
    statLayout->addWidget(createStatCard("🏆", "钻石会员", diamondMemberLabel, QColor("#f56c6c")));
    
    mainLayout->addLayout(statLayout);

    // 3. 紧贴表格的操作栏 (左侧批量按钮，右侧搜索/筛选/新增)
    QHBoxLayout *operationLayout = new QHBoxLayout();
    operationLayout->setContentsMargins(0, 0, 0, 0);
    
    // -- 左侧：批量操作 --
    QPushButton *batchDeleteBtn = new QPushButton("批量删除");
    batchDeleteBtn->setCursor(Qt::PointingHandCursor);
    batchDeleteBtn->setFixedHeight(32);
    batchDeleteBtn->setStyleSheet(
        "QPushButton { background-color: #fef0f0; color: #f56c6c; border: 1px solid #fbc4c4; border-radius: 6px; font-size: 12px; padding: 0 15px; }"
        "QPushButton:hover { background-color: #f56c6c; color: white; }"
    );
    connect(batchDeleteBtn, &QPushButton::clicked, this, &MemberModule::onBatchDelete);

    operationLayout->addWidget(batchDeleteBtn);
    operationLayout->addStretch();

    // -- 右侧：搜索与筛选 --
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索姓名、手机号、ID...");
    searchEdit->setFixedWidth(200);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );

    levelFilterCombo = new QComboBox();
    levelFilterCombo->setFixedWidth(110);
    levelFilterCombo->setFixedHeight(32);
    levelFilterCombo->addItems({"全部等级", "普通会员", "黄金会员", "铂金会员", "钻石会员"});
    levelFilterCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 4px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; }"
    );
    connect(levelFilterCombo, &QComboBox::currentTextChanged, this, [=](){ this->onSearchTextChanged(searchEdit->text()); });

    QPushButton *addBtn = new QPushButton("+ 新增会员档案");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setFixedHeight(32);
    addBtn->setStyleSheet("QPushButton { background: #67c23a; color: white; padding: 0 15px; border-radius: 4px; font-size: 13px; } QPushButton:hover { background: #85ce61; }");
    connect(addBtn, &QPushButton::clicked, this, &MemberModule::showAddMemberDialog);

    operationLayout->addWidget(searchEdit);
    operationLayout->addSpacing(8);
    operationLayout->addWidget(levelFilterCombo);
    operationLayout->addSpacing(12);
    operationLayout->addWidget(addBtn);

    mainLayout->addLayout(operationLayout);

    // 3. 表格 (直接加入主布局，与其他模块一致，确保自动撑满)
    memTable = new QTableWidget();
    memTable->setColumnCount(12);
    memTable->setHorizontalHeaderLabels({"选择", "会员ID", "会员姓名", "性别", "手机号码", "会员等级", "储值余额", "累计消费金额", "可用积分", "最后到店", "宠物档案", "操作"});

    // 固定的列宽策略与居中对齐
    memTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    // 默认自动拉伸填满宽度，特定的列手动固定
    memTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); 
    
    // 给第一列选择框固定宽度
    memTable->setColumnWidth(0, 48); 
    memTable->setColumnWidth(3, 50); // 性别列紧凑
    
    // 宠物档案列：不需要太长
    memTable->horizontalHeader()->setSectionResizeMode(10, QHeaderView::Fixed);
    memTable->setColumnWidth(10, 150); 
    
    // 管理操作列：需要足够宽度容纳 4 个按钮
    memTable->horizontalHeader()->setSectionResizeMode(11, QHeaderView::Fixed);
    memTable->setColumnWidth(11, 350); 
    
    // 最后到店日期
    memTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Fixed);
    memTable->setColumnWidth(9, 120); 

    memTable->setShowGrid(false);
    memTable->setAlternatingRowColors(false);
    memTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memTable->setSelectionMode(QAbstractItemView::SingleSelection);
    memTable->setFocusPolicy(Qt::NoFocus);
    memTable->verticalHeader()->setVisible(false);
    memTable->verticalHeader()->setDefaultSectionSize(48); // 统一行高，确保按钮放得下

    // 【修复1：样式调整】强制字体为纯黑，并自定义选中后的背景色，保证选中时清晰可见
    memTable->setStyleSheet(
        "QTableWidget { "
        "   border: 1px solid #ebeef5; "
        "   background-color: white; "
        "   color: black; " /* 全局默认纯黑色文字 */
        "} "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QTableWidget::item:selected { background-color: #b3d8ff; } " 
        "QHeaderView::section { background-color: #f5f7fa; padding: 12px; border: none; border-bottom: 1px solid #ebeef5; color: #606266; font-size: 13px;  font-weight: bold; } "
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

    mainLayout->addWidget(memTable);

    // 5. 统计栏（固定高度，不抢夺表格空间）
    QFrame *statFrame = new QFrame();
    statFrame->setFixedHeight(50);
    statFrame->setStyleSheet("QFrame { background: #f8f9fb; border-top: 1px solid #ebeef5; padding: 0 12px; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(statFrame);

    QLabel *footerInfo = new QLabel("会员档案管理");
    footerInfo->setStyleSheet("color: #909399; font-size: 13px;");
    footerLayout->addWidget(footerInfo);
    footerLayout->addStretch();

    // 分页控件
    jumpEdit = new QLineEdit();

    jumpEdit->setFixedWidth(36); // 修改为只能显示3位的宽度
    jumpEdit->setMaxLength(3);   // 上限为3位数
    jumpEdit->setFixedHeight(24);
    jumpEdit->setAlignment(Qt::AlignCenter);
    jumpEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 4px; padding: 0; font-size: 13px; background: white; margin: 0; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    // 初始化验证器，随着页数改变会动态更新最大的可输入值
    jumpValidator = new QIntValidator(1, 1, this);
    jumpEdit->setValidator(jumpValidator);

    QLabel *jumpPrefix = new QLabel("跳转到第");
    jumpPrefix->setStyleSheet("color: #606266; font-size: 13px; margin: 0; padding: 0;");
    
    QLabel *jumpSuffix = new QLabel("页");
    jumpSuffix->setStyleSheet("color: #606266; font-size: 13px; margin: 0; padding: 0;");

    QPushButton *jumpBtn = new QPushButton("确认");
    jumpBtn->setCursor(Qt::PointingHandCursor);
    jumpBtn->setFixedSize(44, 24); // 适当缩小并保证字能完整展示
    jumpBtn->setStyleSheet(
        "QPushButton { border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; font-size: 12px; padding: 2px 0px; text-align: center; }"
        "QPushButton:hover { border-color: #409eff; color: #409eff; }"
    );

    prevBtn = new QPushButton("上一页");
    nextBtn = new QPushButton("下一页");
    pageLabel = new QLabel("第 1 页 / 共 1 页");

    QString pageStyle = "QPushButton { height: 24px; border: 1px solid #dcdfe6; border-radius: 4px; background: white; color: #606266; font-size: 12px; padding: 0 8px; text-align: center; } "
                        "QPushButton:hover { border-color: #409eff; color: #409eff; } "
                        "QPushButton:disabled { background: #f5f7fa; color: #c0c4cc; border-color: #e4e7ed; }";
    prevBtn->setStyleSheet(pageStyle);
    nextBtn->setStyleSheet(pageStyle);

    prevBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setCursor(Qt::PointingHandCursor);
    pageLabel->setStyleSheet("color: #909399; font-size: 13px; margin: 0; padding: 0 2px;");

    // 将跳转控制器单独包裹，以便设置更紧凑的间距，防止间隔过大
    QWidget *jumpGroup = new QWidget();
    jumpGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed); // 强制不拉伸拉宽
    QHBoxLayout *jumpLayout = new QHBoxLayout(jumpGroup);
    jumpLayout->setContentsMargins(0, 0, 0, 0);
    jumpLayout->setSpacing(2); // 间距收缩至极小
    jumpLayout->addWidget(jumpPrefix);
    jumpLayout->addWidget(jumpEdit);
    jumpLayout->addWidget(jumpSuffix);
    jumpLayout->addWidget(jumpBtn);

    // 将翻页控制器单独包裹，防止布局将其强行拉伸
    QWidget *pageGroup = new QWidget();
    pageGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *pageLayout = new QHBoxLayout(pageGroup);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(2); // 极小缝隙
    pageLayout->addWidget(prevBtn);
    pageLayout->addWidget(pageLabel);
    pageLayout->addWidget(nextBtn);

    footerLayout->addWidget(jumpGroup);
    footerLayout->addSpacing(8);
    footerLayout->addWidget(pageGroup);

    mainLayout->addWidget(statFrame);

    // 绑定事件
    connect(prevBtn, &QPushButton::clicked, this, &MemberModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &MemberModule::onNextPage);
    connect(jumpBtn, &QPushButton::clicked, this, &MemberModule::onJumpPage);
    connect(jumpEdit, &QLineEdit::returnPressed, this, &MemberModule::onJumpPage);

    // 绑定事件
    connect(searchEdit, &QLineEdit::textChanged, this, &MemberModule::onSearchTextChanged);
    connect(memTable, &QTableWidget::cellClicked, this, &MemberModule::onCellClicked);

    // 禁用右键菜单
    memTable->setContextMenuPolicy(Qt::NoContextMenu);

    addSampleData();
    updateStatistics();
}

void MemberModule::addRow(const QString &id, const QString &name, const QString &gender, const QString &birthday, const QString &phone, const QString &level, double balance, double consume_amt, int pts, const QString &lastVisit, const QString &pets)
{
    int r = memTable->rowCount();
    memTable->insertRow(r);

    auto createItem = [&](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFont(QFont("Microsoft YaHei", 9));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    // 复选框
    QWidget *chkWidget = new QWidget();
    QHBoxLayout *chkLayout = new QHBoxLayout(chkWidget);
    chkLayout->setContentsMargins(0, 0, 0, 0);
    QCheckBox *chkBox = new QCheckBox();
    chkLayout->addWidget(chkBox, 0, Qt::AlignCenter);
    memTable->setCellWidget(r, 0, chkWidget);

    memTable->setItem(r, 1, createItem(id));
    
    QTableWidgetItem *nameItem = createItem(name);
    nameItem->setData(Qt::UserRole, birthday); // 隐式存储生日
    memTable->setItem(r, 2, nameItem);
    
    memTable->setItem(r, 3, createItem(gender));

    memTable->setItem(r, 4, createItem(phone));
    memTable->setItem(r, 5, createItem(level));
    memTable->setItem(r, 6, createItem(QString::number(balance, 'f', 2)));
    memTable->setItem(r, 7, createItem(QString::number(consume_amt, 'f', 2)));
    memTable->setItem(r, 8, createItem(QString::number(pts)));
    memTable->setItem(r, 9, createItem(lastVisit));
    
    // 【美化：使用 Element UI 风格的下拉框】
    QStringList petNames;
    if (pets != "无" && !pets.isEmpty()) {
        petNames = pets.split(", ", Qt::SkipEmptyParts);
    }

    QWidget *petRenderWidget = nullptr;

    if (petNames.isEmpty()) {
        QLabel *noPetLabel = new QLabel("暂无宠物");
        noPetLabel->setStyleSheet("color: #909399; font-size: 13px;");
        noPetLabel->setAlignment(Qt::AlignCenter);
        petRenderWidget = noPetLabel;
    } else {
        QComboBox *petCombo = new QComboBox();
        petCombo->setCursor(Qt::PointingHandCursor);
        petCombo->setFixedHeight(32);
        petCombo->setStyleSheet(
            "QComboBox { "
            "   border: 1px solid #dcdfe6; "
            "   border-radius: 4px; "
            "   padding: 2px 12px; "
            "   background: #f5f7fa; "
            "   color: #606266; "
            "   font-size: 13px; "
            "} "
            "QComboBox:hover { border-color: #c0c4cc; } "
            "QComboBox:focus { border-color: #409eff; background: white; } "
            "QComboBox::drop-down { border: none; width: 24px; } "
            "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; height: 14px; } "
            "QComboBox QAbstractItemView { border: 1px solid #ebeef5; border-radius: 4px; background-color: white; outline: none; padding: 4px 0px; } "
            "QComboBox QAbstractItemView::item { height: 35px; padding-left: 12px; color: #606266; background-color: white; } "
            "QComboBox QAbstractItemView::item:selected { background-color: #f0f7ff; color: #409eff; } "
            "QComboBox QScrollBar:vertical { width: 8px; background: transparent; } "
            "QComboBox QScrollBar::handle:vertical { background: #dcdfe6; border-radius: 4px; min-height: 20px; } "
            "QComboBox QScrollBar::handle:vertical:hover { background: #c0c4cc; } "
            "QComboBox QScrollBar::add-line:vertical, QComboBox QScrollBar::sub-line:vertical { height: 0px; } "
        );

        petCombo->addItems(petNames);

        connect(petCombo, QOverload<int>::of(&QComboBox::activated), this, [=](int index){
            QString petLabel = petCombo->itemText(index);

            // 提取原本宠物姓名，移除括号内种类名称进行跳页查询
            QString petName = petLabel.split("（").first();

            if (CustomMessageDialog::confirm(this, "页面跳转确认", 
                QString("检测到您选择了宠物档案：[%1]\n是否立即跳转到“宠物健康档案中心”查看详情？").arg(petLabel))) {
                emit sig_requestPetJump(name, petName);
            }
        });

        // 为 ComboBox 增加一个容器来控制垂直偏移 (向下移动一点点)
        QWidget *comboWrapper = new QWidget();
        QVBoxLayout *comboLayout = new QVBoxLayout(comboWrapper);
        comboLayout->setContentsMargins(5, 4, 5, 0); // 上边距 4px
        comboLayout->addWidget(petCombo);
        
        petRenderWidget = comboWrapper;
    }

    memTable->setCellWidget(r, 10, petRenderWidget);
    memTable->setItem(r, 10, new QTableWidgetItem()); // 必须占位

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

    // 生成显眼的按钮
    QPushButton *rechargeBtn = createBtn("充值", "#f0f9eb", "#67c23a", "#c2e7b0"); // 绿色
    QPushButton *editBtn = createBtn("修改", "#ecf5ff", "#409eff", "#b3d8ff"); // 蓝色
    QPushButton *petBtn = createBtn("添加宠物", "#fdf6ec", "#e6a23c", "#f5dab1"); // 橙色
    QPushButton *deleteBtn = createBtn("删除", "#fef0f0", "#f56c6c", "#fbc4c4"); // 红色

    // 这里特别限制操作按钮的最大宽度为 70左右
    rechargeBtn->setFixedWidth(66);
    editBtn->setFixedWidth(70);
    petBtn->setFixedWidth(70);
    deleteBtn->setFixedWidth(70);

    actionLayout->addWidget(rechargeBtn);
    actionLayout->addWidget(editBtn);
    actionLayout->addWidget(petBtn);
    actionLayout->addWidget(deleteBtn);
    actionLayout->addStretch();

    if (m_role == STAFF) {
        deleteBtn->setVisible(false);
    }

    // 【充值按钮逻辑】
    connect(rechargeBtn, &QPushButton::clicked, this, [=](){
        QMessageBox::information(this, "充值", QString("即将为会员 [%1] 充值... (待接续接口)").arg(name));
    });

    // 【按钮逻辑】
    connect(editBtn, &QPushButton::clicked, this, [=](){
        AddMemberDialog dialog(this);
        MemberInfo info;
        info.id = memTable->item(r, 1)->text();
        info.name = memTable->item(r, 2)->text();
        info.gender = memTable->item(r, 3)->text();
        info.birthday = memTable->item(r, 2)->data(Qt::UserRole).toString(); // 从隐式数据读取
        info.phone = phone; // 原始手机号（避免读取脱敏后的）
        info.level = memTable->item(r, 5)->text();
        info.balance = memTable->item(r, 6)->text().toDouble();
        info.consume_amt = memTable->item(r, 7)->text().toDouble();
        info.points = memTable->item(r, 8)->text().toInt();
        
        dialog.setInitialData(info);
        if (dialog.exec() == QDialog::Accepted) {
            MemberInfo newInfo = dialog.getMemberInfo();
            // 更新 UI
            memTable->item(r, 2)->setText(newInfo.name);
            memTable->item(r, 2)->setData(Qt::UserRole, newInfo.birthday);
            memTable->item(r, 3)->setText(newInfo.gender);
            memTable->item(r, 4)->setText(newInfo.phone);
            memTable->item(r, 5)->setText(newInfo.level);
            updateStatistics();
        }
    });

    connect(petBtn, &QPushButton::clicked, this, [=](){
        // 查找当前行
        int currentRow = -1;
        for (int i = 0; i < memTable->rowCount(); ++i) {
            if (memTable->cellWidget(i, 11) == actionWidget) {
                currentRow = i;
                break;
            }
        }
        if (currentRow == -1) return;

        // 获取当前会员信息用于预填
        QString memberId = memTable->item(currentRow, 1)->text();
        QString memberName = memTable->item(currentRow, 2)->text();

        AddPetDialog dialog(this);
        
        // 预设主人信息
        PetInfo initialInfo;
        initialInfo.ownerId = memberId;
        initialInfo.ownerName = memberName;
        dialog.setPetInfo(initialInfo);

        if (dialog.exec() == QDialog::Accepted) {
            PetInfo pet = dialog.getPetInfo();
            
            // 获取当前行的下拉框并更新
            // 注意这里现在外面包裹了一层 QWidget
            QWidget *wrapper = memTable->cellWidget(currentRow, 10);
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
                QString newPetLabel = QString("%1（%2）").arg(pet.name, pet.breed);
                petCombo->addItem(newPetLabel);
                petCombo->setCurrentText(newPetLabel);
            }
            
            emit sig_petAdded(pet);
            
            CustomMessageDialog::showWarning(this, "添加成功", 
                QString("已为会员 %1 成功添加新宠物档案：%2").arg(memTable->item(currentRow, 2)->text(), pet.name));
        }
    });

    connect(deleteBtn, &QPushButton::clicked, this, [=](){
        // 动态定位被点击按钮所在的行
        int currentRow = -1;
        for (int i = 0; i < memTable->rowCount(); ++i) {
            if (memTable->cellWidget(i, 11) == actionWidget) {
                currentRow = i;
                break;
            }
        }

        if (currentRow >= 0) {
            QString memberName = memTable->item(currentRow, 2)->text();
            if(CustomMessageDialog::confirm(this, "业务确认", "确定移除会员 [" + memberName + "] 的档案吗？")) {
                memTable->removeRow(currentRow);
                updateStatistics();
            }
        }
    });

    memTable->setCellWidget(r, 11, actionWidget);
    
    updatePagination();
}

void MemberModule::addSampleData()
{
    addRow("M001", "张三", "先生", "1990-05-20", "13800138000", "黄金会员", 500.00, 1250.00, 125, "2026-03-10", "团团（波斯猫）");
    addRow("M002", "李芳", "女士", "1995-10-12", "13912345678", "普通会员", 0.00, 100.00, 10, "2026-02-22", "豆豆（柴犬）, 咪咪（银渐层）");
    addRow("M003", "王五", "先生", "1988-03-05", "13777777777", "铂金会员", 1200.00, 3500.00, 350, "2025-12-05", "旺财（金毛犬）");
    addRow("M004", "赵六", "先生", "1992-07-15", "13666666666", "钻石会员", 2500.00, 8800.00, 880, "2026-01-15", "小雪（萨摩耶）, 可可（泰迪）");
    addRow("M005", "孙七", "女士", "1993-11-20", "18189294306", "普通会员", 50.00, 100.00, 10, "2026-03-25", "大黑（拉布拉多）");
    addRow("M006", "周八", "先生", "1991-01-30", "13511112222", "黄金会员", 300.00, 600.00, 60, "2025-08-10", "皮皮（柯基）");
    addRow("M007", "吴九", "女士", "1994-06-18", "13433334444", "普通会员", 20.00, 50.00, 5, "2026-04-01", "球球（英短）, 花花（加菲猫）");
    addRow("M008", "郑十", "先生", "1989-12-25", "13355556666", "铂金会员", 800.00, 2000.00, 200, "2025-11-11", "小白（比熊）");
    addRow("M009", "钱十一", "先生", "1992-03-14", "13277778888", "钻石会员", 1500.00, 5000.00, 500, "2026-04-05", "黑豹（孟加拉豹猫）");
    addRow("M010", "陈十二", "女士", "1996-08-08", "13199990000", "普通会员", 10.00, 20.00, 2, "2026-01-20", "多多（阿拉斯加）");
    addRow("M011", "林十三", "先生", "1990-09-09", "13012123434", "黄金会员", 450.00, 1100.00, 110, "2026-03-15", "发财（柴犬）, 欢欢（巴哥）");
}

void MemberModule::updateStatistics()
{
    int total = memTable->rowCount();
    int regular = 0, gold = 0, platinum = 0, diamond = 0;
    
    for(int i=0; i<total; ++i) {
        if(memTable->item(i, 5)) {
            QString level = memTable->item(i, 5)->text();
            if(level.contains("普通")) regular++;
            else if(level.contains("黄金")) gold++;
            else if(level.contains("铂金")) platinum++;
            else if(level.contains("钻石")) diamond++;
        }
    }
    
    if (totalMemberLabel) totalMemberLabel->setText(QString::number(total));
    if (regularMemberLabel) regularMemberLabel->setText(QString::number(regular));
    if (goldMemberLabel) goldMemberLabel->setText(QString::number(gold));
    if (platinumMemberLabel) platinumMemberLabel->setText(QString::number(platinum));
    if (diamondMemberLabel) diamondMemberLabel->setText(QString::number(diamond));
    
    updatePagination();
}

void MemberModule::onSearchTextChanged(const QString &text)
{
    m_currentPage = 1; // 搜索时重置页码
    updatePagination();
    
    // 检查是否全空，决定显示表格还是空状态
    QString selectedLevel = levelFilterCombo->currentText();
    
    int visibleCount = 0;
    for (int i = 0; i < memTable->rowCount(); ++i) {
        bool textMatch = (memTable->item(i, 1)->text().contains(text, Qt::CaseInsensitive) ||
                          memTable->item(i, 2)->text().contains(text, Qt::CaseInsensitive) ||
                          memTable->item(i, 4)->text().contains(text));
                          
        bool levelMatch = true;
        if (selectedLevel != "全部等级" && memTable->item(i, 5)) {
            levelMatch = (memTable->item(i, 5)->text() == selectedLevel);
        }

        if (textMatch && levelMatch) {
            memTable->setRowHidden(i, false);
            visibleCount++;
        } else {
            memTable->setRowHidden(i, true);
        }
    }

    // 表格始终可见，搜索结果为空时表格自然显示空行
}

void MemberModule::onPrevPage()
{
    if (m_currentPage > 1) {
        m_currentPage--;
        updatePagination();
    }
}

void MemberModule::onNextPage()
{
    m_currentPage++;
    updatePagination();
}

void MemberModule::onJumpPage()
{
    int page = jumpEdit->text().toInt();
    if (page < 1) return;
    
    m_currentPage = page;
    updatePagination();
    jumpEdit->clear();
    jumpEdit->clearFocus();
}

void MemberModule::updatePagination()
{
    QString searchText = searchEdit->text();
    QList<int> visibleRows;

    // 1. 筛选符合搜索条件的行
    for (int i = 0; i < memTable->rowCount(); ++i) {
        bool match = searchText.isEmpty() || 
                    (memTable->item(i, 0)->text().contains(searchText, Qt::CaseInsensitive) ||
                     memTable->item(i, 1)->text().contains(searchText, Qt::CaseInsensitive) ||
                     memTable->item(i, 2)->text().contains(searchText));
        
        if (match) {
            visibleRows.append(i);
        }
        memTable->setRowHidden(i, true); // 先统统隐藏
    }

    // 2. 计算分页
    int totalVisible = visibleRows.size();
    int totalPages = qMax(1, (totalVisible + m_pageSize - 1) / m_pageSize);

    // 动态更新页面跳跃输入框的数字输入上限
    if (jumpValidator) jumpValidator->setTop(totalPages);

    if (m_currentPage > totalPages) m_currentPage = totalPages;
    if (m_currentPage < 1) m_currentPage = 1;

    // 3. 显示当前页的行
    int start = (m_currentPage - 1) * m_pageSize;
    int end = qMin(start + m_pageSize, totalVisible);

    for (int i = start; i < end; ++i) {
        memTable->setRowHidden(visibleRows[i], false);
    }

    // 4. 更新控件状态
    pageLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_currentPage).arg(totalPages));
    prevBtn->setEnabled(m_currentPage > 1);
    nextBtn->setEnabled(m_currentPage < totalPages);
    
    // 如果没有匹配结果，按钮也屏蔽
    if (totalVisible == 0) {
        prevBtn->setEnabled(false);
        nextBtn->setEnabled(false);
        pageLabel->setText("第 0 页 / 共 0 页");
    }
}

void MemberModule::showAddMemberDialog()
{
    AddMemberDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        MemberInfo info = dialog.getMemberInfo();
        
        // 如果 ID 为空（新增模式），自动生成递增 ID
        if (info.id.isEmpty()) {
            info.id = generateNextMemberId();
        }

        // 加入默认最后到店日期
        addRow(info.id, info.name, info.gender, info.birthday, info.phone, info.level, info.balance, info.consume_amt, info.points, QDate::currentDate().toString("yyyy-MM-dd"), "无");
        updateStatistics();
    }
}

void MemberModule::exportData()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("导出选项");
    msgBox.setText("请选择要导出的数据范围：");
    QPushButton *btnSearch = msgBox.addButton("导出检索结果", QMessageBox::ActionRole);
    QPushButton *btnAll = msgBox.addButton("导出全部数据", QMessageBox::ActionRole);
    QPushButton *btnCancel = msgBox.addButton("取消", QMessageBox::RejectRole);
    (void)btnSearch; // 消除 unused variable 警告
    msgBox.exec();

    if (msgBox.clickedButton() == btnCancel) return;
    bool exportAll = (msgBox.clickedButton() == btnAll);

    QString path = QFileDialog::getSaveFileName(this, "导出", "members.csv", "CSV (*.csv)");
    if(path.isEmpty()) return;
    QFile file(path);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&file);
        ts.setGenerateByteOrderMark(true);
        ts << "会员ID,姓名,性别,手机号,会员等级,储值余额,累计消费金额,可用积分,最后到店,宠物档案\n";
        
        QString searchText = searchEdit->text();
        QString selectedLevel = levelFilterCombo->currentText();

        for(int r=0; r<memTable->rowCount(); ++r) {
            bool shouldExport = true;
            if (!exportAll) {
                bool textMatch = (memTable->item(r, 1)->text().contains(searchText, Qt::CaseInsensitive) ||
                                  memTable->item(r, 2)->text().contains(searchText, Qt::CaseInsensitive) ||
                                  memTable->item(r, 4)->text().contains(searchText));
                bool levelMatch = true;
                if (selectedLevel != "全部等级" && memTable->item(r, 5)) levelMatch = (memTable->item(r, 5)->text() == selectedLevel);
                shouldExport = textMatch && levelMatch;
            }

            if (shouldExport) {
                QString petsStr = "无";
                QWidget *wrapper = memTable->cellWidget(r, 10);
                if (wrapper) {
                    QComboBox *c = wrapper->findChild<QComboBox*>();
                    if (c) {
                        QStringList ps;
                        for(int j=0; j<c->count(); j++) ps << c->itemText(j);
                        petsStr = ps.join("; ");
                    }
                }
                ts << QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10\n").arg(
                          memTable->item(r,1)->text(), 
                          memTable->item(r,2)->text(), 
                          memTable->item(r,3)->text(), 
                          memTable->item(r,4)->text(), 
                          memTable->item(r,5)->text(), 
                          memTable->item(r,6)->text(),
                          memTable->item(r,7)->text(),
                          memTable->item(r,8)->text(),
                          memTable->item(r,9)->text(),
                          petsStr);
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

QString MemberModule::generateNextMemberId()
{
    int maxId = 0;
    // 遍历表格所有行（包括跨页的数据，因为数据都在 memTable 里，只是部分行 hidden）
    for (int i = 0; i < memTable->rowCount(); ++i) {
        QTableWidgetItem *item = memTable->item(i, 1); // ID 列索引为 1
        if (item) {
            QString idStr = item->text();
            if (idStr.startsWith("M") && idStr.length() > 1) {
                int currentId = idStr.mid(1).toInt();
                if (currentId > maxId) {
                    maxId = currentId;
                }
            }
        }
    }
    
    // 生成 M001, M002 ... 格式的 ID
    return QString("M%1").arg(maxId + 1, 3, 10, QChar('0'));
}

void MemberModule::onBatchDelete()
{
    QList<int> rowsToDelete;
    for (int i = 0; i < memTable->rowCount(); ++i) {
        QWidget *widget = memTable->cellWidget(i, 0);
        if (widget) {
            QCheckBox *cb = widget->findChild<QCheckBox*>();
            if (cb && cb->isChecked()) {
                rowsToDelete << i;
            }
        }
    }

    if (rowsToDelete.isEmpty()) {
        CustomMessageDialog::showWarning(this, "提示", "请先勾选需要删除的会员");
        return;
    }

    if (CustomMessageDialog::confirm(this, "确认删除", QString("确定要删除选中的 %1 名会员吗？").arg(rowsToDelete.size()))) {
        // 从后往前删，避免索引偏移
        std::sort(rowsToDelete.begin(), rowsToDelete.end(), std::greater<int>());
        for (int row : rowsToDelete) {
            memTable->removeRow(row);
        }
        updateStatistics();
        updatePagination();
        CustomMessageDialog::showWarning(this, "成功", "批量删除成功");
    }
}
