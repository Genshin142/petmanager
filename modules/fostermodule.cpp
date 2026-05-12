#include "fostermodule.h"
#include "../utils/imageutils.h"
#include "addpetdialog.h"
#include "petdatamanager.h"
#include "globallightbox.h"
#include "custommessagedialog.h"
#include "compactcalendar.h"
#include "fosterganttmodel.h"
#include "fosterganttdelegate.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include "logisticsmanager.h"
#include "pricemanager.h"
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>
#include <QDateEdit>
#include "pettimelinewidget.h"
#include <QLabel>
#include <QPropertyAnimation>
#include <QMenu>
#include <QAction>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QPushButton>
#include <QTimer>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QProgressBar>
#include <QAbstractButton>
#include <QBuffer>
#include <QByteArray>

// 辅助函数：生成圆点形式的性别徽章 HTML (解决 Qt 不支持 HTML border-radius 的问题)
static QString getGenderBadgeHtml(const QString &gender) {
    if (gender != "公" && gender != "母") return "";
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(gender == "公" ? QColor("#409eff") : QColor("#f56c6c"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, 24, 24);
    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(16);
    p.setFont(f);
    p.drawText(pix.rect(), Qt::AlignCenter, gender == "公" ? "♂" : "♀");
    p.end();

    QByteArray ba;
    QBuffer bu(&ba);
    bu.open(QIODevice::WriteOnly);
    pix.save(&bu, "PNG");
    return QString("<img src='data:image/png;base64,%1' width='16' height='16' style='vertical-align: middle;'>").arg(QString(ba.toBase64()));
}
#include <QTextEdit>
#include <QComboBox>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QLineEdit>
#include <QCompleter>
#include <QButtonGroup>
#include <QMessageBox>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QTableView>
#include <QHeaderView>
#include "custom_calendar_edit.h"

// 全屏图片预览弹窗实现
FullImagePreviewDialog::FullImagePreviewDialog(const QStringList &paths, int startIndex, QWidget *parent) 
    : QDialog(parent), m_paths(paths), m_currentIndex(startIndex) 
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // 核心修复：虽然是 Dialog，但要强制覆盖整个主界面区域
    QWidget *root = this;
    while (root->parentWidget()) root = root->parentWidget();
    setGeometry(root->geometry());
    
    QVBoxLayout *mainL = new QVBoxLayout(this);
    mainL->setContentsMargins(0, 0, 0, 0);
    
    // 移除顶部操作栏，保持与头像放大一致的极简风格
    mainL->addStretch();
    
    m_imgL = new QLabel();
    m_imgL->setAlignment(Qt::AlignCenter);
    m_imgL->setStyleSheet("background: transparent;");
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(60); 
    shadow->setColor(QColor(0, 0, 0, 180));
    m_imgL->setGraphicsEffect(shadow);
    
    mainL->addWidget(m_imgL, 1, Qt::AlignCenter);
    
    mainL->addStretch();
    
    // 底部计数器
    m_counterL = new QLabel();
    m_counterL->setAlignment(Qt::AlignCenter);
    m_counterL->setStyleSheet("color: white; font-size: 15px; font-weight: bold; background: rgba(0,0,0,120); border-radius: 18px; padding: 6px 24px; margin-bottom: 40px;");
    mainL->addWidget(m_counterL, 0, Qt::AlignHCenter);

    updateImage();
}

void FullImagePreviewDialog::updateImage()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_paths.size()) return;
    
    QPixmap pix = ImageUtils::loadPixmap(m_paths[m_currentIndex]);
    if (pix.isNull()) return;
    
    // 关键修复：按主程序窗口的 80% 进行缩放
    QWidget *root = this;
    while (root->parentWidget()) root = root->parentWidget();
    QSize targetSize = root->size() * 0.8;
    
    m_imgL->setPixmap(pix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    m_counterL->setText(QString("%1 / %2").arg(m_currentIndex + 1).arg(m_paths.size()));
    m_counterL->setVisible(m_paths.size() > 1);
}

void FullImagePreviewDialog::mousePressEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    close(); // 与头像放大一致，点击任意位置关闭
}

void FullImagePreviewDialog::wheelEvent(QWheelEvent *event)
{
    if (m_paths.size() <= 1) return;
    
    if (event->angleDelta().y() > 0) {
        // 向上滚：上一张
        m_currentIndex = (m_currentIndex - 1 + m_paths.size()) % m_paths.size();
    } else {
        // 向下滚：下一张
        m_currentIndex = (m_currentIndex + 1) % m_paths.size();
    }
    updateImage();
}

void FullImagePreviewDialog::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 200));
}

// 可点击标签实现
ClickableLabel::ClickableLabel(QWidget *parent) : QLabel(parent) {
    setCursor(Qt::PointingHandCursor);
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent *e) {
    if (rect().contains(e->pos())) emit clicked();
}

// 屏蔽滚轮/键盘误触改值，只允许通过日历弹窗选择日期
class NoScrollDateEdit : public QDateEdit {
public:
    explicit NoScrollDateEdit(QWidget *parent = nullptr) : QDateEdit(parent) {
        setCalendarPopup(true);
        // 核心修正：禁止键盘直接输入文本，强制只能通过下拉日历选择
        if (lineEdit()) {
            lineEdit()->setReadOnly(true);
        }
    }
protected:
    void wheelEvent(QWheelEvent *e) override { e->ignore(); }
    void keyPressEvent(QKeyEvent *e) override {
        // 全面屏蔽改值类按键，仅保留 Tab 键用于切换焦点
        if (e->key() == Qt::Key_Tab || e->key() == Qt::Key_Backtab) {
            QDateEdit::keyPressEvent(e);
        } else {
            e->ignore();
        }
    }
};

// ========================
// FosterCard 实现
// ========================

