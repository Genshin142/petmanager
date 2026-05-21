#include "appointment_controller.h"
#include "../database/connectionpool.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QVariant>
#include <QUuid>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include "../logger_compat.h"

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
    query.prepare("SELECT a.*, p.pet_name, p.breed as pet_breed, p.avatar_path as pet_avatar, "
                  "m.name as member_name, m.phone as member_phone, "
                  "s.name as service_name, e.real_name as staff_name "
                  "FROM appointments a "
                  "LEFT JOIN pets p ON a.pet_id = p.pet_id "
                  "LEFT JOIN members m ON a.member_id = m.member_id "
                  "LEFT JOIN services s ON a.service_id = s.service_id "
                  "LEFT JOIN sys_employees e ON a.staff_id = e.emp_id "
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
            item["notes"] = query.value("notes").toString();
            item["group_id"] = query.value("group_id").toString();
            item["pet_name"] = query.value("pet_name").toString();
            item["pet_breed"] = query.value("pet_breed").toString();
            item["pet_avatar"] = query.value("pet_avatar").toString();
            item["member_name"] = query.value("member_name").toString();
            item["member_phone"] = query.value("member_phone").toString();
            item["service_name"] = query.value("service_name").toString();
            item["staff_name"] = query.value("staff_name").toString();
            item["room_no"] = query.value("room_no").toString();
            item["boarding_end_date"] = query.value("boarding_end_date").toDate().toString("yyyy-MM-dd");
            item["duration"] = query.value("duration").toInt();
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

    QSqlQuery query(db);
    QString groupId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    query.prepare("INSERT INTO appointments (group_id, member_id, pet_id, service_id, staff_id, appt_time, appt_type, notes, address, amount, room_no, boarding_end_date, duration) "
                  "VALUES (:group, :mid, :pid, :sid, :staff, :time, :type, :notes, :addr, :amt, :rno, :bend, :dur)");
    query.bindValue(":group", groupId);
    query.bindValue(":mid", body["member_id"].toInt());
    query.bindValue(":pid", body["pet_id"].toInt());
    query.bindValue(":sid", body["service_id"].toInt());
    
    int staffId = body["staff_id"].toInt();
    if (staffId > 0) {
        query.bindValue(":staff", staffId);
    } else {
        query.bindValue(":staff", QVariant(QMetaType::fromType<int>()));
    }
    query.bindValue(":time", body["appt_time"].toString());
    query.bindValue(":type", body["appt_type"].toString());
    query.bindValue(":notes", body["notes"].toString());
    query.bindValue(":addr", body["address"].toString());
    query.bindValue(":amt", body["amount"].toDouble());
    query.bindValue(":rno", body["room_no"].toString());
    
    if (body.contains("boarding_end_date") && !body["boarding_end_date"].toString().isEmpty()) {
        query.bindValue(":bend", body["boarding_end_date"].toString());
    } else {
        query.bindValue(":bend", QVariant(QMetaType::fromType<QDate>()));
    }
    query.bindValue(":dur", body["duration"].toInt());

    if (!query.exec()) {
        qDebug() << "[ERROR] [APPOINTMENT] INSERT failed:" << query.lastError().text();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Insert appointment failed: " + query.lastError().text();
        client->sendPacket(Protocol::CMD_ADD_APPOINTMENT, QJsonDocument(response).toJson(QJsonDocument::Compact));
        ConnectionPool::instance().closeConnection(db);
        return;
    }
    
    int newApptId = query.lastInsertId().toInt();
    qDebug() << "[INFO] [APPOINTMENT] Successfully inserted appointment, ID:" << newApptId;

    // 寄养处理
    QString roomNo = body["room_no"].toString();
    if (body["appt_type"].toString() == "Boarding" && !roomNo.isEmpty()) {
        QSqlQuery roomQuery(db);
        roomQuery.prepare("UPDATE boarding_rooms SET status = '已预订' WHERE room_no = :rno AND status = '空闲'");
        roomQuery.bindValue(":rno", roomNo);
        if (!roomQuery.exec()) {
            qDebug() << "[ERROR] [APPOINTMENT] Update room status failed:" << roomQuery.lastError().text();
        }
    }

    QJsonObject res;
    res["status"] = Protocol::STATUS_OK;
    res["appt_id"] = newApptId;
    client->sendPacket(Protocol::CMD_ADD_APPOINTMENT, QJsonDocument(res).toJson(QJsonDocument::Compact));

    // 广播通知
    QJsonObject notify;
    notify["module"] = "appointment";
    m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);

    ConnectionPool::instance().closeConnection(db);
}

void AppointmentController::handleUpdateStatus(ClientHandler *client, const QJsonObject &body)
{
    int apptId = body["appt_id"].toInt();
    QString newStatus = body["status"].toString();
    QString staffName = body["staff"].toString(); // 👈 获取服务执行人姓名

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QJsonObject response;

    QSqlQuery query(db);

    // 👈 智能查询员工对应的 emp_id 
    QVariant staffId = QVariant(QMetaType::fromType<int>());
    if (!staffName.isEmpty()) {
        QSqlQuery empQuery(db);
        empQuery.prepare("SELECT emp_id FROM sys_employees WHERE real_name = ? AND is_deleted = 0");
        empQuery.addBindValue(staffName);
        if (empQuery.exec() && empQuery.next()) {
            staffId = empQuery.value("emp_id").toInt();
        }
    }

    // 👈 若传入了执行人，一并更新 appointments 表的 staff_id
    if (!staffName.isEmpty() && staffId.isValid() && !staffId.isNull()) {
        query.prepare("UPDATE appointments SET status = ?, staff_id = ? WHERE appt_id = ?");
        query.addBindValue(newStatus);
        query.addBindValue(staffId);
        query.addBindValue(apptId);
    } else {
        query.prepare("UPDATE appointments SET status = ? WHERE appt_id = ?");
        query.addBindValue(newStatus);
        query.addBindValue(apptId);
    }
    
    if (!query.exec()) {
        qDebug() << "[ERROR] [APPOINTMENT] Update status failed:" << query.lastError().text();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Update status failed: " + query.lastError().text();
        ConnectionPool::instance().closeConnection(db);
        client->sendPacket(Protocol::CMD_UPDATE_APPT_STATUS, QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

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
            if (!orderQuery.exec()) {
                qDebug() << "[ERROR] [APPOINTMENT] Insert order failed:" << orderQuery.lastError().text();
            }
        }
    }

    response["status"] = Protocol::STATUS_OK;

    // 广播通知
    QJsonObject notify;
    notify["module"] = "appointment";
    m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);

    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_UPDATE_APPT_STATUS, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
