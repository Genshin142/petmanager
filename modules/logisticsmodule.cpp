#include "logisticsmodule.h"
#include "logisticsmanager.h"
#include "petdatamanager.h"
#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QFrame>
#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include "compactcalendar.h"
#include <QEvent>
#include <QMouseEvent>
#include <QTableWidget>
#include <QHeaderView>
#include <QMouseEvent>

class AvatarEventFilter : public QObject {
public:
    AvatarEventFilter(QLabel* avatar, QDialog* parentDialog, QString* pathPtr, QObject* parent = nullptr)
        : QObject(parent), m_avatar(avatar), m_dialog(parentDialog), m_pathPtr(pathPtr), m_previewDialog(nullptr) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            if (obj == m_avatar) {
                if (m_pathPtr && !m_pathPtr->isEmpty()) {
                    showBigImage(*m_pathPtr);
                }
                return true;
            } else if (m_previewDialog && (obj == m_previewDialog || obj->parent() == m_previewDialog)) {
                m_previewDialog->close();
                m_previewDialog = nullptr;
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
private:
    void showBigImage(const QString& path) {
        QPixmap pix(path);
        if (pix.isNull()) pix.load(":/images/load_img.jpg");
        
        QPixmap whiteBg(pix.size());
        whiteBg.fill(Qt::white);
        QPainter p(&whiteBg);
        p.drawPixmap(0, 0, pix);
        p.end();

        QWidget *topWin = m_dialog->parentWidget() ? m_dialog->parentWidget()->window() : m_dialog->window();

        m_previewDialog = new QDialog(m_dialog, Qt::FramelessWindowHint);
        m_previewDialog->setGeometry(topWin->geometry());
        m_previewDialog->setAttribute(Qt::WA_TranslucentBackground);
        
        QVBoxLayout *layout = new QVBoxLayout(m_previewDialog);
        layout->setContentsMargins(0, 0, 0, 0);
        
        QFrame *bg = new QFrame();
        bg->setStyleSheet("background-color: rgba(0, 0, 0, 220);");
        layout->addWidget(bg);
        
        QVBoxLayout *bgLayout = new QVBoxLayout(bg);
        bgLayout->setContentsMargins(0, 0, 0, 0);
        bgLayout->setAlignment(Qt::AlignCenter);
        
        QLabel *imgLabel = new QLabel();
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setStyleSheet("border: none; background: transparent;");
        
        int maxWidth = qMin(topWin->width() * 0.8, 800.0);
        int maxHeight = qMin(topWin->height() * 0.8, 800.0);
        imgLabel->setPixmap(whiteBg.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        bgLayout->addWidget(imgLabel, 0, Qt::AlignCenter);
        
        m_previewDialog->installEventFilter(this);
        imgLabel->installEventFilter(this);
        bg->installEventFilter(this);
        connect(m_previewDialog, &QDialog::finished, m_previewDialog, &QDialog::deleteLater);
        m_previewDialog->show();
        m_previewDialog->raise();
    }

    QLabel* m_avatar;
    QDialog* m_dialog;
    QString* m_pathPtr;
    QDialog* m_previewDialog;
};

LogisticsModule::LogisticsModule(QWidget *parent) : QWidget(parent)
{
    setupUI();
    connect(LogisticsManager::instance(), &LogisticsManager::logisticsDataChanged, this, &LogisticsModule::refreshTasks);
    refreshTasks();
    startTimer(500); // 500ms for breathing animation & time checking
}

void LogisticsModule::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(25, 25, 25, 25);
    m_mainLayout->setSpacing(20);

    // --- 1. Header Area ---
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("🚗 车辆调度指挥大厅");
    titleLabel->setStyleSheet("font-size: 28px; color: #303133; font-weight: 900; letter-spacing: 1px;");
    
    QPushButton *createBtn = new QPushButton("🚀 新建派车单");
    createBtn->setFixedHeight(40);
    createBtn->setCursor(Qt::PointingHandCursor);
    createBtn->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #409eff, stop:1 #66b1ff); "
        "color: white; border: none; border-radius: 8px; font-weight: bold; font-size: 15px; padding: 0 20px; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #66b1ff, stop:1 #8cc5ff); }"
    );
    connect(createBtn, &QPushButton::clicked, this, &LogisticsModule::showCreateTaskDialog);

