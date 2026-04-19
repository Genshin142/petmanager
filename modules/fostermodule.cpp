#include "fostermodule.h"
#include "custommessagedialog.h"
#include "compactcalendar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>
#include <QDateEdit>
#include <QLabel>
#include <QPropertyAnimation>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QProgressBar>
#include <QAbstractButton>

// 屏蔽滚轮/键盘误触改值，只允许通过日历弹窗选择日期
class NoScrollDateEdit : public QDateEdit {
public:
    explicit NoScrollDateEdit(QWidget *parent = nullptr) : QDateEdit(parent) {
        // 允许显示日历弹出按钮
        setCalendarPopup(true);
    }
protected:
    void wheelEvent(QWheelEvent *e) override { e->ignore(); }
    void keyPressEvent(QKeyEvent *e) override {
        // 屏蔽上下键改值
        if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down ||
            e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown) {
            e->ignore(); return;
        }
        QDateEdit::keyPressEvent(e);
    }
};

// ========================
// FosterCard 实现
// ========================

FosterCard::FosterCard(int roomNo, const QString &status, const QString &petId, const QString &petName, const QString &petBreed, const QString &ownerName, QWidget *parent)
    : QFrame(parent), m_roomNo(roomNo), m_status(status), m_petId(petId), m_petName(petName), m_petBreed(petBreed), m_ownerName(ownerName)
{
    setFixedSize(210, 155);
    setCursor(Qt::PointingHandCursor);

    QString bg, border, textColor;

    if (status == "occupied") {
        bg = "#4A90E2"; border = "#3178c6"; textColor = "white";
    } else if (status == "cleaning" || status == "maintenance") {
        // 橙黄色预警色系
        bool isMaint = (status == "maintenance");
        bg = isMaint ? "#FFF2E8" : "#FFF7E6"; // 维护比清洁稍微深一点
        border = isMaint ? "#FF9C6E" : "#FFA940";
        textColor = isMaint ? "#873800" : "#7c4a00";
    } else {
        bg = "#FFFFFF"; border = "#ADD8E6"; textColor = "#4a5c6b";
    }

    setStyleSheet(QString(
        "FosterCard { background: %1; border-radius: 14px; border: 2px solid %2; }"
        "QLabel { border: none; background: transparent; }"
    ).arg(bg, border));

    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(10);
    m_shadow->setColor(QColor(0, 0, 0, 30));
    m_shadow->setOffset(0, 2);
    setGraphicsEffect(m_shadow);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(4);

    // -- 顶行：房间号 + 状态图标 --
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    QLabel *roomLabel = new QLabel(QString("Room %1").arg(roomNo));
    roomLabel->setStyleSheet(QString("font-size: 14px; font-weight: 900; color: %1;").arg(textColor));

    QString statusIcon;
    if (status == "occupied") statusIcon = "🐾";
    else if (status == "cleaning") statusIcon = "🧹";
    else if (status == "maintenance") statusIcon = "🔧";
    else statusIcon = "✅";

    QLabel *iconLabel = new QLabel(statusIcon);
    iconLabel->setStyleSheet("font-size: 15px;");
    iconLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    topRow->addWidget(roomLabel);
    topRow->addStretch();
    topRow->addWidget(iconLabel);
    layout->addLayout(topRow);

    // -- 分割线 --
    QFrame *line = new QFrame();
    line->setFixedHeight(1);
    QString lineColor = (status == "occupied") ? "rgba(255,255,255,0.25)" : (status == "cleaning" ? "rgba(255,169,64,0.3)" : "rgba(173,216,230,0.5)");
    line->setStyleSheet(QString("background: %1; border: none;").arg(lineColor));
    layout->addWidget(line);
    layout->addSpacing(2);

    if (status == "occupied" && !petId.isEmpty()) {
        // ===== 已入住：头像 + 宠物名(主人) + ID =====
        QHBoxLayout *infoRow = new QHBoxLayout();
        infoRow->setContentsMargins(0, 0, 0, 0);
        infoRow->setSpacing(10);

        // 圆形头像
        m_avatar = new QLabel("🐾");
        m_avatar->setFixedSize(64, 64);
        m_avatar->setAlignment(Qt::AlignCenter);
        m_avatar->setCursor(Qt::PointingHandCursor);
        m_avatar->setStyleSheet(
            "font-size: 32px; background: rgba(255,255,255,0.25); "
            "border-radius: 32px; border: 2px solid rgba(255,255,255,0.5);"
        );

        // 右侧文字信息
        QVBoxLayout *textCol = new QVBoxLayout();
        textCol->setSpacing(2);
        textCol->setContentsMargins(0, 0, 0, 0);

        // 宠物名
        QLabel *nameLabel = new QLabel(petName);
        nameLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 900;").arg(textColor));

        // 品种徽章 (小)
        QLabel *breedTag = new QLabel(petBreed);
        QString breedBg = (status == "occupied") ? "rgba(255,255,255,0.2)" : "rgba(74, 144, 226, 0.1)";
        QString breedText = (status == "occupied") ? "white" : "#4A90E2";
        breedTag->setStyleSheet(QString(
            "color: %1; font-size: 9px; font-weight: bold; "
            "padding: 1px 6px; background: %2; border-radius: 4px;"
        ).arg(breedText, breedBg));

        // 编号标签 (增强直观性)
        QLabel *idTag = new QLabel("ID: " + petId);
        QString idColor = (status == "occupied") ? "rgba(255,255,255,0.9)" : "#4A5D6A";
        idTag->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(idColor));

        textCol->addWidget(nameLabel);
        textCol->addWidget(breedTag);
        textCol->addWidget(idTag);

        infoRow->addWidget(m_avatar);
        infoRow->addLayout(textCol, 1);
        layout->addLayout(infoRow);

        // 底部：入住天数提示
        QLabel *dayLabel = new QLabel(QString("已入住 %1 天").arg(QRandomGenerator::global()->bounded(1, 14)));
        dayLabel->setStyleSheet("color: rgba(255,255,255,0.65); font-size: 11px;");
        dayLabel->setAlignment(Qt::AlignRight);
        layout->addStretch();
        layout->addWidget(dayLabel);

    } else if (status == "cleaning" || status == "maintenance") {
        bool isMaint = (status == "maintenance");
        // ===== 清洁/维护中：动态图标 + 文字 =====
        QLabel *maintIcon = new QLabel(isMaint ? "🔧" : "🧹");
        maintIcon->setStyleSheet("font-size: 28px;");
        maintIcon->setAlignment(Qt::AlignCenter);
        layout->addSpacing(4);
        layout->addWidget(maintIcon);

        QLabel *maintText = new QLabel(isMaint ? "维护中" : "清洁中");
        maintText->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 900;").arg(isMaint ? "#873800" : "#c47f00"));
        maintText->setAlignment(Qt::AlignCenter);
        layout->addWidget(maintText);

        QLabel *reasonLabel = new QLabel(isMaint ? "设施报修排查中" : "例行消毒与深度清洁");
        reasonLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(isMaint ? "#873800" : "#d4a24c"));
        reasonLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(reasonLabel);
        layout->addStretch();

    } else {
        // ===== 空闲：大图标 + 可预约 =====
        QLabel *freeIcon = new QLabel("🏠");
        freeIcon->setStyleSheet("font-size: 28px;");
        freeIcon->setAlignment(Qt::AlignCenter);
        layout->addSpacing(4);
        layout->addWidget(freeIcon);

        QLabel *freeText = new QLabel("空闲");
        freeText->setStyleSheet("color: #5a8a9e; font-size: 13px; font-weight: 900;");
        freeText->setAlignment(Qt::AlignCenter);
        layout->addWidget(freeText);

        QLabel *bookLabel = new QLabel("可预约入住");
        bookLabel->setStyleSheet("color: #7ba8bd; font-size: 11px;");
        bookLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(bookLabel);
        layout->addStretch();
    }
}


