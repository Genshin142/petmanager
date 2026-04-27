#include "logisticsmanager.h"
#include <QUuid>

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
    LogisticsTask mockTask;
    mockTask.taskId = "T" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    mockTask.petId = "P1008"; // 可可
    mockTask.type = "单程接宠";
    mockTask.status = "待处理";
    mockTask.address = "阳光小区 3栋 201";
    mockTask.appointmentTime = "2026-04-26 14:00";
    mockTask.relatedModule = "Foster";
    mockTask.relatedRoomId = "109";
    m_tasks.insert(mockTask.taskId, mockTask);
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
