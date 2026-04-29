#include "logisticsdetaildrawer.h"
#include "petdatamanager.h"
#include "logisticsmanager.h"
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QDateTime>
#include <QGraphicsDropShadowEffect>
#include <QDialog>
#include <QEvent>

LogisticsDetailDrawer::LogisticsDetailDrawer(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(400); // Fixed width
    setStyleSheet("LogisticsDetailDrawer { background: white; border-left: 1px solid #ebeef5; }");
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 15));
    shadow->setOffset(-2, 0);
    setGraphicsEffect(shadow);

    setupUI();
    showEmpty();
}

void LogisticsDetailDrawer::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header with gradient-like background
    QWidget *header = new QWidget();
    header->setFixedHeight(220);
    header->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f0f7ff, stop:1 #ffffff); border-bottom: 1px solid #f0f2f5;");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(25, 40, 20, 20);
    headerLayout->setSpacing(20);
    headerLayout->setAlignment(Qt::AlignVCenter);

    m_avatarLabel = new QLabel();
    m_avatarLabel->setFixedSize(100, 100);
    m_avatarLabel->setCursor(Qt::PointingHandCursor);
    m_avatarLabel->installEventFilter(this);
    m_avatarLabel->setStyleSheet("background: transparent; border: none;");
    headerLayout->addWidget(m_avatarLabel);

    QVBoxLayout *nameCol = new QVBoxLayout();
    nameCol->setSpacing(10);
    nameCol->setAlignment(Qt::AlignVCenter);

    m_nameLabel = new QLabel("宠物名称");
    m_nameLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #303133; background: transparent;");
    
    m_idLabel = new QLabel("ID: --");
    m_idLabel->setStyleSheet("font-size: 14px; color: #606266; background: transparent; font-weight: 500;");
    
    m_statusTag = new QLabel("待处理");
    m_statusTag->setStyleSheet("background: #fa8c16; color: white; padding: 4px 14px; border-radius: 12px; font-size: 12px; font-weight: bold;");
    m_statusTag->setFixedWidth(70);
    m_statusTag->setAlignment(Qt::AlignCenter);

    nameCol->addWidget(m_nameLabel);
    nameCol->addWidget(m_idLabel);
    nameCol->addWidget(m_statusTag);
    
    headerLayout->addLayout(nameCol);
    headerLayout->addStretch();

    mainLayout->addWidget(header);

    // Scrollable Content
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: white;");
    
    m_contentArea = new QWidget();
    m_contentArea->setStyleSheet("background: white;");
    m_contentLayout = new QVBoxLayout(m_contentArea);
    m_contentLayout->setContentsMargins(20, 25, 20, 25);
    m_contentLayout->setSpacing(25);
    m_contentLayout->setAlignment(Qt::AlignTop);
    
    scroll->setWidget(m_contentArea);
    mainLayout->addWidget(scroll);

    // Footer Actions
    m_footer = new QWidget();
    m_footer->setStyleSheet("background: white; border-top: 1px solid #ebeef5;");
    QVBoxLayout *footerLayout = new QVBoxLayout(m_footer);
    footerLayout->setContentsMargins(20, 20, 20, 20);
    footerLayout->setSpacing(12);

    m_primaryBtn = new QPushButton("司机出发");
    m_primaryBtn->setFixedHeight(48);
    m_primaryBtn->setCursor(Qt::PointingHandCursor);
    m_primaryBtn->setStyleSheet("QPushButton { background: #fa8c16; color: white; border-radius: 8px; font-weight: bold; font-size: 15px; border: none; } "
                                "QPushButton:hover { background: #ffd591; }");
    connect(m_primaryBtn, &QPushButton::clicked, this, [=](){
        if (m_currentTask.status == "待处理") {
            LogisticsManager::instance()->updateTaskStatus(m_currentTask.taskId, "进行中");
            PetInfo info = PetDataManager::instance()->getPet(m_currentTask.petId);
            if (!info.id.isEmpty()) { info.status = "接送中 (在途)"; PetDataManager::instance()->updatePet(info); }
            emit taskCompleted(m_currentTask.taskId); // triggers refresh
        } else if (m_currentTask.status == "进行中") {
            LogisticsManager::instance()->updateTaskStatus(m_currentTask.taskId, "已完成");
            PetActivityLog log; log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
            log.type = "备注"; log.icon = ""; log.remark = QString("接送任务 [%1] 已圆满完成。").arg(m_currentTask.type); log.operatorName = "派车系统";
            PetDataManager::instance()->addActivityLog(m_currentTask.petId, log);
            emit taskCompleted(m_currentTask.taskId);
        }
    });

    QHBoxLayout *minorBtns = new QHBoxLayout();
    minorBtns->setSpacing(12);
    QPushButton *editBtn = new QPushButton("修改信息");
    editBtn->setFixedHeight(40);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #606266; font-size: 13px; font-weight: bold; } "
                           "QPushButton:hover { background: #f5f7fa; }");
    
    QPushButton *cancelBtn = new QPushButton("取消派单");
    cancelBtn->setFixedHeight(40);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet("QPushButton { background: #fef0f0; border: 1px solid #fde2e2; border-radius: 8px; color: #f56c6c; font-size: 13px; font-weight: bold; } "
                             "QPushButton:hover { background: #fde2e2; }");

    minorBtns->addWidget(editBtn);
    minorBtns->addWidget(cancelBtn);

    footerLayout->addWidget(m_primaryBtn);
    footerLayout->addLayout(minorBtns);
    mainLayout->addWidget(m_footer);
}