void FosterCard::enterEvent(QEnterEvent *event) {
    // 仅增强阴影，不移动位置（避免布局偏移）
    m_shadow->setBlurRadius(20);
    m_shadow->setColor(QColor(0, 0, 0, 50));
    QFrame::enterEvent(event);
}

void FosterCard::leaveEvent(QEvent *event) {
    // 恢复默认阴影
    m_shadow->setBlurRadius(10);
    m_shadow->setColor(QColor(0, 0, 0, 30));
    QFrame::leaveEvent(event);
}

void FosterCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 判定是否点击了头像区域
        if (m_avatar && m_avatar->geometry().contains(event->pos())) {
            AvatarZoomDialog dlg(m_avatar->text(), this->window());
            dlg.exec();
        } else {
            emit clicked();
        }
    }
    QFrame::mousePressEvent(event);
}

// ========================
// FosterDetailDialog 实现
// ========================

// ========================
// 辅助：创建信息行
// ========================
static QHBoxLayout* makeInfoRow(const QString &label, const QString &value, const QString &valueColor = "#303133") {
    QHBoxLayout *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    QLabel *lbl = new QLabel(label);
    lbl->setStyleSheet("color: #909399; font-size: 13px; min-width: 80px;");
    QLabel *val = new QLabel(value);
    val->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;").arg(valueColor));
    val->setWordWrap(true);
    row->addWidget(lbl);
    row->addWidget(val, 1);
    return row;
}