    m_filterCombo = new QComboBox();
    m_filterCombo->addItems({"全部", "待处理", "进行中", "已完成"});
    m_filterCombo->setFixedWidth(160);
    m_filterCombo->setFixedHeight(40);
    m_filterCombo->setStyleSheet("QComboBox { padding: 5px 15px; border: 1.5px solid #dcdfe6; border-radius: 8px; font-size: 14px; background: white; }"
                                 "QComboBox::drop-down { border: none; width: 30px; }");
    connect(m_filterCombo, &QComboBox::currentTextChanged, this, &LogisticsModule::renderTaskCards);

    QPushButton *historyBtn = new QPushButton("📜 派单历史");
    historyBtn->setFixedHeight(40);
    historyBtn->setCursor(Qt::PointingHandCursor);
    historyBtn->setStyleSheet(
        "QPushButton { background: white; border: 1.5px solid #dcdfe6; color: #606266; "
        "border-radius: 8px; font-size: 14px; padding: 0 15px; }"
        "QPushButton:hover { background: #f5f7fa; border-color: #c0c4cc; color: #409eff; }"
    );
    connect(historyBtn, &QPushButton::clicked, this, &LogisticsModule::showHistoryDialog);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    QLabel *filterLbl = new QLabel("状态筛选:");
    filterLbl->setStyleSheet("font-size: 14px; font-weight: bold; color: #606266;");
    headerLayout->addWidget(filterLbl);
    headerLayout->addWidget(m_filterCombo);
    headerLayout->addSpacing(15);
    headerLayout->addWidget(historyBtn);
    headerLayout->addSpacing(10);
    headerLayout->addWidget(createBtn);

    m_mainLayout->addLayout(headerLayout);

    // --- 2. Statistics Dashboard ---
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(20);

    auto createStatCard = [](const QString &title, const QString &color, QLabel *&valLabel) -> QFrame* {
        QFrame *f = new QFrame();
        f->setFixedHeight(100);
        f->setStyleSheet(QString("QFrame { background: white; border-radius: 12px; border: 1px solid #ebeef5; border-left: 6px solid %1; }").arg(color));
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(15); shadow->setColor(QColor(0,0,0,15)); shadow->setOffset(0, 4);
        f->setGraphicsEffect(shadow);

        QVBoxLayout *l = new QVBoxLayout(f);
        QLabel *t = new QLabel(title);
        t->setStyleSheet("color: #909399; font-size: 14px; font-weight: bold; border: none;");
        valLabel = new QLabel("0");
        valLabel->setStyleSheet(QString("color: %1; font-size: 32px; font-weight: 900; border: none;").arg(color));
        
        l->addWidget(t);
        l->addWidget(valLabel);
        l->setAlignment(Qt::AlignVCenter);
        return f;
    };

    statsLayout->addWidget(createStatCard("今日总单量", "#303133", m_lblTotal), 1);
    statsLayout->addWidget(createStatCard("待指派/待处理", "#fa8c16", m_lblPending), 1);
    statsLayout->addWidget(createStatCard("司机运送中", "#409eff", m_lblTransit), 1);
    statsLayout->addWidget(createStatCard("已完成记录", "#67c23a", m_lblCompleted), 1);
    
    m_mainLayout->addLayout(statsLayout);

    // --- 3. Scroll Area (Task Grid) ---
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("QScrollArea { background: transparent; } QScrollArea > QWidget > QWidget { background: transparent; }");
    
    m_cardsContainer = new QWidget();
    m_scrollArea->setWidget(m_cardsContainer);
    
    m_mainLayout->addWidget(m_scrollArea);
}

void LogisticsModule::refreshTasks()
{
    updateStatistics();
    renderTaskCards(m_filterCombo->currentText());
}

void LogisticsModule::updateStatistics()
{
    auto tasks = LogisticsManager::instance()->getAllTasks();
    int pending = 0, transit = 0, completed = 0;
    
    for (const auto &t : tasks) {
        if (t.status == "待处理") pending++;
        else if (t.status == "进行中") transit++;
        else if (t.status == "已完成") completed++;
    }
    
    m_lblTotal->setText(QString::number(tasks.size()));
    m_lblPending->setText(QString::number(pending));
    m_lblTransit->setText(QString::number(transit));
    m_lblCompleted->setText(QString::number(completed));
}

