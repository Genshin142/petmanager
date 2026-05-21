#include "logisticsmanager.h"
#include "petdatamanager.h"
#include "logdatamanager.h"
#include "../utils/networkmanager.h"
#include <QUuid>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

#include <algorithm>

LogisticsManager* LogisticsManager::m_instance = nullptr;

LogisticsManager* LogisticsManager::instance()
{
    if (!m_instance) {
        m_instance = new LogisticsManager();
    }
    return m_instance;
}

LogisticsManager::LogisticsManager(QObject *parent) : QObject(parent)
{
    // 模拟数据已清空，改为纯联网模式
    connect(&NetworkManager::instance(), &NetworkManager::packetReceived, this, &LogisticsManager::onPacketReceived);
    
    // 初始化时请求一次数据
    QDate today = QDate::currentDate();
    requestLogisticsList(today.addDays(-1), today.addDays(1));

    // 设置自动更新计时器 (每30秒检查一次过期任务)
    m_autoUpdateTimer = new QTimer(this);
    connect(m_autoUpdateTimer, &QTimer::timeout, this, &LogisticsManager::checkAndAutoUpdateTasks);
    m_autoUpdateTimer->start(30000); 

    // 初始化时立即检查一次
    QTimer::singleShot(500, this, &LogisticsManager::checkAndAutoUpdateTasks);
}

void LogisticsManager::checkAndAutoUpdateTasks()
{
    bool changed = false;
    QDateTime now = QDateTime::currentDateTime();

    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        LogisticsTask &task = it.value();
        
        // 如果已经是“已完成”或“已取消”，则不再处理
        if (task.status == "已完成" || task.status == "已取消") continue;

        QString timeStr = task.appointmentTime;
        QDateTime taskStartTime;
        QDateTime taskEndTime;

        if (timeStr.contains(" - ")) {
            // 格式: "2026-04-26 14:00 - 16:00"
            QStringList parts = timeStr.split(" - ");
            QString datePart = parts.at(0).left(10);
            QString startTimePart = parts.at(0).right(5);
            QString endTimePart = parts.at(1);
            
            taskStartTime = QDateTime::fromString(datePart + " " + startTimePart, "yyyy-MM-dd HH:mm");
            taskEndTime = QDateTime::fromString(datePart + " " + endTimePart, "yyyy-MM-dd HH:mm");
        } else {
            // 格式: "2026-04-26 14:00"
            taskStartTime = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm");
            taskEndTime = taskStartTime.addSecs(3600); // 准点预约默认 1 小时行程
        }

        if (!taskStartTime.isValid()) continue;

        QString newStatus = task.status;
        
        // 自动流转逻辑：仅处理“待处理” -> “进行中”的自动切换
        if (task.status == "待处理" && now >= taskStartTime) {
            newStatus = "进行中";
        }

        // 状态只能向前推进，不能后退
        if (newStatus != task.status) {
            // 如果从待处理变更为进行中，同步更新宠物状态
            if (task.status == "待处理" && newStatus == "进行中") {
                PetInfo info = PetDataManager::instance()->getPet(task.petId);
                if (!info.id.isEmpty()) {
                    info.status = "接送中";
                    PetDataManager::instance()->updatePet(info);
                }
            }
            task.status = newStatus;
            
            // 同步更新关联的预约单状态
            if (!task.relatedAppointmentId.isEmpty()) {
                AppointmentInfo appt = PetDataManager::instance()->getAppointment(task.relatedAppointmentId);
                if (!appt.id.isEmpty()) {
                    if (newStatus == "进行中") appt.status = "In-Service";
                    else if (newStatus == "已完成") appt.status = "Completed";
                    PetDataManager::instance()->updateAppointment(appt);
                }
            }
            changed = true;
        }
    }

    if (changed) {
        emit logisticsDataChanged();
    }
}