static QPushButton* makeActionBtn(const QString &text, const QString &bg, const QString &hoverBg) {
    QPushButton *btn = new QPushButton(text);
    btn->setFixedHeight(36);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QString(
        "QPushButton { background: %1; color: white; border: none; border-radius: 8px; font-weight: bold; font-size: 13px; padding: 0 20px; }"
        "QPushButton:hover { background: %2; }"
    ).arg(bg, hoverBg));
    return btn;
}

// ========================
// FosterDetailDialog 实现
// ========================

FosterDetailDialog::FosterDetailDialog(int roomId, const QString &status, const QString &petId, const QString &petName, const QString &petBreed, const QString &ownerName, QWidget *parent)
    : QDialog(parent), m_status(status)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    QFrame *container = new QFrame();
    container->setFixedWidth(500);
    container->setStyleSheet("QFrame { background: white; border-radius: 16px; } QLabel { border: none; }");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    QVBoxLayout *contentLayout = new QVBoxLayout(container);
    contentLayout->setContentsMargins(28, 24, 28, 20);
    contentLayout->setSpacing(0);

    // 根据状态分发不同视图
    if (status == "occupied") {
        buildOccupiedView(contentLayout, roomId, petId, petName, petBreed, ownerName);
    } else if (status == "free") {
        buildFreeView(contentLayout, roomId);
    } else {
        buildCleaningView(contentLayout, roomId);
    }

    mainLayout->addWidget(container);
    if (parent) resize(parent->size());
}

void FosterDetailDialog::buildOccupiedView(QVBoxLayout *layout, int roomId, const QString &petId, const QString &petName, const QString &petBreed, const QString &ownerName) {
    // ---- 顶部状态栏：Room (左) + 状态胶囊 (右) ----
    QHBoxLayout *headerRow = new QHBoxLayout();
    QLabel *roomLabel = new QLabel(QString("Room %1").arg(roomId));
    roomLabel->setStyleSheet("color: #303133; font-size: 24px; font-weight: 900;");
    
    QLabel *statusBadge = new QLabel("● 已入住");
    statusBadge->setStyleSheet(
        "color: #4A90E2; background: rgba(74, 144, 226, 0.1); font-size: 14px; font-weight: bold; "
        "padding: 6px 16px; border-radius: 14px;"
    );
    statusBadge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    headerRow->addWidget(roomLabel);
    headerRow->addStretch();
    headerRow->addWidget(statusBadge);
    layout->addLayout(headerRow);
    layout->addSpacing(20);

    // ---- 宠物信息区 ----
    QFrame *petFrame = new QFrame();
    petFrame->setStyleSheet("QFrame { background: #f0f7ff; border-radius: 10px; border: 1px solid #d9ecff; } QLabel { border: none; background: transparent; }");
    QHBoxLayout *petRow = new QHBoxLayout(petFrame);
    petRow->setContentsMargins(14, 12, 14, 12);

    QLabel *avatar = new QLabel("🐾");
    avatar->setFixedSize(72, 72);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setCursor(Qt::PointingHandCursor);
    avatar->setStyleSheet("font-size: 36px; background: white; border-radius: 36px; border: 2px solid #b3d8ff;");
    
    // 点击详情头像也能放大
    avatar->installEventFilter(this);
    avatar->setProperty("isAvatar", true); // 标记以便识别
    
    QVBoxLayout *petInfo = new QVBoxLayout();
    petInfo->setSpacing(8);
    petInfo->setAlignment(Qt::AlignVCenter);

    // 第一行：名字 + 品种标签
    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->setSpacing(12);
    QLabel *nameLabel = new QLabel(petName.isEmpty() ? "未知宠物" : petName);
    nameLabel->setStyleSheet("color: #303133; font-size: 22px; font-weight: 900;");
    
    QLabel *breedLabel = new QLabel(petBreed.isEmpty() ? "通用/未知" : petBreed);
    breedLabel->setStyleSheet(
        "color: #4A90E2; font-size: 13px; font-weight: bold; background: rgba(74, 144, 226, 0.1); "
        "border-radius: 6px; padding: 4px 12px;"
    );
    breedLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    nameRow->addWidget(nameLabel);
    nameRow->addWidget(breedLabel);
    nameRow->addStretch();

    // 第二行：宠物编号 (增强直观性)
    QLabel *idLabel = new QLabel(QString("ID: %1").arg(petId));
    idLabel->setStyleSheet("color: #4A5D6A; font-size: 15px; font-weight: bold;");
    
    petInfo->addLayout(nameRow);
    petInfo->addWidget(idLabel);

    petRow->addWidget(avatar);
    petRow->addSpacing(10);
    petRow->addLayout(petInfo);
    petRow->addStretch();
    layout->addWidget(petFrame);
    layout->addSpacing(14);

    // ---- 核心详情 ----
    QString fullOwnerInfo = ownerName.isEmpty() ? "张大伟 138-0000-0000" : QString("%1 138-0000-0000").arg(ownerName);
    layout->addLayout(makeInfoRow("主人联系", fullOwnerInfo));
    layout->addSpacing(8);
    layout->addLayout(makeInfoRow("入住日期", QDate::currentDate().addDays(-3).toString("yyyy-MM-dd")));
    layout->addSpacing(8);
    layout->addLayout(makeInfoRow("预计离店", QDate::currentDate().addDays(2).toString("yyyy-MM-dd"), "#e6a23c"));
    layout->addSpacing(8);

    // 饮食禁忌高亮
    QFrame *warnFrame = new QFrame();
    warnFrame->setStyleSheet("QFrame { background: #fdf6ec; border-radius: 8px; border: 1px solid #faecd8; } QLabel { border: none; background: transparent; }");
    QHBoxLayout *warnRow = new QHBoxLayout(warnFrame);
    warnRow->setContentsMargins(12, 8, 12, 8);
    QLabel *warnIcon = new QLabel("⚠️");
    warnIcon->setStyleSheet("font-size: 16px;");
    QLabel *warnText = new QLabel("饮食禁忌：不吃禽类 · 每日代喂 3 次 · 需口服拜宠清");
    warnText->setStyleSheet("color: #c47f00; font-size: 12px; font-weight: bold;");
    warnText->setWordWrap(true);
    warnRow->addWidget(warnIcon);
    warnRow->addWidget(warnText, 1);
    layout->addWidget(warnFrame);
    layout->addSpacing(18);

    // ---- 底部操作区 ----
    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *historyBtn = makeActionBtn("📋 往期寄养记录", "#f0f2f5", "#e9eaec");
    historyBtn->setStyleSheet(
        "QPushButton { background: #f0f2f5; color: #606266; border: 1px solid #dcdfe6; border-radius: 8px; font-weight: bold; font-size: 13px; padding: 0 16px; }"
        "QPushButton:hover { background: #e9eaec; color: #409eff; border-color: #b3d8ff; }"
    );
    connect(historyBtn, &QPushButton::clicked, this, [this, roomId]() { emit requestHistory(roomId); });
    QPushButton *closeBtn = makeActionBtn("确认关闭", "#4A90E2", "#66b1ff");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(historyBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);
}

