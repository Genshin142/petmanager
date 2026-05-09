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
#include <QTimer>
#include <QColor>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QIntValidator>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include "rechargedialog.h"
#include "custommessagedialog.h"
#include "memberdatamanager.h"

// --- 复刻：全行圆角边框选中委托 ---
class MemberRowDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillRect(opt.rect, Qt::white);

        if (opt.state & QStyle::State_Selected) {
            bool isFirst = (index.column() == 0);
            bool isLast = (index.column() == index.model()->columnCount() - 1);
            QRect rect = opt.rect.adjusted(1, 4, -1, -4);
            int radius = 8;
            QColor borderColor("#3b82f6");
            QColor bgColor("#eff6ff");

            painter->fillRect(opt.rect, bgColor);
            painter->setPen(QPen(borderColor, 2));
            
            if (isFirst) {
                QPainterPath path;
                path.moveTo(opt.rect.right() + 1, rect.top()); 
                path.lineTo(rect.left() + radius, rect.top());
                path.arcTo(QRect(rect.left(), rect.top(), radius*2, radius*2), 90, 90);
                path.lineTo(rect.left(), rect.bottom() - radius);
                path.arcTo(QRect(rect.left(), rect.bottom() - radius*2, radius*2, radius*2), 180, 90);
                path.lineTo(opt.rect.right() + 1, rect.bottom());
                painter->drawPath(path);
            } else if (isLast) {
                QPainterPath path;
                path.moveTo(opt.rect.left() - 1, rect.top());
                path.lineTo(rect.right() - radius, rect.top());
                path.arcTo(QRect(rect.right() - radius*2, rect.top(), radius*2, radius*2), 90, -90);
                path.lineTo(rect.right(), rect.bottom() - radius);
                path.arcTo(QRect(rect.right() - radius*2, rect.bottom() - radius*2, radius*2, radius*2), 0, -90);
                path.lineTo(opt.rect.left() - 1, rect.bottom());
                painter->drawPath(path);
            } else {
                painter->drawLine(QPoint(opt.rect.left() - 1, rect.top()), QPoint(opt.rect.right() + 1, rect.top()));
                painter->drawLine(QPoint(opt.rect.left() - 1, rect.bottom()), QPoint(opt.rect.right() + 1, rect.bottom()));
            }
        } else {
            painter->setPen(QPen(QColor("#f1f5f9"), 1));
            painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());
        }

        // 绘制文本
        painter->setPen(QColor((opt.state & QStyle::State_Selected) ? "#1e40af" : "#303133"));
        QFont font = painter->font();
        font.setWeight(opt.state & QStyle::State_Selected ? QFont::Bold : QFont::Normal);
        font.setPointSize(10);
        painter->setFont(font);
        QRect textRect = opt.rect.adjusted(10, 0, -10, 0);
        painter->drawText(textRect, opt.displayAlignment | Qt::AlignVCenter, opt.text);
        
        painter->restore();
    }
};

MemberModule::MemberModule(UserRole role, QWidget *parent) : QWidget(parent), m_role(role), m_currentPage(1), m_pageSize(30)
{
    setupUI();
}

