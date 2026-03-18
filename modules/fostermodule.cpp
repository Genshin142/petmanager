#include "fostermodule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

FosterModule::FosterModule(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void FosterModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // 1. 标题区
    QLabel *titleLabel = new QLabel("寄养房态实时监控面板");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #303133;");
    mainLayout->addWidget(titleLabel);

    // 图例说明
    QHBoxLayout *legendLayout = new QHBoxLayout();
    legendLayout->setAlignment(Qt::AlignLeft);
    auto addLegend = [&](const QString &color, const QString &text) {
        QLabel *dot = new QLabel();
        dot->setFixedSize(14, 14);
        dot->setStyleSheet(QString("background: %1; border-radius: 7px;").arg(color));
        QLabel *lbl = new QLabel(text);
        lbl->setStyleSheet("color: #606266; font-size: 13px; margin-right: 20px;");
        legendLayout->addWidget(dot);
        legendLayout->addSpacing(4);
        legendLayout->addWidget(lbl);
    };
    addLegend("#409eff", "已入住");
    addLegend("#67c23a", "空闲可用");
    addLegend("#e6a23c", "清洁维护中");
    legendLayout->addStretch();
    mainLayout->addLayout(legendLayout);

    // 搜索和筛选区域
    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(10);
    filterLayout->setAlignment(Qt::AlignLeft);
    mainLayout->addLayout(filterLayout);



    // 2. 统计概览
    QHBoxLayout *statLayout = new QHBoxLayout();
    auto createStatCard = [&](const QString &icon, const QString &title, QLabel* &valLabel, const QString &color) {
        QFrame *card = new QFrame();
        card->setFixedHeight(100);
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f1f2f5; } ");
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 20));
        shadow->setOffset(0, 2);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *cl = new QHBoxLayout(card);
        cl->setContentsMargins(20, 15, 20, 15);

        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(50, 50);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("font-size: 26px; color: %1; background: #f5f7fa; border-radius: 10px; border: none;").arg(color));
        
        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(2);
        QLabel *tl = new QLabel(title); tl->setStyleSheet("color: #909399; font-size: 12px; border: none; background: transparent;");
        valLabel = new QLabel("0"); valLabel->setStyleSheet("color: #303133; font-size: 22px; font-weight: bold; border: none; background: transparent;");
        vl->addWidget(tl); vl->addWidget(valLabel);
        vl->addStretch();

        cl->addWidget(iconLabel); cl->addSpacing(12); cl->addLayout(vl); cl->addStretch();
        return card;
    };

    statLayout->addWidget(createStatCard("🏠", "总房间数", totalRoomsLabel, "#409eff"));
    statLayout->addWidget(createStatCard("🐾", "已入住", occupiedLabel, "#f56c6c"));
    statLayout->addWidget(createStatCard("✅", "空闲可用", freeLabel, "#67c23a"));
    statLayout->addWidget(createStatCard("🧹", "清洁维护", cleaningLabel, "#e6a23c"));
    mainLayout->addLayout(statLayout);

    // 3. 房态网格区 (可滚动)
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; }");

    QWidget *gridContainer = new QWidget();
    gridContainer->setStyleSheet("background: transparent;");
    roomGrid = new QGridLayout(gridContainer);
    roomGrid->setSpacing(15);
    roomGrid->setContentsMargins(5, 5, 5, 5);

    // 房间数据：roomNo, status, petId, petName
    struct RoomData {
        int no; QString status; QString petId; QString petName;
    };

    QList<RoomData> rooms = {
        {101, "occupied", "P1001", "二哈"},
        {102, "free", "", ""},
        {103, "free", "", ""},
        {104, "occupied", "P1004", "小雪"},
        {105, "free", "", ""},
        {106, "free", "", ""},
        {107, "cleaning", "", ""},
        {108, "occupied", "P1003", "旺财"},
        {109, "free", "", ""},
        {110, "cleaning", "", ""},
        {111, "free", "", ""},
        {112, "occupied", "P1002", "团团"},
        {113, "free", "", ""},
        {114, "free", "", ""},
        {115, "occupied", "P1005", "豆豆"},
    };

    int cols = 5;
    for (int i = 0; i < rooms.size(); ++i) {
        QWidget *card = createRoomCard(rooms[i].no, rooms[i].status, rooms[i].petId, rooms[i].petName);
        roomGrid->addWidget(card, i / cols, i % cols);
    }

    scrollArea->setWidget(gridContainer);
    mainLayout->addWidget(scrollArea);

    updateStats();
}