void FosterDetailDialog::buildFreeView(QVBoxLayout *layout, int roomId) {
    // ---- 顶部状态栏 ----
    QHBoxLayout *headerRow = new QHBoxLayout();
    QLabel *statusBadge = new QLabel("● 空闲");
    statusBadge->setStyleSheet(
        "color: white; background: #67c23a; font-size: 14px; font-weight: bold; "
        "padding: 6px 16px; border-radius: 14px;"
    );
    statusBadge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QLabel *roomLabel = new QLabel(QString("Room %1").arg(roomId));
    roomLabel->setStyleSheet("color: #303133; font-size: 24px; font-weight: 900;");
    headerRow->addWidget(roomLabel);
    headerRow->addStretch();
    headerRow->addWidget(statusBadge);
    layout->addLayout(headerRow);
    layout->addSpacing(16);

    // ---- 上次房客简报 ----
    QLabel *sectionTitle = new QLabel("最近一次寄养简报");
    sectionTitle->setStyleSheet("color: #606266; font-size: 14px; font-weight: bold;");
    layout->addWidget(sectionTitle);
    layout->addSpacing(10);

    QFrame *briefFrame = new QFrame();
    briefFrame->setStyleSheet("QFrame { background: #f8faf5; border-radius: 12px; border: 1px solid #e1f3d8; } QLabel { border: none; background: transparent; }");
    QVBoxLayout *briefLayout = new QVBoxLayout(briefFrame);
    briefLayout->setContentsMargins(18, 16, 18, 16);
    briefLayout->setSpacing(14);

    // ---- 访客名片风格 ----
    QHBoxLayout *petRow = new QHBoxLayout();
    petRow->setContentsMargins(0, 0, 0, 0);
    petRow->setSpacing(12);

    QLabel *avatar = new QLabel("🐾");
    avatar->setFixedSize(72, 72);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setCursor(Qt::PointingHandCursor);
    avatar->setStyleSheet("font-size: 36px; background: white; border-radius: 36px; border: 2px solid #e1f3d8;");
    
    avatar->installEventFilter(this);
    avatar->setProperty("isAvatar", true);
    
    QVBoxLayout *petInfo = new QVBoxLayout();
    petInfo->setSpacing(6);
    petInfo->setAlignment(Qt::AlignVCenter);

    // 第一行：名字 + 品种
    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->setSpacing(8);
    QLabel *nameLabel = new QLabel("团团 (吴晓飞)");
    nameLabel->setStyleSheet("color: #303133; font-size: 20px; font-weight: 900;");

    QLabel *breedLabel = new QLabel("金毛巡回犬");
    breedLabel->setStyleSheet("color: #67c23a; font-size: 13px; font-weight: bold; background: #f0f9eb; border-radius: 6px; padding: 4px 12px;");
    breedLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    nameRow->addWidget(nameLabel);
    nameRow->addWidget(breedLabel);
    nameRow->addStretch();

    // 第二行：ID
    QLabel *idLabel = new QLabel("ID: P1012");
    idLabel->setStyleSheet("color: #909399; font-size: 14px; font-weight: 500;");
    
    petInfo->addLayout(nameRow);
    petInfo->addWidget(idLabel);

    petRow->addWidget(avatar);
    petRow->addLayout(petInfo);
    petRow->addStretch();
    briefLayout->addLayout(petRow);

    // 分割线
    QFrame *innerLine = new QFrame();
    innerLine->setFixedHeight(1);
    innerLine->setStyleSheet("background: rgba(103, 194, 58, 0.15); border: none;");
    briefLayout->addWidget(innerLine);

    // 辅助详情
    briefLayout->addLayout(makeInfoRow("离店时间", QDate::currentDate().addDays(-1).toString("yyyy-MM-dd") + " 14:30"));

    QHBoxLayout *cleanRow = new QHBoxLayout();
    QLabel *cleanLabel = new QLabel("消毒状态");
    cleanLabel->setStyleSheet("color: #909399; font-size: 13px; min-width: 80px;");
    QLabel *cleanBadge = new QLabel("✅ 已消毒");
    cleanBadge->setStyleSheet(
        "color: #67c23a; font-size: 12px; font-weight: bold; background: #f0f9eb; "
        "padding: 4px 12px; border-radius: 8px; border: 1px solid #e1f3d8;"
    );
    cleanBadge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    cleanRow->addWidget(cleanLabel);
    cleanRow->addWidget(cleanBadge);
    cleanRow->addStretch();
    briefLayout->addLayout(cleanRow);

    layout->addWidget(briefFrame);
    layout->addSpacing(22);

    // ---- 操作区 ----
    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *bookBtn = makeActionBtn("🐾 立即排单入住", "#67c23a", "#85ce61");
    connect(bookBtn, &QPushButton::clicked, this, [this, roomId]() {
        emit requestBooking(roomId);
        accept();
    });
    QPushButton *closeBtn = makeActionBtn("返回", "#f0f2f5", "#e9eaec");
    closeBtn->setStyleSheet(
        "QPushButton { background: #f0f2f5; color: #606266; border: 1px solid #dcdfe6; border-radius: 8px; font-weight: bold; font-size: 13px; padding: 0 20px; }"
        "QPushButton:hover { background: #e9eaec; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(closeBtn);
    btnRow->addStretch();
    btnRow->addWidget(bookBtn);
    layout->addLayout(btnRow);
}

void FosterDetailDialog::buildCleaningView(QVBoxLayout *layout, int roomId) {
    // ---- 顶部状态栏 ----
    QHBoxLayout *headerRow = new QHBoxLayout();
    bool isMaint = (m_status == "maintenance");
    QLabel *statusBadge = new QLabel(isMaint ? "● 维护中" : "● 清洁中");
    statusBadge->setStyleSheet(QString(
        "color: white; background: %1; font-size: 12px; font-weight: bold; "
        "padding: 4px 12px; border-radius: 10px;"
    ).arg(isMaint ? "#FF9C6E" : "#FFA940"));
    statusBadge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QLabel *roomLabel = new QLabel(QString("Room %1").arg(roomId));
    roomLabel->setStyleSheet("color: #303133; font-size: 20px; font-weight: 900;");
    headerRow->addWidget(roomLabel);
    headerRow->addStretch();
    headerRow->addWidget(statusBadge);
    layout->addLayout(headerRow);
    layout->addSpacing(16);

    // ---- 维护信息 ----
    QFrame *maintFrame = new QFrame();
    maintFrame->setStyleSheet("QFrame { background: #FFF7E6; border-radius: 10px; border: 1px solid #faecd8; } QLabel { border: none; background: transparent; }");
    QVBoxLayout *maintLayout = new QVBoxLayout(maintFrame);
    maintLayout->setContentsMargins(14, 14, 14, 14);
    maintLayout->setSpacing(8);

    QLabel *maintIcon = new QLabel(isMaint ? "🔧" : "🧹");
    maintIcon->setStyleSheet("font-size: 36px;");
    maintIcon->setAlignment(Qt::AlignCenter);
    maintLayout->addWidget(maintIcon);

    QLabel *maintTitle = new QLabel(isMaint ? "设施故障维修中" : "深度清洁与环境消毒");
    maintTitle->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 900;").arg(isMaint ? "#873800" : "#7c4a00"));
    maintTitle->setAlignment(Qt::AlignCenter);
    maintLayout->addWidget(maintTitle);
    maintLayout->addSpacing(6);

    maintLayout->addLayout(makeInfoRow(isMaint ? "报修原因" : "清洁原因", isMaint ? "空调滤网损坏 / 门把手松动" : "常规循环消毒 + 地面除味"));
    maintLayout->addLayout(makeInfoRow("发起时间", QDate::currentDate().toString("yyyy-MM-dd") + " 09:00"));
    maintLayout->addLayout(makeInfoRow("预计完成", QDate::currentDate().toString("yyyy-MM-dd") + (isMaint ? " 明日 12:00" : " 17:00"), "#e6a23c"));

    layout->addWidget(maintFrame);
    layout->addSpacing(20);

    // ---- 操作区 ----
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    QPushButton *closeBtn = makeActionBtn("知道了", "#FFA940", "#ffbb54");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);
}

