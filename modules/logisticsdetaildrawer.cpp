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
    setFixedWidth(480); // 略微增加宽度以补偿边距
    setStyleSheet("LogisticsDetailDrawer { background: transparent; border: none; }");
    
    setupUI();
    showEmpty();
}

void LogisticsDetailDrawer::setupUI()
{
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(10, 20, 10, 20); // 左边留出 10px 间距，右、上、下留出 20px 间距
    outerLayout->setSpacing(0);

    QFrame *mainContainer = new QFrame();
    mainContainer->setObjectName("mainContainer");
    mainContainer->setStyleSheet("#mainContainer { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    
    outerLayout->addWidget(mainContainer);

    QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header with pure white background
    QWidget *header = new QWidget();
    header->setFixedHeight(180); // 稍微缩小高度，更精致
    header->setStyleSheet("background: white; border-top-left-radius: 12px; border-top-right-radius: 12px;");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(25, 30, 20, 10);
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
    m_idLabel->setStyleSheet("font-size: 14px; color: #606266; background: transparent; ");
    
    m_statusTag = new QLabel("待处理");
    m_statusTag->setStyleSheet("background: #ffedd5; color: #9a3412; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
    m_statusTag->setFixedWidth(80);
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
    // 关键：滚动区域也要设置底部圆角，以防 footer 隐藏时底部变尖
    scroll->setStyleSheet("QScrollArea { background: white; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; } "
                          "QWidget#scrollContent { background: white; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }");
    
    m_contentArea = new QWidget();
    m_contentArea->setObjectName("scrollContent");
    m_contentArea->setStyleSheet("background: white;");
    m_contentLayout = new QVBoxLayout(m_contentArea);
    m_contentLayout->setContentsMargins(20, 25, 20, 25);
    m_contentLayout->setSpacing(25);
    m_contentLayout->setAlignment(Qt::AlignTop);
    
    scroll->setWidget(m_contentArea);
    mainLayout->addWidget(scroll);

    // Footer Actions
    m_footer = new QWidget();
    m_footer->setStyleSheet("background: white; border-top: 1px solid #f0f2f5; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px;");
    QVBoxLayout *footerLayout = new QVBoxLayout(m_footer);
    footerLayout->setContentsMargins(20, 20, 20, 20);
    footerLayout->setSpacing(12);

    m_primaryBtn = new QPushButton("司机出发");
    m_primaryBtn->setFixedHeight(48);
    m_primaryBtn->setCursor(Qt::PointingHandCursor);
    m_primaryBtn->setStyleSheet("QPushButton { background: #fa8c16; color: white; border-radius: 8px; font-weight: bold; font-size: 15px; border: none; text-align: center; padding: 0px; } "
                                "QPushButton:hover { background: #ffd591; }");
    connect(m_primaryBtn, &QPushButton::clicked, this, [=](){
        if (m_currentTask.status == "进行中") {
            LogisticsManager::instance()->updateTaskStatus(m_currentTask.taskId, "已完成");
            
            // 核心闭环：生成财务订单
            if (m_currentTask.amount > 0) {
                PetInfo info = PetDataManager::instance()->getPet(m_currentTask.petId);
                OrderInfo order;
                order.id = "ORD" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
                order.sourceModule = "Logistics";
                order.relatedId = m_currentTask.taskId;
                order.petId = m_currentTask.petId;
                order.petName = info.name;
                order.memberName = info.ownerName;
                order.itemDetails = QString("接送服务 (%1)").arg(m_currentTask.type);
                order.totalAmount = m_currentTask.amount;
                order.finalAmount = m_currentTask.amount;
                order.status = "Unpaid";
                order.createTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
                PetDataManager::instance()->addOrder(order);
            }

            PetActivityLog log; 
            log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
            log.type = "备注"; 
            log.icon = ""; 
            log.remark = QString("接送任务 [%1] 已圆满完成。").arg(m_currentTask.type); 
            log.operatorName = "派车系统";
            PetDataManager::instance()->addActivityLog(m_currentTask.petId, log);
            emit taskCompleted(m_currentTask.taskId);
        }
    });

    QHBoxLayout *minorBtns = new QHBoxLayout();
    minorBtns->setSpacing(12);
    QPushButton *editBtn = new QPushButton("修改信息");
    editBtn->setFixedHeight(40);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #dcdfe6; border-radius: 8px; color: #606266; font-size: 13px; font-weight: bold; text-align: center; padding: 0px; } "
                           "QPushButton:hover { background: #f5f7fa; }");
    editBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    QPushButton *cancelBtn = new QPushButton("取消派单");
    cancelBtn->setFixedHeight(40);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet("QPushButton { background: #fef0f0; border: 1px solid #fde2e2; border-radius: 8px; color: #f56c6c; font-size: 13px; font-weight: bold; text-align: center; padding: 0px; } "
                             "QPushButton:hover { background: #fde2e2; }");
    cancelBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(editBtn, &QPushButton::clicked, this, [=](){
        emit requestEditTask(m_currentTask.taskId);
    });
    
    connect(cancelBtn, &QPushButton::clicked, this, [=](){
        emit requestCancelTask(m_currentTask.taskId);
    });

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
        m_statusTag->setStyleSheet("background: #ffedd5; color: #9a3412; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
        m_primaryBtn->setVisible(false); // 隐藏主按钮，等待自动出发
        m_footer->setVisible(true); // 修改/取消按钮仍可见
    } else if (task.status == "进行中") {
        m_statusTag->setStyleSheet("background: #e0f2fe; color: #0369a1; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
        m_primaryBtn->setText("确认送达");
        m_primaryBtn->setStyleSheet("QPushButton { background: #166534; color: white; border-radius: 8px; font-weight: bold; font-size: 15px; border: none; text-align: center; padding: 0px; } "
                                    "QPushButton:hover { background: #15803d; }");
        m_footer->setVisible(true);
    } else {
        m_statusTag->setStyleSheet("background: #dcfce7; color: #166534; padding: 4px 12px; border-radius: 12px; font-size: 11px; font-weight: bold;");
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
        title->setStyleSheet("font-size: 15px; color: #334155; font-weight: bold; margin-left: 4px; margin-bottom: 5px; border: none; background: transparent;");
        return title;
    };


    auto addInfoRow = [](QGridLayout *grid, int row, const QString &label, const QString &value) {
        QLabel *lLbl = new QLabel(label + "：");
        lLbl->setStyleSheet("color: #94a3b8; font-size: 13px; background: transparent; border: none;");
        lLbl->setFixedWidth(80);
        QLabel *vLbl = new QLabel(value);
        vLbl->setStyleSheet("color: #1e293b; font-size: 13px; font-weight: bold; background: transparent; border: none;");
        vLbl->setWordWrap(true);
        grid->addWidget(lLbl, row, 0, Qt::AlignTop);
        grid->addWidget(vLbl, row, 1, Qt::AlignTop);
    };

    auto addSection = [&](const QString &title, std::function<void(QGridLayout*)> filler) {
        m_contentLayout->addWidget(createSectionTitle(title));
        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame { background: #f8f9fb; border-radius: 12px; border: 1px solid #e2e8f0; } QLabel { background: transparent; border: none; }");
        QGridLayout *grid = new QGridLayout(card);
        grid->setContentsMargins(16, 16, 16, 16);
        grid->setVerticalSpacing(12);
        grid->setHorizontalSpacing(10);
        filler(grid);
        m_contentLayout->addWidget(card);
        m_contentLayout->addSpacing(10);
    };

    // --- Pet Info Section ---
    addSection("宠物档案", [&](QGridLayout *g) {
        addInfoRow(g, 0, "品种", pet.breed);
        addInfoRow(g, 1, "年龄", pet.age);
    });

    // --- Owner Section ---
    addSection("主客关系", [&](QGridLayout *g) {
        addInfoRow(g, 0, "姓名", pet.ownerName + " (" + pet.ownerId + ")");
        addInfoRow(g, 1, "电话", pet.ownerPhone);
    });

    // --- Task Details Section ---
    addSection("调度详情", [&](QGridLayout *g) {
        QString fullTime = task.appointmentTime;
        QString datePart = fullTime.left(10);
        QString slotPart = fullTime.mid(11);

        addInfoRow(g, 0, "业务类型", task.type);
        addInfoRow(g, 1, "预约日期", datePart);
        addInfoRow(g, 2, "预约时段", slotPart);
        addInfoRow(g, 3, "详细地址", task.address);
    });
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