void MemberModule::setupUI()
{
    // --- 全局水平布局 (左侧内容 + 右侧全高抽屉) ---
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 左侧容器
    QWidget *leftContainer = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(leftContainer);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 1. 顶部统计与标题容器 (复刻员工模块风格)
    QFrame *topContainer = new QFrame();
    topContainer->setObjectName("TopStatisticsContainer");
    topContainer->setFixedHeight(160); // 限制高度，防止过度拉伸
    topContainer->setStyleSheet("#TopStatisticsContainer { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QVBoxLayout *topLayout = new QVBoxLayout(topContainer);
    topLayout->setContentsMargins(25, 15, 25, 15);
    topLayout->setSpacing(12);

    // 1.1 顶部标题
    QLabel *titleLabel = new QLabel("会员信息管理", this);
    titleLabel->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold; border: none; background: transparent;");
    topLayout->addWidget(titleLabel);

    // 1.2 统计卡片行
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(15);
    topLayout->addLayout(statLayout);

    auto createStatCard = [&](const QString &icon, const QString &label, QLabel* &outValueLabel, const QColor &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(80);
        card->setStyleSheet("QFrame { background: #f8fafc; border-radius: 8px; border: 1px solid #f1f5f9; } ");
        
        QHBoxLayout *l = new QHBoxLayout(card);
        l->setContentsMargins(20, 10, 20, 10);

        QLabel *iconLabel = new QLabel(icon);
        if (icon.isEmpty()) {
            iconLabel->hide();
        } else {
            iconLabel->setFixedSize(40, 40);
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setStyleSheet(QString("font-size: 20px; color: %1; background: white; border-radius: 8px; border: 1px solid #f1f5f9;").arg(color.name()));
        }
        if (!icon.isEmpty()) {
            l->addWidget(iconLabel);
            l->addSpacing(12);
        }

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);
        
        QLabel *labelTitle = new QLabel(label);
        labelTitle->setStyleSheet("color: #94a3b8; font-size: 12px; border: none; background: transparent;");
        
        outValueLabel = new QLabel("--");
        outValueLabel->setStyleSheet("font-size: 20px; color: #1e293b; border: none; background: transparent; font-weight: bold;");
        
        textLayout->addWidget(labelTitle);
        textLayout->addWidget(outValueLabel);
        textLayout->addStretch();

        l->addLayout(textLayout);
        l->addStretch();

        return card;
    };

    statLayout->addWidget(createStatCard("", "总计会员", totalMemberLabel, QColor("#3b82f6")));
    statLayout->addWidget(createStatCard("", "普通会员", regularMemberLabel, QColor("#94a3b8")));
    statLayout->addWidget(createStatCard("", "黄金会员", goldMemberLabel, QColor("#e6a23c")));
    statLayout->addWidget(createStatCard("", "铂金会员", platinumMemberLabel, QColor("#3b82f6")));
    statLayout->addWidget(createStatCard("", "钻石会员", diamondMemberLabel, QColor("#f56c6c")));
    
    mainLayout->addWidget(topContainer);

    // 3. 紧贴表格的操作栏 (增加卡片容器包裹)
    QFrame *operationCard = new QFrame();
    operationCard->setObjectName("OperationCard");
    operationCard->setStyleSheet("#OperationCard { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    
    QHBoxLayout *operationLayout = new QHBoxLayout(operationCard);
    operationLayout->setContentsMargins(25, 12, 25, 12);
    operationLayout->setSpacing(0);
    
    // -- 搜索与筛选 (移动到左侧) --
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索姓名、手机号、ID...");
    searchEdit->setFixedWidth(200);
    searchEdit->setFixedHeight(36);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );

    levelFilterCombo = new QComboBox();
    levelFilterCombo->setFixedWidth(110);
    levelFilterCombo->setFixedHeight(36);
    levelFilterCombo->addItems({"全部等级", "普通会员", "黄金会员", "铂金会员", "钻石会员"});
    levelFilterCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox:hover { border-color: #409eff; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }"
    );
    connect(levelFilterCombo, &QComboBox::currentTextChanged, this, [=](){ this->onSearchTextChanged(searchEdit->text()); });

    operationLayout->addWidget(searchEdit);
    operationLayout->addSpacing(8);
    operationLayout->addWidget(levelFilterCombo);
    
    // 中间弹簧
    operationLayout->addStretch();

    // -- 右侧：新增 --
    QPushButton *addBtn = new QPushButton("录入会员");
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setFixedHeight(36);
    addBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #3b82f6; color: #3b82f6; padding: 0 15px; border-radius: 6px; font-size: 13px; font-weight: bold; } "
        "QPushButton:hover { background: #eff6ff; }"
    );
    connect(addBtn, &QPushButton::clicked, this, &MemberModule::showAddMemberDialog);
    operationLayout->addWidget(addBtn);

    mainLayout->addWidget(operationCard);

    // 4. 表格卡片容器 (12px 圆角)
    QFrame *tableCard = new QFrame();
    tableCard->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #ebeef5; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 5, 0, 0);

    memTable = new QTableWidget();
    memTable->setColumnCount(12);
    memTable->setHorizontalHeaderLabels({"会员ID", "会员姓名", "性别", "手机号码", "会员等级", "状态", "储值余额", "累计消费金额", "可用积分", "最后到店", "宠物档案", "操作"});
    memTable->setItemDelegate(new MemberRowDelegate(memTable));

    // 固定的列宽策略与居中对齐
    memTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    memTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); 
     
    memTable->setColumnWidth(2, 50);
    memTable->horizontalHeader()->setSectionResizeMode(10, QHeaderView::Fixed);
    memTable->setColumnWidth(10, 150); 
    memTable->horizontalHeader()->setSectionResizeMode(11, QHeaderView::Fixed);
    memTable->setColumnWidth(11, 240); 
    memTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Fixed);
    memTable->setColumnWidth(9, 120); 

    memTable->setShowGrid(false);
    memTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memTable->setSelectionMode(QAbstractItemView::SingleSelection);
    memTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memTable->setFocusPolicy(Qt::NoFocus);
    memTable->verticalHeader()->setVisible(false);
    memTable->verticalHeader()->setDefaultSectionSize(60);

    memTable->setStyleSheet(
        "QTableWidget { border: none; background: white; outline: none; border-radius: 6px; } "
        "QHeaderView { border: none; background: transparent; border-radius: 12px 12px 0 0; }"
    );

    // 隐藏已在详情页显示的列
    memTable->setColumnHidden(6, true); // 余额
    memTable->setColumnHidden(7, true); // 消费
    memTable->setColumnHidden(8, true); // 积分
    memTable->setColumnHidden(9, true); // 最后到店
    memTable->setColumnHidden(10, true); // 宠物档案

    tableLayout->addWidget(memTable);
    mainLayout->addWidget(tableCard);

    // 5. 底部统计栏（固定高度，并入表格卡片）
    QFrame *statFrame = new QFrame();
    statFrame->setFixedHeight(50);
    statFrame->setStyleSheet("QFrame { background: white; border: none; padding: 0 12px; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(statFrame);
    tableLayout->addWidget(statFrame);

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

    QString pageStyle = "QPushButton { height: 28px; border: 1px solid #e2e8f0; border-radius: 6px; background: white; color: #64748b; font-size: 12px; padding: 0 12px; text-align: center; font-weight: bold; } "
                        "QPushButton:hover { border-color: #3b82f6; color: #3b82f6; background: #eff6ff; } "
                        "QPushButton:disabled { background: white; color: #cbd5e1; border-color: #f1f5f9; }";
    prevBtn->setStyleSheet(pageStyle);
    nextBtn->setStyleSheet(pageStyle);

    prevBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setCursor(Qt::PointingHandCursor);
    pageLabel->setStyleSheet("color: #64748b; font-size: 13px; font-weight: bold; margin: 0 10px;");

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

    footerLayout->addStretch();
    footerLayout->addWidget(prevBtn);
    footerLayout->addSpacing(20);
    footerLayout->addWidget(pageLabel);
    footerLayout->addSpacing(20);
    footerLayout->addWidget(nextBtn);

    // --- 组装根布局 (左侧主体容器 + 右侧全高详情抽屉) ---
    rootLayout->addWidget(leftContainer, 1);
    
    m_detailDrawer = new MemberDetailDrawer(this);
    rootLayout->addWidget(m_detailDrawer);

    // 绑定事件
    connect(prevBtn, &QPushButton::clicked, this, &MemberModule::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &MemberModule::onNextPage);
    connect(jumpBtn, &QPushButton::clicked, this, &MemberModule::onJumpPage);
    connect(jumpEdit, &QLineEdit::returnPressed, this, &MemberModule::onJumpPage);
    connect(memTable, &QTableWidget::cellClicked, this, &MemberModule::onCellClicked);

    // 处理详情页的新增宠物请求
    connect(m_detailDrawer, &MemberDetailDrawer::sig_jumpToPetRequested, this, &MemberModule::sig_jumpToPetModule);
    connect(m_detailDrawer, &MemberDetailDrawer::sig_addPetRequested, this, [=](const QString &memberId, const QString &memberName){
        AddPetDialog dialog(this);
        PetInfo initialInfo;
        initialInfo.ownerId = memberId;
        initialInfo.ownerName = memberName;
        dialog.setPetInfo(initialInfo);

        if (dialog.exec() == QDialog::Accepted) {
            PetInfo pet = dialog.getPetInfo();
            emit sig_petAdded(pet);

            // 定位表格中的行并更新宠物列（即使列已隐藏）
            for (int i = 0; i < memTable->rowCount(); ++i) {
                if (memTable->item(i, 0)->text() == memberId) {
                    QWidget *wrapper = memTable->cellWidget(i, 10);
                    QComboBox *petCombo = wrapper ? wrapper->findChild<QComboBox*>() : nullptr;
                    if (petCombo) {
                        if (!petCombo->isEnabled() || petCombo->itemText(0) == "无") petCombo->clear();
                        petCombo->setEnabled(true);
                        petCombo->addItem(QString("%1（%2）").arg(pet.name, pet.breed));
                    }
                    // 重新触发点击事件以刷新抽屉内容
                    onCellClicked(i, 0);
                    break;
                }
            }
            CustomMessageDialog::showWarning(this, "成功", QString("已为会员 %1 添加新宠物：%2").arg(memberName, pet.name));
        }
    });
 
    // 处理详情页的修改资料请求
    connect(m_detailDrawer, &MemberDetailDrawer::sig_editMemberRequested, this, [=](const MemberInfo &info){
        AddMemberDialog dialog(this);
        dialog.setInitialData(info);
        if (dialog.exec() == QDialog::Accepted) {
            MemberInfo newInfo = dialog.getMemberInfo();
            
            // 同步更新数据管理器
            MemberDataManager::instance()->updateMember(newInfo);
            
            // 定位表格中的行并同步更新
            for (int i = 0; i < memTable->rowCount(); ++i) {
                if (memTable->item(i, 0)->text() == newInfo.id) {
                    memTable->item(i, 1)->setText(newInfo.name);
                    memTable->item(i, 1)->setData(Qt::UserRole, newInfo.birthday);
                    memTable->item(i, 2)->setText(newInfo.gender);
                    memTable->item(i, 3)->setText(newInfo.phone);
                    memTable->item(i, 4)->setText(newInfo.level);
                    
                    // 更新状态列 (index 5)
                    QWidget *statusTagContainer = new QWidget();
                    QHBoxLayout *statusTagLayout = new QHBoxLayout(statusTagContainer);
                    statusTagLayout->setContentsMargins(0, 0, 0, 0);
                    statusTagLayout->setAlignment(Qt::AlignCenter);
                    QLabel *statusTag = new QLabel(newInfo.status);
                    QString statusStyle = "padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; ";
                    if (newInfo.status == "正常") statusStyle += "background: #dcfce7; color: #166534; border: 1px solid #dcfce7;";
                    else if (newInfo.status == "已注销") statusStyle += "background: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0;";
                    else statusStyle += "background: #fff7ed; color: #c2410c; border: 1px solid #fed7aa;";
                    statusTag->setStyleSheet(statusStyle);
                    statusTagLayout->addWidget(statusTag);
                    memTable->setCellWidget(i, 5, statusTagContainer);

                    // 重新触发点击以刷新详情面板
                    onCellClicked(i, 0);
                    break;
                }
            }
            updateStatistics();
            CustomMessageDialog::showWarning(this, "修改成功", QString("会员 %1 的资料已更新").arg(newInfo.name));
        }
    });
 
    // 搜索联动
    connect(searchEdit, &QLineEdit::textChanged, this, &MemberModule::onSearchTextChanged);

    // 禁用右键菜单
    memTable->setContextMenuPolicy(Qt::NoContextMenu);

    // --- 联网：数据同步逻辑 ---
    connect(MemberDataManager::instance(), &MemberDataManager::dataChanged, this, [=](){
        addSampleData();
        updateStatistics();
        
        // 数据回来后，如果表里有数据且当前没选中，则自动选中第一行
        if (memTable->rowCount() > 0 && memTable->currentRow() < 0) {
            memTable->selectRow(0);
            onCellClicked(0, 0); // 触发抽屉展示
        }
    });
    
    // 初始请求数据
    MemberDataManager::instance()->requestMemberList();
}