void FosterDetailDialog::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 120));
}

bool FosterDetailDialog::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QLabel *label = qobject_cast<QLabel*>(watched);
        if (label && label->property("isAvatar").toBool()) {
            AvatarZoomDialog dlg(label->text(), this->window());
            dlg.exec();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

// ========================
// FosterModule 实现
// ========================

FosterModule::FosterModule(QWidget *parent) : QWidget(parent) {
    setupUI();
    m_currentForecastDate = QDate::currentDate();
    onForecastDateChanged(m_currentForecastDate); 
}

void FosterModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    // 1. 标题与控制中心 (重排：左标题，右控)
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("寄养房态实时交互看板");
    titleLabel->setStyleSheet("font-size: 28px; color: #303133; font-weight: 900; letter-spacing: 1px;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    QHBoxLayout *rightActions = new QHBoxLayout();
    rightActions->setSpacing(12);

    QLabel *foreLbl = new QLabel("为下方房态预测日期：");
    foreLbl->setStyleSheet("color: #606266; font-size: 13px;");
    foreLbl->setToolTip("选择一个日期，模拟展示当天的房间入住状态分布");
    rightActions->addWidget(foreLbl);

    // NoScrollDateEdit：防止滚轮/键盘意外改值，只能通过日历弹窗选日期
    auto *noScrollEdit = new NoScrollDateEdit();
    noScrollEdit->setCalendarPopup(true);
    noScrollEdit->setDate(QDate::currentDate());
    noScrollEdit->setDisplayFormat("yyyy 年 MM 月 dd 日");
    noScrollEdit->setMinimumWidth(168);
    noScrollEdit->setFixedHeight(34);
    noScrollEdit->setStyleSheet(
        "QDateEdit { "
        "    border: 1px solid #dcdfe6; "
        "    border-radius: 8px; "
        "    padding: 0 30px 0 12px; " // 右侧留出 30px 给箭头
        "    font-size: 13px; "
        "    font-weight: bold; "
        "    color: #303133; "
        "    background: white; "
        "}"
        "QDateEdit:hover { border-color: #409eff; }"
        "QDateEdit::drop-down { "
        "    subcontrol-origin: padding; "
        "    subcontrol-position: top right; "
        "    width: 30px; "
        "    border-left: none; "
        "}"
        "QDateEdit::down-arrow { "
        "    image: none; "
        "    border: 1.5px solid #909399; "
        "    border-top: none; "
        "    border-left: none; "
        "    width: 6px; "
        "    height: 6px; "
        "    transform: rotate(45deg); "
        "    margin-top: -3px; " // 纯 CSS 绘制小箭头
        "}"
        "QDateEdit::down-arrow:hover { border-color: #409eff; }"
    );

    // 挂载 CompactCalendar 样式日历（不传颜色数据，仅共享外观）
    m_calendar = new CompactCalendar();
    noScrollEdit->setCalendarWidget(m_calendar);

    forecastDateEdit = noScrollEdit;
    connect(forecastDateEdit, &QDateEdit::dateChanged, this, &FosterModule::onForecastDateChanged);
    rightActions->addWidget(forecastDateEdit);

    headerLayout->addLayout(rightActions);
    mainLayout->addLayout(headerLayout);


    // 2. 统计概览层 (铺满宽度，均衡分布)
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(16);
    auto createStatCard = [&](const QString &icon, const QString &title, QLabel* &valLabel, const QString &accentColor) {
        QFrame *card = new QFrame();
        card->setFixedHeight(82);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // 均匀拉伸
        card->setStyleSheet("QFrame { background: white; border-radius: 12px; border: 1px solid #f0f0f0; } QLabel { border: none; }");
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(8); // 减小模糊，更平整
        shadow->setColor(QColor(0, 0, 0, 12));
        shadow->setOffset(0, 2);
        card->setGraphicsEffect(shadow);

        QHBoxLayout *cl = new QHBoxLayout(card);
        cl->setContentsMargins(16, 12, 16, 12);
        cl->setSpacing(12);
        
        QLabel *iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(40, 40);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString("font-size: 20px; background: %1; border-radius: 10px;").arg(accentColor + "18"));
        
        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(1);
        QLabel *tl = new QLabel(title);
        tl->setStyleSheet("color: #909399; font-size: 12px;");
        valLabel = new QLabel("0");
        valLabel->setStyleSheet(QString("color: %1; font-size: 22px; font-weight: 800;").arg(accentColor));
        vl->addWidget(tl);
        vl->addWidget(valLabel);
        cl->addWidget(iconLabel);
        cl->addLayout(vl);
        cl->addStretch();
        return card;
    };
    statLayout->addWidget(createStatCard("🏢", "房间总量", totalRoomsLabel, "#409eff"));
    statLayout->addWidget(createStatCard("🐕", "入住房间数", occupiedLabel, "#f56c6c"));
    statLayout->addWidget(createStatCard("🍀", "空闲房间", freeLabel, "#67c23a"));
    statLayout->addWidget(createStatCard("🧹", "清洁/维护", cleaningLabel, "#e6a23c"));
    mainLayout->addLayout(statLayout);

    // 3. 房态卡片自适应矩阵 (滚动条样式对齐)
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; } "
        "QScrollBar:vertical { width: 8px; background: transparent; margin: 0px; } "
        "QScrollBar::handle:vertical { background: #dcdfe6; border-radius: 4px; min-height: 40px; } "
        "QScrollBar::handle:vertical:hover { background: #c0c4cc; } "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } "
    );

    m_gridContainer = new QWidget();
    m_gridContainer->setStyleSheet("background: transparent;");
    roomGrid = new QGridLayout(m_gridContainer);
    roomGrid->setHorizontalSpacing(15);
    roomGrid->setVerticalSpacing(15);
    roomGrid->setContentsMargins(15, 10, 15, 15);
    roomGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft); // AlignLeft 确保从左侧紧密排列，避免大间距空洞

    m_scrollArea->setWidget(m_gridContainer);
    mainLayout->addWidget(m_scrollArea);
}