FosterCard::FosterCard(int roomNo, const QString &status, const QString &roomType, const QString &petId, const QString &petName, const QString &petBreed, const QString &ownerName, QWidget *parent)
    : QFrame(parent), m_roomNo(roomNo), m_status(status), m_roomType(roomType), m_petId(petId), m_petName(petName), m_petBreed(petBreed), m_ownerName(ownerName)
{
    setFixedSize(210, 155);
    setCursor(Qt::PointingHandCursor);

    // --- 颜色定义 (对照用户图片) ---
    QString headerBg, headerText, cardBorder, bodyBg, statusIcon;
    if (m_roomType == "豪华房") {
        headerBg = "#311b92"; headerText = "white"; cardBorder = "#1a237e";
    } else if (m_roomType == "多宠房") {
        headerBg = "#ff7043"; headerText = "white"; cardBorder = "#d84315";
    } else {
        // 标准房
        headerBg = "#b2dfdb"; headerText = "#004d40"; cardBorder = "#80cbc4";
    }

    if (status == "occupied") {
        bodyBg = "#f0f7ff"; 
    } else if (status == "booked") {
        bodyBg = "#f9f0ff"; 
    } else if (status == "cleaning") {
        bodyBg = "#e0f7fa"; // 极浅青（清洁感）
    } else if (status == "maintenance") {
        bodyBg = "#fff3e0"; // 极浅橙（警告/维护感）
    } else {
        bodyBg = "white";
    }

    setStyleSheet(QString(
        "FosterCard { background: %1; border-radius: 14px; border: 2px solid %2; }"
    ).arg(bodyBg, cardBorder));

    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(12);
    m_shadow->setColor(QColor(0, 0, 0, 25));
    m_shadow->setOffset(0, 3);
    setGraphicsEffect(m_shadow);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // -- 顶部标题栏 --
    QFrame *header = new QFrame();
    header->setFixedHeight(38);
    header->setStyleSheet(QString(
        "QFrame { "
        "  background: %1; "
        "  border-top-left-radius: 12px; "
        "  border-top-right-radius: 12px; "
        "  border: none; "
        "} "
    ).arg(headerBg));
    
    QHBoxLayout *headerL = new QHBoxLayout(header);
    headerL->setContentsMargins(12, 0, 12, 0);
    
    QLabel *titleLabel = new QLabel(QString("Room %1  |  %2").arg(roomNo).arg(roomType));
    titleLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 800;").arg(headerText));
    headerL->addWidget(titleLabel);
    headerL->addStretch();

    // 状态标识
    if (status == "free") {
        QLabel *checkIcon = new QLabel("✅"); 
        checkIcon->setStyleSheet("font-size: 12px;");
        headerL->addWidget(checkIcon);
    }
    mainLayout->addWidget(header);

    // -- 内容区 --
    QFrame *content = new QFrame();
    content->setStyleSheet("background: transparent; border: none;");
    mainLayout->addWidget(content, 1);

    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(14, 10, 14, 12);
    layout->setSpacing(4);

    if (status == "occupied" && !petId.isEmpty()) {
        // ===== 已入住：头像 + 宠物名(主人) + ID =====
        QHBoxLayout *infoRow = new QHBoxLayout();
        infoRow->setContentsMargins(0, 0, 0, 0);
        infoRow->setSpacing(10);

        m_avatar = new QLabel();
        m_avatar->setFixedSize(64, 64);
        m_avatar->setAlignment(Qt::AlignCenter);
        m_avatar->setStyleSheet(
            "background: white; border-radius: 32px; border: 2px solid #e1e4e8;"
        );

        QVBoxLayout *textCol = new QVBoxLayout();
        textCol->setSpacing(2);
        textCol->setContentsMargins(0, 0, 0, 0);

        QLabel *nameLabel = new QLabel(petName);
        nameLabel->setStyleSheet("color: #2d3436; font-size: 17px; font-weight: bold;");

        PetInfo info = PetDataManager::instance()->getPet(petId);
        QString genderHtml = getGenderBadgeHtml(info.gender);
        QLabel *breedTag = new QLabel(QString("%1 %2").arg(petBreed, genderHtml));
        breedTag->setStyleSheet("color: #636e72; font-size: 12px;");

        QLabel *idTag = new QLabel("ID: " + petId);
        idTag->setStyleSheet("color: #b2bec3; font-size: 11px; font-weight: bold;");

        textCol->addWidget(nameLabel);
        textCol->addWidget(breedTag);
        textCol->addWidget(idTag);

        // 加载真实头像 (逻辑保持)
        QPixmap srcPix = PetDataManager::instance()->getPetPixmap(petId);
        if (!srcPix.isNull()) {
            QPixmap target(64, 64);
            target.fill(Qt::transparent);
            QPainter p(&target);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            QPainterPath path;
            path.addEllipse(0, 0, 64, 64);
            p.setClipPath(path);
            
            QPixmap scaled = srcPix.scaled(64, 64, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            int x = (64 - scaled.width()) / 2;
            int y = (64 - scaled.height()) / 2;
            p.drawPixmap(x, y, scaled);
            m_avatar->setPixmap(target);
            m_avatar->setProperty("avatarPath", info.avatarPath); // 确保点击放大有效
        } else {
            m_avatar->setPixmap(QPixmap(":/images/load_img.jpg").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        infoRow->addWidget(m_avatar);
        infoRow->addLayout(textCol, 1);
        layout->addLayout(infoRow);

        QDate sDate = QDate::fromString(info.fosterStartTime, "yyyy-MM-dd");
        int stayDays = sDate.isValid() ? qMax(1, (int)sDate.daysTo(QDate::currentDate()) + 1) : 1;
        QLabel *dayLabel = new QLabel(QString("已入住 %1 天").arg(stayDays));
        dayLabel->setStyleSheet("color: #95a5a6; font-size: 11px;");
        dayLabel->setAlignment(Qt::AlignRight);
        layout->addStretch();
        layout->addWidget(dayLabel);

    } else if (status == "cleaning" || status == "maintenance") {
        bool isMaint = (status == "maintenance");
        QLabel *statusTitle = new QLabel(isMaint ? "维护中" : "清洁中");
        
        // 明显的区分色：清洁用青色，维护用橙色
        QString titleColor = isMaint ? "#e65100" : "#00838f";
        QString subtitleColor = isMaint ? "#ef6c00" : "#0097a7";
        
        statusTitle->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 900;").arg(titleColor));
        statusTitle->setAlignment(Qt::AlignCenter);
        layout->addStretch();
        layout->addWidget(statusTitle);

        QLabel *reasonLabel = new QLabel(isMaint ? "设施报修排查中" : "例行消毒与深度清洁");
        reasonLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(subtitleColor));
        reasonLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(reasonLabel);
        layout->addStretch();

    } else if (status == "booked") {
        QHBoxLayout *infoRow = new QHBoxLayout();
        infoRow->setContentsMargins(0, 0, 0, 0);
        infoRow->setSpacing(10);

        m_avatar = new QLabel();
        m_avatar->setFixedSize(60, 60);
        m_avatar->setStyleSheet("background: #f8f9fa; border-radius: 30px; border: 1.5px dashed #d1d5da;");
        
        PetInfo info;
        if (!petId.isEmpty()) {
            info = PetDataManager::instance()->getPet(petId);
            QPixmap srcPix = PetDataManager::instance()->getPetPixmap(petId);
            if (!srcPix.isNull()) {
                QPixmap target(60, 60);
                target.fill(Qt::transparent);
                QPainter p(&target);
                p.setRenderHint(QPainter::Antialiasing);
                p.setRenderHint(QPainter::SmoothPixmapTransform);
                QPainterPath path;
                path.addEllipse(0, 0, 60, 60);
                p.setClipPath(path);
                QPixmap scaled = srcPix.scaled(60, 60, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                int x = (60 - scaled.width()) / 2;
                int y = (60 - scaled.height()) / 2;
                p.drawPixmap(x, y, scaled);
                
                p.setClipping(false);
                p.setPen(QPen(QColor(179, 127, 235, 180), 1.5, Qt::DashLine));
                p.drawEllipse(1, 1, 58, 58);
                
                m_avatar->setPixmap(target);
                m_avatar->setStyleSheet("background: transparent; border: none;");
                m_avatar->setProperty("avatarPath", info.avatarPath);
            }
        }
        
        QVBoxLayout *textCol = new QVBoxLayout();
        textCol->setSpacing(1);
        textCol->setContentsMargins(0, 0, 0, 0);
        
        QLabel *nameLabel = new QLabel(petName.isEmpty() ? "待定预约" : petName);
        nameLabel->setStyleSheet("color: #6f42c1; font-size: 16px; font-weight: bold;");
        
        QString genderHtml = info.id.isEmpty() ? "" : getGenderBadgeHtml(info.gender);
        QLabel *breedTag = new QLabel(QString("%1 %2").arg(m_petBreed, genderHtml));
        breedTag->setStyleSheet("color: #636e72; font-size: 11px;");

        QLabel *idTag = new QLabel("ID: " + petId);
        idTag->setStyleSheet("color: #b2bec3; font-size: 10px; font-weight: bold;");
        
        textCol->addWidget(nameLabel);
        textCol->addWidget(breedTag);
        textCol->addWidget(idTag);
        
        infoRow->addWidget(m_avatar);
        infoRow->addLayout(textCol, 1);
        layout->addLayout(infoRow);

        QLabel *statusLabel = new QLabel("房位已预留");
        statusLabel->setStyleSheet("color: #8a63d2; font-size: 11px; font-style: italic;");
        statusLabel->setAlignment(Qt::AlignRight);
        layout->addStretch();
        layout->addWidget(statusLabel);

    } else {
        // 空闲
        QLabel *freeText = new QLabel("空闲");
        freeText->setStyleSheet("color: #2c3e50; font-size: 16px; font-weight: 800;");
        freeText->setAlignment(Qt::AlignCenter);
        layout->addStretch();
        layout->addWidget(freeText);

        QLabel *bookLabel = new QLabel("可预约入住");
        bookLabel->setStyleSheet("color: #7f8c8d; font-size: 12px;");
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

static QHBoxLayout* makeInfoRow(const QString &label, const QString &value, const QString &valueColor = "#303133") {
    QHBoxLayout *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    QLabel *lbl = new QLabel(label);
    lbl->setStyleSheet("color: #909399; font-size: 13px; min-width: 70px;");
    QLabel *val = new QLabel(value);
    val->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;").arg(valueColor));
    row->addWidget(lbl);
    row->addWidget(val);
    row->addStretch();
    return row;
}

void FosterCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 判定是否点击了头像区域
        if (m_avatar && m_avatar->geometry().contains(event->pos())) {
            QString path = m_avatar->property("avatarPath").toString();
            if (!path.isEmpty()) {
                (new FullImagePreviewDialog(QStringList{path}, 0, this->window()))->show();
            } else {
                (new AvatarZoomDialog(m_avatar->text(), this->window()))->show();
            }
        } else {
            emit clicked();
        }
    }
    QFrame::mousePressEvent(event);
}

// ========================
// FosterDetailDialog 实现 (Legacy)
// ========================

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
    } else if (status == "booked") {
        buildBookedView(contentLayout, roomId, petId, petName, petBreed, ownerName);
    } else if (status == "free") {
        buildFreeView(contentLayout, roomId);
    } else {
        buildCleaningView(contentLayout, roomId);
    }

    mainLayout->addWidget(container);
    if (QWidget *p = parentWidget()) resize(p->size());
}

void FosterDetailDialog::buildBookedView(QVBoxLayout *layout, int roomId, const QString &petId, const QString &petName, const QString &petBreed, const QString &ownerName) {
    // ---- 顶部状态栏 ----
    QHBoxLayout *headerRow = new QHBoxLayout();
    QLabel *roomLabel = new QLabel(QString("Room %1").arg(roomId));
    roomLabel->setStyleSheet("color: #303133; font-size: 24px; font-weight: bold;");
    
    QLabel *statusBadge = new QLabel("● 已预约");
    statusBadge->setStyleSheet(
        "color: #722ED1; background: rgba(114, 46, 209, 0.1); font-size: 14px; font-weight: bold; "
        "padding: 6px 16px; border-radius: 14px;"
    );
    statusBadge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel *backBtn = new QLabel("返回");
    backBtn->setFixedSize(60, 28);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setAlignment(Qt::AlignCenter);
    backBtn->setProperty("isBack", true);
    backBtn->installEventFilter(this);
    backBtn->setStyleSheet(
        "QLabel { "
        "  border: 1px solid #DCDFE6; border-radius: 14px; "
        "  color: #606266; background: white; font-size: 12px; font-weight: bold; "
        "} "
        "QLabel:hover { "
        "  background: #F5F7FA; border-color: #409EFF; color: #409EFF; "
        "}"
    );

    headerRow->addWidget(roomLabel);
    headerRow->addStretch();
    headerRow->addWidget(statusBadge);
    headerRow->addSpacing(10);
    headerRow->addWidget(backBtn);
    layout->addLayout(headerRow);
    layout->addSpacing(20);

    // ---- 宠物信息区 (紫色主题) ----
    QFrame *petFrame = new QFrame();
    petFrame->setStyleSheet("QFrame { background: #F9F0FF; border-radius: 10px; border: 1px solid #D3ADF7; } QLabel { border: none; background: transparent; }");
    QHBoxLayout *petRow = new QHBoxLayout(petFrame);
    petRow->setContentsMargins(14, 12, 14, 12);

    QLabel *avatar = new QLabel();
    avatar->setFixedSize(72, 72);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setCursor(Qt::PointingHandCursor);
    avatar->setStyleSheet("font-size: 36px; background: white; border-radius: 36px; border: 2px solid #B37FEB;");
    
    // 点击详情头像也能放大
    avatar->installEventFilter(this);
    avatar->setProperty("isAvatar", true);
    
    QVBoxLayout *petInfo = new QVBoxLayout();
    petInfo->setSpacing(8);
    petInfo->setAlignment(Qt::AlignVCenter);

    // 第一行：名字 + 品种标签
    QHBoxLayout *nameRow = new QHBoxLayout();
    nameRow->setSpacing(12);
    QLabel *nameLabel = new QLabel(petName.isEmpty() ? "预约宠物" : petName);
    nameLabel->setStyleSheet("color: #531DAB; font-size: 22px; font-weight: bold;");
    
    PetInfo info = PetDataManager::instance()->getPet(petId);
    QString genderHtml = getGenderBadgeHtml(info.gender);
    QLabel *breedLabel = new QLabel(QString("%1 · %2").arg(petBreed.isEmpty() ? "预约待入" : petBreed, genderHtml));
    breedLabel->setStyleSheet(
        "color: #722ED1; font-size: 13px; font-weight: bold; background: rgba(114, 46, 209, 0.1); "
        "border-radius: 6px; padding: 4px 12px;"
    );
    breedLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    nameRow->addWidget(nameLabel);
    nameRow->addWidget(breedLabel);
    nameRow->addStretch();

    QLabel *idLabel = new QLabel(QString("ID: %1").arg(petId));
    idLabel->setStyleSheet("color: #722ED1; font-size: 15px; font-weight: bold;");
    
    petInfo->addLayout(nameRow);
    petInfo->addWidget(idLabel);

    // 加载真实头像
    QPixmap srcPix = PetDataManager::instance()->getPetPixmap(petId);
    if (!srcPix.isNull()) {
        avatar->setText("");
        QPixmap target(72, 72);
        target.fill(Qt::transparent);
        QPainter p(&target);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath path;
        path.addEllipse(0, 0, 72, 72);
        p.setClipPath(path);
        
        QPixmap scaled = srcPix.scaled(72, 72, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int x = (72 - scaled.width()) / 2;
        int y = (72 - scaled.height()) / 2;
        p.drawPixmap(x, y, scaled);
        
        // 手动绘制内边框
        p.setClipping(false);
        // 动态选择边框颜色：紫色(Booked) 或 蓝色(Occupied)
        QColor borderColor = (m_status == "booked") ? QColor(179, 127, 235, 180) : QColor(74, 144, 226, 150);
        p.setPen(QPen(borderColor, 2));
        p.drawEllipse(1, 1, 70, 70);

        avatar->setPixmap(target);
        avatar->setStyleSheet("background: transparent; border: none;");
        avatar->setProperty("avatarPath", info.avatarPath);
    }

    petRow->addWidget(avatar);
    petRow->addSpacing(10);
    petRow->addLayout(petInfo);
    petRow->addStretch();
    layout->addWidget(petFrame);
    layout->addSpacing(18);

    // ---- 预约详情 ----
    layout->addLayout(makeInfoRow("预约客户", ownerName.isEmpty() ? "预约客户 (138-xxxx-xxxx)" : ownerName));
    layout->addSpacing(8);
    layout->addLayout(makeInfoRow("预计入住", "明日 10:00 (2026-04-22)", "#722ED1"));
    layout->addSpacing(8);
    layout->addLayout(makeInfoRow("预约备注", "客户要求提前进行房间紫外线消毒", "#9254DE"));
    layout->addSpacing(25);

    // ---- 操作按钮 ----
    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *checkinBtn = makeActionBtn("办理入住", "#722ed1", "#9254de");
    QPushButton *cancelBtn = makeActionBtn("取消预约", "#f0f2f5", "#e9eaec");
    cancelBtn->setStyleSheet(
        "QPushButton { background: #f0f2f5; color: #606266; border: 1px solid #dcdfe6; border-radius: 8px; font-weight: bold; font-size: 13px; padding: 0 16px; }"
        "QPushButton:hover { background: #e9eaec; }"
    );
    
    connect(checkinBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnRow->addWidget(checkinBtn, 1);
    btnRow->addSpacing(10);
    btnRow->addWidget(cancelBtn, 1);
    layout->addLayout(btnRow);
}

void FosterDetailDialog::buildOccupiedView(QVBoxLayout *layout, int roomId, const QString &petId, const QString &petName, const QString &petBreed, const QString &ownerName) {
    // ---- 顶部状态栏：Room (左) + 状态胶囊 (右) ----
    QHBoxLayout *headerRow = new QHBoxLayout();
    QLabel *roomLabel = new QLabel(QString("Room %1").arg(roomId));
    roomLabel->setStyleSheet("color: #303133; font-size: 24px; font-weight: bold;");
    
    QLabel *statusBadge = new QLabel("● 已入住");
    statusBadge->setStyleSheet(
        "color: #4A90E2; background: rgba(74, 144, 226, 0.1); font-size: 14px; font-weight: bold; "
        "padding: 6px 16px; border-radius: 14px;"
    );
    statusBadge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel *backBtn = new QLabel("返回");
    backBtn->setFixedSize(60, 28);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setAlignment(Qt::AlignCenter);
    backBtn->setProperty("isBack", true);
    backBtn->installEventFilter(this);
    backBtn->setStyleSheet(
        "QLabel { "
        "  border: 1px solid #DCDFE6; border-radius: 14px; "
        "  color: #606266; background: white; font-size: 12px; "
        "  font-weight: bold; "
        "} "
        "QLabel:hover { "
        "  background: #F5F7FA; border-color: #409EFF; color: #409EFF; "
        "}"
    );
    
    headerRow->addWidget(roomLabel);
    headerRow->addStretch();
    headerRow->addWidget(statusBadge);
    headerRow->addSpacing(10);
    headerRow->addWidget(backBtn);
    layout->addLayout(headerRow);
    layout->addSpacing(20);

    // ---- 宠物信息区 ----
    QFrame *petFrame = new QFrame();
    petFrame->setStyleSheet("QFrame { background: #f0f7ff; border-radius: 10px; border: 1px solid #d9ecff; } QLabel { border: none; background: transparent; }");
    QHBoxLayout *petRow = new QHBoxLayout(petFrame);
    petRow->setContentsMargins(14, 12, 14, 12);

    QLabel *avatar = new QLabel();
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
    nameLabel->setStyleSheet("color: #303133; font-size: 22px; font-weight: bold;");
    
    PetInfo info = PetDataManager::instance()->getPet(petId);
    QString genderHtml = getGenderBadgeHtml(info.gender);
    QLabel *breedLabel = new QLabel(QString("%1 · %2").arg(petBreed.isEmpty() ? "通用/未知" : petBreed, genderHtml));
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

    // 加载真实头像
    QPixmap srcPix = PetDataManager::instance()->getPetPixmap(petId);
    if (!srcPix.isNull()) {
        avatar->setText("");
        QPixmap target(72, 72);
        target.fill(Qt::transparent);
        QPainter p(&target);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath path;
        path.addEllipse(0, 0, 72, 72);
        p.setClipPath(path);
        
        QPixmap scaled = srcPix.scaled(72, 72, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int x = (72 - scaled.width()) / 2;
        int y = (72 - scaled.height()) / 2;
        p.drawPixmap(x, y, scaled);
        avatar->setPixmap(target);
        avatar->setProperty("avatarPath", info.avatarPath);
    }

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
    btnRow->setSpacing(10);

    QPushButton *historyBtn = makeActionBtn("寄养全纪录", "#f0f2f5", "#e9eaec");
    historyBtn->setStyleSheet(
        "QPushButton { background: #f0f2f5; color: #606266; border: 1px solid #dcdfe6; border-radius: 8px; font-weight: bold; font-size: 13px; padding: 0 16px; }"
        "QPushButton:hover { background: #e9eaec; color: #409eff; border-color: #b3d8ff; }"
    );
    connect(historyBtn, &QPushButton::clicked, this, [this, roomId, petId, petName]() { emit requestHistory(roomId, petId, petName); });
    
    QPushButton *addMediaBtn = makeActionBtn("录入影像/日志", "#67c23a", "#85ce61");
    connect(addMediaBtn, &QPushButton::clicked, this, [this, petId]() {
        MediaUploadDialog dlg(petId, this->window());
        
        // 关键：连接信号实现实时同步更新右侧查看面板
        connect(&dlg, &MediaUploadDialog::recordAdded, this, [this](const QString &cat, const QString &log, const QStringList &imgs) {
            // 获取当前日期时间
            QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
            
            // 1. 更新时间轴日志
            PetActivityLog newLog;
            newLog.time = timeStr;
            newLog.type = cat;
            newLog.remark = log.isEmpty() ? QString("进行了%1活动").arg(cat) : log;
            newLog.icon = "";
            
            // 2. 更新影像留档 (Mock 同步到 UI)
            if (!imgs.isEmpty()) {
                PetMedia newMedia;
                newMedia.type = "image";
                newMedia.title = cat + "记录";
                newMedia.urls = imgs;
                for(int i=0; i<imgs.size(); ++i) newMedia.timestamps << timeStr;
                
                // 获取当前正在展示的详情组件
                // 如果后续需要同步更新右侧预览区域，可以在此处添加逻辑
            }
        });
        
        dlg.exec();
    });

    QPushButton *checkoutBtn = makeActionBtn("办理退房", "#f56c6c", "#feb2b2");
    checkoutBtn->setStyleSheet(
        "QPushButton { background: #fff1f0; color: #f5222d; border: 1px solid #ffa39e; border-radius: 8px; font-weight: bold; font-size: 13px; padding: 0 16px; }"
        "QPushButton:hover { background: #f5222d; color: white; border-color: #f5222d; }"
    );
    connect(checkoutBtn, &QPushButton::clicked, this, [this, petId, roomId]() {
        if (CustomMessageDialog::confirm(this, "退房确认", "确定要为该宠物办理退房吗？房位将被释放并生成清结算订单。")) {
            PetInfo info = PetDataManager::instance()->getPet(petId);
            if (!info.id.isEmpty()) {
                // 1. 生成清结算订单
                OrderInfo order;
                order.id = "ORD" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
                order.sourceModule = "Boarding";
                order.relatedId = QString("ROOM-%1-%2").arg(roomId).arg(QDate::currentDate().toString("yyyyMMdd"));
                order.petId = petId;
                order.petName = info.name;
                order.memberName = info.ownerName;
                
                QDate sDate = QDate::fromString(info.fosterStartTime, "yyyy-MM-dd");
                int stayDays = sDate.isValid() ? qMax(1, (int)sDate.daysTo(QDate::currentDate()) + 1) : 1;
                double dailyPrice = PriceManager::instance()->calculateFinalAmount("标准间", info.breed);
                order.totalAmount = dailyPrice * stayDays;
                order.finalAmount = order.totalAmount;
                order.itemDetails = QString("寄养服务 (%1天)").arg(stayDays);
                order.status = "Unpaid";
                order.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
                PetDataManager::instance()->addOrder(order);

                // 2. 更新状态与记录
                info.status = "离店";
                info.roomNo = "";
                PetDataManager::instance()->updatePet(info);
                
                PetActivityLog log;
                log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
                log.type = "离店";
                log.icon = "";
                log.remark = QString("办理退房，结算金额: ¥%1。").arg(order.totalAmount, 0, 'f', 2);
                log.operatorName = "系统管理员";
                PetDataManager::instance()->addActivityLog(petId, log);

                CustomMessageDialog::showSuccess(this, "退房成功", "已成功办理退房，清结算单已发送至订单中心。");
                accept(); 
            }
        }
    });


    btnRow->addWidget(historyBtn);
    btnRow->addWidget(addMediaBtn);
    btnRow->addStretch();
    btnRow->addWidget(checkoutBtn);
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
    roomLabel->setStyleSheet("color: #303133; font-size: 24px; font-weight: bold;");

    QLabel *backBtn = new QLabel("返回");
    backBtn->setFixedSize(60, 28);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setAlignment(Qt::AlignCenter);
    backBtn->setProperty("isBack", true);
    backBtn->installEventFilter(this);
    backBtn->setStyleSheet(
        "QLabel { "
        "  border: 1px solid #DCDFE6; border-radius: 14px; "
        "  color: #606266; background: white; font-size: 12px; "
        "  font-weight: bold; "
        "} "
        "QLabel:hover { "
        "  background: #F5F7FA; border-color: #409EFF; color: #409EFF; "
        "}"
    );
    
    headerRow->addWidget(roomLabel);
    headerRow->addStretch();
    headerRow->addWidget(statusBadge);
    headerRow->addSpacing(10);
    headerRow->addWidget(backBtn);
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

    QLabel *avatar = new QLabel();
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
    nameLabel->setStyleSheet("color: #303133; font-size: 20px; font-weight: bold;");

    QLabel *breedLabel = new QLabel("金毛巡回犬");
    breedLabel->setStyleSheet("color: #67c23a; font-size: 13px; font-weight: bold; background: #f0f9eb; border-radius: 6px; padding: 4px 12px;");
    breedLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    nameRow->addWidget(nameLabel);
    nameRow->addWidget(breedLabel);
    nameRow->addStretch();

    // 第二行：ID
    QLabel *idLabel = new QLabel("ID: P1012");
    idLabel->setStyleSheet("color: #909399; font-size: 14px; ");
    
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
    QPushButton *bookBtn = makeActionBtn("立即排单入住", "#67c23a", "#85ce61");
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

    QLabel *backBtn = new QLabel("返回");
    backBtn->setFixedSize(60, 28);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setAlignment(Qt::AlignCenter);
    backBtn->setProperty("isBack", true);
    backBtn->installEventFilter(this);
    backBtn->setStyleSheet(
        "QLabel { "
        "  border: 1px solid #DCDFE6; border-radius: 14px; "
        "  color: #606266; background: white; font-size: 12px; "
        "  font-weight: bold; "
        "} "
        "QLabel:hover { "
        "  background: #F5F7FA; border-color: #409EFF; color: #409EFF; "
        "}"
    );

    QLabel *statusBadge = new QLabel(isMaint ? "● 维护中" : "● 清洁中");
    statusBadge->setStyleSheet(QString(
        "color: white; background: %1; font-size: 12px; font-weight: bold; "
        "padding: 4px 12px; border-radius: 10px;"
    ).arg(isMaint ? "#FF9C6E" : "#FFA940"));
    statusBadge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QLabel *roomLabel = new QLabel(QString("Room %1").arg(roomId));
    roomLabel->setStyleSheet("color: #303133; font-size: 20px; font-weight: bold;");
    
    headerRow->addWidget(roomLabel);
    headerRow->addStretch();
    headerRow->addWidget(statusBadge);
    headerRow->addSpacing(10);
    headerRow->addWidget(backBtn);
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
    maintTitle->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold;").arg(isMaint ? "#873800" : "#7c4a00"));
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

// ========================
// HistoryItemCard 实现
// ========================
HistoryItemCard::HistoryItemCard(const QString &petName, const QString &roomInfo, const QString &period, const QString &status, QWidget *parent) 
    : QFrame(parent) {
    setObjectName("HistoryItemCard");
    setFixedSize(300, 95); // 宽度从 280 增加到 300，高度略微增加以适应长文本
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("QFrame#HistoryItemCard { background: white; border-radius: 12px; border: 1px solid #f0f2f5; } QLabel { background: transparent; border: none; }");
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 15));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 0, 15, 0);
    layout->setSpacing(12);

    // 图标
    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(36, 36);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 16px; color: #409eff; background: #f0f7ff; border-radius: 18px;");
    
    // 信息区
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(1);
    infoLayout->setAlignment(Qt::AlignVCenter);
    
    QLabel *nameLabel = new QLabel(petName);
    nameLabel->setStyleSheet("color: #303133; font-size: 14px; font-weight: bold;");
    
    QLabel *detailLabel = new QLabel(QString("%1\n%2").arg(roomInfo, period));
    detailLabel->setStyleSheet("color: #909399; font-size: 11px;");
    
    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(detailLabel);
    
    // 状态胶囊 (小)
    QLabel *statusLabel = new QLabel(status);
    statusLabel->setFixedSize(50, 20);
    statusLabel->setAlignment(Qt::AlignCenter);
    QString statusStyle = "border-radius: 10px; font-size: 10px; font-weight: bold; border: 1px solid; ";
    if (status == "进行中") {
        statusStyle += "color: #409eff; background: #ecf5ff; border-color: #d9ecff;";
    } else {
        statusStyle += "color: #67c23a; background: #f0f9eb; border-color: #e1f3d8;";
    }
    statusLabel->setStyleSheet(statusStyle);

    layout->addWidget(iconLabel);
    layout->addLayout(infoLayout, 1);
    layout->addWidget(statusLabel);
}

void HistoryItemCard::setSelected(bool selected) {
    if (selected) {
        setStyleSheet("QFrame#HistoryItemCard { background: #f0f7ff; border: 2px solid #409eff; border-radius: 12px; } QLabel { background: transparent; border: none; }");
    } else {
        setStyleSheet("QFrame#HistoryItemCard { background: white; border: 1px solid #f0f2f5; border-radius: 12px; } QLabel { background: transparent; border: none; }");
    }
}

void HistoryItemCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) emit clicked();
    QFrame::mousePressEvent(event);
}

// ========================
// 寄养详情增强组件实现
// ========================

