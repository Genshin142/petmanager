#ifndef LOGISTICSMANAGER_H
#define LOGISTICSMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include "../common_types.h"

class LogisticsManager : public QObject
{
    Q_OBJECT
public:
    static LogisticsManager* instance();

    // 核心接口
    void addLogisticsTask(const LogisticsTask &task);
    void updateTaskStatus(const QString &taskId, const QString &status);
    QList<LogisticsTask> getAllTasks() const;
    QList<LogisticsTask> getTasksForPet(const QString &petId) const;

signals:
    void logisticsDataChanged();

private:
    explicit LogisticsManager(QObject *parent = nullptr);
    static LogisticsManager *m_instance;

    QMap<QString, LogisticsTask> m_tasks;
};

#endif // LOGISTICSMANAGER_H
