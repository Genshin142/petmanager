#ifndef LOGDATAMANAGER_H
#define LOGDATAMANAGER_H

#include <QObject>
#include <QList>
#include "../common_types.h"

class LogDataManager : public QObject {
    Q_OBJECT
public:
    explicit LogDataManager(QObject *parent = nullptr);
    bool initTable();
    QList<SysOperationLog> fetchLogs(int limit, int offset, const QString &startDate = "", const QString &endDate = "", const QString &operatorName = "");
    int getTotalCount(const QString &startDate = "", const QString &endDate = "", const QString &operatorName = "");
    
    // For testing
    bool insertMockLog(const SysOperationLog &log);
};

#endif // LOGDATAMANAGER_H
