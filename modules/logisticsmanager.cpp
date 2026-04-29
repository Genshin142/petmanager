#include "logisticsmanager.h"
#include "petdatamanager.h"
#include <QUuid>

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
    // 初始化一些模拟数据用于测试
    auto addTask = [&](const QString &pId, const QString &type, const QString &status, const QString &addr, const QString &time, const QString &module, const QString &room) {
        LogisticsTask task;
        task.taskId = "T" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        task.petId = pId;
        task.type = type;
        task.status = status;
        task.address = addr;
        task.appointmentTime = QDate::currentDate().toString("yyyy-MM-dd") + " " + time;
        task.relatedModule = module;
        task.relatedRoomId = room;
        m_tasks.insert(task.taskId, task);
    };

    addTask("P1008", "单程接宠", "待处理", "阳光小区 3栋 201", "14:00 - 16:00", "寄养入住", "109");
    addTask("P1001", "单程接宠", "待处理", "滨江花园 5号楼", "09:00 - 11:00", "单纯洗护", "103");
    addTask("P1002", "单程送宠", "进行中", "万达广场 A座 1502", "11:00 - 14:00", "寄养送回", "101");
    addTask("P1003", "单程接宠", "待处理", "月亮湾 8号别墅", "16:00 - 18:00", "就医体检", "102");

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
        
        // 三段式自动流转逻辑
        if (now >= taskEndTime) {
            newStatus = "已完成";
        } else if (now >= taskStartTime) {
            newStatus = "进行中";
        }

        // 状态只能向前推进，不能后退
        if (newStatus != task.status) {
            // 如果从待处理变更为进行中，同步更新宠物状态
            if (task.status == "待处理" && newStatus == "进行中") {
                PetInfo info = PetDataManager::instance()->getPet(task.petId);
                if (!info.id.isEmpty()) {
                    info.status = "接送中 (在途)";
                    PetDataManager::instance()->updatePet(info);
                }
            }
            task.status = newStatus;
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
    emit logisticsDataChanged();
}

void LogisticsManager::updateTaskStatus(const QString &taskId, const QString &status)
{
    if (m_tasks.contains(taskId)) {
        m_tasks[taskId].status = status;
        emit logisticsDataChanged();
    }
}

QList<LogisticsTask> LogisticsManager::getAllTasks() const
{
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
