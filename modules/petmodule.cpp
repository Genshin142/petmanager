#include "petmodule.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QLineEdit>
#include <QComboBox>
#include <QFrame>
#include <QAbstractItemView>
#include <QFont>
#include <QColor>

PetModule::PetModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void PetModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 1. 顶部标题与搜索
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("宠物数字化健康档案中心");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #303133;");

    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(10);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(" 搜索宠物名称、品种、主人姓名...");
    searchEdit->setFixedWidth(280);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(
        "QLineEdit { border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 15px; font-size: 13px; background: white; } "
        "QLineEdit:focus { border-color: #409eff; outline: none; }"
    );
    connect(searchEdit, &QLineEdit::textChanged, this, &PetModule::onSearch);
    filterLayout->addWidget(searchEdit);

    QPushButton *searchBtn = new QPushButton("搜 索");
    searchBtn->setFixedWidth(110); searchBtn->setFixedHeight(34);
    searchBtn->setCursor(Qt::PointingHandCursor);
    searchBtn->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border-radius: 17px; border: none; font-weight: bold; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { background: #66b1ff; } "
        "QPushButton:pressed { background: #3a8ee6; }"
    );
    filterLayout->addWidget(searchBtn);

    QPushButton *resetBtn = new QPushButton("重 置");
    resetBtn->setFixedWidth(100); resetBtn->setFixedHeight(34);
    resetBtn->setCursor(Qt::PointingHandCursor);
    resetBtn->setStyleSheet(
        "QPushButton { background: white; color: #606266; border-radius: 17px; border: 1px solid #dcdfe6; font-size: 13px; text-align: center; padding: 0 5px; } "
        "QPushButton:hover { border-color: #409eff; color: #409eff; background: #fdfdfd; } "
        "QPushButton:pressed { background: #f5f7fa; }"
    );
    filterLayout->addWidget(resetBtn);
    connect(resetBtn, &QPushButton::clicked, this, [this](){ searchEdit->clear(); });

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addLayout(filterLayout);
    mainLayout->addLayout(headerLayout);

    // 2. 统计卡片区
    QHBoxLayout *statLayout = new QHBoxLayout();
    auto createStatCard = [&](const QString &icon, const QString &title, QLabel* &valLabel, const QString &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet(QString(
            "QFrame { background: white; border-radius: 12px; border: 1px solid #f0f2f5; } "
        ));

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 30));
        shadow->setOffset(0, 2);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *cl = new QHBoxLayout(card);
        cl->setContentsMargins(20, 15, 20, 15);

        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(50, 50);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("font-size: 24px; color: %1; background: #f5f7fa; border-radius: 10px; border: none;").arg(color));

        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(2);
        QLabel *tl = new QLabel(title);
        tl->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
        valLabel = new QLabel("0");
        valLabel->setStyleSheet("color: #303133; font-size: 24px; font-weight: bold; border: none; background: transparent;");
        vl->addWidget(tl);
        vl->addWidget(valLabel);
        vl->addStretch();

        cl->addWidget(iconLabel);
        cl->addSpacing(15);
        cl->addLayout(vl);
        cl->addStretch();
        return card;
    };

    statLayout->addWidget(createStatCard("🐾", "在册宠物", totalPetsLabel, "#409eff"));
    statLayout->addWidget(createStatCard("🏠", "在店寄养", boardingPetsLabel, "#67c23a"));
    statLayout->addWidget(createStatCard("🛁", "洗护进行中", groomingPetsLabel, "#e6a23c"));
    mainLayout->addLayout(statLayout);

    // 3. 档案表格
    petTable = new QTableWidget();
    petTable->setColumnCount(8);
    petTable->setHorizontalHeaderLabels({"宠物ID", "宠物画像", "品种/信息", "健康状况", "疫苗保护", "当前状态", "主人ID", "主人姓名"});

    petTable->setShowGrid(false);
    petTable->setAlternatingRowColors(true);
    petTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    petTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    petTable->verticalHeader()->setVisible(false);
    petTable->verticalHeader()->setDefaultSectionSize(70);

    petTable->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; background-color: white; color: black; outline: none; } "
        "QTableWidget::item { border-bottom: 1px solid #f0f2f5; } "
        "QHeaderView::section { background-color: #f5f7fa; padding: 12px; border: none; color: #606266; font-weight: bold; font-size: 13px; } "
    );

    QHeaderView *header = petTable->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setSectionResizeMode(QHeaderView::Stretch);
    petTable->setColumnWidth(0, 70);   // ID
    petTable->setColumnWidth(2, 140);  // 品种
    petTable->setColumnWidth(5, 90);   // 状态

    mainLayout->addWidget(petTable);

    // 注入演示数据
    addPetRow("P1001", "二哈", "哈士奇", "男", "2岁", "健康 (轻微掉毛)", "2025-10-12 (已齐)", "寄养中", "M1001", "张三");
    addPetRow("P1002", "团团", "布偶猫", "女", "1岁", "过敏体质 (避开牛肉类)", "2025-08-20 (已齐)", "正常", "M1002", "李四");
    addPetRow("P1003", "旺财", "金毛犬", "男", "4岁", "由于年龄偏大 建议轻量运动", "2025-12-05 (已齐)", "洗护中", "M1003", "王五");
    addPetRow("P1004", "小雪", "萨摩耶", "女", "2岁", "良好", "2026-01-15 (预约)", "寄养中", "M1004", "赵六");

    updateStats();
}

