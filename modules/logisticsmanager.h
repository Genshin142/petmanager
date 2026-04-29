#ifndef LOGISTICSMANAGER_H
#define LOGISTICSMANAGER_H

#include <QTimer>
#include <QDateTime>
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
    QList<LogisticsTask> getTasksByDate(const QDate &date) const;
    int getTaskCountForDate(const QDate &date) const;
    QList<LogisticsTask> getTasksForPet(const QString &petId) const;

signals:
    void logisticsDataChanged();

private slots:
    void checkAndAutoUpdateTasks();

private:
    explicit LogisticsManager(QObject *parent = nullptr);
    static LogisticsManager *m_instance;

    QMap<QString, LogisticsTask> m_tasks;
    QTimer *m_autoUpdateTimer;
};

#endif // LOGISTICSMANAGER_H