void MemberModule::addRow(const MemberInfo &info, const QString &lastVisit, const QString &pets)
{
    int r = memTable->rowCount();
    memTable->insertRow(r);
    updateRowInPlace(r, info, lastVisit, pets);
}

void MemberModule::updateRowInPlace(int r, const MemberInfo &info, const QString &lastVisit, const QString &pets)
{

    auto createItem = [&](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setFont(QFont("Microsoft YaHei", 9));
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    QTableWidgetItem *idItem = createItem(info.id);
    idItem->setData(Qt::UserRole, QVariant::fromValue(info));
    memTable->setItem(r, 0, idItem);
    
    QTableWidgetItem *nameItem = createItem(info.name);
    nameItem->setData(Qt::UserRole, info.birthday); 
    memTable->setItem(r, 1, nameItem);
    
    memTable->setItem(r, 2, createItem(info.gender));
    memTable->setItem(r, 3, createItem(info.phone));
    memTable->setItem(r, 4, createItem(info.level));
    
    // 状态列 (带样式标签)
    QWidget *statusTagContainer = new QWidget();
    QHBoxLayout *statusTagLayout = new QHBoxLayout(statusTagContainer);
    statusTagLayout->setContentsMargins(0, 0, 0, 0);
    statusTagLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *statusTag = new QLabel(info.status);
    QString statusStyle = "padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; ";
    if (info.status == "正常") {
        statusStyle += "background: #dcfce7; color: #166534; border: 1px solid #dcfce7;";
    } else if (info.status == "已注销") {
        statusStyle += "background: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0;"; // 灰色注销样式
    } else {
        statusStyle += "background: #fff7ed; color: #c2410c; border: 1px solid #fed7aa;"; // 橙色锁定/异常样式
    }
    statusTag->setStyleSheet(statusStyle);
    statusTagLayout->addWidget(statusTag);
    memTable->setCellWidget(r, 5, statusTagContainer);
    memTable->setItem(r, 5, new QTableWidgetItem()); // 占位

    memTable->setItem(r, 6, createItem(QString::number(info.balance, 'f', 2)));
    memTable->setItem(r, 7, createItem(QString::number(info.consume_amt, 'f', 2)));
    memTable->setItem(r, 8, createItem(QString::number(info.points)));
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
                emit sig_requestPetJump(info.name, petName);
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
                               "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; font-size: 12px; padding: 0 5px; text-align: center; }"
                               "QPushButton:hover { opacity: 0.8; background-color: %2; color: white; }"
                               ).arg(bgColor, textColor, borderColor));
        return btn;
    };

    // 生成显眼的按钮
    QPushButton *rechargeBtn = createBtn("充值", "#f0f9eb", "#67c23a", "#c2e7b0"); // 绿色
    QPushButton *deleteBtn = createBtn("删除", "#fef0f0", "#f56c6c", "#fbc4c4"); // 红色

    // 统一样式：正常状态宽度
    rechargeBtn->setFixedWidth(70);
    deleteBtn->setFixedWidth(70);

    if (info.status == "已注销") {
        // --- “已注销”状态下的按钮替换逻辑 ---
        rechargeBtn->setText("恢复");
        rechargeBtn->setStyleSheet(
            "QPushButton { background-color: #eff6ff; color: #3b82f6; border: 1px solid #dbeafe; border-radius: 4px; font-size: 12px; padding: 0; text-align: center; }"
            "QPushButton:hover { background-color: #3b82f6; color: white; }"
        );
        rechargeBtn->setFixedWidth(70); // 统一为 70px
        
        if (m_role == ADMIN) {
            deleteBtn->setText("彻底删除");
            deleteBtn->setStyleSheet(
                "QPushButton { background-color: #fef2f2; color: #dc2626; border: 1px solid #fee2e2; border-radius: 4px; font-size: 12px; padding: 0; text-align: center; }"
                "QPushButton:hover { background-color: #dc2626; color: white; }"
            );
            deleteBtn->setFixedWidth(70); // 统一为 70px
            deleteBtn->setVisible(true);
        } else {
            deleteBtn->setVisible(false);
        }
    } else {
        // 正常状态下的显示逻辑
        if (m_role == STAFF) {
            deleteBtn->setVisible(false);
        }
    }

    actionLayout->addStretch(); // 左侧弹簧
    actionLayout->addWidget(rechargeBtn);
    actionLayout->addWidget(deleteBtn);
    actionLayout->addStretch(); // 右侧弹簧

    // 【按钮逻辑绑定】
    connect(rechargeBtn, &QPushButton::clicked, this, [=](){
        if (info.status == "已注销") {
            // 恢复逻辑
            if (CustomMessageDialog::confirm(this, "恢复确认", QString("确定恢复会员 [%1] 的档案吗？").arg(info.name))) {
                MemberDataManager::instance()->restoreMember(info.id);
                refreshTablePreservingSelection(info.id);
                updateStatistics();
                CustomMessageDialog::showSuccess(this, "恢复成功", QString("会员 %1 已恢复正常状态").arg(info.name));
            }
        } else {
            // 充值逻辑
            // 获取当前最新数据
            int rowIdx = -1;
            for (int i = 0; i < memTable->rowCount(); ++i) {
                if (memTable->cellWidget(i, 11) == actionWidget) {
                    rowIdx = i;
                    break;
                }
            }
            if (rowIdx < 0) return;

            MemberInfo currentInfo;
            currentInfo.id = memTable->item(rowIdx, 0)->text();
            currentInfo.name = memTable->item(rowIdx, 1)->text();
            currentInfo.level = memTable->item(rowIdx, 4)->text();
            currentInfo.balance = memTable->item(rowIdx, 6)->text().toDouble();

            RechargeDialog dlg(currentInfo, this);
            if (dlg.exec() == QDialog::Accepted) {
                double rechargeAmt = dlg.getRechargeAmount();
                double newBalance = currentInfo.balance + rechargeAmt;
                
                MemberInfo updatedInfo = MemberDataManager::instance()->getMember(currentInfo.id);
                updatedInfo.balance = newBalance;
                MemberDataManager::instance()->updateMember(updatedInfo);

                memTable->item(rowIdx, 6)->setText(QString::number(newBalance, 'f', 2));
                if (m_detailDrawer && m_detailDrawer->isVisible() && currentInfo.id == memTable->item(rowIdx, 0)->text()) {
                    m_detailDrawer->updateBalance(newBalance);
                }
                CustomMessageDialog::showSuccess(this, "充值成功", 
                    QString("会员 [%1] 充值成功！\n\n充值金额: ¥%2\n当前余额: ¥%3")
                    .arg(currentInfo.name).arg(rechargeAmt).arg(newBalance));
            }
        }
    });

    connect(deleteBtn, &QPushButton::clicked, this, [=](){
        if (info.status == "已注销") {
            // 彻底删除逻辑 (仅 ADMIN 可见)
            if (CustomMessageDialog::confirm(this, "彻底删除警示", 
                QString("确定要彻底抹除会员 [%1] 的所有档案吗？\n此操作不可撤销，且会清理所有关联数据。").arg(info.name))) {
                MemberDataManager::instance()->hardDeleteMember(info.id);
                if (m_detailDrawer) m_detailDrawer->hideDrawer();
                addSampleData();
                updateStatistics();
                CustomMessageDialog::showSuccess(this, "清理成功", QString("会员 %1 的数据已被彻底移除").arg(info.name));
            }
        } else {
            // 逻辑删除逻辑
            if(CustomMessageDialog::confirm(this, "业务确认", 
                QString("确定注销会员 [%1] 的档案吗？\n注销后将隐藏其资料，但保留历史消费凭证。").arg(info.name))) {
                MemberDataManager::instance()->removeMember(info.id);
                refreshTablePreservingSelection(info.id);
                updateStatistics();
                CustomMessageDialog::showSuccess(this, "注销成功", QString("会员 %1 已成功注销").arg(info.name));
            }
        }
    });

    memTable->setCellWidget(r, 11, actionWidget);
    
    updatePagination();
}