void PetModule::addPetRow(const QString &id, const QString &name, const QString &breed,
                          const QString &gender, const QString &age, const QString &health,
                          const QString &vaccine, const QString &status, const QString &ownerId, const QString &owner)
{
    int row = petTable->rowCount();
    petTable->insertRow(row);

    auto setItem = [&](int col, const QString &text, bool bold = false) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        if (bold) item->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        petTable->setItem(row, col, item);
    };

    setItem(0, id);
    setItem(1, name, true);
    setItem(2, QString("%1 · %2 · %3").arg(breed, gender, age));

    // 健康状况
    QTableWidgetItem *healthItem = new QTableWidgetItem(health);
    healthItem->setTextAlignment(Qt::AlignCenter);
    healthItem->setFont(QFont("Microsoft YaHei", 8));
    petTable->setItem(row, 3, healthItem);

    setItem(4, vaccine);

    // 状态标签
    QWidget *statusContainer = new QWidget();
    QHBoxLayout *statusLayout = new QHBoxLayout(statusContainer);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setAlignment(Qt::AlignCenter);
    QLabel *statusTag = new QLabel(status);
    QString tagStyle = "padding: 2px 10px; border-radius: 12px; font-size: 11px; font-weight: bold; ";
    if (status == "正常") tagStyle += "background-color: #f0f9eb; color: #67c23a; border: 1px solid #e1f3d8;";
    else if (status == "寄养中") tagStyle += "background-color: #ecf5ff; color: #409eff; border: 1px solid #d9ecff;";
    else tagStyle += "background-color: #fdf6ec; color: #e6a23c; border: 1px solid #faecd8;";
    statusTag->setStyleSheet(tagStyle);
    statusLayout->addWidget(statusTag);
    petTable->setCellWidget(row, 5, statusContainer);

    setItem(6, ownerId);
    setItem(7, owner);
}

void PetModule::updateStats()
{
    int total = petTable->rowCount();
    int boarding = 0;
    int grooming = 0;

    for (int i = 0; i < total; ++i) {
        if (petTable->isRowHidden(i)) continue; // 排除被搜索隐藏的行
        QWidget *w = petTable->cellWidget(i, 5);
        if (w) {
            QLabel *lbl = w->findChild<QLabel*>();
            if (lbl) {
                if (lbl->text() == "寄养中") boarding++;
                else if (lbl->text() == "洗护中") grooming++;
            }
        }
    }

    int visibleCount = 0;
    for (int i = 0; i < total; ++i)
        if (!petTable->isRowHidden(i)) visibleCount++;

    totalPetsLabel->setText(QString("%1只").arg(visibleCount));
    boardingPetsLabel->setText(QString("%1只").arg(boarding));
    groomingPetsLabel->setText(QString("%1只").arg(grooming));
}

void PetModule::onSearch(const QString &keyword)
{
    QString kw = keyword.trimmed().toLower();
    for (int i = 0; i < petTable->rowCount(); ++i) {
        if (kw.isEmpty()) {
            petTable->setRowHidden(i, false);
            continue;
        }
        bool match = false;
        // 搜索列: 0=宠物ID, 1=名称, 2=品种信息, 6=主人ID, 7=主人姓名
        QList<int> searchCols = {0, 1, 2, 6, 7};
        for (int col : searchCols) {
            QTableWidgetItem *item = petTable->item(i, col);
            if (item && item->text().toLower().contains(kw)) {
                match = true;
                break;
            }
        }
        petTable->setRowHidden(i, !match);
    }
    updateStats();
}
