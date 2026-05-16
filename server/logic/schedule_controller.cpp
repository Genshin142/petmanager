#include "schedule_controller.h"
#include "../database/connectionpool.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include "../logger_compat.h"
#include <QJsonDocument>

ScheduleController::ScheduleController(ServerCore *core, QObject *parent)
    : QObject(parent)
{
    core->registerHandler(Protocol::CMD_GET_SCHEDULE, [this](ClientHandler *client, const QJsonObject &body) {
        handleGetSchedule(client, body);
    });
    core->registerHandler(Protocol::CMD_UPDATE_SCHEDULE, [this](ClientHandler *client, const QJsonObject &body) {
        handleUpdateSchedule(client, body);
    });
    core->registerHandler(Protocol::CMD_BATCH_UPDATE_SCHEDULE, [this](ClientHandler *client, const QJsonObject &body) {
        handleBatchUpdateSchedule(client, body);
    });
}

void ScheduleController::handleGetSchedule(ClientHandler *client, const QJsonObject &body)
{
    QString startDate = body["start_date"].toString();
    QString endDate = body["end_date"].toString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT s.emp_id, s.work_date, s.shift_type, s.plan_start, s.plan_end "
                  "FROM sys_schedules s "
                  "WHERE s.work_date BETWEEN :start AND :end AND s.is_deleted = 0");
    query.bindValue(":start", startDate);
    query.bindValue(":end", endDate);

    QJsonObject response;
    if (query.exec()) {
        QJsonArray data;
        while (query.next()) {
            QJsonObject item;
            int dbEmpId = query.value("emp_id").toInt();
            item["emp_id"] = QString("E%1").arg(dbEmpId, 3, 10, QChar('0'));
            item["work_date"] = query.value("work_date").toString();
            item["shift_type"] = query.value("shift_type").toString();
            item["plan_start"] = query.value("plan_start").isNull() ? "" : query.value("plan_start").toString();
            item["plan_end"] = query.value("plan_end").isNull() ? "" : query.value("plan_end").toString();
            data.append(item);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = data;
    } else {
        LOG_E("[DB] Get schedule failed: " << query.lastError().text().toStdString());
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Query failed";
    }
    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_GET_SCHEDULE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

bool ScheduleController::upsertSchedule(const QJsonObject &obj)
{
    QString empIdStr = obj["emp_id"].toString();
    int empId = empIdStr.mid(1).toInt(); // "E001" -> 1
    QString workDate = obj["work_date"].toString();
    QString shiftType = obj["shift_type"].toString();
    QString planStart = obj["plan_start"].toString();
    QString planEnd = obj["plan_end"].toString();

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    query.prepare("INSERT INTO sys_schedules (emp_id, work_date, shift_type, plan_start, plan_end) "
                  "VALUES (:emp_id, :work_date, :shift_type, :plan_start, :plan_end) "
                  "ON DUPLICATE KEY UPDATE "
                  "shift_type = VALUES(shift_type), plan_start = VALUES(plan_start), plan_end = VALUES(plan_end)");
    
    query.bindValue(":emp_id", empId);
    query.bindValue(":work_date", workDate);
    query.bindValue(":shift_type", shiftType);
    
    if (planStart.isEmpty()) query.bindValue(":plan_start", QVariant(QMetaType::fromType<QString>()));
    else query.bindValue(":plan_start", planStart);
    
    if (planEnd.isEmpty()) query.bindValue(":plan_end", QVariant(QMetaType::fromType<QString>()));
    else query.bindValue(":plan_end", planEnd);

    bool ok = query.exec();
    if (!ok) {
        LOG_E("[DB] Upsert schedule failed: " << query.lastError().text().toStdString());
    }
    ConnectionPool::instance().closeConnection(db);
    return ok;
}

void ScheduleController::handleUpdateSchedule(ClientHandler *client, const QJsonObject &body)
{
    QJsonObject response;
    if (upsertSchedule(body)) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "success";

        // 广播通知
        QJsonObject notify;
        notify["module"] = "schedule";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Database error";
    }
    client->sendPacket(Protocol::CMD_UPDATE_SCHEDULE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void ScheduleController::handleBatchUpdateSchedule(ClientHandler *client, const QJsonObject &body)
{
    QJsonArray schedules = body["schedules"].toArray();
    bool allOk = true;
    for (int i = 0; i < schedules.size(); ++i) {
        if (!upsertSchedule(schedules[i].toObject())) {
            allOk = false;
        }
    }
    
    QJsonObject response;
    response["status"] = allOk ? Protocol::STATUS_OK : Protocol::STATUS_ERROR;
    response["message"] = allOk ? "success" : "Partial failure";
    
    if (allOk) {
        // 广播通知
        QJsonObject notify;
        notify["module"] = "schedule";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    }

    client->sendPacket(Protocol::CMD_BATCH_UPDATE_SCHEDULE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