SummaryCard::SummaryCard(QWidget *parent) : QFrame(parent) {
    setObjectName("SummaryCard");
    setFixedHeight(100);
    setStyleSheet("QFrame#SummaryCard { background: #fdfdfd; border: 1px solid #edf0f5; border-radius: 12px; } QLabel { background: transparent; border: none; }");
    
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 10, 15, 10);
    layout->setSpacing(20); // 缩小间距以适应内容

    auto addItem = [&](const QString &title, const QString &val, const QString &icon) {
        QVBoxLayout *v = new QVBoxLayout();
        v->setSpacing(5);
        QLabel *tL = new QLabel(icon + " " + title);
        tL->setStyleSheet("color: #909399; font-size: 12px; font-weight: bold;");
        QLabel *vL = new QLabel(val);
        vL->setStyleSheet("color: #303133; font-size: 15px; font-weight: bold;");
        v->addWidget(tL);
        v->addWidget(vL);
        layout->addLayout(v);
    };

    addItem("专属管家", "店员小利", "");
    addItem("体重监测", "4.5kg → 4.6kg", "⚖️");
    addItem("健康评价", "精神极佳，食欲旺盛", "");
    addItem("特殊消耗", "使用了店面备用粮", "");
    
    layout->addStretch();
}

// ========================
// MediaGallery 实现
// ========================

MediaGallery::MediaGallery(QWidget *parent) : QWidget(parent) {
    // 移除固定高度限制，让其在 850x750 的弹窗中能自由伸展
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void MediaGallery::setMedia(const QString &petId, const QList<PetMedia> &mediaList) {
    m_petId = petId;
    // 1. 定义固定分类并预填充
    QStringList fixedTitles = {"运动", "洗澡", "喂食", "睡觉", "入住存证", "离店存证"};
    QMap<QString, PetMedia> groups;
    for (const QString &title : fixedTitles) {
        PetMedia m; m.title = title; m.type = "image";
        groups[title] = m;
    }

    // 2. 将传入影像映射到固定分类 (支持同义词映射)
    for (const auto &m : mediaList) {
        QString targetTitle = m.title;
        // 移除常见的“记录”后缀以便归类
        if (targetTitle.endsWith("记录")) targetTitle.chop(2);
        if (targetTitle.endsWith("照片")) targetTitle.chop(2);
        if (targetTitle.endsWith("存证")) targetTitle.chop(0); // 存证保留

        if (targetTitle == "洗护") targetTitle = "洗澡";
        if (targetTitle == "投喂") targetTitle = "喂食";
        if (targetTitle == "运动记录") targetTitle = "运动";
        if (targetTitle == "入住存证照片") targetTitle = "入住存证";
        if (targetTitle == "离店存证照片") targetTitle = "离店存证";

        if (groups.contains(targetTitle)) {
            groups[targetTitle].urls.append(m.urls);
            groups[targetTitle].timestamps.append(m.timestamps);
        } else {
            groups[m.title] = m;
        }
    }

    // 3. 彻底清空现有布局
    QLayoutItem *child;
    while (layout()->count() > 0 && (child = layout()->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->setParent(nullptr);
            delete child->widget();
        }
        delete child;
    }

    // 4. 直接在主布局上构建网格
    QGridLayout *grid = new QGridLayout();
    grid->setContentsMargins(10, 10, 10, 10);
    grid->setSpacing(40); // 增加卡片间距

    int idx = 0;
    QStringList displayOrder = fixedTitles;
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        if (!displayOrder.contains(it.key())) displayOrder << it.key();
    }

    for (const QString &title : displayOrder) {
        const PetMedia &m = groups[title];
        QPushButton *card = new QPushButton();
        card->setFixedSize(220, 160); 
        card->setCursor(Qt::PointingHandCursor);
        
        QString cover = m.urls.isEmpty() ? "" : m.urls.first();
        QString style = QString(
            "QPushButton { "
            "  background-color: #f8f9fb; "
            "  border-radius: 20px; "
            "  border: 1px solid #f1f5f9; "
            "  background-image: url(%1); "
            "  background-position: center; "
            "  background-repeat: no-repeat; "
            "  background-size: cover; " // 核心修复：全覆盖，不再留白
            "} "
            "QPushButton:hover { "
            "  border-color: #409eff; "
            "  background-color: #f0f7ff; "
            "}").arg(cover);
        card->setStyleSheet(style);

        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(0);
        
        cl->addStretch();

        // 渐变文字背景，增强专业感
        QLabel *titleBar = new QLabel(QString("%1 (%2)").arg(title, QString::number(m.urls.size())));
        titleBar->setFixedHeight(44);
        titleBar->setAlignment(Qt::AlignCenter);
        titleBar->setStyleSheet(
            "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(0,0,0,0), stop:1 rgba(0,0,0,180)); "
            "color: white; font-size: 14px; font-weight: bold; "
            "border-bottom-left-radius: 18px; border-bottom-right-radius: 18px;"
        );
        cl->addWidget(titleBar);

        connect(card, &QPushButton::clicked, this, [this, m]() {
            if (m.urls.isEmpty()) return;
            emit categorySelected(m); // 改为发出信号，由父窗口处理视图切换
        });

        grid->addWidget(card, idx / 3, idx % 3, Qt::AlignCenter);
        idx++;
    }
    
    // 如果只有一排，增加底部拉伸防止卡片浮空
    if (idx <= 3) grid->setRowStretch(1, 1);
    
    static_cast<QVBoxLayout*>(layout())->addLayout(grid);
    static_cast<QVBoxLayout*>(layout())->addStretch(); // 整体顶对齐
}

// ========================
// MediaDetailDialog 实现
// ========================
MediaDetailDialog::MediaDetailDialog(const QString &petId, const PetMedia &media, QWidget *parent) 
    : QDialog(parent), m_petId(petId) 
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    QFrame *container = new QFrame();
    container->setFixedSize(800, 600);
    container->setStyleSheet("background: white; border-radius: 24px;");
    
    QVBoxLayout *content = new QVBoxLayout(container);
    content->setContentsMargins(0, 0, 0, 0);

    // 顶部条
    QWidget *topBar = new QWidget();
    topBar->setFixedHeight(60);
    QHBoxLayout *topL = new QHBoxLayout(topBar);
    topL->setContentsMargins(25, 0, 25, 0);
    
    QLabel *titleL = new QLabel(media.title);
    titleL->setStyleSheet("font-size: 20px; font-weight: bold; color: #18181B;");
    
    QPushButton *backBtn = new QPushButton("返回分类列表");
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setFixedHeight(36); // 增加高度
    backBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #f0f2f5; "
        "  color: #303133; " // 默认显示深色文字
        "  border: 1px solid #dcdfe6; "
        "  border-radius: 18px; "
        "  font-size: 14px; "
        "  "
        "  padding: 0 15px; " // 增加左右间距
        "} "
        "QPushButton:hover { "
        "  background-color: #409eff; "
        "  color: white; "
        "  border-color: #409eff; "
        "}"
    );
    connect(backBtn, &QPushButton::clicked, this, &QDialog::accept);

    topL->addWidget(titleL);
    topL->addStretch();
    topL->addWidget(backBtn);
    content->addWidget(topBar);

    // 主内容区：多图画廊网格
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: #f8f9fb; border-bottom-left-radius: 24px; border-bottom-right-radius: 24px; }");
    
    QWidget *gallery = new QWidget();
    gallery->setStyleSheet("background: transparent;");
    QGridLayout *grid = new QGridLayout(gallery);
    grid->setContentsMargins(25, 10, 25, 25);
    grid->setSpacing(20);
    
    for (int i = 0; i < media.urls.size(); ++i) {
        QWidget *itemWidget = new QWidget();
        itemWidget->setFixedSize(350, 292); // 260 (图) + 32 (时间) = 292
        QVBoxLayout *l = new QVBoxLayout(itemWidget);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
        l->setAlignment(Qt::AlignTop); // 确保内部紧凑堆叠

        ClickableLabel *imgL = new ClickableLabel();
        imgL->setFixedSize(350, 260); // 强制固定尺寸，防止布局拉伸
        QPixmap pix(media.urls[i]);
        if (!pix.isNull()) {
            imgL->setPixmap(pix.scaled(350, 260, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        imgL->setStyleSheet("background: white; border-top-left-radius: 12px; border-top-right-radius: 12px; border: 1px solid #ebeef5; padding: 0px; border-bottom: none;"); // 移除 padding
        imgL->setAlignment(Qt::AlignCenter);
        
        imgL->setProperty("path", media.urls[i]);

        // 删除按钮 overlay
        QPushButton *delBtn = new QPushButton("X", imgL);
        delBtn->setFixedSize(26, 26);
        delBtn->setCursor(Qt::PointingHandCursor);
        delBtn->setToolTip("删除此照片");
        delBtn->setStyleSheet(
            "QPushButton { background: #ff4d4f; color: white; border-radius: 13px; font-weight: bold; border: 2px solid white; font-size: 14px; text-align: center; padding: 0; } "
            "QPushButton:hover { background: #ff7875; }"
        );
        // 在显示后进行定位，或者直接使用右对齐偏移
        delBtn->move(350 - 32, 6);

        QString currentUrl = media.urls[i];
        QString currentTitle = media.title;
        connect(delBtn, &QPushButton::clicked, this, [this, currentTitle, currentUrl, itemWidget]() {
            if (CustomMessageDialog::confirm(this, "删除确认", "确定要从档案中永久删除这张照片吗？")) {
                PetDataManager::instance()->deleteMediaPhoto(m_petId, currentTitle, currentUrl);
                itemWidget->hide();
                itemWidget->deleteLater();
            }
        });
        
        int currentIndex = i;
        connect(imgL, &ClickableLabel::clicked, this, [this, currentIndex, media]() {
            // 过滤掉已隐藏的照片路径
            QStringList activeUrls;
            int activeIndex = 0;
            QList<ClickableLabel*> labels = this->findChildren<ClickableLabel*>();
            QString targetUrl = media.urls[currentIndex];
            
            for (auto l : labels) {
                QString p = l->property("path").toString();
                if (!p.isEmpty() && l->isVisible()) {
                    if (p == targetUrl) activeIndex = activeUrls.size();
                    activeUrls << p;
                }
            }
            
            if (!activeUrls.isEmpty()) {
                (new FullImagePreviewDialog(activeUrls, activeIndex, this->window()))->show();
            }
        });
        
        QLabel *timeL = new QLabel(media.timestamps.size() > i ? media.timestamps[i] : "");
        timeL->setFixedHeight(32);
        timeL->setAlignment(Qt::AlignCenter);
        timeL->setStyleSheet("background: #f8fafc; color: #64748b; font-size: 13px; font-weight: bold; "
                           "border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; border: 1px solid #ebeef5; border-top: none; margin: 0;");
        
        l->addWidget(imgL);
        l->addWidget(timeL);
        grid->addWidget(itemWidget, i / 2, i % 2);
    }
    
    // 如果只有一张图且为空（Mock 视频占位逻辑）
    if (media.urls.isEmpty()) {
        QLabel *placeholder = new QLabel();
        placeholder->setAlignment(Qt::AlignCenter);
        if (media.type == "video") {
            placeholder->setText("▶️\n视频播放器占位\n(正在加载 " + media.title + " ...)");
        } else {
            placeholder->setText("🖼️\n高清影像占位\n(" + media.title + ")");
        }
        placeholder->setStyleSheet("font-size: 24px; color: #606266;");
        grid->addWidget(placeholder, 0, 0);
    }
    
    scroll->setWidget(gallery);
    content->addWidget(scroll, 1);
    
    mainLayout->addWidget(container);
    if (parent) resize(parent->size());
}

void MediaDetailDialog::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 200));
}

void MediaDetailDialog::mousePressEvent(QMouseEvent *event) {
    if (rect().contains(event->pos()) && !childAt(event->pos())) {
        accept();
    }
}

FosterHistoryDetailWidget::FosterHistoryDetailWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 20, 25, 20);
    mainLayout->setSpacing(20);
    setStyleSheet("QLabel { background: transparent; }"); // 全局设置此容器下的 Label 透明

    // 1. 顶部标题 (移除按钮行)
    QLabel *secTitle = new QLabel("寄养报告明细");
    secTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #303133; background: transparent;");
    mainLayout->addWidget(secTitle);

    // 2. 总结卡片
    mainLayout->addWidget(new SummaryCard());

    // 3. 影像记录
    QLabel *mediaTitle = new QLabel("📸 影像留档");
    mediaTitle->setStyleSheet("color: #909399; font-size: 13px; font-weight: bold; background: transparent;");
    mainLayout->addWidget(mediaTitle);
    m_gallery = new MediaGallery();
    mainLayout->addWidget(m_gallery);

    // 4. 时间轴
    QLabel *logTitle = new QLabel("🕒 行为日志时间轴");
    logTitle->setStyleSheet("color: #909399; font-size: 13px; font-weight: bold; background: transparent;");
    mainLayout->addWidget(logTitle);
    m_timeline = new PetTimelineWidget();
    m_timeline->setMinimumHeight(300);
    mainLayout->addWidget(m_timeline, 1);

    // 5. 财务足迹
    QFrame *footer = new QFrame();
    footer->setFixedHeight(60);
    footer->setStyleSheet("background: #f0f2f5; border-bottom-right-radius: 20px; border-top-right-radius: 0px;");
    QHBoxLayout *footerLayout = new QHBoxLayout(footer);
    
    QLabel *orderInfo = new QLabel("订单编号: <u>#ORD-2025012001</u>");
    orderInfo->setCursor(Qt::PointingHandCursor);
    orderInfo->setStyleSheet("color: #409eff; font-size: 12px; background: transparent;");
    
    QLabel *totalPrice = new QLabel("消费合计: ¥230.00");
    totalPrice->setStyleSheet("color: #f56c6c; font-size: 16px; font-weight: bold; background: transparent;");
    
    footerLayout->addWidget(orderInfo);
    footerLayout->addStretch();
    footerLayout->addWidget(totalPrice);
    mainLayout->addWidget(footer);
}

void FosterHistoryDetailWidget::setDetails(const QString &petId, const QList<PetActivityLog> &logs, const QList<PetMedia> &media) {
    m_timeline->setLogs(logs);
    m_gallery->setMedia(petId, media);
}

// ========================
// PetMediaArchiveDialog 实现
// ========================

PetMediaArchiveDialog::PetMediaArchiveDialog(const QString &petId, const QString &petName, const QList<PetMedia> &media, QWidget *parent, const QString &startDate, const QString &endDate, bool flatten) 
    : QDialog(parent), m_petId(petId), m_petName(petName), m_media(media) 
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(50, 50, 50, 50); // 为阴影留出空间
    mainLayout->setAlignment(Qt::AlignCenter);

    QFrame *container = new QFrame();
    container->setFixedSize(850, 750);
    container->setStyleSheet("QFrame { background: white; border-radius: 24px; border: 1px solid #ebeef5; }");
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(0, 0, 0, 80));
    container->setGraphicsEffect(shadow);

    QVBoxLayout *content = new QVBoxLayout(container);
    content->setContentsMargins(30, 25, 30, 25);

    QHBoxLayout *header = new QHBoxLayout();
    m_titleLabel = new QLabel(QString("📸 %1 的分类影像档案").arg(petName));
    m_titleLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; color: #18181B; border: none; background: transparent; }");
    
    m_closeBtn = new QPushButton("关闭窗口");
    m_closeBtn->setMinimumWidth(100);
    m_closeBtn->setFixedHeight(36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "QPushButton { background: #f0f2f5; color: #1d1d1f; border: none; border-radius: 18px; font-weight: bold; font-size: 13px; padding: 0 15px; } "
        "QPushButton:hover { background: #409eff; color: white; }"
    );
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        if (m_stack->currentIndex() == 1) showGalleryView();
        else accept();
    });
    
    header->addWidget(m_titleLabel);
    header->addStretch();
    header->addWidget(m_closeBtn);
    
    content->addLayout(header);
    content->addSpacing(15);

    m_stack = new QStackedWidget();
    content->addWidget(m_stack, 1);
    
    // 过滤影像
    QList<PetMedia> filteredMedia;
    QDate s = startDate.isEmpty() ? QDate() : QDate::fromString(startDate, "yyyy-MM-dd");
    QDate e = (endDate == "至今" || endDate.isEmpty()) ? QDate::currentDate() : QDate::fromString(endDate, "yyyy-MM-dd");

    for (const auto &m : media) {
        // 核心修复：如果是寄养分类界面（!flatten），严格排除掉任何服务记录，防止同步更新到寄养界面
        if (!flatten && m.title.contains("服务记录")) continue;
        
        // 如果是平铺模式（flatten），且标题不包含服务记录，则说明是寄养期间的照片，
        // 我们在平铺模式下通常只显示服务照片，除非它就是被选中的那条记录。
        
        if (!startDate.isEmpty()) {
            PetMedia fm = m;
            fm.urls.clear();
            fm.timestamps.clear();
            for (int i = 0; i < m.urls.size(); ++i) {
                QString ts = m.timestamps.size() > i ? m.timestamps[i] : "";
                QDate d = QDate::fromString(ts.split(" ").first(), "yyyy-MM-dd");
                if (d.isValid() && d >= s && d <= e) {
                    fm.urls.append(m.urls[i]);
                    fm.timestamps.append(ts);
                }
            }
            if (!fm.urls.isEmpty()) filteredMedia.append(fm);
        } else {
            filteredMedia.append(m);
        }
    }

    if (flatten) {
        setupDetailUI(filteredMedia.isEmpty() ? PetMedia() : filteredMedia.first());
        m_stack->setCurrentIndex(0);
        m_closeBtn->setText("关闭窗口");
    } else {
        MediaGallery *gallery = new MediaGallery();
        gallery->setMedia(m_petId, filteredMedia);
        connect(gallery, &MediaGallery::categorySelected, this, &PetMediaArchiveDialog::showDetailView);
        m_stack->addWidget(gallery);
        
        m_detailPage = new QWidget();
        m_stack->addWidget(m_detailPage);
        
        m_stack->setCurrentIndex(0);
    }

    mainLayout->addWidget(container);
}

void PetMediaArchiveDialog::showDetailView(const PetMedia &media) {
    setupDetailUI(media);
    m_stack->setCurrentIndex(1);
    m_titleLabel->setText(media.title);
    m_closeBtn->setText("返回分类列表");
}

