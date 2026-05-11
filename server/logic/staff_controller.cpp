#include "staff_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <glog/logging.h>

StaffController::StaffController(ServerCore* core, QObject* parent)
    : QObject(parent), m_core(core)
{
    m_core->registerHandler(Protocol::CMD_GET_STAFF_LIST, std::bind(&StaffController::handleGetStaffList, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_ADD_STAFF, std::bind(&StaffController::handleAddStaff, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_UPDATE_STAFF, std::bind(&StaffController::handleUpdateStaff, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_DELETE_STAFF, std::bind(&StaffController::handleDeleteStaff, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_RESTORE_STAFF, std::bind(&StaffController::handleRestoreStaff, this, std::placeholders::_1, std::placeholders::_2));
}

void StaffController::handleGetStaffList(ClientHandler* client, const QJsonObject& data) {
    LOG(INFO) << "[STAFF] Fetching staff list";
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM sys_employees WHERE is_deleted = 0");
    
    QJsonArray staffArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject staff;
            staff["id"] = query.value("emp_id").toString();
            staff["name"] = query.value("real_name").toString();
            staff["role"] = query.value("role").toString();
            staff["gender"] = query.value("gender").toString();
            staff["age"] = query.value("age").toInt();
            staff["phone"] = query.value("phone").toString();
            staff["email"] = query.value("email").toString();
            staff["idCard"] = query.value("id_card").toString();
            staff["baseSalary"] = query.value("base_salary").toInt();
            staff["status"] = query.value("status").toString();
            staff["imgPath"] = query.value("img_url").toString();
            staff["joinDate"] = query.value("join_date").toDate().toString("yyyy-MM-dd");
            staff["emergencyContact"] = query.value("emergency_contact").toString();
            staff["emergencyPhone"] = query.value("emergency_phone").toString();
            staff["address"] = query.value("address").toString();
            staff["education"] = query.value("education").toString();
            staff["department"] = query.value("department").toString();
            staff["username"] = query.value("username").toString();
            // password usually not sent back for security
            staffArray.append(staff);
        }
        
        QJsonObject response;
        response["status"] = Protocol::STATUS_OK;
        response["data"] = staffArray;
        client->sendPacket(Protocol::CMD_GET_STAFF_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    } else {
        LOG(ERROR) << "[STAFF] Failed to fetch staff list: " << query.lastError().text().toStdString();
        QJsonObject response;
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
        client->sendPacket(Protocol::CMD_GET_STAFF_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
}

void StaffController::handleAddStaff(ClientHandler* client, const QJsonObject& data) {
    LOG(INFO) << "[STAFF] Adding new staff: " << data["name"].toString().toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("INSERT INTO sys_employees (username, password, real_name, role, department, gender, age, phone, email, id_card, base_salary, join_date, emergency_contact, emergency_phone, address, education, status, img_url) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
    query.addBindValue(data["username"].toString());
    query.addBindValue(data["password"].toString());
    query.addBindValue(data["name"].toString());
    query.addBindValue(data["role"].toString());
    query.addBindValue(data["department"].toString());
    query.addBindValue(data["gender"].toString());
    query.addBindValue(data["age"].toInt());
    query.addBindValue(data["phone"].toString());
    query.addBindValue(data["email"].toString());
    query.addBindValue(data["idCard"].toString());
    query.addBindValue(data["baseSalary"].toInt());
    query.addBindValue(data["joinDate"].toString());
    query.addBindValue(data["emergencyContact"].toString());
    query.addBindValue(data["emergencyPhone"].toString());
    query.addBindValue(data["address"].toString());
    query.addBindValue(data["education"].toString());
    query.addBindValue(data["status"].toString());
    query.addBindValue(data["imgPath"].toString());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Staff added successfully";
        response["new_id"] = query.lastInsertId().toString();

        // 广播通知
        QJsonObject notify;
        notify["module"] = "staff";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[STAFF] Failed to add staff: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_ADD_STAFF, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void StaffController::handleUpdateStaff(ClientHandler* client, const QJsonObject& data) {
    LOG(INFO) << "[STAFF] Updating staff ID: " << data["id"].toString().toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    QString sql = "UPDATE sys_employees SET real_name=?, role=?, department=?, gender=?, age=?, phone=?, email=?, id_card=?, base_salary=?, join_date=?, emergency_contact=?, emergency_phone=?, address=?, education=?, status=?, img_url=?, username=?";
    
    if (data.contains("password") && !data["password"].toString().isEmpty()) {
        sql += ", password=?";
    }
    sql += " WHERE emp_id=?";
    
    query.prepare(sql);
    query.addBindValue(data["name"].toString());
    query.addBindValue(data["role"].toString());
    query.addBindValue(data["department"].toString());
    query.addBindValue(data["gender"].toString());
    query.addBindValue(data["age"].toInt());
    query.addBindValue(data["phone"].toString());
    query.addBindValue(data["email"].toString());
    query.addBindValue(data["idCard"].toString());
    query.addBindValue(data["baseSalary"].toInt());
    query.addBindValue(data["joinDate"].toString());
    query.addBindValue(data["emergencyContact"].toString());
    query.addBindValue(data["emergencyPhone"].toString());
    query.addBindValue(data["address"].toString());
    query.addBindValue(data["education"].toString());
    query.addBindValue(data["status"].toString());
    query.addBindValue(data["imgPath"].toString());
    query.addBindValue(data["username"].toString());
    
    if (data.contains("password") && !data["password"].toString().isEmpty()) {
        query.addBindValue(data["password"].toString());
    }
    query.addBindValue(data["id"].toString().toInt());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Staff updated successfully";

        // 广播通知
        QJsonObject notify;
        notify["module"] = "staff";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[STAFF] Failed to update staff: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_UPDATE_STAFF, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void StaffController::handleDeleteStaff(ClientHandler* client, const QJsonObject& data) {
    LOG(INFO) << "[STAFF] Deleting staff ID: " << data["id"].toString().toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE sys_employees SET is_deleted = 1, status = '离职' WHERE emp_id = ?");
    query.addBindValue(data["id"].toString().toInt());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Staff deleted successfully";

        // 广播通知
        QJsonObject notify;
        notify["module"] = "staff";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[STAFF] Failed to delete staff: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_DELETE_STAFF, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void StaffController::handleRestoreStaff(ClientHandler* client, const QJsonObject& data) {
    LOG(INFO) << "[STAFF] Restoring staff ID: " << data["id"].toString().toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE sys_employees SET is_deleted = 0, status = '在职' WHERE emp_id = ?");
    query.addBindValue(data["id"].toString().toInt());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Staff restored successfully";
    } else {
        LOG(ERROR) << "[STAFF] Failed to restore staff: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_RESTORE_STAFF, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
