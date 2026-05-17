#include "log_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QDateTime>
#include <QJsonDocument>
#include "../logger_compat.h"

LogController::LogController(ServerCore *server, QObject *parent)
    : QObject(parent), m_server(server)
{
    m_server->registerHandler(Protocol::CMD_GET_LOG_LIST, std::bind(&LogController::handleGetLogList, this, std::placeholders::_1, std::placeholders::_2));
    m_server->registerHandler(Protocol::CMD_ADD_LOG, std::bind(&LogController::handleAddLog, this, std::placeholders::_1, std::placeholders::_2));
}

void LogController::handleGetLogList(ClientHandler *client, const QJsonObject &data)
{
    LOG_I("[LOG] Received get log list request");

    QString startDate = data["startDate"].toString();
    QString endDate = data["endDate"].toString();
    QString operatorName = data["operatorName"].toString();
    QString module = data["module"].toString();
    int limit = data["limit"].toInt();
    int offset = data["offset"].toInt();

    if (limit <= 0) limit = 10;

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QJsonObject response;

    if (!db.isOpen() && !db.open()) {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "DB Open Error: " + db.lastError().text();
        client->sendPacket(Protocol::CMD_GET_LOG_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    // 构建过滤 SQL 条件
    QStringList conds;
    QVariantList binds;

    if (!startDate.isEmpty()) {
        conds.append("created_at >= ?");
        binds.append(startDate + " 00:00:00");
    }
    if (!endDate.isEmpty()) {
        conds.append("created_at <= ?");
        binds.append(endDate + " 23:59:59");
    }
    if (!operatorName.isEmpty()) {
        conds.append("operator_name LIKE ?");
        binds.append("%" + operatorName + "%");
    }
    if (!module.isEmpty()) {
        conds.append("module = ?");
        binds.append(module);
    }

    QString whereClause = "";
    if (!conds.isEmpty()) {
        whereClause = "WHERE " + conds.join(" AND ");
    }

    // 1. 先查询总记录数
    QSqlQuery countQuery(db);
    countQuery.prepare("SELECT COUNT(*) FROM sys_operation_logs " + whereClause);
    for (const auto &val : binds) {
        countQuery.addBindValue(val);
    }

    int totalCount = 0;
    if (countQuery.exec() && countQuery.next()) {
        totalCount = countQuery.value(0).toInt();
    } else {
        LOG_E("[LOG] Count query error: " << countQuery.lastError().text().toStdString());
    }

    // 2. 查询分页数据
    QSqlQuery dataQuery(db);
    QString dataSql = "SELECT log_id, operator_name, module, action, details, created_at FROM sys_operation_logs " 
                      + whereClause + " ORDER BY created_at DESC LIMIT ? OFFSET ?";
    dataQuery.prepare(dataSql);
    for (const auto &val : binds) {
        dataQuery.addBindValue(val);
    }
    dataQuery.addBindValue(limit);
    dataQuery.addBindValue(offset);

    QJsonArray logList;
    if (dataQuery.exec()) {
        while (dataQuery.next()) {
            QJsonObject o;
            o["id"] = QString::number(dataQuery.value("log_id").toLongLong());
            o["operatorName"] = dataQuery.value("operator_name").toString();
            o["module"] = dataQuery.value("module").toString();
            o["action"] = dataQuery.value("action").toString();
            o["details"] = dataQuery.value("details").toString();
            o["timestamp"] = dataQuery.value("created_at").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            logList.append(o);
        }
        
        response["status"] = Protocol::STATUS_OK;
        response["data"] = logList;
        response["totalCount"] = totalCount;
        LOG_I("[LOG] Found " << logList.size() << " logs. Total matching count: " << totalCount);
    } else {
        LOG_E("[LOG] Data query error: " << dataQuery.lastError().text().toStdString());
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = dataQuery.lastError().text();
    }

    client->sendPacket(Protocol::CMD_GET_LOG_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void LogController::handleAddLog(ClientHandler *client, const QJsonObject &data)
{
    QString operatorName = data["operatorName"].toString();
    QString module = data["module"].toString();
    QString action = data["action"].toString();
    QString details = data["details"].toString();

    LOG_I("[LOG] Adding system log: " << operatorName.toStdString() << " | " << module.toStdString() << " | " << action.toStdString());

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QJsonObject response;

    if (!db.isOpen() && !db.open()) {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "DB Open Error: " + db.lastError().text();
        if (client) client->sendPacket(Protocol::CMD_ADD_LOG, QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO sys_operation_logs (operator_name, module, action, details, created_at) "
                  "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(operatorName);
    query.addBindValue(module);
    query.addBindValue(action);
    query.addBindValue(details);
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Success";
        LOG_I("[LOG] System log saved successfully.");
    } else {
        LOG_E("[LOG] Failed to save system log: " << query.lastError().text().toStdString());
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }

    if (client) {
        client->sendPacket(Protocol::CMD_ADD_LOG, QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
}