void PetMediaArchiveDialog::showGalleryView() {
    m_stack->setCurrentIndex(0);
    m_titleLabel->setText(QString("📸 %1 的分类影像档案").arg(m_petName));
    m_closeBtn->setText("关闭窗口");
}

void PetMediaArchiveDialog::setupDetailUI(const PetMedia &media) {
    if (!m_detailPage) {
        m_detailPage = new QWidget();
        if (m_stack) m_stack->addWidget(m_detailPage);
    }
    
    // 清理旧页面内容
    if (m_detailPage->layout()) {
        QLayoutItem *child;
        while ((child = m_detailPage->layout()->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
    } else {
        new QVBoxLayout(m_detailPage);
    }

    QVBoxLayout *layout = static_cast<QVBoxLayout*>(m_detailPage->layout());
    layout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea { background: #f8f9fb; border-radius: 20px; } "
        "QScrollBar:vertical { background: transparent; width: 10px; margin: 0px 2px 0px 0px; } "
        "QScrollBar::handle:vertical { background: #dcdfe6; min-height: 30px; border-radius: 5px; } "
        "QScrollBar::handle:vertical:hover { background: #c0c4cc; } "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );
    
    QWidget *gallery = new QWidget();
    gallery->setStyleSheet("background: transparent;");
    QGridLayout *grid = new QGridLayout(gallery);
    grid->setContentsMargins(25, 20, 25, 25);
    grid->setSpacing(25);

    for (int i = 0; i < media.urls.size(); ++i) {
        QWidget *itemWidget = new QWidget();
        itemWidget->setFixedSize(350, 292);
        QVBoxLayout *l = new QVBoxLayout(itemWidget);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
        l->setAlignment(Qt::AlignTop);

        ClickableLabel *imgL = new ClickableLabel();
        imgL->setFixedSize(350, 260);
        QPixmap pix(media.urls[i]);
        if (pix.isNull()) pix.load(":/images/load_img.jpg");
        imgL->setPixmap(pix.scaled(350, 260, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        imgL->setStyleSheet("background: white; border-top-left-radius: 12px; border-top-right-radius: 12px; border: 1px solid #ebeef5; padding: 0px; border-bottom: none;");
        imgL->setAlignment(Qt::AlignCenter);
        imgL->setProperty("path", media.urls[i]);
        
        connect(imgL, &ClickableLabel::clicked, this, [this, media, i]() {
            (new FullImagePreviewDialog(media.urls, i, this->window()))->show();
        });

        QLabel *timeL = new QLabel(media.timestamps.size() > i ? media.timestamps[i] : "");
        timeL->setFixedHeight(32);
        timeL->setAlignment(Qt::AlignCenter);
        timeL->setStyleSheet("background: #f8fafc; color: #64748b; font-size: 13px; font-weight: bold; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; border: 1px solid #ebeef5; border-top: none; margin: 0;");
        
        l->addWidget(imgL);
        l->addWidget(timeL);
        grid->addWidget(itemWidget, i / 2, i % 2);
    }
    
    scroll->setWidget(gallery);
    layout->addWidget(scroll, 1);
}

void PetMediaArchiveDialog::paintEvent(QPaintEvent *) {
}

// ========================
// HistoryRecordDialog 实现
// ========================

HistoryRecordDialog::HistoryRecordDialog(int roomId, const QString &petId, const QString &petName, QWidget *parent) : QDialog(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setupUI(roomId, petId, petName);
}

void HistoryRecordDialog::setupUI(int roomId, const QString &petId, const QString &petName) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    QFrame *container = new QFrame();
    container->setFixedSize(1050, 700); // 宽度从 900 增加到 1050，高度微调
    container->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 20px; }");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    QVBoxLayout *contentLayout = new QVBoxLayout(container);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // 头部
    QWidget *header = new QWidget();
    header->setFixedHeight(70);
    header->setStyleSheet("background: white; border-top-left-radius: 20px; border-top-right-radius: 20px; border-bottom: 1px solid #f0f2f5;");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(25, 0, 0, 0); 
    
    QLabel *titleLabel = new QLabel(petName.isEmpty() ? QString("Room %1 寄养全纪录").arg(roomId) : QString("%1 的寄养全纪录").arg(petName));
    titleLabel->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold;");
    
    QPushButton *closeBtn = new QPushButton();
    closeBtn->setFixedSize(30, 30);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { border: none; background: transparent; } "
        "QPushButton:hover { background-color: #f5f7fa; border-radius: 15px; }"
    );
    QVBoxLayout *btnL = new QVBoxLayout(closeBtn);
    btnL->setContentsMargins(0, 0, 0, 0);
    QLabel *iconL = new QLabel();
    iconL->setPixmap(QPixmap(":/images/close.png").scaled(14, 14, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconL->setAlignment(Qt::AlignCenter);
    btnL->addWidget(iconL);
    
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn, 0, Qt::AlignVCenter);
    headerLayout->addSpacing(25); 

    contentLayout->addWidget(header);

    // 主内容区：左右分栏
    QHBoxLayout *bodyLayout = new QHBoxLayout();
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // 左侧：批次列表 (Master)
    QWidget *leftPanel = new QWidget();
    leftPanel->setFixedWidth(340);
    leftPanel->setStyleSheet("background: white; border-right: 1px solid #f0f2f5; border-bottom-left-radius: 20px;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(15);
    leftLayout->setAlignment(Qt::AlignTop);

    QLabel *listTitle = new QLabel("寄养批次");
    listTitle->setStyleSheet("color: #909399; font-size: 13px; font-weight: bold; margin-bottom: 5px;");
    leftLayout->addWidget(listTitle);

    CustomCalendarEdit *dateFilter = new CustomCalendarEdit();
    dateFilter->setPlaceholderText("📅 按日期筛选...");
    dateFilter->setFixedHeight(36);
    dateFilter->setStyleSheet(
        "QLineEdit { "
        "  border: 1px solid #dcdfe6; "
        "  border-radius: 6px; "
        "  padding-left: 10px; "
        "  background: #f8f9fb; "
        "  font-size: 13px; "
        "} "
        "QLineEdit:focus { border: 1px solid #409eff; background: white; }"
    );
    leftLayout->addWidget(dateFilter);
    leftLayout->addSpacing(10);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: transparent; }");
    
    QWidget *listContainer = new QWidget();
    QVBoxLayout *listV = new QVBoxLayout(listContainer);
    listV->setContentsMargins(0, 0, 10, 0);
    listV->setSpacing(12);
    listV->setAlignment(Qt::AlignTop);

    // 右侧：增强型寄养详情 (Detail)
    QScrollArea *detailScroll = new QScrollArea();
    detailScroll->setWidgetResizable(true);
    detailScroll->setFrameShape(QFrame::NoFrame);
    detailScroll->setStyleSheet("QScrollArea { background: #f8f9fb; }");
    
    FosterHistoryDetailWidget *detailWidget = new FosterHistoryDetailWidget();
    detailScroll->setWidget(detailWidget);

    // 联动逻辑
    m_cards.clear();
    auto addBatch = [&](const QString &pName, const QString &rInfo, const QString &period, const QString &status, const QList<PetActivityLog> &mockLogs, const QList<PetMedia> &mockMedia) {
        HistoryItemCard *card = new HistoryItemCard(pName, rInfo, period, status, listContainer);
        m_cards.append(card);
        listV->addWidget(card);
        connect(card, &HistoryItemCard::clicked, this, [this, card, detailWidget, mockLogs, mockMedia, petId]() {
            for (auto c : m_cards) c->setSelected(false);
            card->setSelected(true);
            detailWidget->setDetails(petId, mockLogs, mockMedia);
        });
    };

    // 构造丰富的 Mock 数据
    QList<PetActivityLog> logs0, logs1, logs2, logs3;
    QList<PetMedia> media0, media1, media2, media3;
    
    // 批次 0: 接入真实数据（当前寄养）
    if (!petId.isEmpty()) {
        logs0 = PetDataManager::instance()->getLogs(petId);
        media0 = PetDataManager::instance()->getMedia(petId);
    } else {
        logs0 << PetActivityLog{"2025-04-18 09:00", "入店", "旺财已准时到达，状态兴奋。", "📤"}
              << PetActivityLog{"2025-04-18 10:30", "投喂", "早餐正常食用，表现活泼。", "🍖"}
              << PetActivityLog{"2025-04-19 14:00", "洗护", "今日进行了精细刷毛。", "🛁"};
        media0 << PetMedia{QStringList{":/images/foster_outdoor_3.png"}, "image", "今日活动", QStringList{"2025-04-19 14:15"}};
    }

    media1 << PetMedia{QStringList{":/images/foster_bath.png", ":/images/foster_bath_2.png"}, 
                       "image", "洗澡记录", QStringList{"2025-01-17 14:00", "2025-01-17 15:30"}}
           << PetMedia{QStringList{":/images/foster_outdoor.png", ":/images/foster_outdoor_2.png", ":/images/foster_outdoor_3.png", 
                                  ":/images/foster_outdoor_4.png", ":/images/foster_outdoor_5.png", ":/images/foster_outdoor_6.png"}, 
                       "image", "户外草坪", QStringList{"2025-01-19 10:00", "2025-01-19 10:15", "2025-01-19 10:30", 
                                                      "2025-01-19 10:45", "2025-01-19 11:00", "2025-01-19 11:15"}}
           << PetMedia{QStringList{":/images/foster_sleep.png", ":/images/foster_sleep_2.png"}, 
                       "image", "午睡状态", QStringList{"2025-01-15 13:00", "2025-01-16 13:30"}};
           
    media2 << PetMedia{QStringList(), "image", "伤口观察", QStringList()}
           << PetMedia{QStringList(), "image", "精神恢复", QStringList()};
           
    media3 << PetMedia{QStringList(), "image", "结业纪念", QStringList()};
    
    // 批次 1: 5 天的高质量寄养
    logs1 << PetActivityLog{"2025-01-15 09:00", "投喂", "早餐：进口无谷粮 50g，食欲极佳。", "🍖"}
          << PetActivityLog{"2025-01-15 14:30", "运动", "在室内活动区游玩 30 分钟，精力旺盛。", "🏃"}
          << PetActivityLog{"2025-01-15 21:00", "备注", "准备入睡，情绪稳定。", "🌙"}
          << PetActivityLog{"2025-01-16 08:30", "投喂", "早餐：搭配了 1/4 罐头，全部吃完。", "🍖"}
          << PetActivityLog{"2025-01-16 11:00", "检查", "例行体检：体重 4.5kg，耳道清洁，状态良好。", "🩺"}
          << PetActivityLog{"2025-01-17 15:00", "洗护", "进行了深度洗烘，全程表现配合。", "🛁"}
          << PetActivityLog{"2025-01-18 09:00", "投喂", "常规饮食，饮水量正常。", "🍖"}
          << PetActivityLog{"2025-01-19 10:00", "运动", "户外草坪互动，抓球游戏非常开心。", "🥎"}
          << PetActivityLog{"2025-01-20 16:00", "离店", "主人已接走，本次寄养圆满结束。", "🏠"};
          
    // 批次 2: 带有异常预警的记录
    logs2 << PetActivityLog{"2024-12-01 10:00", "入店", "宠物状态尚可，稍微有些紧张。", "📥"}
          << PetActivityLog{"2024-12-01 19:00", "投喂", "晚饭剩了一半，建议持续观察。", "🍖"}
          << PetActivityLog{"2024-12-02 03:00", "异常", "凌晨发生呕吐，已清理并测量体温。", "⚠️", true}
          << PetActivityLog{"2024-12-02 09:00", "检查", "体温 39.2，精神稍显萎靡，已通知主人。", "🩺", true}
          << PetActivityLog{"2024-12-02 14:00", "备注", "遵医嘱喂服了益生菌，目前正在休息。", "💊"}
          << PetActivityLog{"2024-12-03 09:00", "检查", "体温降至 38.6，呕吐停止，精神恢复。", "🩺"}
          << PetActivityLog{"2024-12-04 10:00", "投喂", "恢复饮食，少量多次喂食，状态转好。", "🍼"}
          << PetActivityLog{"2024-12-05 15:00", "备注", "完全恢复活力，表现活泼。", "✨"};

    // 批次 3: 简单记录
    logs3 << PetActivityLog{"2024-11-10 09:00", "投喂", "常规早餐，表现正常。", "🍖"}
          << PetActivityLog{"2024-11-12 14:00", "洗护", "修剪了指甲，表现乖巧。", "🛁"}
          << PetActivityLog{"2024-11-15 16:00", "备注", "顺利结业离店。", "👋"};

    if (petName.isEmpty()) {
        addBatch("旺财", "Room 101", "2025-04-18 至 至今", "进行中", logs0, media0);
        addBatch("团团", "Room 101", "2025-01-01 至 2025-01-05", "已完成", logs1, media1);
        addBatch("小黑", "Room 101", "2024-12-01 至 2024-12-05", "已完成", logs2, media2);
        addBatch("豆豆", "Room 101", "2024-11-10 至 2024-11-15", "已完成", logs3, media3);
    } else {
        addBatch(petName, "Room 108", "2025-04-18 至 至今", "进行中", logs0, media0);
        addBatch(petName, "Room 108", "2025-01-15 至 2025-01-20", "已完成", logs1, media1);
        addBatch(petName, "Room 203", "2024-12-01 至 2024-12-05", "已完成", logs2, media2);
        addBatch(petName, "Room 101", "2024-11-10 至 2024-11-15", "已完成", logs3, media3);
    }

    // 默认选中第一项 (当前进行中的)
    if (!m_cards.isEmpty()) {
        m_cards.first()->setSelected(true);
        detailWidget->setDetails(petId, logs0, media0);
    }

    scroll->setWidget(listContainer);
    leftLayout->addWidget(scroll);

    bodyLayout->addWidget(leftPanel);
    bodyLayout->addWidget(detailScroll, 1);
    contentLayout->addLayout(bodyLayout);
    
    mainLayout->addWidget(container);
    if (QWidget *p = parentWidget()) resize(p->size());
}

// ========================
// MediaUploadDialog 实现
// ========================
MediaUploadDialog::MediaUploadDialog(const QString &petId, QWidget *parent) : QDialog(parent), m_petId(petId) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setupUI();
}

void MediaUploadDialog::setupUI() {
    PetInfo pet = PetDataManager::instance()->getPet(m_petId);
    QString petName = pet.name;
    int roomId = pet.roomNo.toInt(); // 假设房号是数字
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    QFrame *container = new QFrame();
    container->setFixedSize(700, 500);
    container->setStyleSheet("QFrame { background: white; border-radius: 20px; } QLabel { border: none; background: transparent; }");
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 80));
    container->setGraphicsEffect(shadow);

    QVBoxLayout *content = new QVBoxLayout(container);
    content->setContentsMargins(35, 30, 35, 30);
    content->setSpacing(25);

    // 头部
    QHBoxLayout *header = new QHBoxLayout();
    QLabel *titleL = new QLabel("📸 日常行为与影像录入");
    titleL->setStyleSheet("font-size: 20px; font-weight: bold; color: #18181B;");
    
    QLabel *backBtn = new QLabel("返回");
    backBtn->setFixedSize(60, 28);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setAlignment(Qt::AlignCenter);
    backBtn->setProperty("isBack", true);
    backBtn->installEventFilter(this);
    backBtn->setStyleSheet(
        "QLabel { "
        "  border: 1px solid #DCDFE6; border-radius: 14px; "
        "  color: #606266; background: white; font-size: 12px; "
        "  font-weight: bold; "
        "} "
        "QLabel:hover { "
        "  background: #F5F7FA; border-color: #409EFF; color: #409EFF; "
        "}"
    );
    
    header->addWidget(titleL);
    header->addStretch();
    header->addWidget(backBtn);
    content->addLayout(header);

    // 左右分栏
    QHBoxLayout *body = new QHBoxLayout();
    body->setSpacing(30);

    // 左侧：影像上传
    QVBoxLayout *left = new QVBoxLayout();
    left->setSpacing(10); // 统一较小的间距
    
    // 新增：影像分类选择
    QHBoxLayout *mediaTypeRow = new QHBoxLayout();
    QLabel *typeLbl = new QLabel("影像类别:");
    typeLbl->setStyleSheet("font-size: 13px; font-weight: bold; color: #606266;");
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"运动", "洗护", "饮食", "睡觉", "医疗", "入住存证", "离店存证", "其他"});
    m_typeCombo->setFixedHeight(32);
    m_typeCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox:hover { border-color: #409eff; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }"
    );
    mediaTypeRow->addWidget(typeLbl);
    mediaTypeRow->addWidget(m_typeCombo, 1);
    
    QLabel *imgLbl = new QLabel("影像资料 (支持多图/视频)");
    imgLbl->setStyleSheet("font-size: 13px; font-weight: bold; color: #606266; margin-top: 5px;"); // 减小顶部边距
    
    m_previewArea = new QFrame();
    m_previewArea->setObjectName("uploadBox");
    m_previewArea->setFixedSize(280, 240);
    m_previewArea->setStyleSheet("QFrame#uploadBox { border: 2px dashed #dcdfe6; border-radius: 16px; background: #f8f9fb; } "
                                 "QFrame#uploadBox:hover { border-color: #409eff; background: #f0f7ff; }");
    m_previewArea->installEventFilter(this);

    QVBoxLayout *ul = new QVBoxLayout(m_previewArea);
    m_uIcon = new QLabel("➕");
    m_uIcon->setStyleSheet("font-size: 36px; color: #909399; background: transparent;");
    m_uIcon->setAlignment(Qt::AlignCenter);
    m_uText = new QLabel("点击上传或将媒体文件拖入此处\n(支持 JPG, PNG, MP4)");
    m_uText->setStyleSheet("font-size: 11px; color: #909399; background: transparent;");
    m_uText->setAlignment(Qt::AlignCenter);
    ul->addStretch();
    ul->addWidget(m_uIcon);
    ul->addWidget(m_uText);
    ul->addStretch();
    
    left->addLayout(mediaTypeRow);
    left->addWidget(imgLbl);
    left->addWidget(m_previewArea);
    left->addStretch();

    // 右侧：日志记录
    QVBoxLayout *right = new QVBoxLayout();
    QLabel *logLbl = new QLabel("日常观察日志");
    logLbl->setStyleSheet("font-size: 13px; font-weight: bold; color: #606266;");
    
    // 新增：类型与经办人选择
    QHBoxLayout *typeRow = new QHBoxLayout();
    typeRow->setSpacing(15);
    
    m_logTypeCombo = new QComboBox();
    m_logTypeCombo->addItems({"投喂", "洗护", "运动", "医疗", "备注"});
    m_logTypeCombo->setFixedHeight(32);
    m_logTypeCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox:hover { border-color: #409eff; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }"
    );

    m_operatorCombo = new QComboBox();
    m_operatorCombo->addItems({"店员小利", "管理员", "店员小张"});
    m_operatorCombo->setFixedHeight(32);
    m_operatorCombo->setStyleSheet(
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox:hover { border-color: #409eff; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }"
    );

    auto createComboLayout = [&](const QString &label, QComboBox *cb) {
        QHBoxLayout *h = new QHBoxLayout();
        QLabel *l = new QLabel(label);
        l->setStyleSheet("color: #909399; font-size: 12px;");
        h->addWidget(l);
        h->addWidget(cb, 1);
        return h;
    };
    
    typeRow->addLayout(createComboLayout("类型:", m_logTypeCombo));
    typeRow->addLayout(createComboLayout("经办人:", m_operatorCombo));
    
    m_logEdit = new QTextEdit();
    m_logEdit->setPlaceholderText("请输入宠物的最新表现，例如：食欲极佳、排便正常、精神状态活泼等...");
    m_logEdit->setStyleSheet("QTextEdit { border: 1px solid #dcdfe6; border-radius: 12px; padding: 12px; font-size: 13px; background: white; } "
                         "QTextEdit:focus { border: 1px solid #409eff; }");
    
    right->addWidget(logLbl);
    right->addLayout(typeRow); // 插入选择行
    right->addWidget(m_logEdit);

    body->addLayout(left);
    body->addLayout(right);
    content->addLayout(body);

    // 底部按钮
    QHBoxLayout *footer = new QHBoxLayout();
    QLabel *petInfo = new QLabel(QString("当前录入对象: <b>%1</b> (Room %2)").arg(petName, QString::number(roomId)));
    petInfo->setStyleSheet("color: #909399; font-size: 12px;");
    
    QPushButton *saveBtn = new QPushButton("✅ 确认归档并同步");
    saveBtn->setFixedSize(180, 44);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet("QPushButton { background: white; color: #3b82f6; border: 1px solid #3b82f6; border-radius: 10px; font-weight: bold; font-size: 14px; } "
                            "QPushButton:hover { background: #eff6ff; }");
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        QString mediaCategory = m_typeCombo->currentText();
        QString logType = m_logTypeCombo->currentText();
        QString operatorName = m_operatorCombo->currentText();
        QString remark = m_logEdit->toPlainText();
        
        // 1. 构造并存入日志
        PetActivityLog log;
        log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
        log.type = logType; // 使用右侧选择的日志类型
        log.remark = remark.isEmpty() ? QString("进行了%1活动").arg(logType) : remark;
        
        // 智能映射图标
        if (logType == "投喂" || logType == "饮食") log.icon = "🍖";
        else if (logType == "洗护") log.icon = "🛁";
        else if (logType == "运动") log.icon = "🏃";
        else if (logType == "医疗") log.icon = "🩺";
        else if (logType == "睡觉") log.icon = "💤";
        else if (logType == "异常") log.icon = "⚠️";
        else log.icon = "📝";
        
        log.operatorName = operatorName; // 使用右侧选择的经办人
        PetDataManager::instance()->addActivityLog(m_petId, log);
        
        // 触发全局同步信号，确保宠物中心等模块感知到变化
        PetDataManager::instance()->notifyGlobalDataChanged();

        // 2. 构造并存入影像
        if (!m_filePaths.isEmpty()) {
            PetMedia media;
            media.type = "image";
            media.title = mediaCategory + "记录"; // 影像使用左侧分类
            media.urls = m_filePaths;
            for(int i=0; i<m_filePaths.size(); ++i) {
                media.timestamps << log.time;
            }
            PetDataManager::instance()->addMedia(m_petId, media);
        }

        emit recordAdded(logType, remark, m_filePaths);
        CustomMessageDialog::showSuccess(this, "录入成功", "日常影像与观察日志已成功归档并同步至中央数据源。");
        accept();
    });
    
    footer->addWidget(petInfo);
    footer->addStretch();
    footer->addWidget(saveBtn);
    content->addLayout(footer);

    mainLayout->addWidget(container);
    if (QWidget *p = parentWidget()) resize(p->size());
}