void FosterModule::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    relayoutGrid();
}

void FosterModule::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    // 首次真实显示后，视口宽度已就绪，重排一次以获得精确列数
    QTimer::singleShot(0, this, [this]() { relayoutGrid(); });
}

void FosterModule::relayoutGrid() {
    if (!roomGrid || roomGrid->count() == 0) return;

    const int cardWidth = 210;  // 与 FosterCard::setFixedSize(210,155) 一致
    const int spacing = 15;     // 与 roomGrid->setHorizontalSpacing(15) 一致
    const int defaultCols = 4;

    int availableWidth = m_scrollArea->viewport()->width();
    int cols = (availableWidth > cardWidth)
               ? qMax(1, (availableWidth + spacing) / (cardWidth + spacing))
               : defaultCols;

    // 收集所有已有卡片
    QList<QWidget*> widgets;
    for (int i = 0; i < roomGrid->count(); ++i) {
        QLayoutItem *it = roomGrid->itemAt(i);
        if (it && it->widget()) widgets << it->widget();
    }

    // 清除布局（不删除 widget）
    while (roomGrid->count() > 0) {
        QLayoutItem *it = roomGrid->takeAt(0);
        delete it;
    }

    // 按新列数重排，并强制每列等宽
    for (int i = 0; i < widgets.size(); ++i) {
        roomGrid->addWidget(widgets[i], i / cols, i % cols, Qt::AlignLeft | Qt::AlignTop);
    }
    for (int c = 0; c < cols; ++c) {
        roomGrid->setColumnMinimumWidth(c, cardWidth);
    }
}

