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
#include <QTimer>
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

LogisticsModule::LogisticsModule(QWidget *parent) : QWidget(parent),
    m_statTotalLabel(nullptr),
    m_statPendingLabel(nullptr),
    m_statOngoingLabel(nullptr),
    m_statCompletedLabel(nullptr)
{
    m_currentDate = QDate::currentDate();
    setupUI();
    connect(LogisticsManager::instance(), &LogisticsManager::logisticsDataChanged, this, &LogisticsModule::refreshTasks);
    
    QTimer::singleShot(0, this, &LogisticsModule::refreshTasks);
    startTimer(500); // 500ms for breathing animation & time checking
}

void LogisticsModule::setupUI()
{
    QHBoxLayout *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QWidget *mainColumn = new QWidget();
    m_mainLayout = new QVBoxLayout(mainColumn);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    m_mainLayout->setSpacing(10);

    // Header Area
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("接送调度中心");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #1a1a1a;");
    
    QPushButton *newBtn = new QPushButton("新建派车单");
    newBtn->setMinimumHeight(36);
    newBtn->setCursor(Qt::PointingHandCursor);
    newBtn->setStyleSheet("QPushButton { padding: 4px 16px; background: #409eff; color: white; border-radius: 8px; font-weight: bold; font-size: 13px; border: none; box-shadow: 0 4px 12px rgba(64,158,255,0.3); } "
                          "QPushButton:hover { background: #66b1ff; }");
    connect(newBtn, &QPushButton::clicked, this, &LogisticsModule::showCreateTaskDialog);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(newBtn);
    m_mainLayout->addLayout(headerLayout);

    // Statistics Area
    QHBoxLayout *statLayout = new QHBoxLayout();
    statLayout->setSpacing(20);
    statLayout->addWidget(createStatCard("今日总任务", "0", "#409eff", &m_statTotalLabel), 1);
    statLayout->addWidget(createStatCard("待处理", "0", "#f56c6c", &m_statPendingLabel), 1);
    statLayout->addWidget(createStatCard("派送中", "0", "#e6a23c", &m_statOngoingLabel), 1);
    statLayout->addWidget(createStatCard("已完成", "0", "#67c23a", &m_statCompletedLabel), 1);
    m_mainLayout->addLayout(statLayout);

    // Date Picker Controls
    QHBoxLayout *datePickerLayout = new QHBoxLayout();
    datePickerLayout->setAlignment(Qt::AlignLeft);
    datePickerLayout->setSpacing(15);
    datePickerLayout->setContentsMargins(0, 5, 0, 10);

    m_prevDayBtn = new QPushButton("< 上一天");
    m_prevDayBtn->setFixedSize(90, 40);
    m_prevDayBtn->setCursor(Qt::PointingHandCursor);
    m_prevDayBtn->setStyleSheet("QPushButton { text-align: center; padding: 5px 10px; background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #606266; font-weight: bold; } "
                                 "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #409eff; }");

    m_datePickerBtn = new QPushButton("今天");
    m_datePickerBtn->setFixedSize(120, 40);
    m_datePickerBtn->setCursor(Qt::PointingHandCursor);
    m_datePickerBtn->setStyleSheet("QPushButton { text-align: center; padding: 5px 10px; background: #e1f0ff; border: 1px solid #b3d8ff; border-radius: 8px; color: #409eff; font-weight: bold; font-size: 14px; } "
                                    "QPushButton:hover { background: #c6e2ff; }");

    m_nextDayBtn = new QPushButton("下一天 >");
    m_nextDayBtn->setFixedSize(90, 40);
    m_nextDayBtn->setCursor(Qt::PointingHandCursor);
    m_nextDayBtn->setStyleSheet("QPushButton { text-align: center; padding: 5px 10px; background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #606266; font-weight: bold; } "
                                 "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #409eff; }");

    connect(m_prevDayBtn, &QPushButton::clicked, this, &LogisticsModule::onPrevDayClicked);
    connect(m_datePickerBtn, &QPushButton::clicked, this, &LogisticsModule::onDatePickerClicked);
    connect(m_nextDayBtn, &QPushButton::clicked, this, &LogisticsModule::onNextDayClicked);

    m_todayBtn = new QPushButton("回到今天");
    m_todayBtn->setFixedSize(100, 40);
    m_todayBtn->setCursor(Qt::PointingHandCursor);
    m_todayBtn->setStyleSheet("QPushButton { text-align: center; padding: 5px 10px; background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #606266; font-weight: bold; } "
                               "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #409eff; }");
    connect(m_todayBtn, &QPushButton::clicked, this, [this]() {
        onDateChanged(QDate::currentDate());
    });

    datePickerLayout->addWidget(m_prevDayBtn);
    datePickerLayout->addWidget(m_datePickerBtn);
    datePickerLayout->addWidget(m_nextDayBtn);
    datePickerLayout->addSpacing(10);
    datePickerLayout->addWidget(m_todayBtn);
    datePickerLayout->addStretch();
    m_mainLayout->addLayout(datePickerLayout);

    // Kanban Area
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 8px; background: transparent; margin: 0px; }"
        "QScrollBar::handle:vertical { background: #dcdfe6; min-height: 40px; border-radius: 4px; }"
        "QScrollBar::handle:vertical:hover { background: #c0c4cc; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
    );
    m_kanbanArea = new QWidget();
    QHBoxLayout *kanbanCols = new QHBoxLayout(m_kanbanArea);
    kanbanCols->setContentsMargins(0, 0, 15, 0);
    kanbanCols->setSpacing(0);
    kanbanCols->setAlignment(Qt::AlignTop); // FIX: Prevents vertical centering when empty

    // Column 1
    QWidget *col1 = new QWidget();
    QVBoxLayout *col1L = new QVBoxLayout(col1);
    col1L->setContentsMargins(0, 0, 30, 0);
    col1L->setAlignment(Qt::AlignTop);
    m_todayTitle = new QLabel("今日任务 (Today)");
    m_todayTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #2563eb; margin-bottom: 8px;");
    col1L->addWidget(m_todayTitle);
    m_todayListLayout = new QVBoxLayout();
    m_todayListLayout->setSpacing(12);
    m_todayListLayout->setAlignment(Qt::AlignTop);
    col1L->addLayout(m_todayListLayout);
    kanbanCols->addWidget(col1, 45);

    // Separator
    QFrame *vLine = new QFrame();
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFrameShadow(QFrame::Plain);
    vLine->setStyleSheet("color: #ebeef5; margin-top: 10px; margin-bottom: 10px;");
    kanbanCols->addWidget(vLine, 0);

    // Column 2
    QWidget *col2 = new QWidget();
    QVBoxLayout *col2L = new QVBoxLayout(col2);
    col2L->setContentsMargins(30, 0, 0, 0);
    col2L->setAlignment(Qt::AlignTop);
    m_tomorrowTitle = new QLabel("明日预告 (Tomorrow)");
    m_tomorrowTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #606266; margin-bottom: 8px;");
    col2L->addWidget(m_tomorrowTitle);
    m_tomorrowListLayout = new QVBoxLayout();
    m_tomorrowListLayout->setSpacing(12);
    m_tomorrowListLayout->setAlignment(Qt::AlignTop);
    col2L->addLayout(m_tomorrowListLayout);
    kanbanCols->addWidget(col2, 45);

    m_scrollArea->setWidget(m_kanbanArea);
    m_mainLayout->addWidget(m_scrollArea, 1);

    outerLayout->addWidget(mainColumn, 1);

    m_detailDrawer = new LogisticsDetailDrawer(this);
    outerLayout->addWidget(m_detailDrawer, 0);
}