void MediaUploadDialog::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 180));
}

bool MediaUploadDialog::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (watched == m_previewArea) {
            onUploadClicked();
            return true;
        }
        QLabel *lbl = qobject_cast<QLabel*>(watched);
        if (lbl && lbl->property("isBack").toBool()) {
            reject();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void MediaUploadDialog::onUploadClicked() {
    QStringList files = QFileDialog::getOpenFileNames(
        this, "选择影像资料", "", "Images (*.png *.jpg *.jpeg *.bmp);;Videos (*.mp4 *.avi)"
    );

    if (!files.isEmpty()) {
        m_filePaths = files;
        
        // 更新预览：取第一张图片作为预览
        QPixmap pix(m_filePaths.first());
        if (!pix.isNull()) {
            m_uIcon->setPixmap(pix.scaled(240, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_uText->setText(QString("已选择 %1 个文件").arg(m_filePaths.size()));
            m_uText->setStyleSheet("font-size: 12px; font-weight: bold; color: #409eff; background: transparent;");
        } else {
            m_uIcon->setText("🎬");
            m_uIcon->setStyleSheet("font-size: 48px; color: #409eff; background: transparent;");
            m_uText->setText(QString("已选择 %1 个视频文件").arg(m_filePaths.size()));
        }
    }
}

void HistoryRecordDialog::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 150));
}

void FosterDetailDialog::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 120));
}

bool FosterDetailDialog::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QLabel *label = qobject_cast<QLabel*>(watched);
        if (label) {
            if (label->property("isAvatar").toBool()) {
                QString path = label->property("avatarPath").toString();
                if (!path.isEmpty()) {
                    (new FullImagePreviewDialog(QStringList{path}, 0, this->window()))->show();
                } else {
                    (new AvatarZoomDialog(label->text(), this->window()))->show();
                }
                return true;
            }
            if (label->property("isBack").toBool()) {
                reject();
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

// ========================
// PillButton 实现
// ========================

PillButton::PillButton(const QString &text, const QString &accentColor, QWidget *parent) 
    : QPushButton(text, parent), m_accent(accentColor) 
{
    setCheckable(true);
    setFixedHeight(32);
    setCursor(Qt::PointingHandCursor);
    updateStyle();
    connect(this, &QPushButton::toggled, this, &PillButton::updateStyle);
}

void PillButton::updateStyle() {
    if (isChecked()) {
        setStyleSheet(QString("QPushButton { background: %1; color: white; border: none; border-radius: 16px; padding: 0 16px; font-weight: bold; }").arg(m_accent));
    } else {
        setStyleSheet("QPushButton { background: #f0f2f5; color: #606266; border: 1px solid #dcdfe6; border-radius: 16px; padding: 0 16px; font-weight: normal; } QPushButton:hover { background: #e4e7ed; }");
    }
}

// ========================
// FosterActionPanel 实现
// ========================

FosterActionPanel::FosterActionPanel(QWidget *parent) : QFrame(parent) {
    setupUI();
}

void FosterActionPanel::setupUI() {
    setObjectName("FosterActionPanel");
    setStyleSheet("#FosterActionPanel { background: white; border-radius: 12px; border: 1px solid #f0f2f5; }");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea { border: none; background: transparent; } "
        "QScrollBar:vertical { background: transparent; width: 6px; margin: 0px; } "
        "QScrollBar::handle:vertical { background: #e0e0e0; min-height: 20px; border-radius: 3px; } "
        "QScrollBar::handle:vertical:hover { background: #d0d0d0; } "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; } "
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; } "
        "QScrollBar:horizontal { background: transparent; height: 6px; margin: 0px; } "
        "QScrollBar::handle:horizontal { background: #e0e0e0; min-width: 20px; border-radius: 3px; } "
        "QScrollBar::handle:horizontal:hover { background: #d0d0d0; } "
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; } "
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }"
    );
    
    QWidget *container = new QWidget();
    container->setStyleSheet("background: transparent;");
    m_contentLayout = new QVBoxLayout(container);
    m_contentLayout->setContentsMargins(20, 20, 20, 25); // 增加底部边距到 25
    m_contentLayout->setSpacing(15);
    
    scroll->setWidget(container);
    mainLayout->addWidget(scroll);
    
    showCheckInForm();
}

void FosterActionPanel::showEmptyPlaceholder() {
    clearContentLayout();
    m_currentWidget = new QWidget();
    m_currentWidget->setStyleSheet("QLabel { border: none; background: transparent; }");
    QVBoxLayout *layout = new QVBoxLayout(m_currentWidget);
    layout->setAlignment(Qt::AlignCenter);
    
    m_contentLayout->addWidget(m_currentWidget);
}

