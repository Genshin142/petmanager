#ifndef LOGDATAMANAGER_H
#define LOGDATAMANAGER_H

#include <QObject>
#include <QList>
#include <QJsonObject>
#include "../common_types.h"
#include "../protocol_codes.h"

class LogDataManager : public QObject {
    Q_OBJECT
public:
    explicit LogDataManager(QObject *parent = nullptr);
    
    // 静态接口：用于记录和写入日志
    static void setCurrentUser(const QString &username);
    static void writeLog(const QString &module, const QString &action, const QString &details);
    static void writeLog(const QString &module, const QString &action, const QJsonObject &diffDetails);

    // 动态接口：向服务器请求分页过滤日志
    void fetchLogs(int limit, int offset, const QString &startDate = "", const QString &endDate = "", const QString &operatorName = "", const QString &module = "");
    
signals:
    void logsReceived(const QList<SysOperationLog> &logs, int totalCount);

private slots:
    void onPacketReceived(const Protocol::NetPacket &packet);

private:
    static QString s_currentUser;
};

#endif // LOGDATAMANAGER_H