void FosterModule::onForecastDateChanged(const QDate &date) {
    m_currentForecastDate = date;
    
    // 视觉像素级联动：将选中日期的所在周设置为高亮范围，模拟记录界面的高级填充感
    if (m_calendar) {
        // 计算当前日期所在的那一周 (周一到周日) 作为高亮区间
        int daysToMon = date.dayOfWeek() - 1;
        QDate weekStart = date.addDays(-daysToMon);
        QDate weekEnd = weekStart.addDays(6);
        m_calendar->setFosterRange(weekStart, weekEnd);
        m_calendar->update();
    }

    // 清空现有网格（删除 widget 本身）
    while (roomGrid->count() > 0) {
        QLayoutItem *item = roomGrid->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }

    quint32 seed = date.toJulianDay();
    QRandomGenerator randGen(seed);

    int totalCount = 20;
    const int cardWidth = 210;  // 与 FosterCard::setFixedSize(210,155) 一致
    const int spacing = 15;     // 与 roomGrid->setHorizontalSpacing(15) 一致
    const int defaultCols = 4;
    int availableWidth = m_scrollArea->viewport()->width();
    int cols = (availableWidth > cardWidth)
               ? qMax(1, (availableWidth + spacing) / (cardWidth + spacing))
               : defaultCols;

    QStringList petNames = {"小白", "旺财", "蛋黄", "豆豆", "团团", "哈士奇", "妙脆角", "可乐"};
    QStringList ownerNames = {"张大伟", "李晓明", "王志强", "赵芳芳", "刘德华", "陈淑芬", "周杰伦", "吴青峰"};
    QStringList breeds = {"柯基", "柴犬", "金毛", "比熊", "英短蓝猫", "布偶猫", "萨摩耶", "边境牧羊犬"};

    for (int i = 0; i < totalCount; ++i) {
        int st = randGen.bounded(100);
        QString status, pid, pname, oname, breed;
        if (st < 40) status = "free";
        else if (st < 80) {
            status = "occupied";
            pid = QString("P%1").arg(1000 + randGen.bounded(50));
            int idx = randGen.bounded(petNames.size());
            pname = petNames[idx];
            oname = ownerNames[randGen.bounded(ownerNames.size())];
            breed = breeds[randGen.bounded(breeds.size())];
        } else if (st < 90) {
            status = "cleaning";
        } else {
            status = "maintenance";
        }

        FosterCard *card = new FosterCard(101 + i, status, pid, pname, breed, oname, this);
        connect(card, &FosterCard::clicked, this, &FosterModule::onCardClicked);
        roomGrid->addWidget(card, i / cols, i % cols, Qt::AlignLeft | Qt::AlignTop);
    }
    // 强制每列等宽，避免卡片错位
    for (int c = 0; c < cols; ++c) {
        roomGrid->setColumnMinimumWidth(c, cardWidth);
    }

    updateStats();
}

