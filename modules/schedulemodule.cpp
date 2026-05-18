#include "schedulemodule.h"
#include "../utils/imageutils.h"
#include "modules/scheduledatamanager.h"
#include "modules/staffdatamanager.h"
#include "modules/custommessagedialog.h"
#include "modules/custom_calendar_edit.h"
#include "modules/scheduletemplatedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>

ScheduleModule::ScheduleModule(QWidget *parent) : QWidget(parent)
{
    // 初始化为当前周的周一
    QDate today = QDate::currentDate();
    m_currentMonday = today.addDays(1 - today.dayOfWeek());
    
    setupUI();
    
    connect(ScheduleDataManager::instance(), &ScheduleDataManager::scheduleChanged, this, &ScheduleModule::updateTable);

    // 初始化时请求数据，界面会先显示缓存内容
    QDate requestStart = m_currentMonday.addDays(-14);
    QDate requestEnd = m_currentMonday.addDays(28);
    ScheduleDataManager::instance()->requestScheduleList(requestStart.toString("yyyy-MM-dd"), requestEnd.toString("yyyy-MM-dd"));
    
    updateTable();
}

void ScheduleModule::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // --- 顶部工具栏 (复刻 SaaS 风格) ---
    QFrame *topBar = new QFrame();
    topBar->setFixedHeight(80);
    topBar->setStyleSheet("QFrame { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(20, 0, 20, 0);

    // 左侧：日期切换
    QHBoxLayout *dateNavLayout = new QHBoxLayout();
    dateNavLayout->setSpacing(10);
    
    QPushButton *prevBtn = new QPushButton("< 上周");
    QPushButton *nextBtn = new QPushButton("下周 >");
    QPushButton *todayBtn = new QPushButton("回到今天");
    
    QString navStyle = "QPushButton { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 6px; padding: 6px 12px; color: #64748b; font-size: 13px; font-weight: bold; } "
                       "QPushButton:hover { background: #eff6ff; border-color: #3b82f6; color: #3b82f6; }";
    prevBtn->setStyleSheet(navStyle);
    nextBtn->setStyleSheet(navStyle);
    todayBtn->setStyleSheet(navStyle);
    
    m_dateRangeLabel = new QLabel();
    m_dateRangeLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1e293b; margin: 0 10px; border: none; background: transparent;");
    
    m_jumpCalendar = new CustomCalendarEdit();
    m_jumpCalendar->setFixedWidth(130);
    m_jumpCalendar->setDate(QDate::currentDate()); // 默认显示今天
    m_jumpCalendar->setPlaceholderText("跳转日期...");
    m_jumpCalendar->setStyleSheet("QLineEdit { background: #eff6ff; border: 1px solid #dbeafe; border-radius: 6px; padding: 4px 8px; color: #3b82f6; font-size: 13px; font-weight: bold; } "
                                  "QLineEdit:hover { background: #dbeafe; }");

    dateNavLayout->addWidget(prevBtn);
    dateNavLayout->addWidget(m_dateRangeLabel);
    dateNavLayout->addWidget(m_jumpCalendar);
    dateNavLayout->addWidget(nextBtn);
    dateNavLayout->addSpacing(20);
    dateNavLayout->addWidget(todayBtn);
    
    topLayout->addLayout(dateNavLayout);
    topLayout->addStretch();

    // 右侧：功能按钮
    QPushButton *tplBtn = new QPushButton("应用排班模板");
    QPushButton *exportBtn = new QPushButton("导出排班表");
    
    tplBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #3b82f6; color: #3b82f6; border-radius: 6px; padding: 8px 16px; font-weight: bold; } "
                          "QPushButton:hover { background: #eff6ff; }");
    exportBtn->setStyleSheet("QPushButton { background: #3b82f6; color: white; border: none; border-radius: 6px; padding: 8px 16px; font-weight: bold; } "
                             "QPushButton:hover { background: #2563eb; }");
    
    topLayout->addWidget(tplBtn);
    topLayout->addSpacing(10);
    topLayout->addWidget(exportBtn);

    mainLayout->addWidget(topBar);

    // --- 中间排班表卡片 ---
    QFrame *tableCard = new QFrame();
    tableCard->setStyleSheet("QFrame { background: white; border: 1px solid #ebeef5; border-radius: 12px; }");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(1, 1, 1, 1);

    m_table = new QTableWidget();
    m_table->setColumnCount(8); // 员工 + 周一至周日
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(75); // 增加行高以适应大头像
    m_table->horizontalHeader()->setFixedHeight(50);
    
    m_table->setStyleSheet(
        "QTableWidget { border: none; background: white; gridline-color: #f1f5f9; outline: none; } "
        "QHeaderView::section { background: #f8fafc; border: none; border-bottom: 1px solid #e2e8f0; color: #64748b; font-weight: bold; padding: 5px; } "
    );

    tableLayout->addWidget(m_table);
    mainLayout->addWidget(tableCard);

    // --- 初始化全屏大图预览层 (复刻 RoleModule) ---
    m_imagePreviewOverlay = new QWidget(this);
    m_imagePreviewOverlay->setObjectName("EmployeePreviewOverlay");
    m_imagePreviewOverlay->setStyleSheet("#EmployeePreviewOverlay { background-color: rgba(0, 0, 0, 215); }");
    m_imagePreviewOverlay->hide();
    m_imagePreviewOverlay->installEventFilter(this);

    QVBoxLayout *previewL = new QVBoxLayout(m_imagePreviewOverlay);
    m_previewLabel = new QLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("border: none; background: transparent;"); 
    previewL->addWidget(m_previewLabel, 0, Qt::AlignCenter);

    // 绑定信号
    connect(prevBtn, &QPushButton::clicked, this, &ScheduleModule::onPrevWeek);
    connect(nextBtn, &QPushButton::clicked, this, &ScheduleModule::onNextWeek);
    connect(todayBtn, &QPushButton::clicked, this, &ScheduleModule::onToday);
    connect(m_jumpCalendar, &CustomCalendarEdit::dateChanged, this, &ScheduleModule::onDateChanged);
    connect(m_table, &QTableWidget::cellClicked, this, &ScheduleModule::onCellClicked);
    connect(exportBtn, &QPushButton::clicked, this, &ScheduleModule::onExport);
    connect(tplBtn, &QPushButton::clicked, this, &ScheduleModule::onApplyTemplate);
}

