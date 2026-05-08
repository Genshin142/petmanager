#include "scheduletemplatedialog.h"
#include "scheduledatamanager.h"
#include "staffdatamanager.h"
#include "custommessagedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QPainter>
#include <QPainterPath>

ScheduleTemplateDialog::ScheduleTemplateDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("标准周排班模板编辑");
    setMinimumSize(1000, 600);
    setStyleSheet("QDialog { background: #f8fafc; }");
    
    m_allStaff = StaffDataManager::instance()->allStaff();
    loadTemplate();
    setupUI();
}

void ScheduleTemplateDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // --- 顶部说明 ---
    QLabel *header = new QLabel("周排班模板编辑器");
    header->setStyleSheet("font-size: 22px; font-weight: bold; color: #1e293b;");
    mainLayout->addWidget(header);

    QLabel *tip = new QLabel("💡 提示：点击下方网格单元格可快速循环切换班次（休息 -> 早班 -> 晚班）。");
    tip->setStyleSheet("color: #64748b; font-size: 13px;");
    mainLayout->addWidget(tip);

    // --- 网格卡片 ---
    QFrame *card = new QFrame();
    card->setStyleSheet("QFrame { background: white; border: 1px solid #e2e8f0; border-radius: 12px; }");
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(1, 1, 1, 1);

    m_table = new QTableWidget();
    m_table->setColumnCount(8);
    m_table->setRowCount(m_allStaff.size());
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(75); // 增加行高以适应大头像
    m_table->horizontalHeader()->setFixedHeight(50);
    m_table->setStyleSheet("QTableWidget { border: none; background: white; gridline-color: #f1f5f9; outline: none; } "
                           "QHeaderView::section { background: #f8fafc; border: none; border-bottom: 1px solid #e2e8f0; color: #64748b; font-weight: bold; }");

    QStringList headers;
    headers << "员工信息" << "周一" << "周二" << "周三" << "周四" << "周五" << "周六" << "周日";
    m_table->setHorizontalHeaderLabels(headers);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 200); // 拓宽员工列到 200px
    for(int i=1; i<8; ++i) m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);

    // 填充数据
    for (int i = 0; i < m_allStaff.size(); ++i) {
        const auto &staff = m_allStaff[i];
        
        // 员工列 (复刻主界面视觉)
        QWidget *staffWidget = new QWidget();
        QHBoxLayout *sl = new QHBoxLayout(staffWidget);
        sl->setContentsMargins(15, 0, 10, 0);
        sl->setSpacing(12);

        QLabel *avatar = new QLabel();
        avatar->setFixedSize(44, 44);
        avatar->setStyleSheet("border: none; background: transparent;");
        QPixmap pix = staff.imgPath.isEmpty() ? QPixmap(staff.gender == "女" ? ":/images/female.png" : ":/images/male.png") : QPixmap(staff.imgPath);
        avatar->setPixmap(createCircularAvatar(pix, 44));
        
        QVBoxLayout *textL = new QVBoxLayout();
        textL->setSpacing(1);
        
        QLabel *name = new QLabel(staff.name);
        name->setStyleSheet("font-weight: bold; color: #334155; font-size: 14px; border: none; background: transparent;");
        
        QLabel *idRole = new QLabel(QString("%1 | %2").arg(staff.id).arg(staff.role));
        idRole->setStyleSheet("color: #94a3b8; font-size: 11px; border: none; background: transparent;");
        
        textL->addWidget(name);
        textL->addWidget(idRole);
        
        sl->addWidget(avatar);
        sl->addLayout(textL);
        sl->addStretch();
        m_table->setCellWidget(i, 0, staffWidget);

        // 排班列
        QList<ScheduleInfo> weekly = m_editingTemplate[staff.id];
        for (int day = 0; day < 7; ++day) {
            m_table->setCellWidget(i, day + 1, createShiftWidget(weekly[day]));
        }
    }

    cardLayout->addWidget(m_table);
    mainLayout->addWidget(card);

    // --- 底部按钮 ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setMinimumWidth(100);
    cancelBtn->setFixedHeight(40);
    cancelBtn->setStyleSheet("QPushButton { background: white; border: 1px solid #d1d5db; border-radius: 8px; color: #374151; font-weight: bold; padding: 0 20px; } QPushButton:hover { background: #f9fafb; }");

    QPushButton *saveApplyBtn = new QPushButton("保存并应用到本周");
    saveApplyBtn->setMinimumWidth(160);
    saveApplyBtn->setFixedHeight(40);
    saveApplyBtn->setStyleSheet("QPushButton { background: #3b82f6; border: none; border-radius: 8px; color: white; font-weight: bold; padding: 0 25px; } QPushButton:hover { background: #2563eb; }");

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveApplyBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_table, &QTableWidget::cellClicked, this, &ScheduleTemplateDialog::onCellClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveApplyBtn, &QPushButton::clicked, this, &ScheduleTemplateDialog::onSaveAndApply);
}

void ScheduleTemplateDialog::loadTemplate()
{
    for (const auto &staff : m_allStaff) {
        m_editingTemplate[staff.id] = ScheduleDataManager::instance()->getTemplateSchedule(staff.id);
    }
}

QWidget* ScheduleTemplateDialog::createShiftWidget(const ScheduleInfo &info)
{
    QWidget *w = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(6, 6, 6, 6);

    QLabel *tag = new QLabel();
    tag->setAlignment(Qt::AlignCenter);
    
    QString style = "QLabel { border-radius: 6px; font-size: 12px; font-weight: bold; padding: 4px; } ";
    if (info.type == SHIFT_OFF) {
        tag->setText("休息");
        style += "QLabel { background: #f1f5f9; color: #64748b; }";
    } else if (info.type == SHIFT_MORNING) {
        tag->setText("早班");
        style += "QLabel { background: #fff7ed; color: #f97316; border: 1px solid #ffedd5; }";
    } else {
        tag->setText("晚班");
        style += "QLabel { background: #f5f3ff; color: #8b5cf6; border: 1px solid #ede9fe; }";
    }
    tag->setStyleSheet(style);
    l->addWidget(tag);
    return w;
}

void ScheduleTemplateDialog::onCellClicked(int row, int col)
{
    if (col == 0) return;
    QString staffId = m_allStaff[row].id;
    int dayIdx = col - 1;

    // 循环切换逻辑
    auto &weekly = m_editingTemplate[staffId];
    if (weekly[dayIdx].type == SHIFT_OFF) {
        weekly[dayIdx].type = SHIFT_MORNING;
        weekly[dayIdx].startTime = "09:00";
        weekly[dayIdx].endTime = "18:00";
    } else if (weekly[dayIdx].type == SHIFT_MORNING) {
        weekly[dayIdx].type = SHIFT_EVENING;
        weekly[dayIdx].startTime = "13:00";
        weekly[dayIdx].endTime = "22:00";
    } else {
        weekly[dayIdx].type = SHIFT_OFF;
        weekly[dayIdx].startTime = "";
        weekly[dayIdx].endTime = "";
    }

    m_table->setCellWidget(row, col, createShiftWidget(weekly[dayIdx]));
}

void ScheduleTemplateDialog::onSaveAndApply()
{
    // 1. 保存到模板库
    for (const auto &staff : m_allStaff) {
        ScheduleDataManager::instance()->saveTemplateSchedule(staff.id, m_editingTemplate[staff.id]);
    }

    accept();
}

QPixmap ScheduleTemplateDialog::createCircularAvatar(const QPixmap &src, int size)
{
    QPixmap target(size, size);
    target.fill(Qt::transparent);
    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, size, size, src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    return target;
}
