#include "editattendancedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>

EditAttendanceDialog::EditAttendanceDialog(const ScheduleInfo &info, QWidget *parent)
    : QDialog(parent), m_info(info)
{
    setWindowTitle(QString("编辑排班与考勤 - %1").arg(info.date));
    setFixedWidth(380);
    setStyleSheet(
        "QDialog { background: #ffffff; border-radius: 12px; } "
        "QLabel { color: #475569; font-size: 13px; font-weight: 500; } "
        "QLineEdit { border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; font-size: 13px; color: #1e293b; background: #ffffff; } "
        "QLineEdit:focus { border-color: #3b82f6; } "
        "QComboBox { border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; background: white; font-size: 13px; color: #1e293b; } "
        "QComboBox:hover { border-color: #3b82f6; } "
        "QComboBox::drop-down { border: none; width: 24px; } "
        "QComboBox::down-arrow { image: url(:/images/chevron-down.svg); width: 12px; height: 12px; } "
        "QComboBox QAbstractItemView { border: 1px solid #e2e8f0; border-radius: 8px; background: white; selection-background-color: #f1f5f9; selection-color: #3b82f6; outline: none; padding: 5px; } "
        "QPushButton { font-size: 13px; padding: 6px 14px; border-radius: 6px; font-weight: 600; } "
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 顶部日期提示
    QLabel *headerLabel = new QLabel(QString("调整员工考勤及排班 (工号: %1)").arg(info.employeeId));
    headerLabel->setStyleSheet("color: #64748b; font-size: 12px; font-weight: bold; margin-bottom: 5px;");
    mainLayout->addWidget(headerLabel);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"休息", "早班", "晚班", "自定义"});
    formLayout->addRow("排班班次:", m_typeCombo);

    m_timeContainer = new QWidget();
    QFormLayout *timeForm = new QFormLayout(m_timeContainer);
    timeForm->setContentsMargins(0, 0, 0, 0);
    timeForm->setSpacing(10);
    m_planStartEdit = new QLineEdit(info.startTime.isEmpty() ? "09:00" : info.startTime);
    m_planStartEdit->setPlaceholderText("格式: HH:mm (如 09:00)");
    m_planEndEdit = new QLineEdit(info.endTime.isEmpty() ? "18:00" : info.endTime);
    m_planEndEdit->setPlaceholderText("格式: HH:mm (如 18:00)");
    timeForm->addRow("计划上班:", m_planStartEdit);
    timeForm->addRow("计划下班:", m_planEndEdit);
    formLayout->addRow(m_timeContainer);

    m_clockContainer = new QWidget();
    QFormLayout *clockForm = new QFormLayout(m_clockContainer);
    clockForm->setContentsMargins(0, 0, 0, 0);
    clockForm->setSpacing(10);
    m_clockInEdit = new QLineEdit(info.clockIn);
    m_clockInEdit->setPlaceholderText("格式: HH:mm (如 08:52)");
    m_clockOutEdit = new QLineEdit(info.clockOut);
    m_clockOutEdit->setPlaceholderText("格式: HH:mm (如 18:04)");
    clockForm->addRow("补签上班:", m_clockInEdit);
    clockForm->addRow("补签下班:", m_clockOutEdit);
    formLayout->addRow(m_clockContainer);

    m_noteEdit = new QLineEdit(info.note);
    m_noteEdit->setPlaceholderText("如: 忘记刷卡，店长代补签");
    formLayout->addRow("考勤备注:", m_noteEdit);

    mainLayout->addLayout(formLayout);

    // 交互联动逻辑：如果是休息，隐藏计划时间及打卡栏
    auto updateVisibility = [this]() {
        bool isOff = m_typeCombo->currentText() == "休息";
        m_timeContainer->setVisible(!isOff);
        m_clockContainer->setVisible(!isOff);
        adjustSize();
    };
    connect(m_typeCombo, &QComboBox::currentTextChanged, this, updateVisibility);
    
    // 初始化选择框值
    if (info.type == SHIFT_MORNING) m_typeCombo->setCurrentText("早班");
    else if (info.type == SHIFT_EVENING) m_typeCombo->setCurrentText("晚班");
    else if (info.type == SHIFT_CUSTOM) m_typeCombo->setCurrentText("自定义");
    else m_typeCombo->setCurrentText("休息");
    updateVisibility();

    // 底部操作按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    QPushButton *cancelBtn = new QPushButton("取消");
    cancelBtn->setStyleSheet("background: #f1f5f9; color: #475569; border: 1px solid #cbd5e1;");
    QPushButton *saveBtn = new QPushButton("确认保存");
    saveBtn->setStyleSheet("background: #3b82f6; color: #ffffff; border: none;");
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &QDialog::accept);
}

ScheduleInfo EditAttendanceDialog::getUpdatedInfo() const {
    ScheduleInfo info = m_info;
    QString text = m_typeCombo->currentText();
    if (text == "早班") info.type = SHIFT_MORNING;
    else if (text == "晚班") info.type = SHIFT_EVENING;
    else if (text == "自定义") info.type = SHIFT_CUSTOM;
    else info.type = SHIFT_OFF;

    if (info.type != SHIFT_OFF) {
        info.startTime = m_planStartEdit->text().trimmed();
        info.endTime = m_planEndEdit->text().trimmed();
        info.clockIn = m_clockInEdit->text().trimmed();
        info.clockOut = m_clockOutEdit->text().trimmed();
    } else {
        info.startTime = "";
        info.endTime = "";
        info.clockIn = "";
        info.clockOut = "";
    }
    info.note = m_noteEdit->text().trimmed();
    return info;
}
