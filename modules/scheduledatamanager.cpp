#include "scheduledatamanager.h"
#include "staffdatamanager.h"

ScheduleDataManager* ScheduleDataManager::m_instance = nullptr;

ScheduleDataManager* ScheduleDataManager::instance()
{
    if (!m_instance) {
        m_instance = new ScheduleDataManager();
    }
    return m_instance;
}

ScheduleDataManager::ScheduleDataManager(QObject *parent) : QObject(parent)
{
    initMockData();
}



ScheduleInfo ScheduleDataManager::getSchedule(const QString &employeeId, const QDate &date)
{
    QString key = QString("%1_%2").arg(employeeId).arg(date.toString("yyyy-MM-dd"));
    if (m_schedules.contains(key)) {
        return m_schedules.value(key);
    }

    // 默认休息
    ScheduleInfo info;
    info.employeeId = employeeId;
    info.date = date.toString("yyyy-MM-dd");
    info.type = SHIFT_OFF;
    return info;
}

QList<ScheduleInfo> ScheduleDataManager::getWeeklySchedule(const QString &staffId, const QDate &monday)
{
    QList<ScheduleInfo> list;
    for (int i = 0; i < 7; ++i) {
        list.append(getSchedule(staffId, monday.addDays(i)));
    }
    return list;
}

void ScheduleDataManager::saveSchedule(const ScheduleInfo &info)
{
    QString key = QString("%1_%2").arg(info.employeeId).arg(info.date);
    m_schedules[key] = info;
    emit scheduleChanged();
}

void ScheduleDataManager::setSchedule(const ScheduleInfo &info)
{
    saveSchedule(info);
}

QList<ScheduleInfo> ScheduleDataManager::getTemplateSchedule(const QString &staffId)
{
    QList<ScheduleInfo> templateList;
    for (int i = 0; i < 7; ++i) {
        QString key = QString("TPL_%1_%2").arg(staffId).arg(i);
        if (m_schedules.contains(key)) {
            templateList.append(m_schedules[key]);
        } else {
            // 默认休息
            ScheduleInfo info;
            info.employeeId = staffId;
            info.type = SHIFT_OFF;
            templateList.append(info);
        }
    }
    return templateList;
}

void ScheduleDataManager::saveTemplateSchedule(const QString &staffId, const QList<ScheduleInfo> &weeklyTemplate)
{
    for (int i = 0; i < weeklyTemplate.size() && i < 7; ++i) {
        QString key = QString("TPL_%1_%2").arg(staffId).arg(i);
        m_schedules[key] = weeklyTemplate[i];
    }
}

void ScheduleDataManager::clearAllData() { m_schedules.clear(); }

void ScheduleDataManager::setSchedules(const QList<ScheduleInfo> &infos)
{
    for (const auto &info : infos) {
        QString key = QString("%1_%2").arg(info.employeeId).arg(info.date);
        m_schedules[key] = info;
    }
    emit scheduleChanged();
}

QList<ScheduleInfo> ScheduleDataManager::getSchedulesInRange(const QDate &start, const QDate &end)
{
    QList<ScheduleInfo> result;
    for (const auto &info : m_schedules) {
        QDate d = QDate::fromString(info.date, "yyyy-MM-dd");
        if (d >= start && d <= end) {
            result.append(info);
        }
    }
    return result;
}

void ScheduleDataManager::initMockData()
{
    // 获取当前周的日期
    QDate today = QDate::currentDate();
    int dayOfWeek = today.dayOfWeek();
    QDate monday = today.addDays(1 - dayOfWeek);

    auto allStaff = StaffDataManager::instance()->allStaff();

    // 简单的模拟规律：
    // E001-E004 轮班
    // E005 (店长) 每天都在
    
    for (const auto &staff : allStaff) {
        for (int i = 0; i < 14; ++i) { // 模拟两周
            QDate d = monday.addDays(i);
            ScheduleInfo info;
            info.employeeId = staff.id;
            info.date = d.toString("yyyy-MM-dd");

            if (staff.role == "店长") {
                info.type = SHIFT_MORNING;
                info.startTime = "09:00";
                info.endTime = "18:00";
            } else {
                // 简单的轮班：1,2早班，3,4晚班，5休息...
                int pattern = (staff.id.mid(1).toInt() + i) % 3;
                if (pattern == 0) {
                    info.type = SHIFT_MORNING;
                    info.startTime = "09:00";
                    info.endTime = "18:00";
                } else if (pattern == 1) {
                    info.type = SHIFT_EVENING;
                    info.startTime = "13:00";
                    info.endTime = "22:00";
                } else {
                    info.type = SHIFT_OFF;
                }
            }
            QString key = QString("%1_%2").arg(info.employeeId).arg(info.date);
            m_schedules[key] = info;
        }
    }
}