void LogisticsModule::onDateChanged(const QDate &date)
{
    m_currentDate = date;

    // Helper for title formatting
    auto formatDate = [](const QDate &d) {
        if (d == QDate::currentDate()) return QString("今天 (%1)").arg(d.toString("MM/dd"));
        if (d == QDate::currentDate().addDays(1)) return QString("明天 (%1)").arg(d.toString("MM/dd"));
        return d.toString("yyyy/MM/dd");
    };

    // Update Titles
    QString leftDateStr = formatDate(date);
    auto formatTitle = [](const QDate &d) {
        if (d == QDate::currentDate()) return QString("今天");
        if (d == QDate::currentDate().addDays(1)) return QString("明天");
        return d.toString("MM/dd");
    };
    QString leftTitleStr = formatTitle(date);
    QString rightTitleStr = formatTitle(date.addDays(1));
    
    m_todayTitle->setText(leftTitleStr + (leftTitleStr == "今天" ? "任务安排" : " 任务"));
    m_tomorrowTitle->setText(rightTitleStr + " 预告");

    // Update DatePicker Button Text and Style
    m_datePickerBtn->setText(leftDateStr);
    if (date == QDate::currentDate()) {
        m_datePickerBtn->setStyleSheet("QPushButton { text-align: center; padding: 5px; background: #e1f0ff; border: 1px solid #b3d8ff; border-radius: 8px; color: #409eff; font-weight: bold; font-size: 14px; } "
                                        "QPushButton:hover { background: #c6e2ff; }");
    } else {
        m_datePickerBtn->setStyleSheet("QPushButton { text-align: center; padding: 5px; background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #606266; font-weight: bold; font-size: 14px; } "
                                        "QPushButton:hover { background: #f5f7fa; color: #409eff; border-color: #409eff; }");
    }

    updateStatistics();

    // Auto-select first task ONLY if nothing selected
    auto tasksToday = LogisticsManager::instance()->getTasksByDate(date);
    auto tasksTomorrow = LogisticsManager::instance()->getTasksByDate(date.addDays(1));
    
    if (m_selectedTaskId.isEmpty()) {
        if (!tasksToday.isEmpty()) {
            m_selectedTaskId = tasksToday.first().taskId;
            m_detailDrawer->showTask(tasksToday.first());
        } else if (!tasksTomorrow.isEmpty()) {
            m_selectedTaskId = tasksTomorrow.first().taskId;
            m_detailDrawer->showTask(tasksTomorrow.first());
        } else {
            m_detailDrawer->showEmpty();
        }
    } else {
        // Ensure detail drawer shows the currently selected task if it's still in the lists
        bool found = false;
        for (const auto &t : tasksToday) { if (t.taskId == m_selectedTaskId) { m_detailDrawer->showTask(t); found = true; break; } }
        if (!found) {
            for (const auto &t : tasksTomorrow) { if (t.taskId == m_selectedTaskId) { m_detailDrawer->showTask(t); found = true; break; } }
        }
        if (!found) {
            // Selected task not on this date, show empty or pick new first
            if (!tasksToday.isEmpty()) {
                m_selectedTaskId = tasksToday.first().taskId;
                m_detailDrawer->showTask(tasksToday.first());
            } else {
                m_selectedTaskId.clear();
                m_detailDrawer->showEmpty();
            }
        }
    }

    // Refresh Lists
    auto renderList = [=](QVBoxLayout *layout, const QList<LogisticsTask> &tasks, QDate listDate) {
        QLayoutItem *child;
        while ((child = layout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }

        QMap<QString, QList<LogisticsTask>> groupedTasks;
        for (const auto &task : tasks) {
            QString timeStr = task.appointmentTime;
            int spaceIdx = timeStr.indexOf(" ");
            if (spaceIdx != -1) timeStr = timeStr.mid(spaceIdx + 1);
            if (!timeStr.contains("-")) {
                QTime t = QTime::fromString(timeStr, "HH:mm");
                if (t.isValid()) {
                    timeStr = timeStr + " - " + t.addSecs(3600).toString("HH:mm");
                }
            }
            groupedTasks[timeStr].append(task);
        }

        QStringList standardSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
        QStringList slotsToRender = standardSlots;
        
        for (const QString &k : groupedTasks.keys()) {
            if (!slotsToRender.contains(k)) {
                slotsToRender.append(k);
            }
        }

        std::sort(slotsToRender.begin(), slotsToRender.end());

        for (const QString &timeSlot : slotsToRender) {
            const QList<LogisticsTask>& slotTasks = groupedTasks.value(timeSlot);

            QFrame *masterCard = new QFrame();
            masterCard->setObjectName("masterCard");
            masterCard->setStyleSheet("#masterCard { background: white; border-radius: 8px; border: 1px solid #ebeef5; }");
            masterCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            
            QVBoxLayout *ml = new QVBoxLayout(masterCard);
            ml->setContentsMargins(0, 0, 0, 0);
            ml->setSpacing(0);

            QFrame *header = new QFrame();
            header->setObjectName("masterHeader");
            header->setStyleSheet("#masterHeader { background: #fdfdfd; border-top-left-radius: 8px; border-top-right-radius: 8px; border-left: 4px solid #409eff; border-bottom: 1px solid #ebeef5; }");
            QHBoxLayout *hl = new QHBoxLayout(header);
            hl->setContentsMargins(15, 10, 15, 10);
            
            QLabel *timeL = new QLabel(timeSlot);
            timeL->setStyleSheet("font-weight: bold; color: #1e293b; font-size: 15px; letter-spacing: 0.5px;");
            QLabel *badgeL = new QLabel(QString("%1 任务").arg(slotTasks.size()));
            if (slotTasks.isEmpty()) {
                badgeL->setStyleSheet("background: #f4f4f5; color: #909399; border-radius: 10px; padding: 2px 8px; font-size: 11px; font-weight: bold; border: 1px solid #e9e9eb;");
            } else {
                badgeL->setStyleSheet("background: #ecf5ff; color: #409eff; border-radius: 10px; padding: 2px 8px; font-size: 11px; font-weight: bold; border: 1px solid #d9ecff;");
            }

            hl->addWidget(timeL);
            hl->addStretch();
            hl->addWidget(badgeL);
            ml->addWidget(header);

            QFrame *body = new QFrame();
            QVBoxLayout *bl = new QVBoxLayout(body);
            bl->setContentsMargins(0, 0, 0, 0);
            bl->setSpacing(0);
            body->setMinimumHeight(64); // 匹配预约中心卡片大小
            
            if (slotTasks.isEmpty()) {
                bool isPast = false;
                if (listDate < QDate::currentDate()) {
                    isPast = true;
                } else if (listDate == QDate::currentDate()) {
                    QTime startTime = QTime::fromString(timeSlot.left(5), "HH:mm");
                    if (startTime.isValid() && QTime::currentTime() > startTime) {
                        isPast = true;
                    }
                }

                QLabel *emptyLbl = new QLabel(isPast ? "暂无任务" : "暂无任务，可点击排班");
                emptyLbl->setAlignment(Qt::AlignCenter);
                if (isPast) {
                    emptyLbl->setStyleSheet("QLabel { color: #dcdfe6; font-size: 13px; background: transparent; }");
                } else {
                    emptyLbl->setStyleSheet("QLabel { color: #909399; font-size: 13px; background: transparent; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px; } QLabel:hover { color: #409eff; background: #fafafa; }");
                    emptyLbl->setCursor(Qt::PointingHandCursor);
                    emptyLbl->setProperty("emptySlotTime", timeSlot);
                    emptyLbl->installEventFilter(this);
                }
                bl->addWidget(emptyLbl);
            } else {
                for (int i = 0; i < slotTasks.size(); ++i) {
                    const auto &task = slotTasks[i];
                    QFrame *microRow = new QFrame();
                    microRow->setFixedHeight(64);
                    microRow->setObjectName("microRow");
                    
                    bool isSelected = (task.taskId == m_selectedTaskId);
                    QString rowStyle = isSelected 
                        ? "#microRow { background: #f0f7ff; border: 1px solid transparent; }"
                        : "#microRow { background: transparent; border: 1px solid transparent; } #microRow:hover { background: #f5f7fa; }";
                    microRow->setStyleSheet(rowStyle);

                    QHBoxLayout *rl = new QHBoxLayout(microRow);
                    rl->setContentsMargins(12, 8, 12, 8);
                    rl->setSpacing(12);

                    PetInfo pet = PetDataManager::instance()->getPet(task.petId);

                    // Avatar
                    QLabel *avatarLbl = new QLabel();
                    avatarLbl->setFixedSize(40, 40);
                    
                    QPixmap pix(pet.avatarPath);
                    if (pix.isNull()) pix.load(":/images/load_img.jpg");
                    QPixmap target(40, 40);
                    target.fill(Qt::transparent);
                    QPainter p(&target);
                    p.setRenderHint(QPainter::Antialiasing);
                    p.setRenderHint(QPainter::SmoothPixmapTransform);
                    QPainterPath path;
                    path.addEllipse(1, 1, 38, 38);
                    p.setClipPath(path);
                    p.drawPixmap(1, 1, 38, 38, pix.scaled(38, 38, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                    
                    p.setClipping(false);
                    QPen pen(QColor("#ebeef5"), 1);
                    p.setPen(pen);
                    p.drawEllipse(1, 1, 38, 38);
                    
                    avatarLbl->setPixmap(target);
                    avatarLbl->setCursor(Qt::PointingHandCursor);
                    avatarLbl->setProperty("avatarPath", pet.avatarPath);
                    avatarLbl->installEventFilter(this);
                    
                    QVBoxLayout *infoLayout = new QVBoxLayout();
                    infoLayout->setSpacing(4);
                    
                    QString reason = task.relatedModule.split(" ").first();
                    if (reason.isEmpty()) reason = task.type;

                    QLabel *topLbl = new QLabel(QString("<span style='color:#303133; font-weight:bold; font-size:14px;'>%1</span> <span style='color:#909399; font-size:12px;'>%2 &nbsp;·&nbsp; %3</span>").arg(pet.name, pet.breed, reason));
                    topLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
                    
                    QString addr = task.address;
                    QFontMetrics fm(topLbl->font());
                    addr = fm.elidedText(addr, Qt::ElideRight, 150);

                    QLabel *bottomLbl = new QLabel(QString("<span style='color:#606266; font-size:12px;'>%1 &nbsp;|&nbsp; %2</span>").arg(addr, pet.ownerName));
                    bottomLbl->setAttribute(Qt::WA_TransparentForMouseEvents);
                    
                    infoLayout->addWidget(topLbl);
                    infoLayout->addWidget(bottomLbl);
                    infoLayout->addStretch();
                    
                    QLabel *statusTag = new QLabel(task.status);
                    statusTag->setAttribute(Qt::WA_TransparentForMouseEvents);
                    if (task.status == "已完成" || task.status == "已离店 (回家)") {
                        statusTag->setStyleSheet("background: #f0f9eb; color: #67c23a; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; border: 1px solid #e1f3d8;");
                    } else if (task.status == "待处理") {
                        statusTag->setStyleSheet("background: #fff7ed; color: #ea580c; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; border: 1px solid #ffedd5;");
                    } else {
                        statusTag->setStyleSheet("background: #eff6ff; color: #2563eb; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; border: 1px solid #dbeafe;");
                    }

                    rl->addWidget(avatarLbl);
                    rl->addLayout(infoLayout);
                    rl->addStretch();
                    rl->addWidget(statusTag);

                    microRow->setProperty("taskId", task.taskId);
                    microRow->setProperty("taskStatus", task.status);
                    microRow->setProperty("appointmentTime", task.appointmentTime);
                    microRow->installEventFilter(this);
                    microRow->setCursor(Qt::PointingHandCursor);

                    bl->addWidget(microRow);

                    if (i < slotTasks.size() - 1) {
                        QFrame *line = new QFrame();
                        line->setFixedHeight(1);
                        line->setStyleSheet("background: #f0f2f5; margin-left: 10px; margin-right: 10px;");
                        bl->addWidget(line);
                    }
                }
                bl->addStretch();
            }
            
            ml->addWidget(body);
            layout->addWidget(masterCard);
        }
        
        layout->addStretch();
    };

    renderList(m_todayListLayout, tasksToday, date);
    renderList(m_tomorrowListLayout, tasksTomorrow, date.addDays(1));
}

void LogisticsModule::onTaskSelected(const QString &taskId)
{
    m_selectedTaskId = taskId;
    auto tasks = LogisticsManager::instance()->getAllTasks();
    for (const auto &t : tasks) {
        if (t.taskId == taskId) {
            m_detailDrawer->showTask(t);
            break;
        }
    }
    // Re-render lists to show selected state
    onDateChanged(m_currentDate);
}

void LogisticsModule::refreshTasks()
{
    onDateChanged(m_currentDate);
}

void LogisticsModule::onPrevDayClicked()
{
    onDateChanged(m_currentDate.addDays(-1));
}

void LogisticsModule::onNextDayClicked()
{
    onDateChanged(m_currentDate.addDays(1));
}

void LogisticsModule::onDatePickerClicked()
{
    QDialog dialog(this);
    dialog.setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    dialog.setStyleSheet("QDialog { background: white; border: 1px solid #dcdfe6; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.15); }");
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(10, 10, 10, 10);
    
    CompactCalendar *calendar = new CompactCalendar(&dialog);
    calendar->setSelectedDate(m_currentDate);
    layout->addWidget(calendar);
    
    connect(calendar, &QCalendarWidget::clicked, [&dialog, this](const QDate &date){
        onDateChanged(date);
        dialog.accept();
    });
    
    QPoint pos = m_datePickerBtn->mapToGlobal(QPoint(0, m_datePickerBtn->height() + 5));
    dialog.move(pos);
    dialog.exec();
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
    bg->setStyleSheet("background-color: rgba(0, 0, 0, 215);");
    layout->addWidget(bg);
    
    QVBoxLayout *bgLayout = new QVBoxLayout(bg);
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->setAlignment(Qt::AlignCenter);
    
    QLabel *imgLabel = new QLabel();
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setStyleSheet("border: none; background: transparent;");
    
    int maxDim = qMin(this->window()->width(), this->window()->height()) * 0.8;
    imgLabel->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
        // 1. Check avatar click first so it doesn't get swallowed by the row click
        if (watched->property("avatarPath").isValid()) {
            showBigImage(watched->property("avatarPath").toString());
            return true;
        }

        // 2. Click on task card or empty slot
        QWidget *w = qobject_cast<QWidget*>(watched);
        while (w) {
            if (w->property("emptySlotTime").isValid()) {
                m_preselectedTimeSlot = w->property("emptySlotTime").toString();
                showCreateTaskDialog();
                return true;
            }
            if (w->property("taskId").isValid()) {
                onTaskSelected(w->property("taskId").toString());
                return true;
            }
            w = w->parentWidget();
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

    QList<QFrame*> cards = m_kanbanArea->findChildren<QFrame*>("TaskCard");
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
    dialog.setWindowTitle("新建派车调度单");
    dialog.setFixedSize(520, 680);
    dialog.setStyleSheet("QDialog { background: #f8f9fb; } QLabel { color: #606266; font-weight: bold; } "
                         "QLineEdit { background: white; border: 1px solid #dcdfe6; border-radius: 6px; padding-left: 12px; color: #606266; } "
                         "QLineEdit[readOnly=\"true\"] { background: #f5f7fa; color: #909399; border: 1px solid #e4e7ed; } "
                         "QComboBox { border: 1px solid #dcdfe6; border-radius: 6px; padding: 0 10px; background: white; font-size: 13px; } "
                         "QComboBox:hover { border-color: #409eff; } "
                         "QComboBox::drop-down { border: none; width: 24px; } "
                         "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
                         "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; }");

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
    reasonCombo->addItems({"单纯洗护", "寄养入住", "寄养送回", "就医体检", "其他"});
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
        timeC->setFixedHeight(36);

        auto refreshSlots = [=]() {
            QString current = timeC->currentText();
            timeC->clear();
            QStringList allSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
            QTime now = QTime::currentTime();
            bool isToday = (dateC->date() == QDate::currentDate());

            for (const QString &slot : allSlots) {
                if (isToday) {
                    QTime startTime = QTime::fromString(slot.left(5), "HH:mm");
                    // 过滤掉已经开始或已经过去的时段
                    if (now > startTime) continue;
                }
                timeC->addItem(slot);
            }
            if (timeC->findText(current) != -1) timeC->setCurrentText(current);
        };

        refreshSlots();
        connect(dateC, &QDateEdit::dateChanged, [=](){ refreshSlots(); });
        
        lay->addWidget(dateC, 1);
        lay->addWidget(timeC, 1);
        return lay;
    };

    QDateEdit *dateCombo = nullptr;
    QComboBox *timeSlotCombo = nullptr;
    QHBoxLayout *goTimeLayout = createTimeLayout(dateCombo, timeSlotCombo);
    if (timeSlotCombo->count() == 0) {
        dateCombo->setDate(dateCombo->date().addDays(1));
    }

    QDateEdit *returnDateCombo = nullptr;
    QComboBox *returnTimeSlotCombo = nullptr;
    QHBoxLayout *returnTimeLayout = createTimeLayout(returnDateCombo, returnTimeSlotCombo);
    if (returnTimeSlotCombo->count() == 0) {
        returnDateCombo->setDate(returnDateCombo->date().addDays(1));
    }

    if (!m_preselectedTimeSlot.isEmpty()) {
        int idx = timeSlotCombo->findText(m_preselectedTimeSlot);
        if (idx != -1) timeSlotCombo->setCurrentIndex(idx);
        m_preselectedTimeSlot.clear();
    }

    form->addWidget(new QLabel("选择宠物:"), 0, 0);
    form->addWidget(petCombo, 0, 1);

    // Owner Info Section (Auto-filled)
    QLineEdit *ownerIdL = new QLineEdit(); ownerIdL->setReadOnly(true); ownerIdL->setFixedHeight(36);
    QLineEdit *ownerNameL = new QLineEdit(); ownerNameL->setReadOnly(true); ownerNameL->setFixedHeight(36);
    QLineEdit *ownerPhoneL = new QLineEdit(); ownerPhoneL->setReadOnly(true); ownerPhoneL->setFixedHeight(36);

    form->addWidget(new QLabel("主人ID:"), 1, 0);
    form->addWidget(ownerIdL, 1, 1);
    form->addWidget(new QLabel("主人姓名:"), 2, 0);
    form->addWidget(ownerNameL, 2, 1);
    form->addWidget(new QLabel("联系电话:"), 3, 0);
    form->addWidget(ownerPhoneL, 3, 1);

    form->addWidget(new QLabel("业务类型:"), 4, 0);
    form->addWidget(typeCombo, 4, 1);
    form->addWidget(new QLabel("接送原因:"), 5, 0);
    form->addWidget(reasonCombo, 5, 1);
    QLabel *addrLbl = new QLabel("接送地址:");
    form->addWidget(addrLbl, 6, 0);
    form->addWidget(addrEdit, 6, 1);
    
    QLabel *returnAddrLbl = new QLabel("送回地址:");
    QLineEdit *returnAddrEdit = new QLineEdit();
    returnAddrEdit->setPlaceholderText("请输入详细送回地址 (如不填则默认同上)...");
    returnAddrEdit->setFixedHeight(36);
    form->addWidget(returnAddrLbl, 7, 0);
    form->addWidget(returnAddrEdit, 7, 1);
    
    QLabel *goTimeLbl = new QLabel("去程时间:");
    form->addWidget(goTimeLbl, 8, 0);
    form->addLayout(goTimeLayout, 8, 1);

    QLabel *returnTimeLbl = new QLabel("回程时间:");
    form->addWidget(returnTimeLbl, 9, 0);
    form->addLayout(returnTimeLayout, 9, 1);

    // Dynamic UI logic for Round-trip
    auto toggleReturnTime = [=]() {
        bool isRoundTrip = (typeCombo->currentText() == "往返接送");
        returnTimeLbl->setVisible(isRoundTrip);
        returnDateCombo->setVisible(isRoundTrip);
        returnTimeSlotCombo->setVisible(isRoundTrip);
        returnAddrLbl->setVisible(isRoundTrip);
        returnAddrEdit->setVisible(isRoundTrip);
        goTimeLbl->setText(isRoundTrip ? "去程时间:" : "预约时间:");
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
        if (timeSlotCombo->currentText().isEmpty()) {
            CustomMessageDialog::showWarning(&dialog, "时段不可用", "该日期已无可用预约时段，请重新选择日期。");
            return;
        }
        if (typeCombo->currentText() == "往返接送" && returnTimeSlotCombo->currentText().isEmpty()) {
            CustomMessageDialog::showWarning(&dialog, "时段不可用", "该回程日期已无可用时段，请重新选择日期。");
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
            QString retAddr = returnAddrEdit->text().trimmed();
            if (retAddr.isEmpty()) retAddr = addr;

            LogisticsTask taskRet;
            taskRet.petId = petId;
            taskRet.type = "单程送宠";
            taskRet.address = retAddr;
            taskRet.appointmentTime = retTime;
            taskRet.status = "待处理";
            taskRet.relatedModule = reason + " (回程)";
            LogisticsManager::instance()->addLogisticsTask(taskRet);

            PetActivityLog log;
            log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
            log.type = "备注"; log.icon = "";
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
            log.type = "备注"; log.icon = "";
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
    dialog.setWindowTitle("车辆调度历史记录");
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
        QLabel *noData = new QLabel("暂无已完成的历史调度记录");
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

void LogisticsModule::updateStatistics()
{
    auto tasks = LogisticsManager::instance()->getTasksByDate(m_currentDate);
    int total = tasks.size();
    int pending = 0;
    int ongoing = 0;
    int completed = 0;

    for (const auto &task : tasks) {
        if (task.status == "待接送") pending++;
        else if (task.status == "派送中") ongoing++;
        else if (task.status == "已完成") completed++;
    }

    if (m_statTotalLabel) m_statTotalLabel->setText(QString::number(total));
    if (m_statPendingLabel) m_statPendingLabel->setText(QString::number(pending));
    if (m_statOngoingLabel) m_statOngoingLabel->setText(QString::number(ongoing));
    if (m_statCompletedLabel) m_statCompletedLabel->setText(QString::number(completed));
}

QWidget* LogisticsModule::createStatCard(const QString &title, const QString &value, const QString &color, QLabel **outValueLabel)
{
    QFrame *card = new QFrame();
    card->setFixedHeight(80);
    card->setStyleSheet(QString("QFrame { background: white; border: 1px solid #f0f2f5; border-radius: 12px; } "
                                "QFrame:hover { border-color: %1; background: #fafafa; }").arg(color));
    
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 12, 20, 12);
    layout->setSpacing(4);
    
    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("color: #909399; font-size: 13px; border: none; background: transparent;");
    
    *outValueLabel = new QLabel(value);
    (*outValueLabel)->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: bold; border: none; background: transparent; ").arg(color));
    
    layout->addWidget(titleLabel);
    layout->addWidget(*outValueLabel);
    
    return card;
}