void FosterActionPanel::clearContentLayout() {
    if (m_currentWidget) {
        m_contentLayout->removeWidget(m_currentWidget);
        m_currentWidget->deleteLater();
        m_currentWidget = nullptr;
    }
    while (m_contentLayout->count() > 0) {
        QLayoutItem *item = m_contentLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->hide();
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void FosterActionPanel::updatePanel(int roomId, const QString &status, const QString &petId, const QString &petName) {
    Q_UNUSED(petName);
    clearContentLayout();
    
    if (status == "free") showCheckInForm(roomId);
    else if (status == "cleaning" || status == "maintenance") showMaintenanceView(roomId, status);
    else showManagementView(roomId, petId, status);
}

bool FosterActionPanel::hasUnsavedChanges() const {
    if (!m_currentWidget) return false;
    QLineEdit *search = m_currentWidget->findChild<QLineEdit*>();
    if (search && !search->text().isEmpty()) return true;
    QTextEdit *note = m_currentWidget->findChild<QTextEdit*>();
    if (note && !note->toPlainText().isEmpty()) return true;
    return false;
}

void FosterActionPanel::resetChanges() {
    if (!m_currentWidget) return;
    QLineEdit *search = m_currentWidget->findChild<QLineEdit*>();
    if (search) search->clear();
    QTextEdit *note = m_currentWidget->findChild<QTextEdit*>();
    if (note) note->clear();
}

bool FosterActionPanel::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_avatarLabel && event->type() == QEvent::MouseButtonRelease) {
        if (m_avatarLabel->property("action").toString() == "upload_checkin_photo") {
            QString file = QFileDialog::getOpenFileName(this, "选择入住照片", "", "Images (*.png *.jpg *.jpeg)");
            if (!file.isEmpty()) {
                // 这里需要一种方式把文件路径传回 showCheckInForm 中的闭包，或者直接在 eventFilter 处理
                // 由于 stayPhotos 是局部变量，我们改用属性存储
                QStringList photos = m_avatarLabel->property("photos").toStringList();
                photos.clear(); photos << file;
                m_avatarLabel->setProperty("photos", photos);
                
                QPixmap pix(file);
                m_avatarLabel->setPixmap(pix.scaled(76, 76, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                m_avatarLabel->setStyleSheet("border: 2px solid #10b981; border-radius: 12px; overflow: hidden; background: white;");
            }
            return true;
        }
        emit avatarClicked(m_currentInfo.avatarPath);
        return true;
    }
    return QFrame::eventFilter(obj, event);
}

void FosterActionPanel::showCheckInForm(int roomId, const QDate &startDate) {
    clearContentLayout();
    m_currentWidget = new QWidget();
    m_currentWidget->setStyleSheet(
        "QLabel { border: none; background: transparent; } "
        "QLabel#FieldLabel { color: #606266; font-size: 13px; font-weight: bold; } "
        "QLineEdit, QDoubleSpinBox, QTextEdit { "
        "   border: 1px solid #dcdfe6; border-radius: 6px; padding: 6px 12px; background: #fcfcfd; font-size: 13px; color: #303133; "
        "} "
        "QLineEdit:focus, QDoubleSpinBox:focus, QTextEdit:focus { "
        "   border-color: #409eff; background: white; outline: none; "
        "} "
        "QLabel#RoomTag { background: #f0f2f5; color: #909399; font-size: 11px; font-weight: bold; padding: 2px 8px; border-radius: 4px; } "
        "QPushButton#PhotoBtn { "
        "   background: #f8fafc; border: 1px dashed #cbd5e0; border-radius: 6px; color: #64748b; font-size: 13px; text-align: left; padding: 0 12px; "
        "} "
        "QPushButton#PhotoBtn:hover { background: #f1f5f9; border-color: #409eff; } "
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { border: none; background: transparent; width: 0px; } "
        "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; } "
        "QComboBox:hover { border-color: #409eff; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; } "
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(m_currentWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // --- 1. 头部区域 ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);
    
    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(36, 36);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("background: #ecf5ff; color: #409eff; border-radius: 8px;");
    
    // 根据 ID 计算房型
    QString roomType = "标准房";
    if (roomId >= 111 && roomId <= 115) roomType = "豪华房";
    else if (roomId >= 116 && roomId <= 120) roomType = "多宠房";

    QString badgeStyle;
    if (roomType == "豪华房") badgeStyle = "background: #f9f0ff; color: #722ed1; border: 1px solid #d3adf7;";
    else if (roomType == "多宠房") badgeStyle = "background: #fff7e6; color: #fa8c16; border: 1px solid #ffd591;";
    else badgeStyle = "background: #f6ffed; color: #52c41a; border: 1px solid #b7eb8f;";

    QLabel *titleLabel = new QLabel("入住安排");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1a1a1a;");
    
    QString fullRoomText = roomId == -1 ? "待定" : QString("ROOM %1 - %2").arg(roomId).arg(roomType);
    QLabel *roomTag = new QLabel(fullRoomText);
    roomTag->setStyleSheet(badgeStyle + " font-size: 12px; font-weight: bold; padding: 4px 12px; border-radius: 6px;");
    
    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleLabel);
    headerLayout->addSpacing(10);
    headerLayout->addWidget(roomTag);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);


    
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine); line->setFrameShadow(QFrame::Plain);
    line->setStyleSheet("color: #f0f2f5;");
    mainLayout->addWidget(line);

    // --- 2. 核心表单区 (卡片化重构) ---
    auto createSectionCard = [mainLayout]() {
        QFrame *card = new QFrame();
        card->setObjectName("SectionCard");
        card->setStyleSheet("#SectionCard { background: #fcfcfd; border: 1px solid #e2e8f0; border-radius: 8px; } ");
        QVBoxLayout *l = new QVBoxLayout(card);
        l->setContentsMargins(10, 10, 10, 10);
        l->setSpacing(8);
        mainLayout->addWidget(card);
        return l;
    };

    auto createLabel = [](const QString &txt) {
        QLabel *l = new QLabel(txt);
        l->setObjectName("FieldLabel");
        return l;
    };

    // --- 卡片 1: 房位与宠物 ---
    // --- 卡片 2: 寄养时间 ---
    QVBoxLayout *dateL = createSectionCard();
    QHBoxLayout *dateLabels = new QHBoxLayout();
    dateLabels->addWidget(createLabel("入住日期"));
    dateLabels->addWidget(createLabel("预计离店"));
    dateL->addLayout(dateLabels);

    QHBoxLayout *dateEdits = new QHBoxLayout();
    CustomCalendarEdit *startEdit = new CustomCalendarEdit();
    startEdit->setFixedHeight(36);
    startEdit->setMinimumDate(QDate::currentDate());
    QDate finalStartDate = startDate.isValid() ? startDate : QDate::currentDate();
    startEdit->setText(finalStartDate.toString("yyyy-MM-dd"));

    CustomCalendarEdit *endEdit = new CustomCalendarEdit();
    endEdit->setFixedHeight(36);
    endEdit->setMinimumDate(finalStartDate.addDays(1));
    endEdit->setText(finalStartDate.addDays(7).toString("yyyy-MM-dd"));
    dateEdits->addWidget(startEdit);
    dateEdits->addWidget(endEdit);
    dateL->addLayout(dateEdits);

    // --- 卡片 1: 房位与宠物 (后置以捕获日期) ---
    QVBoxLayout *baseInfoL = createSectionCard();
    baseInfoL->addWidget(createLabel("选择房位"));
    QComboBox *roomCombo = new QComboBox();
    roomCombo->setFixedHeight(36);
    
    // ... (roomCombo setup remains similar, just reordered)
    auto makeColorIcon = [](const QColor &color) {
        QPixmap pix(14, 14);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(0, 0, 14, 14, 3, 3);
        return QIcon(pix);
    };

    auto getRoomTypeStr = [](int rid) {
        if (rid >= 111 && rid <= 115) return "豪华房";
        if (rid >= 116 && rid <= 120) return "多宠房";
        return "标准房";
    };

    if (roomId != -1) {
        QString type = getRoomTypeStr(roomId);
        QColor color = (type == "豪华房") ? QColor("#722ed1") : (type == "多宠房" ? QColor("#fa8c16") : QColor("#27ae60"));
        roomCombo->addItem(makeColorIcon(color), QString("Room %1 - %2").arg(roomId).arg(type), roomId);
        roomCombo->setCurrentIndex(0);
    }
    
    connect(roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [roomTag, roomCombo, getRoomTypeStr]() {
        int rid = roomCombo->currentData().toInt();
        if (rid == -1) return;
        QString type = getRoomTypeStr(rid);
        QString style;
        if (type == "豪华房") style = "background: #f9f0ff; color: #722ed1; border: 1px solid #d3adf7;";
        else if (type == "多宠房") style = "background: #fff7e6; color: #fa8c16; border: 1px solid #ffd591;";
        else style = "background: #f6ffed; color: #52c41a; border: 1px solid #b7eb8f;";
        
        roomTag->setText(QString("ROOM %1 - %2").arg(rid).arg(type));
        roomTag->setStyleSheet(style + " font-size: 12px; font-weight: bold; padding: 4px 12px; border-radius: 6px;");
    });
    baseInfoL->addWidget(roomCombo);

    baseInfoL->addSpacing(4);
    baseInfoL->addWidget(createLabel("选择宠物"));
    QComboBox *petCombo = new QComboBox();
    petCombo->setFixedHeight(36);
    
    auto updatePetCombo = [=]() {
        QString currentPetId = petCombo->currentData().toString();
        petCombo->blockSignals(true);
        petCombo->clear();
        petCombo->addItem("请选择宠物...", ""); 
        
        QDate selStart = QDate::fromString(startEdit->text(), "yyyy-MM-dd");
        QDate selEnd = QDate::fromString(endEdit->text(), "yyyy-MM-dd");
        
        QList<PetInfo> pets = PetDataManager::instance()->allPets();
        for (const auto& p : pets) {
            // 实时冲突检查：仅当所选时间段与宠物已有的寄养/预约时段重叠时，才过滤掉该宠物
            if (p.status == "已预约" || p.status == "寄养中" || p.status.contains("入住") || p.status.contains("预约")) {
                QDate pStart = QDate::fromString(p.fosterStartTime, "yyyy-MM-dd");
                QDate pEnd = QDate::fromString(p.fosterEndTime, "yyyy-MM-dd");
                if (pStart.isValid() && pEnd.isValid()) {
                    // 判断两个时间段是否有交集：!(新结束 < 旧开始 || 新开始 > 旧结束)
                    if (!(selEnd < pStart || selStart > pEnd)) {
                        continue; 
                    }
                }
            }
            
            QString label = QString("%1 - [%2] - %3 (%4)").arg(p.name, p.breed, p.ownerName, p.id);
            petCombo->addItem(label, p.id);
        }
        
        int idx = petCombo->findData(currentPetId);
        if (idx != -1) petCombo->setCurrentIndex(idx);
        petCombo->blockSignals(false);
    };
    
    updatePetCombo();
    baseInfoL->addWidget(petCombo);

    // 绑定日期变化自动刷新宠物列表
    connect(startEdit, &QLineEdit::textChanged, this, [=](const QString &text) {
        QDate s = QDate::fromString(text, "yyyy-MM-dd");
        if (s.isValid()) {
            endEdit->setMinimumDate(s.addDays(1));
            QDate e = QDate::fromString(endEdit->text(), "yyyy-MM-dd");
            if (!e.isValid() || e <= s) {
                endEdit->setText(s.addDays(1).toString("yyyy-MM-dd"));
            }
        }
        updatePetCombo();
    });
    connect(endEdit, &QLineEdit::textChanged, this, updatePetCombo);

    // --- 卡片 3: 身体指标与凭证 ---
    QVBoxLayout *healthL = createSectionCard();
    QGridLayout *healthGrid = new QGridLayout();
    healthGrid->addWidget(createLabel("当前体重"), 0, 0);
    healthGrid->addWidget(createLabel("入住照片"), 0, 1);

    QDoubleSpinBox *weightSpin = new QDoubleSpinBox();
    weightSpin->setRange(0.1, 150.0); weightSpin->setDecimals(1);
    weightSpin->setSuffix(" kg");
    weightSpin->setFixedHeight(36);
    weightSpin->setValue(5.0);
    healthGrid->addWidget(weightSpin, 1, 0);

    QPushButton *photoBtn = new QPushButton("上传凭证...");
    photoBtn->setObjectName("PhotoBtn");
    photoBtn->setFixedHeight(36);
    healthGrid->addWidget(photoBtn, 1, 1);
    healthL->addLayout(healthGrid);

    connect(petCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
        QString petId = petCombo->itemData(index).toString();
        if (!petId.isEmpty()) {
            PetInfo p = PetDataManager::instance()->getPet(petId);
            weightSpin->setValue(p.weight);
        }
    });

    // --- 卡片 4: 备注 ---
    QVBoxLayout *noteL = createSectionCard();
    noteL->addWidget(createLabel("入住备注"));
    QTextEdit *noteEdit = new QTextEdit();
    noteEdit->setPlaceholderText("习惯、健康细节...");
    noteEdit->setFixedHeight(48);
    noteL->addWidget(noteEdit);

    mainLayout->addSpacing(10);

    // --- 4. 底部操作区 ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addStretch();

    QPushButton *bookBtn = new QPushButton("预约入住");
    bookBtn->setFixedHeight(40);
    bookBtn->setCursor(Qt::PointingHandCursor);
    bookBtn->setStyleSheet(
        "QPushButton { background: white; color: #7c3aed; border: 1.5px solid #7c3aed; border-radius: 8px; font-weight: bold; padding: 0 20px; } "
        "QPushButton:hover { background: #f5f3ff; }"
    );

    QPushButton *confirmBtn = new QPushButton("立即办理入住");
    confirmBtn->setFixedHeight(40);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    confirmBtn->setStyleSheet(
        "QPushButton { background: #10b981; color: white; border: none; border-radius: 8px; font-weight: bold; font-size: 14px; padding: 0 25px; } "
        "QPushButton:hover { background: #059669; }"
    );

    btnLayout->addWidget(bookBtn);
    btnLayout->addWidget(confirmBtn);

    // --- 3.5 自动房态过滤逻辑 ---
    auto refreshAvailableRooms = [=]() {
        QDate s = QDate::fromString(startEdit->text(), "yyyy-MM-dd");
        QDate e = QDate::fromString(endEdit->text(), "yyyy-MM-dd");
        if (s.isValid() && e.isValid()) {
            int currentRoom = roomCombo->currentData().toInt();
            roomCombo->blockSignals(true);
            roomCombo->clear();
            auto available = PetDataManager::instance()->getAvailableRooms(s, e);
            if (available.isEmpty()) {
                roomCombo->addItem("❌ 无可用房位", -1);
                confirmBtn->setEnabled(false);
                bookBtn->setEnabled(false);
            } else {
                for (int r : available) {
                    QString type = getRoomTypeStr(r);
                    QString text = QString("Room %1 - %2").arg(r).arg(type);
                    QColor color = (type == "豪华房") ? QColor("#722ed1") : (type == "多宠房" ? QColor("#fa8c16") : QColor("#27ae60"));
                    roomCombo->addItem(makeColorIcon(color), text, r);
                }
                int idx = roomCombo->findData(currentRoom);
                if (idx != -1) roomCombo->setCurrentIndex(idx);
                confirmBtn->setEnabled(true);
                bookBtn->setEnabled(true);
            }
            roomCombo->blockSignals(false);
            roomTag->setText(roomCombo->currentText().toUpper());
        }
    };
    connect(startEdit, &QLineEdit::textChanged, this, refreshAvailableRooms);
    connect(endEdit, &QLineEdit::textChanged, this, refreshAvailableRooms);
    refreshAvailableRooms();

    mainLayout->addLayout(btnLayout);

    // --- 5. 房态维护区 (卡片化) ---
    if (roomId != -1) {
        mainLayout->addSpacing(10);
        QVBoxLayout *maintL = createSectionCard();
        maintL->addWidget(createLabel("房态管控 (仅限空置期)"));
        
        QHBoxLayout *maintBtns = new QHBoxLayout();
        maintBtns->setSpacing(12);
        
        QPushButton *maintBtn = new QPushButton("开启维护模式");
        maintBtn->setFixedHeight(46);
        maintBtn->setCursor(Qt::PointingHandCursor);
        maintBtn->setStyleSheet("QPushButton { background: #fff7e6; color: #fa8c16; border: 1.5px solid #ffd591; border-radius: 8px; font-weight: bold; font-size: 13px; padding: 0 10px; } QPushButton:hover { background: #ffe7ba; }");
        
        QPushButton *deepCleanBtn = new QPushButton("启动深度清洁");
        deepCleanBtn->setFixedHeight(46);
        deepCleanBtn->setCursor(Qt::PointingHandCursor);
        deepCleanBtn->setStyleSheet("QPushButton { background: #f6ffed; color: #52c41a; border: 1.5px solid #b7eb8f; border-radius: 8px; font-weight: bold; font-size: 13px; padding: 0 10px; } QPushButton:hover { background: #d9f7be; }");
        
        maintBtns->addWidget(maintBtn, 1);
        maintBtns->addWidget(deepCleanBtn, 1);
        maintL->addLayout(maintBtns);

        connect(maintBtn, &QPushButton::clicked, this, [this, roomId]() {
            RoomStatusPeriod p;
            p.type = "maintenance";
            p.startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
            p.endTime = QDate::currentDate().addDays(1).toString("yyyy-MM-dd") + " 18:00";
            p.reason = "设施故障排查与维护";
            PetDataManager::instance()->addRoomStatusPeriod(roomId, p);
            showCheckInForm(roomId);
            emit dataChanged();
        });
        connect(deepCleanBtn, &QPushButton::clicked, this, [this, roomId]() {
            RoomStatusPeriod p;
            p.type = "cleaning";
            p.startTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
            p.endTime = QDateTime::currentDateTime().addSecs(4 * 3600).toString("yyyy-MM-dd HH:mm");
            p.reason = "紫外线消杀与深度除味";
            PetDataManager::instance()->addRoomStatusPeriod(roomId, p);
            showCheckInForm(roomId);
            emit dataChanged();
        });
    }

    mainLayout->addStretch();
    m_avatarLabel = new QLabel(); 
    photoBtn->setProperty("action", "upload_checkin_photo");

    
    auto handleBusiness = [this, roomCombo, petCombo, startEdit, endEdit, weightSpin, noteEdit, photoBtn](bool isBooking) {
        QString petId = petCombo->currentData().toString();
        
        // 兼容处理：如果未直接选中项，尝试从文本解析ID
        if (petId.isEmpty()) {
            QString text = petCombo->currentText();
            int idStart = text.lastIndexOf("(");
            int idEnd = text.lastIndexOf(")");
            if (idStart != -1 && idEnd != -1) {
                petId = text.mid(idStart + 1, idEnd - idStart - 1);
            }
        }

        if (!petId.isEmpty()) {
            int selRoomId = roomCombo->currentData().toInt();
            
            if (!isBooking) {
                PetDataManager::instance()->executeCheckIn(
                    selRoomId, petId, 
                    QDate::fromString(startEdit->text(), "yyyy-MM-dd"),
                    QDate::fromString(endEdit->text(), "yyyy-MM-dd"),
                    weightSpin->value(),
                    noteEdit->toPlainText()
                );
            } else {
                PetDataManager::instance()->executeBooking(
                    selRoomId, petId,
                    QDate::fromString(startEdit->text(), "yyyy-MM-dd"),
                    QDate::fromString(endEdit->text(), "yyyy-MM-dd"),
                    weightSpin->value()
                );
            }
            
            // 同步影像资料 (无论是预约还是入住，只要上传了照片就存入档案)
            QStringList photos = photoBtn->property("photos").toStringList();
            if (!photos.isEmpty()) {
                PetMedia media;
                media.urls = photos; media.type = "image";
                media.title = "入住存证照片";
                media.timestamps << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
                PetDataManager::instance()->addMedia(petId, media);
            }
            if (!isBooking) {
                // 立即入住：留在当前界面，并切换到该房间的“入住中”详情页
                showManagementView(selRoomId, petId, "occupied");
            } else {
                // 预约入住：清空面板，并按原逻辑切换到时间轴视图
                showCheckInForm();
                emit bookingConfirmed(); // 仅在预约时触发自动切时间轴
            }
            
            emit dataChanged();       // 刷新基础房态
        }
    };

    connect(bookBtn, &QPushButton::clicked, this, [=]() { handleBusiness(true); });
    connect(confirmBtn, &QPushButton::clicked, this, [=]() { handleBusiness(false); });
    connect(photoBtn, &QPushButton::clicked, this, [=]() {
        QStringList files = QFileDialog::getOpenFileNames(this, "选择入住照片", "", "Images (*.png *.jpg *.jpeg)");
        if (!files.isEmpty()) {
            photoBtn->setProperty("photos", files);
            photoBtn->setText(QString("✅ 已上传 (%1) - 点击更换").arg(files.size()));
            photoBtn->setStyleSheet(
                "QPushButton#PhotoBtn { "
                "   background: #f0fdf4; border: 1.5px solid #10b981; border-radius: 6px; color: #10b981; font-weight: bold; font-size: 13px; text-align: left; padding: 0 12px; "
                "} "
            );
        }
    });

    m_contentLayout->addWidget(m_currentWidget);
}

void FosterActionPanel::showMaintenanceView(int roomId, const QString &status) {
    clearContentLayout();
    m_currentWidget = new QWidget();
    m_currentWidget->setStyleSheet("QLabel { border: none; background: transparent; }");
    QVBoxLayout *layout = new QVBoxLayout(m_currentWidget);
    layout->setContentsMargins(25, 40, 25, 20);
    layout->setSpacing(25);

    bool isMaint = (status == "maintenance");
    
    // 1. 图标与大标题
    QLabel *iconLabel = new QLabel(isMaint ? "🔧" : "🧹");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 64px; margin-bottom: 5px;");
    layout->addWidget(iconLabel);

    QLabel *titleLabel = new QLabel(isMaint ? "正在维护中" : "正在清洁中");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #1a1a1a;");
    layout->addWidget(titleLabel);

    // 2. 详情信息卡片
    QFrame *infoCard = new QFrame();
    infoCard->setStyleSheet("background: #fcfcfd; border: 1px solid #f0f2f5; border-radius: 16px;");
    QVBoxLayout *cardLayout = new QVBoxLayout(infoCard);
    cardLayout->setContentsMargins(25, 25, 25, 25);
    cardLayout->setSpacing(20);

    auto addItem = [&](const QString &title, const QString &val, const QString &color = "#303133") {
        QVBoxLayout *v = new QVBoxLayout();
        v->setSpacing(6);
        QLabel *tL = new QLabel(title);
        tL->setStyleSheet("color: #909399; font-size: 13px; font-weight: bold; border: none; background: transparent;");
        QLabel *vL = new QLabel(val);
        vL->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold; border: none; background: transparent;").arg(color));
        v->addWidget(tL);
        v->addWidget(vL);
        cardLayout->addLayout(v);
    };

    auto periods = PetDataManager::instance()->getRoomStatusPeriods(roomId);
    RoomStatusPeriod current;
    for (const auto &p : periods) {
        if (p.type == status) { current = p; break; }
    }

    addItem(isMaint ? "维护原因" : "清洁重点", current.reason.isEmpty() ? (isMaint ? "常规维护" : "深度清洁") : current.reason, "#e6a23c");
    cardLayout->addSpacing(8);
    addItem(isMaint ? "维护起始时间" : "清洁起始时间", current.startTime);
    addItem("预计完成时间", current.endTime, "#10b981");

    layout->addWidget(infoCard);
    layout->addStretch();

    // 3. 底部操作按钮
    QPushButton *completeBtn = new QPushButton(isMaint ? "✅ 完成维护并恢复房态" : "✅ 完成清洁并恢复房态");
    completeBtn->setFixedHeight(52);
    completeBtn->setCursor(Qt::PointingHandCursor);
    completeBtn->setStyleSheet(
        "QPushButton { "
        "  background: #10b981; border: none; border-radius: 12px; "
        "  color: white; font-weight: bold; font-size: 15px; padding: 0 15px; "
        "} "
        "QPushButton:hover { background: #059669; }"
    );
    connect(completeBtn, &QPushButton::clicked, this, [this, roomId, status]() {
        PetDataManager::instance()->removeRoomStatusPeriod(roomId, status);
        showCheckInForm();
        emit dataChanged();
    });
    layout->addWidget(completeBtn);

    QPushButton *bookFutureBtn = new QPushButton("📅 预约该房间后续时段");
    bookFutureBtn->setFixedHeight(52);
    bookFutureBtn->setCursor(Qt::PointingHandCursor);
    bookFutureBtn->setStyleSheet(
        "QPushButton { "
        "  background: white; border: 1.5px solid #722ed1; border-radius: 12px; "
        "  color: #722ed1; font-weight: bold; font-size: 15px; padding: 0 15px; "
        "} "
        "QPushButton:hover { background: #f9f0ff; border-color: #9254de; }"
    );
    connect(bookFutureBtn, &QPushButton::clicked, this, [this, roomId]() {
        showCheckInForm(roomId, QDate::currentDate().addDays(2)); 
    });
    layout->addWidget(bookFutureBtn);

    m_contentLayout->addWidget(m_currentWidget);
}