void LogisticsDetailDrawer::showTask(const LogisticsTask &task)
{
    m_currentTask = task;
    PetInfo pet = PetDataManager::instance()->getPet(task.petId);

    // Update Avatar
    m_currentAvatarPath = pet.avatarPath;
    QPixmap pixmap(pet.avatarPath);
    if (pixmap.isNull()) pixmap.load(":/images/load_img.jpg");
    QSize size(100, 100);
    QPixmap target(size);
    target.fill(Qt::transparent);
    QPainter p(&target);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addEllipse(0, 0, size.width(), size.height());
    p.setClipPath(path);
    p.drawPixmap(0, 0, pixmap.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    m_avatarLabel->setPixmap(target);

    m_nameLabel->setText(pet.name.isEmpty() ? task.petId : pet.name);
    m_idLabel->setText(QString("宠物 ID: %1").arg(task.petId));
    m_statusTag->setText(task.status);
    m_statusTag->setVisible(true);
    
    if (task.status == "待处理") {
        m_statusTag->setStyleSheet("background: #fa8c16; color: white; padding: 4px 14px; border-radius: 12px; font-size: 12px; font-weight: bold;");
        m_primaryBtn->setText("司机出发");
        m_primaryBtn->setStyleSheet("QPushButton { background: #fa8c16; color: white; border-radius: 8px; font-weight: bold; font-size: 15px; border: none; } "
                                    "QPushButton:hover { background: #ffd591; }");
        m_footer->setVisible(true);
    } else if (task.status == "进行中") {
        m_statusTag->setStyleSheet("background: #409eff; color: white; padding: 4px 14px; border-radius: 12px; font-size: 12px; font-weight: bold;");
        m_primaryBtn->setText("确认送达");
        m_primaryBtn->setStyleSheet("QPushButton { background: #67c23a; color: white; border-radius: 8px; font-weight: bold; font-size: 15px; border: none; } "
                                    "QPushButton:hover { background: #a4da89; }");
        m_footer->setVisible(true);
    } else {
        m_statusTag->setStyleSheet("background: #c0c4cc; color: white; padding: 4px 14px; border-radius: 12px; font-size: 12px; font-weight: bold;");
        m_footer->setVisible(false);
    }
    
    // Clear content
    QLayoutItem *child;
    while ((child = m_contentLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }

    m_contentLayout->setSpacing(20);

    auto createSectionTitle = [](const QString &text) {
        QLabel *title = new QLabel(text);
        title->setStyleSheet("font-size: 15px; color: #303133; font-weight: bold; "
                             "border-left: 4px solid #409eff; padding-left: 8px; margin-bottom: 5px;");
        return title;
    };

    auto createSeparator = []() {
        QFrame *line = new QFrame();
        line->setFixedHeight(1);
        line->setStyleSheet("background-color: #ebeef5; border: none;");
        return line;
    };

    auto addInfoRow = [](QGridLayout *grid, int row, const QString &label, const QString &value) {
        QLabel *lLbl = new QLabel(label + "：");
        lLbl->setStyleSheet("color: #606266; font-size: 13px;");
        lLbl->setFixedWidth(70);
        QLabel *vLbl = new QLabel(value);
        vLbl->setStyleSheet("color: #333333; font-size: 14px;");
        vLbl->setWordWrap(true);
        grid->addWidget(lLbl, row, 0, Qt::AlignTop);
        grid->addWidget(vLbl, row, 1, Qt::AlignTop);
    };

    // --- Pet Info Section ---
    QWidget *petBlock = new QWidget();
    QVBoxLayout *petBL = new QVBoxLayout(petBlock);
    petBL->setContentsMargins(0, 0, 0, 0);
    petBL->addWidget(createSectionTitle("宠物档案"));
    
    QGridLayout *petGrid = new QGridLayout();
    petGrid->setVerticalSpacing(10);
    petGrid->setContentsMargins(12, 5, 0, 5);
    addInfoRow(petGrid, 0, "品种", pet.breed);
    addInfoRow(petGrid, 1, "年龄", pet.age);
    petBL->addLayout(petGrid);
    m_contentLayout->addWidget(petBlock);

    m_contentLayout->addWidget(createSeparator());

    // --- Owner Section ---
    QWidget *ownerBlock = new QWidget();
    QVBoxLayout *ownerBL = new QVBoxLayout(ownerBlock);
    ownerBL->setContentsMargins(0, 0, 0, 0);
    ownerBL->addWidget(createSectionTitle("主客关系"));

    QGridLayout *ownerGrid = new QGridLayout();
    ownerGrid->setVerticalSpacing(10);
    ownerGrid->setContentsMargins(12, 5, 0, 5);
    addInfoRow(ownerGrid, 0, "姓名", pet.ownerName + " (" + pet.ownerId + ")");
    addInfoRow(ownerGrid, 1, "电话", pet.ownerPhone);
    ownerBL->addLayout(ownerGrid);
    m_contentLayout->addWidget(ownerBlock);

    m_contentLayout->addWidget(createSeparator());

    // --- Task Details Section ---
    QWidget *taskBlock = new QWidget();
    QVBoxLayout *taskBL = new QVBoxLayout(taskBlock);
    taskBL->setContentsMargins(0, 0, 0, 0);
    taskBL->addWidget(createSectionTitle("调度详情"));
    
    QGridLayout *taskGrid = new QGridLayout();
    taskGrid->setVerticalSpacing(10);
    taskGrid->setContentsMargins(12, 5, 0, 5);

    QString fullTime = task.appointmentTime;
    QString datePart = fullTime.left(10);
    QString slotPart = fullTime.mid(11);

    addInfoRow(taskGrid, 0, "业务类型", task.type);
    addInfoRow(taskGrid, 1, "接送原因", task.relatedModule);
    addInfoRow(taskGrid, 2, "预约日期", datePart);
    addInfoRow(taskGrid, 3, "预约时段", slotPart);
    addInfoRow(taskGrid, 4, "详细地址", task.address);

    taskBL->addLayout(taskGrid);
    m_contentLayout->addWidget(taskBlock);
}

void LogisticsDetailDrawer::showEmpty()
{
    m_avatarLabel->clear();
    m_nameLabel->setText("未选择任务");
    m_statusTag->setVisible(false);
    m_footer->setVisible(false);
    
    QLayoutItem *child;
    while ((child = m_contentLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    
    QWidget *emptyWidget = new QWidget();
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setContentsMargins(0, 60, 0, 0);
    
    QLabel *iconLabel = new QLabel();
    iconLabel->setStyleSheet("font-size: 40px; color: #e4e7ed;");
    iconLabel->setAlignment(Qt::AlignCenter);
    
    QLabel *textLabel = new QLabel("请在左侧选择要查看的调度任务");
    textLabel->setStyleSheet("color: #c0c4cc; font-size: 13px; line-height: 1.5; margin-top: 15px;");
    textLabel->setAlignment(Qt::AlignCenter);
    
    emptyLayout->addWidget(iconLabel);
    emptyLayout->addWidget(textLabel);
    m_contentLayout->addWidget(emptyWidget);
}

void LogisticsDetailDrawer::closeDrawer()
{
    // No longer closing, just show empty
    showEmpty();
}

void LogisticsDetailDrawer::showBigImage(const QString &path)
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

bool LogisticsDetailDrawer::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        if (watched == m_avatarLabel) {
            showBigImage(m_currentAvatarPath);
            return true;
        }
        
        // If the watched object is a preview dialog or its content, close it
        QDialog *dialog = qobject_cast<QDialog*>(watched);
        if (dialog || watched->parent() == nullptr || watched->objectName() == "previewBg") {
            if (watched->isWidgetType()) {
                QWidget *w = qobject_cast<QWidget*>(watched);
                if (w && (w->windowFlags() & Qt::FramelessWindowHint)) {
                    w->window()->close();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