void LogisticsManager::addLogisticsTask(const LogisticsTask &task)
{
    LogisticsTask t = task;
    if (t.taskId.isEmpty()) {
        t.taskId = "T" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    }
    m_tasks.insert(t.taskId, t);

    // 同步到服务端数据库
    QJsonObject body;
    body["task_id"] = t.taskId;
    body["pet_id"] = t.petId.mid(1).toInt(); // "P001" -> 1
    body["appt_id"] = t.relatedAppointmentId.toInt();
    body["task_type"] = t.type;
    body["status"] = t.status;
    body["address"] = t.address;
    // t.appointmentTime 可能是时间段格式 "2026-05-19 09:00 - 11:00"，提取开始时间
    QString schedTime = t.appointmentTime;
    if (schedTime.contains(" - ")) {
        // 提取日期和开始时间: "2026-05-19 09:00 - 11:00" -> "2026-05-19 09:00"
        int dashIdx = schedTime.indexOf(" - ");
        schedTime = schedTime.left(dashIdx).trimmed();
    }
    body["schedule_time"] = schedTime;
    body["amount"] = t.amount;
    NetworkManager::instance().sendRequest(Protocol::CMD_ADD_LOGISTICS_TASK, body);

    // 记录系统操作日志
    LogDataManager::writeLog("车辆调度", "新增/编辑派单任务", 
        QString("任务ID: %1, 宠物ID: %2, 类型: %3, 地址: %4, 状态: %5")
        .arg(t.taskId, t.petId, t.type, t.address, t.status));

    emit logisticsDataChanged();
}

void LogisticsManager::cancelTask(const QString &taskId)
{
    if (m_tasks.contains(taskId)) {
        m_tasks.remove(taskId);

        // 同步取消（设置已取消状态）到服务端
        updateLogisticsStatusRemote(taskId, "已取消");

        // 记录系统操作日志
        LogDataManager::writeLog("车辆调度", "取消派单任务", 
            QString("任务ID: %1").arg(taskId));

        emit logisticsDataChanged();
    }
}

void LogisticsManager::updateTaskStatus(const QString &taskId, const QString &status)
{
    if (m_tasks.contains(taskId)) {
        LogisticsTask &task = m_tasks[taskId];
        task.status = status;
        
        // 同步状态到服务端
        updateLogisticsStatusRemote(taskId, status);

        // 同步更新关联的预约单状态
        if (!task.relatedAppointmentId.isEmpty()) {
            AppointmentInfo appt = PetDataManager::instance()->getAppointment(task.relatedAppointmentId);
            if (!appt.id.isEmpty()) {
                if (status == "进行中") appt.status = "In-Service";
                else if (status == "已送达" || status == "已完成") appt.status = "Completed";
                PetDataManager::instance()->updateAppointment(appt);
            }
        }

        // 记录系统操作日志
        LogDataManager::writeLog("车辆调度", "更新任务状态", 
            QString("任务ID: %1, 新状态: %2").arg(taskId, status));
        
        emit logisticsDataChanged();
    }
}

QList<LogisticsTask> LogisticsManager::getAllTasks() const
{
    QMutexLocker locker(&m_mutex);
    return m_tasks.values();
}

QList<LogisticsTask> LogisticsManager::getTasksByDate(const QDate &date) const
{
    QList<LogisticsTask> result;
    QString dateStr = date.toString("yyyy-MM-dd");
    for (const auto &task : m_tasks) {
        if (task.appointmentTime.startsWith(dateStr)) {
            result.append(task);
        }
    }
    // 按时间点排序 (09:00 -> 21:00)
    std::sort(result.begin(), result.end(), [](const LogisticsTask &a, const LogisticsTask &b){
        return a.appointmentTime < b.appointmentTime;
    });
    return result;
}

int LogisticsManager::getTaskCountForDate(const QDate &date) const
{
    int count = 0;
    QString dateStr = date.toString("yyyy-MM-dd");
    for (const auto &task : m_tasks) {
        if (task.appointmentTime.startsWith(dateStr)) {
            count++;
        }
    }
    return count;
}