void FosterActionPanel::showManagementView(int roomId, const QString &petId, const QString &status) {
    clearContentLayout();
    m_currentWidget = new QWidget();
    m_currentWidget->setStyleSheet("QLabel { border: none; background: transparent; }");
    QVBoxLayout *layout = new QVBoxLayout(m_currentWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    PetInfo info = PetDataManager::instance()->getPet(petId);

    // 1. 顶部：宠物名片 (精简版)
    QHBoxLayout *header = new QHBoxLayout();
    header->setSpacing(15);
    m_currentInfo = info;
    m_currentPeriodStart = info.fosterStartTime;
    m_currentPeriodEnd = info.fosterEndTime;
    m_avatarLabel = new QLabel(info.name.left(1));
    m_avatarLabel->setFixedSize(70, 70);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setStyleSheet("background: #f0f2f5; color: #409eff; font-size: 28px; font-weight: bold; border-radius: 35px; border: 2px solid #fff;");
    m_avatarLabel->setCursor(Qt::PointingHandCursor);
    m_avatarLabel->installEventFilter(this);
    
    // 加载真实头像
    QPixmap pix = PetDataManager::instance()->getPetPixmap(petId);
    if (!pix.isNull()) {
        m_avatarLabel->setText("");
        if (!pix.isNull()) {
            QPixmap target(70, 70);
            target.fill(Qt::transparent);
            QPainter p(&target);
            p.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addEllipse(0, 0, 70, 70);
            p.setClipPath(path);
            p.drawPixmap(0, 0, 70, 70, pix.scaled(70, 70, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            
            p.setClipping(false);
            p.setPen(QPen(Qt::white, 2));
            p.drawEllipse(1, 1, 68, 68);

            m_avatarLabel->setPixmap(target);
            m_avatarLabel->setStyleSheet("background: transparent; border: none;");
        }
    }
    
    QLabel *avatar = m_avatarLabel;
    QVBoxLayout *nameCol = new QVBoxLayout();
    nameCol->setSpacing(4);
    QLabel *nameLbl = new QLabel(info.name);
    nameLbl->setStyleSheet("font-size: 22px; font-weight: bold; color: #303133; border: none; background: transparent;");
    
    QHBoxLayout *detailRow = new QHBoxLayout();
    detailRow->setSpacing(10);
    QString genderHtml = getGenderBadgeHtml(info.gender);
    QLabel *breedLbl = new QLabel(QString("%1 · %2").arg(info.breed, genderHtml));
    breedLbl->setStyleSheet("font-size: 13px; color: #606266; border: none; background: transparent;");
    
    QLabel *ownerIdLbl = new QLabel(QString("主人: %1 | ID: %2").arg(info.ownerName, info.id));
    ownerIdLbl->setStyleSheet("font-size: 12px; color: #909399; border: none; background: transparent;");
    
    detailRow->addWidget(breedLbl);
    detailRow->addStretch();
    
    nameCol->addWidget(nameLbl);
    nameCol->addWidget(ownerIdLbl); // 添加主人与 ID
    nameCol->addLayout(detailRow);
    
    // --- 周期切换按钮 (移至姓名右侧) ---
    QPushButton *periodBtn = new QPushButton();
    auto updatePeriodText = [periodBtn, info](const QString &start, const QString &end) {
        QString displayEnd = (end == "至今" || end.isEmpty()) ? (info.fosterEndTime.isEmpty() ? "未设定" : info.fosterEndTime) : end;
        periodBtn->setText(QString("%1 ~ %2").arg(start, displayEnd));
    };
    
    periodBtn->setCursor(Qt::PointingHandCursor);
    periodBtn->setStyleSheet(
        "QPushButton { background: #f0f7ff; color: #409eff; border: 1px solid #d1e9ff; border-radius: 15px; padding: 4px 15px; font-size: 11px; font-weight: bold; } "
        "QPushButton:hover { background: #e1f0ff; }"
    );
    
    header->addWidget(avatar);
    header->addLayout(nameCol);
    header->addStretch();
    header->addWidget(periodBtn, 0, Qt::AlignVCenter);
    header->addSpacing(20); // 向左偏移，预留右侧间距
    layout->addLayout(header);

    // 2. 全能看板卡片 (全新大卡片布局)
    QFrame *detailCard = new QFrame();
    detailCard->setStyleSheet("background: #fcfcfd; border: 1px solid #f0f2f5; border-radius: 12px;");
    QVBoxLayout *cardLayout = new QVBoxLayout(detailCard);
    cardLayout->setContentsMargins(15, 12, 15, 12);
    cardLayout->setSpacing(10);
    

    // 预留更新接口
    QLabel *weightInL = new QLabel(QString("%1 kg").arg(info.weight, 0, 'f', 1));
    QLabel *weightOutL = new QLabel("尚未结算");
    QLabel *dateInL = new QLabel(info.fosterStartTime);
    QLabel *dateOutL = new QLabel(info.fosterEndTime.isEmpty() ? "至今" : info.fosterEndTime);
    QLabel *durationL = new QLabel("1 天");
    
    auto styleVal = [](QLabel *l, const QString &c) { l->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; border: none; background: transparent;").arg(c)); };
    styleVal(weightInL, "#e6a23c");
    styleVal(weightOutL, "#909399");
    styleVal(dateInL, "#303133");
    styleVal(dateOutL, "#303133");
    styleVal(durationL, "#f56c6c");

    auto addRow = [&](const QString &l1, QLabel *v1, const QString &l2, QLabel *v2) {
        QHBoxLayout *r = new QHBoxLayout();
        auto makePart = [&](const QString &l, QLabel *v) {
            QVBoxLayout *c = new QVBoxLayout(); c->setSpacing(2);
            QLabel *title = new QLabel(l); title->setStyleSheet("color: #909399; font-size: 10px; border: none; background: transparent;");
            c->addWidget(title); c->addWidget(v);
            return c;
        };
        r->addLayout(makePart(l1, v1), 1);
        r->addLayout(makePart(l2, v2), 1);
        cardLayout->addLayout(r);
    };

    addRow("入住体重", weightInL, "离店体重", weightOutL);
    cardLayout->addSpacing(8);
    // 离店时间标题需要动态更新
    QLabel *dateOutTitle = new QLabel("离店时间");
    dateOutTitle->setStyleSheet("color: #909399; font-size: 10px; border: none; background: transparent;");
    
    auto addTimeRow = [&](QLabel *title1Obj, QLabel *v1, QLabel *title2Obj, QLabel *v2) {
        QHBoxLayout *r = new QHBoxLayout();
        auto makePart = [&](QLabel *title, QLabel *v) {
            QVBoxLayout *c = new QVBoxLayout(); c->setSpacing(2);
            title->setStyleSheet("color: #909399; font-size: 10px; border: none; background: transparent;");
            c->addWidget(title); c->addWidget(v);
            return c;
        };
        r->addLayout(makePart(title1Obj, v1), 1);
        r->addLayout(makePart(title2Obj, v2), 1);
        cardLayout->addLayout(r);
    };

    QLabel *dateInTitle = new QLabel("入住时间");
    addTimeRow(dateInTitle, dateInL, dateOutTitle, dateOutL);
    QLabel *emptyPlaceholder = new QLabel();
    emptyPlaceholder->setStyleSheet("border: none; background: transparent;");
    addRow("入住天数", durationL, "", emptyPlaceholder); // 占位

    layout->addWidget(detailCard);

        // --- 接送服务追踪卡片已根据需求移除 ---

    // 辅助函数：根据批次更新卡片
    auto updateCard = [=](const FosterBatch &b) {
        dateInL->setText(b.startTime);
        
        QDate s = QDate::fromString(b.startTime, "yyyy-MM-dd");
        QDate e = b.isActive ? QDate::currentDate() : QDate::fromString(b.endTime, "yyyy-MM-dd");
        int d = s.isValid() && e.isValid() ? s.daysTo(e) + 1 : 1;
        durationL->setText(QString("%1 天").arg(d));
        
        if (b.isActive) {
            dateInTitle->setText(status == "booked" ? "预计入住时间" : "入住时间");
            dateOutTitle->setText(status == "booked" ? "预计离店时间" : "预计离店时间");
            dateOutL->setText(info.fosterEndTime.isEmpty() ? "尚未设定" : info.fosterEndTime);
            
            weightInL->setText(QString("%1 kg").arg(info.weight, 0, 'f', 1));
            weightOutL->setText("尚未结算");
            weightOutL->setStyleSheet("color: #909399; font-size: 14px; font-weight: bold; border: none; background: transparent;");
        } else {
            dateOutTitle->setText("离店时间");
            dateOutL->setText(b.endTime);
            
            // 历史批次模拟数据
            weightInL->setText("20.5 kg");
            weightOutL->setText("21.2 kg");
            weightOutL->setStyleSheet("color: #67c23a; font-size: 14px; font-weight: bold; border: none; background: transparent;");
        }
    };

    // --- 统一初始化：默认加载第一个批次 (通常是当前入住) ---
    auto batches = PetDataManager::instance()->getHistoryBatches(petId);
    if (!batches.isEmpty()) {
        const auto &firstBatch = batches.first();
        updatePeriodText(firstBatch.startTime, firstBatch.endTime);
        updateCard(firstBatch);
    } else {
        updatePeriodText(info.fosterStartTime, info.fosterEndTime);
        updateCard(FosterBatch{"B-CUR", info.fosterStartTime, "至今", true});
    }

    // 3. 记录与动态 (占据主要空间)
    QLabel *planTitle = new QLabel("寄养动态");
    planTitle->setStyleSheet("font-weight: bold; color: #303133; font-size: 14px; margin-top: 5px;");
    layout->addWidget(planTitle);

    PetTimelineWidget *miniTimeline = new PetTimelineWidget();
    miniTimeline->setLogs(PetDataManager::instance()->getLogs(petId));
    layout->addWidget(miniTimeline, 1); 

    // 底部核心操作区
    QFrame *actionFrame = new QFrame();
    actionFrame->setObjectName("ActionFrame"); // 方便后面查找或直接控制
    actionFrame->setStyleSheet("QFrame { background: #f8f9fb; border-top: 1px solid #eee; border-radius: 8px; }");
    QVBoxLayout *btnArea = new QVBoxLayout(actionFrame);
    btnArea->setContentsMargins(10, 15, 10, 10);
    btnArea->setSpacing(10);
    
    // ... (按钮定义保持不变，只需在切换时隐藏 actionFrame)
    QHBoxLayout *topBtns = new QHBoxLayout();
    QPushButton *logBtn = new QPushButton("记录影像");
    // ...
    logBtn->setFixedHeight(48);
    logBtn->setCursor(Qt::PointingHandCursor);
    logBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #722ed1; font-weight: bold; font-size: 13px; padding: 0 10px; } "
        "QPushButton:hover { border-color: #722ed1; background: #f9f0ff; }"
    );
    
    QPushButton *archiveBtn = new QPushButton("影像档案");
    archiveBtn->setFixedHeight(48);
    archiveBtn->setCursor(Qt::PointingHandCursor);
    archiveBtn->setStyleSheet(
        "QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #e6a23c; font-weight: bold; font-size: 13px; padding: 0 10px; } "
        "QPushButton:hover { border-color: #e6a23c; background: #fcf6ec; }"
    );
    topBtns->addWidget(logBtn, 1);
    topBtns->addWidget(archiveBtn, 1);
    btnArea->addLayout(topBtns);

    // 新增：日常清洁记录按钮 (Context-Aware: 入住房重点在服务)
    QPushButton *cleanBtn = new QPushButton("日常清洁 (记录服务)");
    cleanBtn->setFixedHeight(50);
    cleanBtn->setCursor(Qt::PointingHandCursor);
    cleanBtn->setStyleSheet(
        "QPushButton { background: #f0f9eb; border: 1px solid #e1f3d8; border-radius: 8px; color: #67c23a; font-weight: bold; font-size: 14px; padding: 0 15px; } "
        "QPushButton:hover { background: #e1f3d8; }"
    );
    connect(cleanBtn, &QPushButton::clicked, this, [this, petId, roomId]() {
        PetActivityLog log;
        log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
        log.type = "备注";
        log.icon = "";
        log.remark = "已完成房间日常清洁与消毒。";
        log.operatorName = "店员小利";
        log.roomNo = QString::number(roomId);
        PetDataManager::instance()->addActivityLog(petId, log);
        showManagementView(roomId, petId, "occupied"); // 刷新详情页内部列表
        emit dataChanged(); // 刷新外部网格/时间轴
    });
    btnArea->addWidget(cleanBtn);

    QPushButton *checkoutBtn = new QPushButton(status == "occupied" ? "办理 离店与结算" : "取消预约");
    checkoutBtn->setFixedHeight(52);
    checkoutBtn->setCursor(Qt::PointingHandCursor);
    QString checkoutColor = (status == "occupied" ? "#fa8c16" : "#f5222d");
    QString checkoutBg = (status == "occupied" ? "#fff7e6" : "#fff1f0");
    checkoutBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %2; border-radius: 8px; font-weight: bold; font-size: 14px; padding: 0 10px; } "
        "QPushButton:hover { opacity: 0.8; }"
    ).arg(checkoutBg, checkoutColor));
    btnArea->addWidget(checkoutBtn);

    layout->addWidget(actionFrame);

    // --- 交互逻辑 ---
    connect(logBtn, &QPushButton::clicked, this, [this, petId, miniTimeline]() {
        MediaUploadDialog dlg(petId, this->window());
        if (dlg.exec() == QDialog::Accepted) {
            // 录入成功后立即刷新右侧动态时间轴，实现“即录即显”
            miniTimeline->setLogs(PetDataManager::instance()->getLogs(petId));
        }
    });


    connect(archiveBtn, &QPushButton::clicked, this, [this, petId]() {
        PetInfo info = PetDataManager::instance()->getPet(petId);
        PetMediaArchiveDialog dlg(petId, info.name, 
                                 PetDataManager::instance()->getMedia(petId), 
                                 this->window(),
                                 m_currentPeriodStart, m_currentPeriodEnd);
        dlg.exec();
    });

    connect(checkoutBtn, &QPushButton::clicked, this, [this, petId, roomId]() {
        PetInfo info = PetDataManager::instance()->getPet(petId);
        if (info.status == "已预约") {
            // 取消预约逻辑
            if (CustomMessageDialog::confirm(this, "业务确认", "确定要取消本次寄养预约吗？该操作将释放房间配额。")) {
                PetDataManager::instance()->executeCancelBooking(roomId, petId);
                CustomMessageDialog::showSuccess(this, "操作成功", "预约已成功取消。");
                showCheckInForm();
            }
        } else {
            // 原有的离店结算逻辑 (Placeholder)
            CustomMessageDialog::showSuccess(this, "业务办理", QString("正在跳转至结算中心办理 Room %1 的相关手续...").arg(roomId));
        }
    });

    m_contentLayout->addWidget(m_currentWidget);
}

// ========================
// FosterModule 实现
// ========================

FosterModule::FosterModule(QWidget *parent) : QWidget(parent) {
    setupUI();
    m_currentForecastDate = QDate::currentDate();
    
    QTimer::singleShot(0, this, [this]() {
        onForecastDateChanged(m_currentForecastDate);
    });
}

