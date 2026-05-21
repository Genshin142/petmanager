#include "logistics_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include "../logger_compat.h"

LogisticsController::LogisticsController(ServerCore *core, QObject *parent)
    : QObject(parent), m_core(core)
{
    m_core->registerHandler(Protocol::CMD_GET_LOGISTICS_LIST, [this](ClientHandler *client, const QJsonObject &body) {
        handleGetLogisticsList(client, body);
    });
    m_core->registerHandler(Protocol::CMD_UPDATE_LOGISTICS_STATUS, [this](ClientHandler *client, const QJsonObject &body) {
        handleUpdateLogisticsStatus(client, body);
    });
    m_core->registerHandler(Protocol::CMD_ADD_LOGISTICS_TASK, [this](ClientHandler *client, const QJsonObject &body) {
        handleAddLogisticsTask(client, body);
    });
}

void LogisticsController::handleGetLogisticsList(ClientHandler *client, const QJsonObject &body)
{
    QString startDate = body["start_date"].toString();
    QString endDate = body["end_date"].toString();

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM logistics_tasks WHERE DATE(schedule_time) BETWEEN :start AND :end");
    query.bindValue(":start", startDate);
    query.bindValue(":end", endDate);

    QJsonObject response;
    if (query.exec()) {
        QJsonArray data;
        while (query.next()) {
            QJsonObject item;
            item["task_id"] = query.value("task_id").toString();
            item["pet_id"] = query.value("pet_id").toInt();
            int apptId = query.value("appt_id").toInt();
            if (apptId > 0) {
                item["appt_id"] = apptId;
            } else {
                item["appt_id"] = 0;
            }
            item["task_type"] = query.value("task_type").toString();
            item["status"] = query.value("status").toString();
            item["driver_id"] = query.value("driver_id").toInt();
            item["address"] = query.value("address").toString();
            item["schedule_time"] = query.value("schedule_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            item["amount"] = query.value("amount").toDouble();
            data.append(item);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = data;
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Failed to query logistics tasks: " + query.lastError().text();
    }

    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_GET_LOGISTICS_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void LogisticsController::handleUpdateLogisticsStatus(ClientHandler *client, const QJsonObject &body)
{
    QString taskId = body["task_id"].toString();
    QString status = body["status"].toString();

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE logistics_tasks SET status = :status WHERE task_id = :tid");
    query.bindValue(":status", status);
    query.bindValue(":tid", taskId);

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        
        // 广播通知客户端数据发生变化，触发刷新
        QJsonObject notify;
        notify["module"] = "logistics";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Failed to update logistics status: " + query.lastError().text();
    }

    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_UPDATE_LOGISTICS_STATUS, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void LogisticsController::handleAddLogisticsTask(ClientHandler *client, const QJsonObject &body)
{
    QString taskId = body["task_id"].toString();
    int petId = body["pet_id"].toInt();
    int apptId = body["appt_id"].toInt();
    QString taskType = body["task_type"].toString();
    QString status = body["status"].toString();
    QString address = body["address"].toString();
    QString scheduleTime = body["schedule_time"].toString();
    double amount = body["amount"].toDouble();

    // 格式化映射 task_type 到 MySQL 枚举：'接宠', '送宠'
    QString dbTaskType = "接宠";
    if (taskType.contains("送") || taskType.contains("送宠") || taskType.contains("Return") || taskType.contains("回程")) {
        dbTaskType = "送宠";
    }

    // 兼容可能传入的两种时间格式 ("yyyy-MM-dd HH:mm" 或是 "yyyy-MM-dd HH:mm:ss")
    if (scheduleTime.length() == 16) {
        scheduleTime += ":00";
    }

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // 用 REPLACE INTO 实现极速新增与幂等覆盖更新
    query.prepare("REPLACE INTO logistics_tasks (task_id, pet_id, appt_id, task_type, status, address, schedule_time, amount) "
                  "VALUES (:tid, :pid, :aid, :type, :status, :addr, :time, :amt)");
    query.bindValue(":tid", taskId);
    query.bindValue(":pid", petId);
    if (apptId > 0) {
        query.bindValue(":aid", apptId);
    } else {
        query.bindValue(":aid", QVariant(QVariant::Int));
    }
    query.bindValue(":type", dbTaskType);
    query.bindValue(":status", status);
    query.bindValue(":addr", address);
    query.bindValue(":time", scheduleTime);
    query.bindValue(":amt", amount);

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        
        // 广播通知客户端刷新
        QJsonObject notify;
        notify["module"] = "logistics";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        qDebug() << "[ERROR] [LOGISTICS] REPLACE INTO failed:" << query.lastError().text();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Failed to save logistics task: " + query.lastError().text();
    }

    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_ADD_LOGISTICS_TASK, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
