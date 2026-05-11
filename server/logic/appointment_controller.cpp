#include "appointment_controller.h"
#include "../database/connectionpool.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QUuid>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <glog/logging.h>

AppointmentController::AppointmentController(ServerCore *core, QObject *parent)
    : QObject(parent), m_core(core)
{
    m_core->registerHandler(Protocol::CMD_GET_APPOINTMENT_LIST, [this](ClientHandler *client, const QJsonObject &body) {
        handleGetAppointments(client, body);
    });
    m_core->registerHandler(Protocol::CMD_ADD_APPOINTMENT, [this](ClientHandler *client, const QJsonObject &body) {
        handleAddAppointment(client, body);
    });
    m_core->registerHandler(Protocol::CMD_UPDATE_APPT_STATUS, [this](ClientHandler *client, const QJsonObject &body) {
        handleUpdateStatus(client, body);
    });
}

void AppointmentController::handleGetAppointments(ClientHandler *client, const QJsonObject &body)
{
    QString startDate = body["start_date"].toString();
    QString endDate = body["end_date"].toString();

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT a.*, p.pet_name, m.name as member_name "
                  "FROM appointments a "
                  "LEFT JOIN pets p ON a.pet_id = p.pet_id "
                  "LEFT JOIN members m ON a.member_id = m.member_id "
                  "WHERE DATE(a.appt_time) BETWEEN :start AND :end AND a.is_deleted = 0 "
                  "ORDER BY a.appt_time ASC");
    query.bindValue(":start", startDate);
    query.bindValue(":end", endDate);

    QJsonObject response;
    if (query.exec()) {
        QJsonArray data;
        while (query.next()) {
            QJsonObject item;
            item["appt_id"] = query.value("appt_id").toInt();
            item["member_id"] = query.value("member_id").toInt();
            item["pet_id"] = query.value("pet_id").toInt();
            item["service_id"] = query.value("service_id").toInt();
            item["staff_id"] = query.value("staff_id").toInt();
            item["appt_time"] = query.value("appt_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            item["appt_type"] = query.value("appt_type").toString();
            item["status"] = query.value("status").toString();
            item["address"] = query.value("address").toString();
            item["amount"] = query.value("amount").toDouble();
            item["pet_name"] = query.value("pet_name").toString();
            item["member_name"] = query.value("member_name").toString();
            data.append(item);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = data;
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Failed to fetch appointments";
    }

    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_GET_APPOINTMENT_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void AppointmentController::handleAddAppointment(ClientHandler *client, const QJsonObject &body)
{
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QJsonObject response;

    if (!db.transaction()) {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Transaction start failed";
        client->sendPacket(Protocol::CMD_ADD_APPOINTMENT, QJsonDocument(response).toJson(QJsonDocument::Compact));
        ConnectionPool::instance().closeConnection(db);
        return;
    }

    try {
        QSqlQuery query(db);
        QString groupId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        query.prepare("INSERT INTO appointments (group_id, member_id, pet_id, service_id, staff_id, appt_time, appt_type, notes, address, amount) "
                      "VALUES (:group, :mid, :pid, :sid, :staff, :time, :type, :notes, :addr, :amt)");
        query.bindValue(":group", groupId);
        query.bindValue(":mid", body["member_id"].toInt());
        query.bindValue(":pid", body["pet_id"].toInt());
        query.bindValue(":sid", body["service_id"].toInt());
        query.bindValue(":staff", body["staff_id"].toInt());
        query.bindValue(":time", body["appt_time"].toString());
        query.bindValue(":type", body["appt_type"].toString());
        query.bindValue(":notes", body["notes"].toString());
        query.bindValue(":addr", body["address"].toString());
        query.bindValue(":amt", body["amount"].toDouble());

        if (!query.exec()) throw std::runtime_error("Insert appointment failed");
        int newApptId = query.lastInsertId().toInt();

        // 寄养处理
        QString roomNo = body["room_no"].toString();
        if (body["appt_type"].toString() == "Boarding" && !roomNo.isEmpty()) {
            QSqlQuery roomQuery(db);
            roomQuery.prepare("UPDATE rooms SET status = '已预订' WHERE room_no = :rno AND status = '空闲'");
            roomQuery.bindValue(":rno", roomNo);
            roomQuery.exec();
        }

        db.commit();
        
        QJsonObject res;
        res["status"] = Protocol::STATUS_OK;
        res["appt_id"] = newApptId;
        client->sendPacket(Protocol::CMD_ADD_APPOINTMENT, QJsonDocument(res).toJson(QJsonDocument::Compact));

        // 广播通知
        QJsonObject notify;
        notify["module"] = "appointment";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
        
    } catch (const std::exception &e) {
        db.rollback();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = e.what();
        client->sendPacket(Protocol::CMD_ADD_APPOINTMENT, QJsonDocument(response).toJson(QJsonDocument::Compact));
    }

    ConnectionPool::instance().closeConnection(db);
}

void AppointmentController::handleUpdateStatus(ClientHandler *client, const QJsonObject &body)
{
    int apptId = body["appt_id"].toInt();
    QString newStatus = body["status"].toString();

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QJsonObject response;

    if (!db.transaction()) {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Transaction start failed";
        client->sendPacket(Protocol::CMD_UPDATE_APPT_STATUS, QJsonDocument(response).toJson(QJsonDocument::Compact));
        ConnectionPool::instance().closeConnection(db);
        return;
    }

    try {
        QSqlQuery query(db);
        query.prepare("UPDATE appointments SET status = :status WHERE appt_id = :aid");
        query.bindValue(":status", newStatus);
        query.bindValue(":aid", apptId);
        
        if (!query.exec()) throw std::runtime_error("Update status failed");

        if (newStatus == "已完成") {
            query.prepare("SELECT member_id, pet_id, amount FROM appointments WHERE appt_id = :aid");
            query.bindValue(":aid", apptId);
            if (query.exec() && query.next()) {
                double amount = query.value("amount").toDouble();
                int memberId = query.value("member_id").toInt();
                int petId = query.value("pet_id").toInt();
                
                QSqlQuery orderQuery(db);
                QString orderNo = "ORD" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss") + QString::number(apptId);
                orderQuery.prepare("INSERT INTO orders (order_no, source_module, related_id, member_id, pet_id, total_amount, actual_pay, payment_method, status, operator_id) "
                                   "VALUES (:no, 'Appointment', :rel, :mid, :pid, :tot, 0, '未支付', '待支付', 1)");
                orderQuery.bindValue(":no", orderNo);
                orderQuery.bindValue(":rel", apptId);
                orderQuery.bindValue(":mid", memberId);
                orderQuery.bindValue(":pid", petId);
                orderQuery.bindValue(":tot", amount);
                orderQuery.exec();
            }
        }

        db.commit();
        response["status"] = Protocol::STATUS_OK;

        // 广播通知
        QJsonObject notify;
        notify["module"] = "appointment";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } catch (const std::exception &e) {
        db.rollback();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = e.what();
    }

    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_UPDATE_APPT_STATUS, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