QList<LogisticsTask> LogisticsManager::getTasksForPet(const QString &petId) const
{
    QList<LogisticsTask> result;
    for (const auto &task : m_tasks) {
        if (task.petId == petId) {
            result.append(task);
        }
    }
    return result;
}

void LogisticsManager::cancelTaskByAppointmentId(const QString &apptId)
{
    if (apptId.isEmpty()) return;
    bool changed = false;
    for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
        if (it.value().relatedAppointmentId == apptId) {
            if (it.value().status != "已取消") {
                it.value().status = "已取消";
                
                // 同步取消服务端任务
                updateLogisticsStatusRemote(it.key(), "已取消");
                changed = true;
            }
        }
    }
    if (changed) emit logisticsDataChanged();
}

// ================== 网络方法 ==================

void LogisticsManager::requestLogisticsList(const QDate &startDate, const QDate &endDate)
{
    QJsonObject body;
    body["start_date"] = startDate.toString("yyyy-MM-dd");
    body["end_date"] = endDate.toString("yyyy-MM-dd");
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_LOGISTICS_LIST, body);
}

void LogisticsManager::updateLogisticsStatusRemote(const QString &taskId, const QString &status)
{
    QJsonObject body;
    body["task_id"] = taskId;
    body["status"] = status;
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_LOGISTICS_STATUS, body);
}

void LogisticsManager::onPacketReceived(const Protocol::NetPacket &packet)
{
    if (packet.cmdId == Protocol::CMD_GET_LOGISTICS_LIST) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray data = root["data"].toArray();
            {
                QMutexLocker locker(&m_mutex);
                m_tasks.clear();
                for (int i = 0; i < data.size(); ++i) {
                    QJsonObject obj = data[i].toObject();
                    LogisticsTask task;
                    task.taskId = obj["task_id"].toString();
                    task.petId = QString("P%1").arg(obj["pet_id"].toInt(), 5, 10, QChar('0'));
                    task.type = obj["task_type"].toString();
                    task.status = obj["status"].toString();
                    task.address = obj["address"].toString();
                    // 从数据库回来的 schedule_time 是精确时间 "2026-05-19 09:00:00"
                    // 需要映射回时间段格式 "2026-05-19 09:00 - 11:00"
                    QString rawTime = obj["schedule_time"].toString();
                    if (rawTime.length() >= 16) {
                        QString datePart = rawTime.left(10);
                        QString timePart = rawTime.mid(11, 5); // "09:00"
                        QStringList timeSlots = {"09:00 - 11:00", "11:00 - 14:00", "14:00 - 16:00", "16:00 - 18:00", "18:00 - 21:00"};
                        QString mappedSlot = timePart; // 默认
                        for (const auto &slot : timeSlots) {
                            if (timePart >= slot.left(5) && timePart < slot.right(5)) {
                                mappedSlot = slot;
                                break;
                            }
                        }
                        task.appointmentTime = datePart + " " + mappedSlot;
                    } else {
                        task.appointmentTime = rawTime;
                    }
                    task.amount = obj["amount"].toDouble();
                    task.relatedAppointmentId = QString::number(obj["appt_id"].toInt());
                    m_tasks[task.taskId] = task;
                }
            }
            emit logisticsDataChanged();
            qDebug() << "[LOGISTICS] Tasks updated from server. Count:" << data.size();
        }
    }
    else if (packet.cmdId == Protocol::CMD_UPDATE_LOGISTICS_STATUS) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            qDebug() << "[LOGISTICS] Task status updated successfully";
        }
    } else if (packet.cmdId == Protocol::CMD_NOTIFY_REFRESH) {
        QDate today = QDate::currentDate();
        requestLogisticsList(today.addDays(-1), today.addDays(1));
        qDebug() << "[LOGISTICS] Received global refresh notification.";
    }
}
