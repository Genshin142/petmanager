#include "logdatamanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QDateTime>

LogDataManager::LogDataManager(QObject *parent) : QObject(parent) {
    initTable();
}

bool LogDataManager::initTable() {
    QSqlQuery query;
    QString sql = "CREATE TABLE IF NOT EXISTS sys_operation_logs ("
                  "id TEXT PRIMARY KEY, "
                  "timestamp TEXT, "
                  "operatorName TEXT, "
                  "module TEXT, "
                  "action TEXT, "
                  "details TEXT)";
    if (!query.exec(sql)) {
        qWarning() << "Failed to create sys_operation_logs table:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<SysOperationLog> LogDataManager::fetchLogs(int limit, int offset, const QString &startDate, const QString &endDate, const QString &operatorName) {
    QList<SysOperationLog> list;
    QSqlQuery query;
    
    QString sql = "SELECT id, timestamp, operatorName, module, action, details FROM sys_operation_logs WHERE 1=1";
    if (!startDate.isEmpty() && !endDate.isEmpty()) {
        sql += " AND timestamp >= :startDate AND timestamp <= :endDate";
    }
    if (!operatorName.isEmpty()) {
        sql += " AND operatorName LIKE :operatorName";
    }
    
    sql += " ORDER BY timestamp DESC LIMIT :limit OFFSET :offset";
    query.prepare(sql);
    
    if (!startDate.isEmpty() && !endDate.isEmpty()) {
        query.bindValue(":startDate", startDate + " 00:00:00");
        query.bindValue(":endDate", endDate + " 23:59:59");
    }
    if (!operatorName.isEmpty()) {
        query.bindValue(":operatorName", "%" + operatorName + "%");
    }
    
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);
    
    if (!query.exec()) {
        qWarning() << "Fetch logs failed:" << query.lastError().text();
        return list;
    }
    
    while (query.next()) {
        SysOperationLog log;
        log.id = query.value("id").toString();
        log.timestamp = query.value("timestamp").toString();
        log.operatorName = query.value("operatorName").toString();
        log.module = query.value("module").toString();
        log.action = query.value("action").toString();
        log.details = query.value("details").toString();
        list.append(log);
    }
    
    return list;
}

int LogDataManager::getTotalCount(const QString &startDate, const QString &endDate, const QString &operatorName) {
    QSqlQuery query;
    QString sql = "SELECT COUNT(*) FROM sys_operation_logs WHERE 1=1";
    if (!startDate.isEmpty() && !endDate.isEmpty()) {
        sql += " AND timestamp >= :startDate AND timestamp <= :endDate";
    }
    if (!operatorName.isEmpty()) {
        sql += " AND operatorName LIKE :operatorName";
    }
    
    query.prepare(sql);
    
    if (!startDate.isEmpty() && !endDate.isEmpty()) {
        query.bindValue(":startDate", startDate + " 00:00:00");
        query.bindValue(":endDate", endDate + " 23:59:59");
    }
    if (!operatorName.isEmpty()) {
        query.bindValue(":operatorName", "%" + operatorName + "%");
    }
    
    if (!query.exec()) {
        qWarning() << "Get total count failed:" << query.lastError().text();
        return 0;
    }
    
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool LogDataManager::insertMockLog(const SysOperationLog &log) {
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO sys_operation_logs (id, timestamp, operatorName, module, action, details) "
                  "VALUES (:id, :timestamp, :operatorName, :module, :action, :details)");
    query.bindValue(":id", log.id);
    query.bindValue(":timestamp", log.timestamp);
    query.bindValue(":operatorName", log.operatorName);
    query.bindValue(":module", log.module);
    query.bindValue(":action", log.action);
    query.bindValue(":details", log.details);
    
    if (!query.exec()) {
        qWarning() << "Insert mock log failed:" << query.lastError().text();
        return false;
    }
    return true;
}
