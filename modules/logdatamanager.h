#ifndef LOGDATAMANAGER_H
#define LOGDATAMANAGER_H

#include <QObject>
#include <QList>
#include "../common_types.h"

class LogDataManager : public QObject {
    Q_OBJECT
public:
    explicit LogDataManager(QObject *parent = nullptr);
    bool initTable(); // Now just returns true
    QList<SysOperationLog> fetchLogs(int limit, int offset, const QString &startDate = "", const QString &endDate = "", const QString &operatorName = "", const QString &module = "");
    int getTotalCount(const QString &startDate = "", const QString &endDate = "", const QString &operatorName = "", const QString &module = "");
    QStringList fetchDistinctModules();
    QStringList fetchDistinctOperators();
    
    bool insertMockLog(const SysOperationLog &log);

private:
    QList<SysOperationLog> m_mockLogs;
};

#endif // LOGDATAMANAGER_H