void FosterModule::onCardClicked() {
    FosterCard *card = qobject_cast<FosterCard*>(sender());
    if (!card) return;

    FosterDetailDialog *dlg = new FosterDetailDialog(
        card->roomId(), card->status(), card->petId(), card->petName(), card->petBreed(), card->ownerName(), this->window()
    );

    // 连接业务信号
    connect(dlg, &FosterDetailDialog::requestBooking, this, [this](int roomId) {
        emit signal_openBooking(roomId);
    });
    connect(dlg, &FosterDetailDialog::requestHistory, this, [this](int roomId) {
        CustomMessageDialog::showSuccess(this, "历史追溯",
            QString("正在查询 Room %1 的往期寄养记录…\n（此功能将对接数据库模块）").arg(roomId));
    });

    dlg->exec();
    delete dlg;
}

void FosterModule::onToggleViewMode() {
    m_isTimelineMode = !m_isTimelineMode;
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (m_isTimelineMode) {
        btn->setText("切换看板视图");
        btn->setStyleSheet("QPushButton { background: #409eff; color: white; border: none; border-radius: 20px; font-weight: bold; }");
        CustomMessageDialog::showSuccess(this, "模式切换", "甘特图时间轴模式已激活。您现在可以横向观察未来一周的预约排期（Mock 功能）。");
    } else {
        btn->setText("切换甘特图视图 (Beta)");
        btn->setStyleSheet("QPushButton { background: #f0f2f5; color: #606266; border: 1px solid #dcdfe6; border-radius: 20px; font-weight: bold; }");
    }
}

void FosterModule::updateStats() {
    int total = 0, occ = 0, freeTab = 0, unusable = 0;
    for (int i = 0; i < roomGrid->count(); ++i) {
        FosterCard *c = qobject_cast<FosterCard*>(roomGrid->itemAt(i)->widget());
        if (!c) continue;
        total++;
        if (c->status() == "occupied") occ++;
        else if (c->status() == "cleaning" || c->status() == "maintenance") unusable++;
        else freeTab++;
    }
    totalRoomsLabel->setText(QString("%1 间").arg(total));
    occupiedLabel->setText(QString("%1 间").arg(occ));
    freeLabel->setText(QString("%1 间").arg(freeTab));
    cleaningLabel->setText(QString("%1 间").arg(unusable));
}
