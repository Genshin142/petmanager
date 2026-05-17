#include "scheduledatamanager.h"
#include "staffdatamanager.h"
#include "utils/networkmanager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

ScheduleDataManager* ScheduleDataManager::m_instance = nullptr;

ScheduleDataManager* ScheduleDataManager::instance()
{
    if (!m_instance) {
        m_instance = new ScheduleDataManager();
    }
    return m_instance;
}

#include <QStandardPaths>
#include <QDir>

ScheduleDataManager::ScheduleDataManager(QObject *parent) : QObject(parent)
{
    // 1. 先从本地磁盘加载缓存，实现“秒开”
    loadFromDisk();

    // 2. 监听网络回包
    connect(&NetworkManager::instance(), &NetworkManager::packetReceived, this, &ScheduleDataManager::onPacketReceived);
}

void ScheduleDataManager::loadFromDisk()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/schedule_cache.json";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray arr = doc.array();
        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            ScheduleInfo info;
            info.employeeId = obj["emp_id"].toString();
            info.date = obj["work_date"].toString();
            info.type = (ShiftType)obj["type"].toInt();
            info.startTime = obj["start"].toString();
            info.endTime = obj["end"].toString();
            info.clockIn = obj["clock_in"].toString();
            info.clockOut = obj["clock_out"].toString();
            info.note = obj["note"].toString();
            
            QString key = info.employeeId + "_" + info.date;
            m_schedules[key] = info;
        }
        file.close();
        emit scheduleChanged();
    }
}

void ScheduleDataManager::saveToDisk()
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dirPath);
    
    QFile file(dirPath + "/schedule_cache.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray arr;
        for (auto it = m_schedules.begin(); it != m_schedules.end(); ++it) {
            if (it.key().startsWith("TPL_")) continue; // 不保存模板
            QJsonObject obj;
            obj["emp_id"] = it.value().employeeId;
            obj["work_date"] = it.value().date;
            obj["type"] = (int)it.value().type;
            obj["start"] = it.value().startTime;
            obj["end"] = it.value().endTime;
            obj["clock_in"] = it.value().clockIn;
            obj["clock_out"] = it.value().clockOut;
            obj["note"] = it.value().note;
            arr.append(obj);
        }
        file.write(QJsonDocument(arr).toJson());
        file.close();
    }
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
    setSchedule(info);
}

void ScheduleDataManager::setSchedule(const ScheduleInfo &info)
{
    QString key = info.employeeId + "_" + info.date;
    m_schedules[key] = info;

    // 同步到服务端
    QJsonObject obj;
    obj["emp_id"] = info.employeeId;
    obj["work_date"] = info.date;
    
    if (info.type == SHIFT_MORNING) obj["shift_type"] = "早班";
    else if (info.type == SHIFT_EVENING) obj["shift_type"] = "晚班";
    else if (info.type == SHIFT_CUSTOM) obj["shift_type"] = "自定义";
    else obj["shift_type"] = "休息";
    
    obj["plan_start"] = info.startTime;
    obj["plan_end"] = info.endTime;
    obj["clock_in"] = info.clockIn;
    obj["clock_out"] = info.clockOut;
    obj["note"] = info.note;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_SCHEDULE, obj);
    saveToDisk();
    emit scheduleChanged();
}

void ScheduleDataManager::requestScheduleList(const QString &startDate, const QString &endDate)
{
    QJsonObject body;
    body["start_date"] = startDate;
    body["end_date"] = endDate;
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_SCHEDULE, body);
}

void ScheduleDataManager::onPacketReceived(const Protocol::NetPacket &packet)
{
    if (packet.cmdId == Protocol::CMD_GET_SCHEDULE) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray data = root["data"].toArray();
            m_schedules.clear(); // 暂时全量覆盖
            for (int i = 0; i < data.size(); ++i) {
                QJsonObject obj = data[i].toObject();
                ScheduleInfo info;
                info.employeeId = obj["emp_id"].toString();
                info.date = obj["work_date"].toString();
                
                QString typeStr = obj["shift_type"].toString();
                if (typeStr == "早班") info.type = SHIFT_MORNING;
                else if (typeStr == "晚班") info.type = SHIFT_EVENING;
                else if (typeStr == "自定义") info.type = SHIFT_CUSTOM;
                else info.type = SHIFT_OFF;
                
                info.startTime = obj["plan_start"].toString();
                info.endTime = obj["plan_end"].toString();
                info.clockIn = obj["clock_in"].toString();
                info.clockOut = obj["clock_out"].toString();
                info.note = obj["note"].toString();
                
                QString key = info.employeeId + "_" + info.date;
                m_schedules[key] = info;
            }
            saveToDisk();
            emit scheduleChanged();
        }
    }
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
    QJsonArray arr;
    for (const auto &info : infos) {
        QString key = QString("%1_%2").arg(info.employeeId).arg(info.date);
        m_schedules[key] = info;

        QJsonObject obj;
        obj["emp_id"] = info.employeeId;
        obj["work_date"] = info.date;
        if (info.type == SHIFT_MORNING) obj["shift_type"] = "早班";
        else if (info.type == SHIFT_EVENING) obj["shift_type"] = "晚班";
        else if (info.type == SHIFT_CUSTOM) obj["shift_type"] = "自定义";
        else obj["shift_type"] = "休息";
        obj["plan_start"] = info.startTime;
        obj["plan_end"] = info.endTime;
        arr.append(obj);
    }
    
    QJsonObject body;
    body["schedules"] = arr;
    NetworkManager::instance().sendRequest(Protocol::CMD_BATCH_UPDATE_SCHEDULE, body);

    saveToDisk();
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