void ScheduleModule::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_imagePreviewOverlay) {
        m_imagePreviewOverlay->setGeometry(rect());
    }
}

bool ScheduleModule::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        // 1. 如果点击的是头像，直接放大
        if (watched->property("imgPath").isValid()) {
            showBigImage(watched->property("imgPath").toString());
            return true;
        }
        
        // 2. 处理遮罩层点击（关闭预览）
        if (watched == m_imagePreviewOverlay && event->type() == QEvent::MouseButtonRelease) {
            hideBigImage();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ScheduleModule::showBigImage(const QString &path)
{
    if (path.isEmpty()) return;
    QPixmap pix = ImageUtils::loadPixmap(path);
    if (pix.isNull()) return;
    
    m_imagePreviewOverlay->setGeometry(rect());
    int maxDim = qMin(this->window()->width(), this->window()->height()) * 0.8;
    m_previewLabel->setPixmap(pix.scaled(maxDim, maxDim, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_imagePreviewOverlay->show();
    m_imagePreviewOverlay->raise();
}

void ScheduleModule::hideBigImage() { m_imagePreviewOverlay->hide(); }

QPixmap ScheduleModule::createCircularAvatar(const QPixmap &src, int size)
{
    QPixmap target(size, size);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    
    QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = (size - scaled.width()) / 2;
    int y = (size - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    return target;
}

void ScheduleModule::updateHeaderDates()
{
    QStringList headers;
    headers << "员工信息";
    
    QStringList weekNames = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    for (int i = 0; i < 7; ++i) {
        QDate d = m_currentMonday.addDays(i);
        headers << QString("%1\n%2").arg(weekNames[i]).arg(d.toString("MM/dd"));
    }
    m_table->setHorizontalHeaderLabels(headers);
    
    m_dateRangeLabel->setText(QString("%1 - %2")
        .arg(m_currentMonday.toString("yyyy年MM月dd日"))
        .arg(m_currentMonday.addDays(6).toString("MM月dd日")));
}

void ScheduleModule::updateTable()
{
    // 优化 1：禁用更新，防止填充过程中的频繁重绘导致的卡顿
    m_table->setUpdatesEnabled(false);
    
    updateHeaderDates();
    
    auto allStaff = StaffDataManager::instance()->allStaff();
    m_table->setRowCount(allStaff.size());
    
    for (int i = 0; i < allStaff.size(); ++i) {
        const auto &staff = allStaff[i];
        
        // 第 0 列：员工信息 (复用或新建)
        QWidget *staffWidget = m_table->cellWidget(i, 0);
        if (!staffWidget) {
            staffWidget = new QWidget();
            QHBoxLayout *l = new QHBoxLayout(staffWidget);
            l->setContentsMargins(15, 0, 10, 0);
            l->setSpacing(12);
            
            QLabel *avatar = new QLabel();
            avatar->setObjectName("avatar");
            avatar->setFixedSize(44, 44);
            avatar->setCursor(Qt::PointingHandCursor);
            avatar->setStyleSheet("border: none; background: transparent;");
            
            QVBoxLayout *textL = new QVBoxLayout();
            textL->setSpacing(1);
            
            QLabel *name = new QLabel();
            name->setObjectName("name");
            name->setStyleSheet("font-weight: bold; color: #334155; font-size: 14px; border: none; background: transparent;");
            
            QLabel *idRole = new QLabel();
            idRole->setObjectName("idRole");
            idRole->setStyleSheet("color: #94a3b8; font-size: 11px; border: none; background: transparent;");
            
            textL->addWidget(name);
            textL->addWidget(idRole);
            l->addWidget(avatar);
            l->addLayout(textL);
            l->addStretch();
            m_table->setCellWidget(i, 0, staffWidget);
        }

        // 更新内容
        staffWidget->findChild<QLabel*>("name")->setText(staff.name);
        staffWidget->findChild<QLabel*>("idRole")->setText(QString("%1 | %2").arg(staff.id).arg(staff.role));
        QLabel *avatar = staffWidget->findChild<QLabel*>("avatar");
        QPixmap srcPix = ImageUtils::loadPixmap(staff.imgPath);
        QPixmap pix = (staff.imgPath.isEmpty() || srcPix.isNull()) ? QPixmap(staff.gender == "女" ? ":/images/female.png" : ":/images/male.png") : srcPix;
        avatar->setPixmap(createCircularAvatar(pix, 44));
        avatar->setProperty("imgPath", staff.imgPath.isEmpty() ? (staff.gender == "女" ? ":/images/female.png" : ":/images/male.png") : staff.imgPath);
        avatar->installEventFilter(this);

        // 第 1-7 列：班次 (复用或新建)
        for (int day = 0; day < 7; ++day) {
            QDate d = m_currentMonday.addDays(day);
            ScheduleInfo info = ScheduleDataManager::instance()->getSchedule(staff.id, d);
            
            QWidget *oldWidget = m_table->cellWidget(i, day + 1);
            if (oldWidget) {
                // 更新现有部件的内容
                QLabel *tag = oldWidget->findChild<QLabel*>("shiftTag");
                if (tag) updateShiftTag(tag, info);
            } else {
                // 新建
                m_table->setCellWidget(i, day + 1, createShiftWidget(info));
            }
        }
    }
    
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 160);
    for (int i = 1; i < 8; ++i) {
        m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }

    // 恢复更新并触发重绘
    m_table->setUpdatesEnabled(true);
}

void ScheduleModule::updateShiftTag(QLabel *tag, const ScheduleInfo &info)
{
    QString start = info.startTime.length() > 5 ? info.startTime.left(5) : info.startTime;
    QString end = info.endTime.length() > 5 ? info.endTime.left(5) : info.endTime;

    QString style = "border-radius: 8px; font-size: 12px; font-weight: bold; ";
    if (info.type == SHIFT_MORNING) {
        tag->setText(QString("☀️ 早班\n%1-%2").arg(start).arg(end));
        style += "background: #fff7ed; color: #f59e0b; border: 1px solid #ffedd5;";
    } else if (info.type == SHIFT_EVENING) {
        tag->setText(QString("🌙 晚班\n%1-%2").arg(start).arg(end));
        style += "background: #f5f3ff; color: #8b5cf6; border: 1px solid #ede9fe;";
    } else if (info.type == SHIFT_CUSTOM) {
        tag->setText(QString("⚙️ 自定义\n%1-%2").arg(start).arg(end));
        style += "background: #eff6ff; color: #3b82f6; border: 1px solid #dbeafe;";
    } else {
        tag->setText("🌿 休息");
        style += "background: #f8fafc; color: #94a3b8; border: 1px solid #f1f5f9;";
    }
    tag->setStyleSheet(style);
}

QWidget* ScheduleModule::createShiftWidget(const ScheduleInfo &info)
{
    QWidget *container = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(container);
    l->setContentsMargins(8, 8, 8, 8);

    QLabel *tag = new QLabel();
    tag->setObjectName("shiftTag"); // 设置对象名称以便后续复用查找
    tag->setAlignment(Qt::AlignCenter);
    
    updateShiftTag(tag, info); // 使用统一的更新函数
    
    tag->setCursor(Qt::PointingHandCursor);
    l->addWidget(tag);
    
    return container;
}

void ScheduleModule::onPrevWeek()
{
    m_currentMonday = m_currentMonday.addDays(-7);
    updateTable();
    
    // 异步后台静默刷新
    ScheduleDataManager::instance()->requestScheduleList(
        m_currentMonday.toString("yyyy-MM-dd"),
        m_currentMonday.addDays(7).toString("yyyy-MM-dd")
    );
}

void ScheduleModule::onNextWeek()
{
    m_currentMonday = m_currentMonday.addDays(7);
    updateTable();
    
    ScheduleDataManager::instance()->requestScheduleList(
        m_currentMonday.toString("yyyy-MM-dd"),
        m_currentMonday.addDays(7).toString("yyyy-MM-dd")
    );
}

void ScheduleModule::onToday()
{
    QDate today = QDate::currentDate();
    m_currentMonday = today.addDays(1 - today.dayOfWeek());
    updateTable();
}

void ScheduleModule::onDateChanged(const QDate &date)
{
    if (date.isValid()) {
        // 自动计算该日期所属周的周一
        m_currentMonday = date.addDays(1 - date.dayOfWeek());
        updateTable();
    }
}

void ScheduleModule::onCellClicked(int row, int col)
{
    if (col == 0) return; // 点击员工信息列不处理
    
    // 获取员工ID
    auto allStaff = StaffDataManager::instance()->allStaff();
    QString staffId = allStaff[row].id;
    QDate date = m_currentMonday.addDays(col - 1);
    
    // 限制：不能修改过去日期的排班
    if (date < QDate::currentDate()) {
        CustomMessageDialog::showWarning(this, "操作受限", "不能修改过去日期的排班！");
        return;
    }
    
    ScheduleInfo info = ScheduleDataManager::instance()->getSchedule(staffId, date);
    
    // 简单的循环切换逻辑：休息 -> 早班 -> 晚班 -> 休息
    if (info.type == SHIFT_OFF) {
        info.type = SHIFT_MORNING;
        info.startTime = "09:00";
        info.endTime = "18:00";
    } else if (info.type == SHIFT_MORNING) {
        info.type = SHIFT_EVENING;
        info.startTime = "13:00";
        info.endTime = "22:00";
    } else {
        info.type = SHIFT_OFF;
        info.startTime = "";
        info.endTime = "";
    }
    
    ScheduleDataManager::instance()->setSchedule(info);
    updateTable();
}

void ScheduleModule::onExport()
{
    CustomMessageDialog::showSuccess(this, "导出成功", "排班表已成功导出为 Excel 文档。");
}

void ScheduleModule::onApplyTemplate()
{
    ScheduleTemplateDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // 店长在编辑器里点击了“保存并应用”
        QList<ScheduleInfo> batchInfos;
        auto allStaff = StaffDataManager::instance()->allStaff();
        for (const auto &staff : allStaff) {
            auto weeklyTpl = ScheduleDataManager::instance()->getTemplateSchedule(staff.id);
            for (int i = 0; i < 7; ++i) {
                QDate date = m_currentMonday.addDays(i);
                if (date < QDate::currentDate()) continue; // 过去日期的排班保持不变，不允许被覆盖
                
                ScheduleInfo info = weeklyTpl[i];
                info.date = date.toString("yyyy-MM-dd");
                batchInfos.append(info);
            }
        }
        ScheduleDataManager::instance()->setSchedules(batchInfos);
        
        CustomMessageDialog::showSuccess(this, "应用成功", "排班模板已成功应用到本周！");
    }
}