void FosterModule::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20); // 统一 20px 对齐
    mainLayout->setSpacing(20);

    // 1. 顶部看板大容器 (Dashboard Card)
    QFrame *dashboardCard = new QFrame();
    dashboardCard->setObjectName("DashboardCard");
    dashboardCard->setFixedHeight(160); // 固定高度 160px，对齐会员模块
    dashboardCard->setStyleSheet("#DashboardCard { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QVBoxLayout *dashboardLayout = new QVBoxLayout(dashboardCard);
    dashboardLayout->setContentsMargins(25, 15, 25, 15); // 调整内边距对齐
    dashboardLayout->setSpacing(12);

    // 1.1 标题
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("寄养房态实时交互看板");
    titleLabel->setStyleSheet("font-size: 20px; color: #303133; font-weight: bold; border: none; background: transparent;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    dashboardLayout->addLayout(headerLayout);



    // 2. 统计概览层 (铺满宽度，均衡分布)
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(15);
    auto createStatCard = [&](const QString &icon, const QString &title, QLabel* &valLabel, const QString &accentColor) {
        QFrame *card = new QFrame();
        card->setFixedHeight(80);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // 均匀拉伸
        card->setStyleSheet("QFrame { background: white; border-radius: 8px; border: 1px solid #f1f5f9; } QLabel { border: none; background: transparent; }");
        
        QHBoxLayout *cl = new QHBoxLayout(card);
        cl->setContentsMargins(20, 10, 20, 10);
        cl->setSpacing(12);
        
        QLabel *iconLabel = new QLabel(icon);
        if (icon.isEmpty()) {
            iconLabel->hide();
        } else {
            iconLabel->setFixedSize(40, 40);
            iconLabel->setAlignment(Qt::AlignCenter);
            iconLabel->setStyleSheet(QString("font-size: 20px; background: white; border-radius: 8px; border: 1px solid #f1f5f9; color: %1;").arg(accentColor));
        }
        
        QVBoxLayout *vl = new QVBoxLayout();
        vl->setSpacing(2);
        QLabel *tl = new QLabel(title);
        tl->setStyleSheet("color: #94a3b8; font-size: 12px;");
        valLabel = new QLabel("0");
        valLabel->setStyleSheet(QString("color: #1e293b; font-size: 20px; font-weight: bold;"));
        vl->addWidget(tl);
        vl->addWidget(valLabel);
        vl->addStretch();

        if (!icon.isEmpty()) {
            cl->addWidget(iconLabel);
        }
        cl->addLayout(vl);
        cl->addStretch();
        return card;
    };

    statLayout->addWidget(createStatCard("", "房间总量", totalRoomsLabel, "#409eff"));
    statLayout->addWidget(createStatCard("", "入住房间数", occupiedLabel, "#f56c6c"));
    statLayout->addWidget(createStatCard("", "空闲房间", freeLabel, "#67c23a"));
    statLayout->addWidget(createStatCard("", "已被预约", bookedLabel, "#722ed1"));
    statLayout->addWidget(createStatCard("", "清洁/维护", cleaningLabel, "#e6a23c"));
    dashboardLayout->addLayout(statLayout);
    mainLayout->addWidget(dashboardCard);

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
    roomGrid->setHorizontalSpacing(25); // 增大水平间距
    roomGrid->setVerticalSpacing(20);   // 增大垂直间距
    roomGrid->setContentsMargins(25, 20, 25, 25); // 增大页边距
    roomGrid->setAlignment(Qt::AlignTop); // 移除 AlignLeft，允许自动平铺占满宽度

    m_scrollArea->setWidget(m_gridContainer);
    
    // 4. 组装 Master-Detail 布局
    QHBoxLayout *masterDetailLayout = new QHBoxLayout();
    masterDetailLayout->setSpacing(20);
    masterDetailLayout->setContentsMargins(0, 0, 0, 0);
    
    QWidget *leftWidget = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(15);

    // --- 操作中控台 (Operation Console) ---
    QFrame *operationCard = new QFrame();
    operationCard->setFixedHeight(64);
    operationCard->setStyleSheet("QFrame { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QHBoxLayout *filterLayout = new QHBoxLayout(operationCard);
    filterLayout->setContentsMargins(15, 0, 15, 0);
    filterLayout->setSpacing(10);

    QPushButton *toggleBtn = new QPushButton("切换为时间轴");
    toggleBtn->setFixedWidth(110);
    toggleBtn->setFixedHeight(36);
    toggleBtn->setCursor(Qt::PointingHandCursor);
    toggleBtn->setStyleSheet(
        "QPushButton { background: #f8fafc; border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; color: #606266; font-size: 12px; font-weight: bold; } "
        "QPushButton:hover { background: #ecf5ff; border-color: #409eff; color: #409eff; }"
    );
    connect(toggleBtn, &QPushButton::clicked, this, &FosterModule::onToggleViewMode);
    filterLayout->addWidget(toggleBtn);



    leftLayout->addWidget(operationCard);

    // --- 照搬车辆调度模块的日期筛选样式 ---
    filterLayout->addSpacing(10);
    QLabel *foreLbl = new QLabel("预测日期:");
    foreLbl->setStyleSheet("color: #606266; font-size: 13px; font-weight: bold; border: none; background: transparent;");
    filterLayout->addWidget(foreLbl);

    QPushButton *prevBtn = new QPushButton("< 上一天");
    prevBtn->setFixedSize(110, 36);
    prevBtn->setCursor(Qt::PointingHandCursor);
    prevBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 6px; color: #606266; font-weight: bold; font-size: 12px; text-align: center; padding: 0; } "
                           "QPushButton:hover { background: #eff6ff; color: #3b82f6; border-color: #3b82f6; } "
                           "QPushButton:disabled { background: white; color: #cbd5e1; border-color: #f1f5f9; }");
    filterLayout->addWidget(prevBtn);

    forecastDateBtn = new QPushButton("今天");
    forecastDateBtn->setFixedSize(140, 36);
    forecastDateBtn->setCursor(Qt::PointingHandCursor);
    forecastDateBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #3b82f6; border-radius: 6px; color: #3b82f6; font-weight: bold; font-size: 13px; text-align: center; padding: 0; } "
                                   "QPushButton:hover { background: #eff6ff; }");
    filterLayout->addWidget(forecastDateBtn);

    QPushButton *nextBtn = new QPushButton("下一天 >");
    nextBtn->setFixedSize(110, 36);
    nextBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 6px; color: #606266; font-weight: bold; font-size: 12px; text-align: center; padding: 0; } "
                           "QPushButton:hover { background: #eff6ff; color: #3b82f6; border-color: #3b82f6; } "
                           "QPushButton:disabled { background: white; color: #cbd5e1; border-color: #f1f5f9; }");
    filterLayout->addWidget(nextBtn);

    filterLayout->addSpacing(5);
    QPushButton *todayBtn = new QPushButton("回到今天");
    todayBtn->setFixedSize(110, 36);
    todayBtn->setCursor(Qt::PointingHandCursor);
    todayBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 18px; color: #606266; font-weight: bold; font-size: 12px; text-align: center; padding: 0; } "
                            "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #409eff; }");
    filterLayout->addWidget(todayBtn);

    connect(prevBtn, &QPushButton::clicked, this, [=]() { onForecastDateChanged(m_currentForecastDate.addDays(-1)); });
    connect(forecastDateBtn, &QPushButton::clicked, this, &FosterModule::onDatePickerClicked);
    connect(nextBtn, &QPushButton::clicked, this, [=]() { onForecastDateChanged(m_currentForecastDate.addDays(1)); });
    connect(todayBtn, &QPushButton::clicked, this, [=]() { onForecastDateChanged(QDate::currentDate()); });

    // 在日期控件组后添加弹簧，将功能按钮推至最右侧
    filterLayout->addStretch();

    QPushButton *quickBookBtn = new QPushButton("快速预约开单");
    quickBookBtn->setFixedSize(160, 42); // 增大尺寸
    quickBookBtn->setCursor(Qt::PointingHandCursor);
    quickBookBtn->setStyleSheet(
        "QPushButton { "
        "  background: #722ed1; color: white; border: none; border-radius: 8px; "
        "  font-weight: bold; font-size: 14px; padding: 5px 20px; " // 增加内边距
        "} "
        "QPushButton:hover { background: #9254de; }"
    );
    connect(quickBookBtn, &QPushButton::clicked, this, [this]() {
        m_actionPanel->showCheckInForm(-1);
    });
    filterLayout->addWidget(quickBookBtn);


    m_calendar = new CompactCalendar(); // 持久化日历对象用于高亮同步

    // 防抖刷新定时器：多个 globalDataChanged 信号在短时间内连发时，只执行一次刷新
    m_refreshDebounce = new QTimer(this);
    m_refreshDebounce->setSingleShot(true);
    m_refreshDebounce->setInterval(300);
    connect(m_refreshDebounce, &QTimer::timeout, this, [this]() {
        onForecastDateChanged(m_currentForecastDate);
    });

    // 重新连接数据更新信号（通过防抖定时器）
    connect(PetDataManager::instance(), &PetDataManager::globalDataChanged, this, [this]() {
        m_refreshDebounce->start(); // 每次信号都重启计时器，最终只触发一次
    });
    connect(PetDataManager::instance(), &PetDataManager::petDataChanged, this, [this](const QString &) {
        m_refreshDebounce->start();
    });


    // 视图堆栈容器（圆角边框包裹，与顶部看板风格一致）
    QFrame *contentWrapper = new QFrame();
    contentWrapper->setObjectName("FosterContentWrapper");
    contentWrapper->setStyleSheet(
        "#FosterContentWrapper { background: white; border-radius: 12px; border: 1px solid #f0f0f0; }"
    );
    QVBoxLayout *wrapperLayout = new QVBoxLayout(contentWrapper);
    wrapperLayout->setContentsMargins(1, 1, 1, 1); // 留出 1px 避免内容切断边框
    
    m_viewStack = new QStackedWidget();
    m_viewStack->addWidget(m_scrollArea);
    wrapperLayout->addWidget(m_viewStack);
    
    // 初始化时间轴表格
    m_ganttView = new QTableView();
    m_ganttModel = new FosterGanttModel(this);
    m_ganttView->setModel(m_ganttModel);
    m_ganttView->setItemDelegate(new FosterGanttDelegate(this));
    m_ganttView->setSelectionMode(QAbstractItemView::NoSelection);
    m_ganttView->setShowGrid(true);
    m_ganttView->setGridStyle(Qt::DotLine);
    m_ganttView->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_ganttView->horizontalHeader()->setDefaultSectionSize(60);
    m_ganttView->verticalHeader()->setDefaultSectionSize(44);
    m_ganttView->verticalHeader()->setFixedWidth(130); // 略微增加宽度以适应色块

    m_ganttView->verticalHeader()->setStyleSheet(
        "QHeaderView::section { border: none; border-right: 1px solid #f0f0f0; "
        "border-bottom: 1px solid #f0f0f0; padding-left: 5px; font-weight: bold; color: #303133; } "
    );

    m_ganttView->setStyleSheet("QTableView { background: white; border: none; gridline-color: #f0f0f0; }");

    
    m_viewStack->addWidget(m_ganttView);
    leftLayout->addWidget(contentWrapper);

    connect(m_ganttView, &QTableView::clicked, this, [this](const QModelIndex &index) {
        if (!index.isValid()) return;
        QVariant data = m_ganttModel->data(index, Qt::UserRole);
        if (data.isNull() || data.toString().isEmpty()) {
            int roomId = 101 + index.row();
            QDate date = m_ganttModel->startDate().addDays(index.column());
            m_actionPanel->showCheckInForm(roomId, date);
        }
    });





    m_actionPanel = new FosterActionPanel();
    m_actionPanel->setFixedWidth(450);
    
    masterDetailLayout->addWidget(leftWidget, 3);
    masterDetailLayout->addWidget(m_actionPanel, 1);
    
    mainLayout->addLayout(masterDetailLayout);

    connect(m_actionPanel, &FosterActionPanel::avatarClicked, this, &FosterModule::showBigImage);
    connect(m_actionPanel, &FosterActionPanel::dataChanged, this, [this]() {
        onForecastDateChanged(m_currentForecastDate);
    });

    // --- 初始化全屏大图预览层 (抄自 PetModule) ---
    m_imagePreviewOverlay = new QWidget(this);
    m_imagePreviewOverlay->setObjectName("FosterPreviewOverlay");
    m_imagePreviewOverlay->setStyleSheet("#FosterPreviewOverlay { background-color: rgba(0, 0, 0, 180); }");
    m_imagePreviewOverlay->hide();
    m_imagePreviewOverlay->installEventFilter(this); // 点击遮罩任意位置关闭
    
    QVBoxLayout *previewL = new QVBoxLayout(m_imagePreviewOverlay);
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    previewL->addWidget(m_previewLabel);
    
    // 默认显示预约表单，不再让右侧区域留白
    m_actionPanel->showCheckInForm();
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

void FosterModule::onDatePickerClicked() {
    QDialog dialog(this);
    dialog.setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    dialog.setStyleSheet("QDialog { background: white; border: 1px solid #dcdfe6; border-radius: 12px; }");
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(10, 10, 10, 10);
    
    CompactCalendar *cal = new CompactCalendar(&dialog);
    cal->setSelectedDate(m_currentForecastDate);
    layout->addWidget(cal);
    
    connect(cal, &QCalendarWidget::clicked, [&dialog, this](const QDate &date){
        onForecastDateChanged(date);
        dialog.accept();
    });
    
    // 定位到按钮下方
    QPoint pos = forecastDateBtn->mapToGlobal(QPoint(0, forecastDateBtn->height() + 5));
    dialog.move(pos);
    dialog.exec();
}

void FosterModule::relayoutGrid() {
    if (!roomGrid || roomGrid->count() == 0) return;

    const int cardWidth = 210;  // 与 FosterCard::setFixedSize(210,155) 一致
    const int spacing = 15;     // 与 roomGrid->setHorizontalSpacing(15) 一致
    const int defaultCols = 4;

    int availableWidth = m_scrollArea->viewport()->width() - 40;
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

    // --- 文本格式化逻辑：支持“今天/明天”带日期显示 ---
    auto formatDate = [](const QDate &d) {
        if (d == QDate::currentDate()) return QString("今天 (%1)").arg(d.toString("MM/dd"));
        if (d == QDate::currentDate().addDays(1)) return QString("明天 (%1)").arg(d.toString("MM/dd"));
        return d.toString("yyyy/MM/dd");
    };
    forecastDateBtn->setText(formatDate(date));
    
    // 同步高亮样式并确保居中
    if (date == QDate::currentDate()) {
        forecastDateBtn->setStyleSheet("QPushButton { text-align: center; padding: 0 15px; background: #e1f0ff; border: 1px solid #b3d8ff; border-radius: 8px; color: #409eff; font-weight: bold; font-size: 13px; } "
                                       "QPushButton:hover { background: #c6e2ff; }");
    } else {
        forecastDateBtn->setStyleSheet("QPushButton { text-align: center; padding: 0 15px; background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #606266; font-weight: bold; font-size: 13px; } "
                                       "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #409eff; }");
    }

    if (m_ganttModel) m_ganttModel->setStartDate(date.addDays(-2));
    
    if (m_calendar) {
        int daysToMon = date.dayOfWeek() - 1;
        QDate weekStart = date.addDays(-daysToMon);
        QDate weekEnd = weekStart.addDays(6);
        m_calendar->setFosterRange(weekStart, weekEnd);
        m_calendar->update();
    }

    // 异步清空现有网格，防止正在处理事件的控件被立即删除导致崩溃
    while (roomGrid->count() > 0) {
        QLayoutItem *item = roomGrid->takeAt(0);
        if (item->widget()) {
            item->widget()->hide();
            item->widget()->deleteLater();
        }
        delete item;
    }

    const int cardWidth = 210;  // 与 FosterCard::setFixedSize(210,155) 一致
    const int spacing = 25;     // 同步增大间距
    const int defaultCols = 4;

    int availableWidth = m_scrollArea->viewport()->width();
    int cols = (availableWidth > cardWidth)
               ? qMax(1, (availableWidth + spacing) / (cardWidth + spacing))
               : defaultCols;

    QList<BoardingRoom> rooms = PetDataManager::instance()->allRooms();
    if (rooms.isEmpty()) {
        // Fallback for UI if rooms not loaded yet (simulate 20 rooms)
        for (int i = 101; i <= 120; ++i) {
            BoardingRoom r; r.id = i; r.roomNo = QString::number(i);
            r.type = (i >= 111 && i <= 115) ? "豪华房" : (i >= 116 && i <= 120 ? "多宠房" : "标准房");
            r.status = "空闲";
            rooms << r;
        }
    }
    
    QList<PetInfo> allPets = PetDataManager::instance()->allPets();
    
    for (int i = 0; i < rooms.size(); ++i) {
        const BoardingRoom &room = rooms[i];
        int roomIdInt = room.id;
        QString roomIdStr = room.roomNo;
        
        QString status = "free";
        if (room.status == "清理中") status = "cleaning";
        else if (room.status == "维护中") status = "maintenance";
        else if (room.status == "入住中") status = "occupied";

        QString pid, pname, oname, breed;
        
        // 房态匹配逻辑：从真实 PetDataManager 中检索 (日期敏感型)
        bool found = false;
        for (const auto &pet : allPets) {
            bool roomMatch = (pet.roomNo == roomIdStr) || 
                           (pet.roomNo == QString::number(roomIdInt)) ||
                           (!pet.roomNo.isEmpty() && pet.roomNo.toInt() == roomIdInt);

            if (roomMatch) {
                QDate s = QDate::fromString(pet.fosterStartTime, "yyyy-MM-dd");
                QDate e = (pet.fosterEndTime == "至今" || pet.fosterEndTime.isEmpty()) ? QDate::currentDate().addYears(1) : QDate::fromString(pet.fosterEndTime, "yyyy-MM-dd");
                
                // 检查选定日期是否落在此宠物的寄养/预约区间内
                if (date >= s && date <= e) {
                    if (pet.status.contains("寄养中") || pet.status.contains("在店") || pet.status.contains("洗护中")) {
                        status = "occupied";
                    } else if (pet.status.contains("预约") || pet.status.contains("待寄养") || pet.status.contains("待入店")) {
                        status = "booked";
                    } else {
                        continue; // 离店等状态不显示在看板
                    }
                    
                    pid = pet.id;
                    pname = pet.name;
                    oname = pet.ownerName;
                    breed = pet.breed;
                    found = true;
                    break;
                }
            }
        }

        // 如果没有宠物占用，从数据管理器获取维护/清洁状态 (日期敏感)
        if (!found && (status == "free" || status == "occupied")) {
            auto periods = PetDataManager::instance()->getRoomStatusPeriods(roomIdInt);
            for (const auto &p : periods) {
                QDate s = QDate::fromString(p.startTime.split(" ").first(), "yyyy-MM-dd");
                QDate e = QDate::fromString(p.endTime.split(" ").first(), "yyyy-MM-dd");
                if (date >= s && date <= e) {
                    status = p.type;
                    found = true;
                    break;
                }
            }
        }

        FosterCard *card = new FosterCard(roomIdInt, status, room.type, pid, pname, breed, oname, this);
        connect(card, &FosterCard::clicked, this, &FosterModule::onCardClicked);
        roomGrid->addWidget(card, i / cols, i % cols, Qt::AlignTop);
    }
    // 强制每列等宽，避免卡片错位
    for (int c = 0; c < cols; ++c) {
        roomGrid->setColumnMinimumWidth(c, cardWidth);
    }

    updateStats();
}

void FosterCard::setSelected(bool selected) {
    if (m_isSelected == selected) return;
    m_isSelected = selected;
    
    QString cardBorder;
    if (m_roomType == "豪华房") cardBorder = "#1a237e";
    else if (m_roomType == "多宠房") cardBorder = "#d84315";
    else cardBorder = "#80cbc4";

    QString bodyBg;
    if (m_status == "occupied") bodyBg = "#f0f7ff";
    else if (m_status == "booked") bodyBg = "#f9f0ff";
    else if (m_status == "cleaning") bodyBg = "#e0f7fa";
    else if (m_status == "maintenance") bodyBg = "#fff3e0";
    else bodyBg = "white";

    if (m_isSelected) {
        // 选中状态：使用品牌蓝并加粗边框
        setStyleSheet(QString(
            "FosterCard { background: %1; border-radius: 14px; border: 4px solid #1890ff; }"
        ).arg(bodyBg));
    } else {
        setStyleSheet(QString(
            "FosterCard { background: %1; border-radius: 14px; border: 2px solid %2; }"
        ).arg(bodyBg, cardBorder));
    }
}

void FosterModule::onCardClicked() {
    FosterCard *card = qobject_cast<FosterCard*>(sender());
    if (!card) return;

    // 互斥选中：清除之前的选中状态
    for (int i = 0; i < roomGrid->count(); ++i) {
        FosterCard *c = qobject_cast<FosterCard*>(roomGrid->itemAt(i)->widget());
        if (c) c->setSelected(c == card);
    }

    m_actionPanel->resetChanges(); // 切换时直接重置
    m_actionPanel->updatePanel(card->roomId(), card->status(), card->petId(), card->petName());
}


void FosterModule::onToggleViewMode() {
    m_isTimelineMode = !m_isTimelineMode;
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    
    // 查找标题栏中的日期标签，使其更具语义化
    QLabel *foreLbl = nullptr;
    QList<QLabel*> labels = findChildren<QLabel*>();
    for (auto l : labels) {
        if (l->text().contains("日期")) {
            foreLbl = l;
            break;
        }
    }

    if (m_isTimelineMode) {
        if (btn) btn->setText("切换为看板");
        if (foreLbl) foreLbl->setText("时间轴视图日期：");
        m_viewStack->setCurrentWidget(m_ganttView);
        m_ganttModel->setStartDate(m_currentForecastDate.addDays(-2));
    } else {
        if (btn) btn->setText("切换为时间轴");
        if (foreLbl) foreLbl->setText("房态模拟预测日期：");
        m_viewStack->setCurrentWidget(m_scrollArea);
        onForecastDateChanged(m_currentForecastDate);
    }
}

void FosterModule::updateStats() {
    int total = 0, occ = 0, freeTab = 0, unusable = 0, booked = 0;
    for (int i = 0; i < roomGrid->count(); ++i) {
        FosterCard *c = qobject_cast<FosterCard*>(roomGrid->itemAt(i)->widget());
        if (!c) continue;
        total++;
        if (c->status() == "occupied") occ++;
        else if (c->status() == "cleaning" || c->status() == "maintenance") unusable++;
        else if (c->status() == "booked") booked++;
        else freeTab++;
    }
    totalRoomsLabel->setText(QString("%1 间").arg(total));
    occupiedLabel->setText(QString("%1 间").arg(occ));
    freeLabel->setText(QString("%1 间").arg(freeTab));
    bookedLabel->setText(QString("%1 间").arg(booked));
    cleaningLabel->setText(QString("%1 间").arg(unusable));
}

void FosterModule::showBigImage(const QString &path)
{
    if (path.isEmpty()) return;
    (new FullImagePreviewDialog(QStringList{path}, 0, this->window()))->show();
}

void FosterModule::hideBigImage()
{
    m_imagePreviewOverlay->hide();
}

// 补充 eventFilter 处理关闭预览
bool FosterModule::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_imagePreviewOverlay && event->type() == QEvent::MouseButtonRelease) {
        hideBigImage();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}