QWidget* FosterModule::createRoomCard(int roomNo, const QString &status, 
                                      const QString &petId, const QString &petName) {
    QFrame *card = new QFrame();
    card->setFixedSize(150, 120);
    card->setCursor(Qt::PointingHandCursor);

    // 根据状态设置视觉风格
    QString bgGradient, borderColor, statusText, statusIcon, textColor;
    
    if (status == "occupied") {
        bgGradient = "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #409eff, stop:1 #66b1ff)";
        borderColor = "#3a8ee6";
        statusText = "已入住";
        statusIcon = "🐾";
        textColor = "white";
    } else if (status == "cleaning") {
        bgGradient = "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #e6a23c, stop:1 #f0c78a)";
        borderColor = "#cf9236";
        statusText = "清洁中";
        statusIcon = "🧹";
        textColor = "white";
    } else {
        bgGradient = "qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #67c23a, stop:1 #95d475)";
        borderColor = "#5daf34";
        statusText = "空闲";
        statusIcon = "✅";
        textColor = "white";
    }

    card->setStyleSheet(QString(
        "QFrame#roomCard { background: %1; border-radius: 12px; border: 2px solid %2; } "
        "QFrame#roomCard QLabel { border: none; background: transparent; } "
        "QFrame#roomCard QFrame { border: none; } "
    ).arg(bgGradient, borderColor));
    card->setObjectName("roomCard");

    // 阴影
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 3);
    card->setGraphicsEffect(shadow);

    // 卡片内容布局
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(3);

    // 房间号 + 状态图标
    QHBoxLayout *topRow = new QHBoxLayout();
    QLabel *roomLabel = new QLabel(QString("Room %1").arg(roomNo));
    roomLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;").arg(textColor));
    QLabel *iconLabel = new QLabel(statusIcon);
    iconLabel->setStyleSheet("font-size: 16px;");
    iconLabel->setAlignment(Qt::AlignRight);
    topRow->addWidget(roomLabel);
    topRow->addStretch();
    topRow->addWidget(iconLabel);
    layout->addLayout(topRow);

    // 分隔线
    QFrame *line = new QFrame();
    line->setFixedHeight(1);
    line->setStyleSheet("background: rgba(255,255,255,0.3); border: none;");
    layout->addWidget(line);

    // 状态文字
    QLabel *statusLabel = new QLabel(statusText);
    statusLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(textColor));
    layout->addWidget(statusLabel);

    // 宠物信息（仅已入住房间显示）
    if (status == "occupied" && !petId.isEmpty()) {
        QLabel *petLabel = new QLabel(QString("%1 · %2").arg(petId, petName));
        petLabel->setStyleSheet(QString(
            "color: rgba(255,255,255,0.90); font-size: 11px; font-weight: bold; "
            "background: rgba(255,255,255,0.18); border-radius: 8px; padding: 2px 8px;"
        ));
        petLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(petLabel);
    } else {
        layout->addStretch();
    }

    return card;
}

void FosterModule::updateStats() {
    int total = 0, occ = 0, free = 0, clean = 0;

    for (int i = 0; i < roomGrid->count(); ++i) {
        QWidget *w = roomGrid->itemAt(i)->widget();
        if (!w) continue;
        total++;
        
        QString ss = w->styleSheet();
        if (ss.contains("#409eff")) occ++;
        else if (ss.contains("#e6a23c")) clean++;
        else free++;
    }

    totalRoomsLabel->setText(QString("%1间").arg(total));
    occupiedLabel->setText(QString("%1间").arg(occ));
    freeLabel->setText(QString("%1间").arg(free));
    cleaningLabel->setText(QString("%1间").arg(clean));
}