void MemberModule::addSampleData()
{
    memTable->setRowCount(0);
    auto allOnes = MemberDataManager::instance()->allMembers();
    
    if (allOnes.isEmpty()) {
        if (m_detailDrawer) m_detailDrawer->setMemberInfo(MemberInfo()); 
        return;
    }

    // 排序逻辑：注销会员排在最后，其余按工号排序
    std::sort(allOnes.begin(), allOnes.end(), [](const MemberInfo &a, const MemberInfo &b){
        if (a.isActive != b.isActive) return a.isActive; // 活跃在前
        return a.id < b.id;
    });
    
    for (const auto &info : allOnes) {
        // 使用 info.pets，实际业务中可根据需要从 PetDataManager 动态计算，这里保持 DataManager 同步
        QString lastVisit = "2026-03-10";
        addRow(info, lastVisit, info.pets);
    }
}

void MemberModule::updateStatistics()
{
    int total = memTable->rowCount();
    int regular = 0, gold = 0, platinum = 0, diamond = 0;
    
    for(int i=0; i<total; ++i) {
        if(memTable->item(i, 4)) {
            QString level = memTable->item(i, 4)->text();
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
        auto item0 = memTable->item(i, 0);
        auto item1 = memTable->item(i, 1);
        auto item3 = memTable->item(i, 3);
        bool textMatch = ((item0 && item0->text().contains(text, Qt::CaseInsensitive)) ||
                          (item1 && item1->text().contains(text, Qt::CaseInsensitive)) ||
                          (item3 && item3->text().contains(text)));
                          
        bool levelMatch = true;
        auto item4 = memTable->item(i, 4);
        if (selectedLevel != "全部等级" && item4) {
            levelMatch = (item4->text() == selectedLevel);
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
        auto item0 = memTable->item(i, 0);
        auto item1 = memTable->item(i, 1);
        bool match = searchText.isEmpty() ||
                    ((item0 && item0->text().contains(searchText, Qt::CaseInsensitive)) ||
                     (item1 && item1->text().contains(searchText, Qt::CaseInsensitive)));
        
        if (match) {
            visibleRows.append(i);
        }
        memTable->setRowHidden(i, true); // 先统统隐藏
    }

    // 2. 计算分页 (不需要再次排序，因为物理行顺序已经是正确的了)
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
        MemberDataManager::instance()->addMember(info);
        addSampleData();
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
        ts << "会员ID,姓名,性别,手机号,会员等级,状态,储值余额,累计消费金额,可用积分,最后到店,宠物档案\n";
        
        QString searchText = searchEdit->text();
        QString selectedLevel = levelFilterCombo->currentText();

        for(int r=0; r<memTable->rowCount(); ++r) {
            bool shouldExport = true;
            if (!exportAll) {
                bool textMatch = (memTable->item(r, 0)->text().contains(searchText, Qt::CaseInsensitive) ||
                                  memTable->item(r, 1)->text().contains(searchText, Qt::CaseInsensitive) ||
                                  memTable->item(r, 3)->text().contains(searchText));
                bool levelMatch = true;
                if (selectedLevel != "全部等级" && memTable->item(r, 4)) levelMatch = (memTable->item(r, 4)->text() == selectedLevel);
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

                // 获取状态
                QString statusText = "";
                QWidget *sWrapper = memTable->cellWidget(r, 5);
                if (sWrapper) {
                    QLabel *sLabel = sWrapper->findChild<QLabel*>();
                    if (sLabel) statusText = sLabel->text();
                }

                ts << QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11\n").arg(
                          memTable->item(r,0)->text(), 
                          memTable->item(r,1)->text(), 
                          memTable->item(r,2)->text(), 
                          memTable->item(r,3)->text(), 
                          memTable->item(r,4)->text(), 
                          statusText,
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
    Q_UNUSED(column);
    if (row < 0 || row >= memTable->rowCount()) return;

    MemberInfo info;
    auto item0 = memTable->item(row, 0);
    auto item1 = memTable->item(row, 1);
    auto item2 = memTable->item(row, 2);
    auto item3 = memTable->item(row, 3);
    auto item4 = memTable->item(row, 4);
    auto item6 = memTable->item(row, 6);
    auto item7 = memTable->item(row, 7);
    auto item8 = memTable->item(row, 8);
    auto item9 = memTable->item(row, 9);

    if (!item0 || !item1 || !item2 || !item3 || !item4) return;

    info.id = item0->text();
    info.name = item1->text();
    info.gender = item2->text();
    info.phone = item3->text();
    info.level = item4->text();
    
    // 获取状态 (通过 cellWidget 获取，因为 item(row, 5) 只是占位)
    QWidget *statusWrapper = memTable->cellWidget(row, 5);
    if (statusWrapper) {
        QLabel *statusLabel = statusWrapper->findChild<QLabel*>();
        if (statusLabel) info.status = statusLabel->text();
    }

    if (item6) info.balance = item6->text().replace("¥ ", "").toDouble();
    if (item7) info.consume_amt = item7->text().replace("¥ ", "").toDouble();
    if (item8) info.points = item8->text().toInt();
    
    // 生日通常存储在 UserRole 中
    info.birthday = item1->data(Qt::UserRole).toString();
    if(info.birthday.isEmpty()) info.birthday = "1990-01-01"; // Fallback

    QString lastVisit = item9 ? item9->text() : "";
    
    // 获取宠物档案文字
    QString petsStr = "";
    QWidget *petWrapper = memTable->cellWidget(row, 10);
    if (petWrapper) {
        QComboBox *petCombo = petWrapper->findChild<QComboBox*>();
        if (petCombo) {
            for (int i = 0; i < petCombo->count(); ++i) {
                petsStr += petCombo->itemText(i) + (i == petCombo->count() - 1 ? "" : " / ");
            }
        }
    }
    if(petsStr.isEmpty()) petsStr = memTable->item(row, 10) ? memTable->item(row, 10)->text() : "暂无";

    m_detailDrawer->setMember(info, lastVisit, petsStr);
    m_detailDrawer->showDrawer();
}

QString MemberModule::generateNextMemberId()
{
    int maxId = 0;
    // 遍历表格所有行（包括跨页的数据，因为数据都在 memTable 里，只是部分行 hidden）
    for (int i = 0; i < memTable->rowCount(); ++i) {
        QTableWidgetItem *item = memTable->item(i, 0); // ID 列索引为 1
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


void MemberModule::refreshTablePreservingSelection(const QString &targetId)
{
    m_isRefreshing = true;
    int savedPage = m_currentPage;
    addSampleData();
    
    int targetIdx = -1;
    for (int i = 0; i < memTable->rowCount(); ++i) {
        if (memTable->item(i, 0) && memTable->item(i, 0)->text() == targetId) {
            targetIdx = i;
            break;
        }
    }
    
    if (targetIdx != -1) {
        // 计算页码
        QList<int> visibleRows;
        QString searchText = searchEdit->text();
        QString selectedLevel = levelFilterCombo->currentText();
        for (int i = 0; i < memTable->rowCount(); ++i) {
            auto item0 = memTable->item(i, 0);
            auto item1 = memTable->item(i, 1);
            bool match = searchText.isEmpty() || ((item0 && item0->text().contains(searchText, Qt::CaseInsensitive)) || (item1 && item1->text().contains(searchText, Qt::CaseInsensitive)));
            if (selectedLevel != "全部等级" && memTable->item(i, 4)->text() != selectedLevel) match = false;
            if (match) visibleRows.append(i);
        }
        
        int k = visibleRows.indexOf(targetIdx);
        if (k != -1) {
            m_currentPage = (k / m_pageSize) + 1;
            updatePagination();
            memTable->selectRow(targetIdx);
            onCellClicked(targetIdx, 0);
        }
    } else {
        m_currentPage = savedPage;
        updatePagination();
    }
    
    m_isRefreshing = false;
}
void MemberModule::onEditMemberFromDrawer(const MemberInfo &info)
{
    // Update the member table with edited info
    for (int r = 0; r < memTable->rowCount(); ++r) {
        if (memTable->item(r, 0)->text() == info.id) {
            memTable->item(r, 1)->setText(info.name);
            memTable->item(r, 1)->setData(Qt::UserRole, info.birthday);
            memTable->item(r, 2)->setText(info.gender);
            memTable->item(r, 3)->setText(info.phone);
            memTable->item(r, 4)->setText(info.level);
            
            // 更新状态列 (index 5)
            QWidget *statusTagContainer = new QWidget();
            QHBoxLayout *statusTagLayout = new QHBoxLayout(statusTagContainer);
            statusTagLayout->setContentsMargins(0, 0, 0, 0);
            statusTagLayout->setAlignment(Qt::AlignCenter);
            QLabel *statusTag = new QLabel(info.status);
            QString statusStyle = "padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold; ";
            if (info.status == "正常") statusStyle += "background: #dcfce7; color: #166534; border: 1px solid #dcfce7;";
            else if (info.status == "已注销") statusStyle += "background: #f1f5f9; color: #94a3b8; border: 1px solid #e2e8f0;";
            else statusStyle += "background: #fff7ed; color: #c2410c; border: 1px solid #fed7aa;";
            statusTag->setStyleSheet(statusStyle);
            statusTagLayout->addWidget(statusTag);
            memTable->setCellWidget(r, 5, statusTagContainer);

            updateStatistics();
            
            // Refresh drawer
            onCellClicked(r, 0);
            break;
        }
    }
}