void LogisticsModule::renderTaskCards(const QString &filterStatus)
{
    // Clean up old layout
    if (m_cardsContainer->layout()) {
        QLayoutItem *child;
        while ((child = m_cardsContainer->layout()->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }
        delete m_cardsContainer->layout();
    }

    QGridLayout *gridLayout = new QGridLayout(m_cardsContainer);
    gridLayout->setSpacing(20);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    QList<LogisticsTask> allTasks = LogisticsManager::instance()->getAllTasks();
    int row = 0;
    int col = 0;
    int maxCols = 4;

    for (const auto &task : allTasks) {
        if (filterStatus != "全部" && task.status != filterStatus) continue;

        PetInfo petInfo = PetDataManager::instance()->getPet(task.petId);

        QFrame *card = new QFrame();
        card->setFixedSize(360, 240); // Slightly larger to fit avatar
        card->setObjectName("TaskCard");
        card->setProperty("taskStatus", task.status);
        card->setProperty("appointmentTime", task.appointmentTime);
        card->setStyleSheet("QFrame#TaskCard { background: white; border-radius: 12px; border: 1px solid #ebeef5; }"
                            "QFrame#TaskCard:hover { border: 1.5px solid #409eff; }");
        
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
        shadow->setBlurRadius(20); shadow->setColor(QColor(0,0,0,15)); shadow->setOffset(0, 8);
        card->setGraphicsEffect(shadow);
        
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(15, 15, 15, 15);
        cardLayout->setSpacing(8);

        // Top Row: Type and Status
        QHBoxLayout *topRow = new QHBoxLayout();
        QLabel *typeLabel = new QLabel((task.type.contains("接") ? "📥 " : "📤 ") + task.type);
        typeLabel->setStyleSheet("font-weight: 900; color: #303133; font-size: 16px; border: none;");
        
        QLabel *statusLabel = new QLabel(task.status);
        QString statusColor = (task.status == "待处理") ? "#909399" : (task.status == "进行中" ? "#409eff" : "#67c23a");
        statusLabel->setStyleSheet(QString("background: %1; color: white; padding: 3px 8px; border-radius: 4px; font-size: 12px; font-weight: bold; border: none;").arg(statusColor));
        
        topRow->addWidget(typeLabel);
        topRow->addStretch();
        topRow->addWidget(statusLabel);
        cardLayout->addLayout(topRow);

        // Pet & Owner Info with Avatar
        QHBoxLayout *petRow = new QHBoxLayout();
        petRow->setSpacing(12);

        QLabel *avatarLabel = new QLabel();
        avatarLabel->setFixedSize(50, 50);
        QPixmap pixmap(petInfo.avatarPath);
        if (pixmap.isNull()) pixmap.load(":/images/load_img.jpg");
        
        QSize avatarSize(50, 50);
        QPixmap target(avatarSize);
        target.fill(Qt::transparent);
        QPainter p(&target);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addEllipse(0, 0, avatarSize.width(), avatarSize.height());
        p.setClipPath(path);
        
        QPixmap scaledPix = pixmap.scaled(avatarSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int xOff = (scaledPix.width() - avatarSize.width()) / 2;
        int yOff = (scaledPix.height() - avatarSize.height()) / 2;
        p.drawPixmap(0, 0, scaledPix, xOff, yOff, avatarSize.width(), avatarSize.height());
        avatarLabel->setPixmap(target);
        avatarLabel->setStyleSheet("border: none;");
        avatarLabel->setProperty("avatarPath", petInfo.avatarPath);
        avatarLabel->setCursor(Qt::PointingHandCursor);
        avatarLabel->installEventFilter(this);

        QVBoxLayout *petTextLayout = new QVBoxLayout();
        petTextLayout->setSpacing(2);
        
        QLabel *petNameLbl = new QLabel(QString("🐾 %1 (%2)").arg(petInfo.name.isEmpty() ? task.petId : petInfo.name, task.petId));
        petNameLbl->setStyleSheet("color: #303133; font-size: 15px; border: none; font-weight: bold;");
        
        QLabel *ownerLbl = new QLabel(QString("👤 %1 | 📞 %2").arg(petInfo.ownerName.isEmpty() ? "未知" : petInfo.ownerName, 
                                                                   petInfo.ownerPhone.isEmpty() ? "无号码" : petInfo.ownerPhone));
        ownerLbl->setStyleSheet("color: #909399; font-size: 12px; border: none;");
        
        petTextLayout->addWidget(petNameLbl);
        petTextLayout->addWidget(ownerLbl);
        
        petRow->addWidget(avatarLabel);
        petRow->addLayout(petTextLayout);
        petRow->addStretch();
        cardLayout->addLayout(petRow);

        // Address & Time
        QLabel *addrLabel = new QLabel("📍 地址: " + (task.address.isEmpty() ? "未填写" : task.address));
        addrLabel->setStyleSheet("color: #606266; font-size: 13px; border: none;");
        addrLabel->setWordWrap(true);
        cardLayout->addWidget(addrLabel);

        QLabel *timeLabel = new QLabel("⏰ 预约: " + task.appointmentTime);
        timeLabel->setStyleSheet("color: #909399; font-size: 13px; border: none;");
        cardLayout->addWidget(timeLabel);

        cardLayout->addStretch();

        // Action Buttons
        if (task.status != "已完成") {
            QHBoxLayout *btnRow = new QHBoxLayout();
            btnRow->setSpacing(10);
            
            QPushButton *actionBtn = new QPushButton();
            actionBtn->setFixedHeight(44); // Increased height
            actionBtn->setCursor(Qt::PointingHandCursor);
            
            if (task.status == "待处理") {
                actionBtn->setText("🚀 司机出发");
                actionBtn->setStyleSheet(
                    "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #fa8c16, stop:1 #ffd591); color: white; border-radius: 8px; font-weight: bold; border: none; font-size: 15px; padding: 0 20px; }"
                    "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ffd591, stop:1 #fa8c16); }"
                );
                connect(actionBtn, &QPushButton::clicked, this, [=]() {
                    LogisticsManager::instance()->updateTaskStatus(task.taskId, "进行中");
                    
                    // 核心联动：更新宠物档案状态为“接送中 (在途)”
                    PetInfo info = PetDataManager::instance()->getPet(task.petId);
                    if (!info.id.isEmpty()) {
                        info.status = "接送中 (在途)";
                        PetDataManager::instance()->updatePet(info);
                    }
                });
            } else if (task.status == "进行中") {
                actionBtn->setText("🏁 确认送达");
                actionBtn->setStyleSheet(
                    "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #67c23a, stop:1 #a4da89); color: white; border-radius: 8px; font-weight: bold; border: none; font-size: 15px; padding: 0 20px; }"
                    "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #a4da89, stop:1 #67c23a); }"
                );
                connect(actionBtn, &QPushButton::clicked, this, [=]() {
                    if (CustomMessageDialog::confirm(this, "业务确认", "确认已安全送达目的地吗？")) {
                        LogisticsManager::instance()->updateTaskStatus(task.taskId, "已完成");
                        
                        // 写日志
                        PetActivityLog log;
                        log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
                        log.type = "备注"; log.icon = "🏁";
                        log.remark = QString("接送任务 [%1] 已圆满完成。").arg(task.type);
                        log.operatorName = "派车系统";
                        PetDataManager::instance()->addActivityLog(task.petId, log);
                    }
                });
            } else {
                actionBtn->setText("已完成");
                actionBtn->setEnabled(false);
                actionBtn->setStyleSheet("background: #f4f4f5; color: #bcbec2; border-radius: 6px; font-weight: bold; border: none;");
            }
            
            btnRow->addWidget(actionBtn);
            cardLayout->addLayout(btnRow);
        }

        gridLayout->addWidget(card, row, col);
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }
}

void LogisticsModule::showBigImage(const QString &path)
{
    if (path.isEmpty()) return;
    QPixmap pix(path);
    if (pix.isNull()) pix.load(":/images/load_img.jpg");

    QDialog *preview = new QDialog(this, Qt::FramelessWindowHint);
    preview->setGeometry(this->window()->geometry());
    preview->setAttribute(Qt::WA_TranslucentBackground);
    
    QVBoxLayout *layout = new QVBoxLayout(preview);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QFrame *bg = new QFrame();
    bg->setStyleSheet("background-color: rgba(0, 0, 0, 220);");
    layout->addWidget(bg);
    
    QVBoxLayout *bgLayout = new QVBoxLayout(bg);
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *imgLabel = new QLabel();
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setStyleSheet("border: none; background: transparent;");
    
    int maxWidth = qMin(this->window()->width() * 0.8, 800.0);
    int maxHeight = qMin(this->window()->height() * 0.8, 800.0);
    imgLabel->setPixmap(pix.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    bgLayout->addWidget(imgLabel, 0, Qt::AlignCenter);
    
    // Close on click anywhere
    preview->installEventFilter(this);
    imgLabel->installEventFilter(this);
    bg->installEventFilter(this);
    
    connect(preview, &QDialog::finished, preview, &QDialog::deleteLater);
    preview->show();
    preview->raise();
}

bool LogisticsModule::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        // 1. Click on card avatar
        if (watched->property("avatarPath").isValid()) {
            showBigImage(watched->property("avatarPath").toString());
            return true;
        }
        
        // 2. Click on preview overlay to close
        QDialog *previewDlg = qobject_cast<QDialog*>(watched);
        if (!previewDlg) {
             previewDlg = qobject_cast<QDialog*>(watched->parent());
        }
        if (previewDlg && previewDlg->windowFlags() & Qt::FramelessWindowHint) {
            previewDlg->close();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void LogisticsModule::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
    static double angle = 0.0;
    angle += 0.2; // Speed of breathing
    int alpha = static_cast<int>(127 + 127 * std::sin(angle));
    QString glowColor = QString("rgba(250, 140, 22, %1)").arg(alpha); // Orange glow

    QDateTime now = QDateTime::currentDateTime();
    QString todayStr = now.date().toString("yyyy-MM-dd");

    QList<QFrame*> cards = m_cardsContainer->findChildren<QFrame*>("TaskCard");
    for (QFrame* card : cards) {
        if (card->property("taskStatus").toString() != "待处理") {
            card->setStyleSheet("QFrame#TaskCard { background: white; border-radius: 12px; border: 1px solid #ebeef5; } "
                                "QFrame#TaskCard:hover { border: 1.5px solid #409eff; }");
            continue;
        }

        // Parse time slot: "2026-04-26 09:00 - 11:00"
        QString fullTime = card->property("appointmentTime").toString();
        if (fullTime.contains(todayStr)) {
            QString timePart = fullTime.split(" ").last(); // e.g. "09:00" if it was "yyyy-MM-dd HH:mm"
            // Wait, our format is "yyyy-MM-dd HH:mm - HH:mm"
            // Let's check the start time
            QString startTimeStr = fullTime.mid(11, 5); // Extracts "09:00"
            QTime startTime = QTime::fromString(startTimeStr, "HH:mm");
            
            // If current time is within 30 mins before or during the slot
            if (now.time() >= startTime.addSecs(-1800)) { 
                card->setStyleSheet(QString("QFrame#TaskCard { background: white; border-radius: 12px; border: 2px solid %1; }")
                                    .arg(glowColor));
            } else {
                card->setStyleSheet("QFrame#TaskCard { background: white; border-radius: 12px; border: 1px solid #ebeef5; }");
            }
        }
    }
}

void LogisticsModule::showCreateTaskDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("🚀 新建派车调度单");
    dialog.setFixedSize(520, 680);
    dialog.setStyleSheet("QDialog { background: #f8f9fb; } QLabel { color: #606266; font-weight: bold; } "
                         "QLineEdit, QComboBox { background: white; border: 1px solid #dcdfe6; border-radius: 6px; padding-left: 12px; color: #606266; } "
                         "QLineEdit[readOnly=\"true\"] { background: #f5f7fa; color: #909399; border: 1px solid #e4e7ed; } "
                         "QComboBox::drop-down { border: none; width: 30px; } "
                         "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; }");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    // Avatar Label
    QLabel *avatarLbl = new QLabel();
    avatarLbl->setFixedSize(110, 110);
    avatarLbl->setStyleSheet("background: transparent;");
    avatarLbl->setAlignment(Qt::AlignCenter);
    avatarLbl->setCursor(Qt::PointingHandCursor);
    
    // We use a dynamically allocated QString to hold the current avatar path
    QString *currentAvatarPath = new QString();
    
    AvatarEventFilter *filter = new AvatarEventFilter(avatarLbl, &dialog, currentAvatarPath, &dialog);
    avatarLbl->installEventFilter(filter);
    
    QHBoxLayout *avatarLayout = new QHBoxLayout();
    avatarLayout->addStretch();
    avatarLayout->addWidget(avatarLbl);
    avatarLayout->addStretch();
    layout->addLayout(avatarLayout);

    QGridLayout *form = new QGridLayout();
    form->setSpacing(15);

    // Pet Selection
    QComboBox *petCombo = new QComboBox();
    petCombo->setFixedHeight(36);
    auto pets = PetDataManager::instance()->allPets();
    for (const auto &pet : pets) {
        petCombo->addItem(QString("%1 (%2)").arg(pet.name, pet.id), pet.id);
    }

    // Type Selection
    QComboBox *typeCombo = new QComboBox();
    typeCombo->addItems({"单程接宠", "单程送宠", "往返接送"});
    typeCombo->setFixedHeight(36);

    // Reason Selection
    QComboBox *reasonCombo = new QComboBox();
    reasonCombo->addItems({"单纯洗护", "寄养入住", "寄养接回", "就医体检", "其他"});
    reasonCombo->setFixedHeight(36);

    // Address
    QLineEdit *addrEdit = new QLineEdit();
    addrEdit->setPlaceholderText("请输入详细接送地址...");
    addrEdit->setFixedHeight(36);

    // Date & Time Slots Helper
    auto createTimeLayout = [&](QDateEdit*& dateC, QComboBox*& timeC) -> QHBoxLayout* {
        QHBoxLayout *lay = new QHBoxLayout();
        lay->setSpacing(10);
        lay->setContentsMargins(0,0,0,0);
        
        dateC = new QDateEdit(QDate::currentDate());
        dateC->setCalendarPopup(true);
        dateC->setCalendarWidget(new CompactCalendar(dateC));
        dateC->setMinimumDate(QDate::currentDate());
        dateC->setDisplayFormat("yyyy-MM-dd");
        dateC->setFixedHeight(36);
        dateC->setStyleSheet("QDateEdit { background: white; border: 1px solid #dcdfe6; border-radius: 4px; padding-left: 10px; color: #606266; } "
                             "QDateEdit::drop-down { border: none; width: 30px; } "
                             "QDateEdit::down-arrow { image: url(:/images/chevron-down.svg); width: 14px; height: 14px; }");
        
        timeC = new QComboBox();
        timeC->addItems({"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"});
        timeC->setFixedHeight(36);
        
        lay->addWidget(dateC, 1);
        lay->addWidget(timeC, 1);
        return lay;
    };

    QDateEdit *dateCombo = nullptr;
    QComboBox *timeSlotCombo = nullptr;
    QHBoxLayout *goTimeLayout = createTimeLayout(dateCombo, timeSlotCombo);

    QDateEdit *returnDateCombo = nullptr;
    QComboBox *returnTimeSlotCombo = nullptr;
    QHBoxLayout *returnTimeLayout = createTimeLayout(returnDateCombo, returnTimeSlotCombo);

    form->addWidget(new QLabel("🐾 选择宠物:"), 0, 0);
    form->addWidget(petCombo, 0, 1);

    // Owner Info Section (Auto-filled)
    QLineEdit *ownerIdL = new QLineEdit(); ownerIdL->setReadOnly(true); ownerIdL->setFixedHeight(36);
    QLineEdit *ownerNameL = new QLineEdit(); ownerNameL->setReadOnly(true); ownerNameL->setFixedHeight(36);
    QLineEdit *ownerPhoneL = new QLineEdit(); ownerPhoneL->setReadOnly(true); ownerPhoneL->setFixedHeight(36);

    form->addWidget(new QLabel("👤 主人ID:"), 1, 0);
    form->addWidget(ownerIdL, 1, 1);
    form->addWidget(new QLabel("📛 主人姓名:"), 2, 0);
    form->addWidget(ownerNameL, 2, 1);
    form->addWidget(new QLabel("📞 联系电话:"), 3, 0);
    form->addWidget(ownerPhoneL, 3, 1);

    form->addWidget(new QLabel("🏷️ 业务类型:"), 4, 0);
    form->addWidget(typeCombo, 4, 1);
    form->addWidget(new QLabel("❓ 接送原因:"), 5, 0);
    form->addWidget(reasonCombo, 5, 1);
    form->addWidget(new QLabel("📍 详细地址:"), 6, 0);
    form->addWidget(addrEdit, 6, 1);
    
    QLabel *goTimeLbl = new QLabel("⏰ 预约时间:");
    form->addWidget(goTimeLbl, 7, 0);
    form->addLayout(goTimeLayout, 7, 1);

    QLabel *returnTimeLbl = new QLabel("⏰ 回程时间:");
    form->addWidget(returnTimeLbl, 8, 0);
    form->addLayout(returnTimeLayout, 8, 1);

    // Dynamic UI logic for Round-trip
    auto toggleReturnTime = [=]() {
        bool isRoundTrip = (typeCombo->currentText() == "往返接送");
        returnTimeLbl->setVisible(isRoundTrip);
        returnDateCombo->setVisible(isRoundTrip);
        returnTimeSlotCombo->setVisible(isRoundTrip);
        goTimeLbl->setText(isRoundTrip ? "⏰ 去程时间:" : "⏰ 预约时间:");
    };
    
    connect(typeCombo, &QComboBox::currentTextChanged, toggleReturnTime);
    toggleReturnTime(); // init state

    // Auto-fill logic
    auto updatePetInfo = [=](const QString &petId) {
        PetInfo info = PetDataManager::instance()->getPet(petId);
        ownerIdL->setText(info.ownerId);
        ownerNameL->setText(info.ownerName);
        ownerPhoneL->setText(info.ownerPhone.isEmpty() ? "未录入" : info.ownerPhone);
        
        // Update Avatar
        *currentAvatarPath = info.avatarPath;
        QPixmap pixmap(info.avatarPath);
        if (pixmap.isNull()) pixmap.load(":/images/load_img.jpg");
        QSize size(110, 110);
        QPixmap target(size);
        target.fill(Qt::transparent);
        QPainter p(&target);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        
        // Draw background circle
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e4e7ed"));
        p.drawEllipse(1, 1, size.width() - 2, size.height() - 2);

        QPainterPath path;
        path.addEllipse(1, 1, size.width() - 2, size.height() - 2);
        p.setClipPath(path);
        
        // Fix scaling distortion: properly crop the center
        QPixmap scaledPix = pixmap.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int xOffset = (scaledPix.width() - size.width()) / 2;
        int yOffset = (scaledPix.height() - size.height()) / 2;
        p.drawPixmap(0, 0, scaledPix, xOffset, yOffset, size.width(), size.height());
        
        // Draw the white border natively to avoid CSS box-model clipping
        p.setClipping(false);
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(1, 1, size.width() - 2, size.height() - 2);
        
        avatarLbl->setPixmap(target);
    };

    connect(petCombo, &QComboBox::currentTextChanged, [=]() {
        updatePetInfo(petCombo->currentData().toString());
    });
    
    // Initial fill
    if (petCombo->count() > 0) updatePetInfo(petCombo->currentData().toString());

    layout->addLayout(form);
    layout->addStretch();

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setFixedHeight(46);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #606266; font-size: 15px; } "
                             "QPushButton:hover { background: #f5f7fa; border-color: #c0c4cc; }");
    
    QPushButton *submitBtn = new QPushButton("立即派单");
    submitBtn->setFixedHeight(46);
    submitBtn->setCursor(Qt::PointingHandCursor);
    submitBtn->setStyleSheet("QPushButton { background: #409eff; border: none; border-radius: 8px; color: white; font-weight: bold; font-size: 15px; } "
                             "QPushButton:hover { background: #66b1ff; }");

    btnLayout->addWidget(cancelBtn, 1);
    btnLayout->addWidget(submitBtn, 2);
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(submitBtn, &QPushButton::clicked, [&]() {
        if (addrEdit->text().trimmed().isEmpty()) {
            CustomMessageDialog::showWarning(&dialog, "填写不完整", "接送地址不能为空，请补充。");
            return;
        }

        QString petId = petCombo->currentData().toString();
        QString type = typeCombo->currentText();
        QString reason = reasonCombo->currentText();
        QString addr = addrEdit->text();
        
        QString goDate = dateCombo->date().toString("yyyy-MM-dd");
        QString goTime = goDate + " " + timeSlotCombo->currentText();

        if (type == "往返接送") {
            // Task 1: Go Trip (接宠)
            LogisticsTask taskGo;
            taskGo.petId = petId;
            taskGo.type = "单程接宠";
            taskGo.address = addr;
            taskGo.appointmentTime = goTime;
            taskGo.status = "待处理";
            taskGo.relatedModule = reason + " (去程)";
            LogisticsManager::instance()->addLogisticsTask(taskGo);

            // Task 2: Return Trip (送宠)
            QString retDate = returnDateCombo->date().toString("yyyy-MM-dd");
            QString retTime = retDate + " " + returnTimeSlotCombo->currentText();
            LogisticsTask taskRet;
            taskRet.petId = petId;
            taskRet.type = "单程送宠";
            taskRet.address = addr;
            taskRet.appointmentTime = retTime;
            taskRet.status = "待处理";
            taskRet.relatedModule = reason + " (回程)";
            LogisticsManager::instance()->addLogisticsTask(taskRet);

            PetActivityLog log;
            log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
            log.type = "备注"; log.icon = "🚚";
            log.remark = QString("生成【往返接送】调度单。去程：%1，回程：%2").arg(goTime, retTime);
            log.operatorName = "前台调度员";
            PetDataManager::instance()->addActivityLog(petId, log);
            
        } else {
            // Single Trip
            LogisticsTask task;
            task.petId = petId;
            task.type = type;
            task.address = addr;
            task.appointmentTime = goTime;
            task.status = "待处理";
            task.relatedModule = reason;
            LogisticsManager::instance()->addLogisticsTask(task);
            
            PetActivityLog log;
            log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
            log.type = "备注"; log.icon = "🚚";
            log.remark = QString("已生成 [%1] 派车单，时段：%2").arg(task.type, goTime);
            log.operatorName = "前台调度员";
            PetDataManager::instance()->addActivityLog(petId, log);
        }

        dialog.accept();
    });

    dialog.exec();
}

void LogisticsModule::showHistoryDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("📜 车辆调度历史记录");
    dialog.setFixedSize(1000, 600);
    dialog.setStyleSheet("QDialog { background: white; }");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("车辆派遣历史档案");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #303133; margin-bottom: 10px;");
    layout->addWidget(title);

    QTableWidget *table = new QTableWidget();
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"日期/时段", "宠物信息", "主人信息", "业务类型", "接送地址", "关联原因", "任务状态"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // 日期
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // 宠物
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // 主人
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // 类型
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);          // 地址最长
    table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // 原因
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->setGridStyle(Qt::NoPen);
    table->setFocusPolicy(Qt::NoFocus);
    table->setStyleSheet(
        "QTableWidget { border: 1px solid #ebeef5; border-radius: 8px; gridline-color: transparent; }"
        "QHeaderView::section { background: #f5f7fa; padding: 10px; border: none; font-weight: bold; color: #606266; }"
        "QTableWidget::item { padding: 12px; border-bottom: 1px solid #f0f2f5; }"
    );

    auto allTasks = LogisticsManager::instance()->getAllTasks();
    int row = 0;
    for (const auto &task : allTasks) {
        // 只显示已结束的任务
        if (task.status != "已完成" && task.status != "已送达") continue;

        table->insertRow(row);
        PetInfo pet = PetDataManager::instance()->getPet(task.petId);

        table->setItem(row, 0, new QTableWidgetItem(task.appointmentTime));
        table->setItem(row, 1, new QTableWidgetItem(QString("%1 (%2)").arg(pet.name, task.petId)));
        table->setItem(row, 2, new QTableWidgetItem(QString("%1 (%2)").arg(pet.ownerName, pet.ownerId)));
        table->setItem(row, 3, new QTableWidgetItem(task.type));
        table->setItem(row, 4, new QTableWidgetItem(task.address));
        
        // Translate Reason to Chinese
        QString reasonCn = task.relatedModule;
        if (reasonCn == "Foster") reasonCn = "寄养服务";
        else if (reasonCn == "Bath") reasonCn = "洗护服务";
        else if (reasonCn == "Grooming") reasonCn = "美容服务";
        
        table->setItem(row, 5, new QTableWidgetItem(reasonCn));
        
        QTableWidgetItem *statusItem = new QTableWidgetItem(task.status);
        statusItem->setForeground(QBrush(QColor("#67c23a")));
        table->setItem(row, 6, statusItem);
        row++;
    }

    if (row == 0) {
        layout->addStretch();
        QLabel *noData = new QLabel("☕ 暂无已完成的历史调度记录");
        noData->setAlignment(Qt::AlignCenter);
        noData->setStyleSheet("color: #909399; font-size: 18px; font-weight: bold;");
        layout->addWidget(noData);
        layout->addStretch();
        table->hide();
    } else {
        layout->addWidget(table);
    }

    QPushButton *closeBtn = new QPushButton("我知道了");
    closeBtn->setFixedSize(140, 42); // 缩小宽度，更精致
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #67c23a, stop:1 #85ce61); "
                             "color: white; border-radius: 21px; border: none; font-weight: bold; font-size: 14px; } "
                             "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #85ce61, stop:1 #95d475); }");
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    dialog.exec();
}
