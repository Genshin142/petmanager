#include "service_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <glog/logging.h>

ServiceController::ServiceController(ServerCore* core, QObject* parent)
    : QObject(parent), m_core(core)
{
    m_core->registerHandler(Protocol::CMD_GET_SERVICE_LIST, std::bind(&ServiceController::handleGetServiceList, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_ADD_SERVICE, std::bind(&ServiceController::handleAddService, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_UPDATE_SERVICE, std::bind(&ServiceController::handleUpdateService, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_DELETE_SERVICE, std::bind(&ServiceController::handleDeleteService, this, std::placeholders::_1, std::placeholders::_2));
}

void ServiceController::handleGetServiceList(ClientHandler* client, const QJsonObject& /*data*/) {
    LOG(INFO) << "[SERVICE] Fetching service list";
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    // 简化查询，只取提成值
    query.prepare("SELECT * FROM services WHERE is_deleted = 0");
    
    QJsonArray serviceArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject svc;
            svc["id"] = query.value("service_id").toString();
            svc["name"] = query.value("name").toString();
            svc["category"] = query.value("category").toString();
            svc["price"] = query.value("price").toDouble();
            svc["durationMinutes"] = query.value("duration_min").toInt();
            svc["commissionFixed"] = query.value("commission_value").toDouble(); // 统一为固定金额
            svc["description"] = query.value("description").toString();
            svc["iconPath"] = query.value("icon_path").toString();
            svc["isActive"] = (query.value("status").toInt() == 1);
            serviceArray.append(svc);
        }
        
        QJsonObject response;
        response["status"] = Protocol::STATUS_OK;
        response["data"] = serviceArray;
        client->sendPacket(Protocol::CMD_GET_SERVICE_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    } else {
        LOG(ERROR) << "[SERVICE] Failed to fetch service list: " << query.lastError().text().toStdString();
        QJsonObject response;
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
        client->sendPacket(Protocol::CMD_GET_SERVICE_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
}

void ServiceController::handleAddService(ClientHandler* client, const QJsonObject& data) {
    LOG(INFO) << "[SERVICE] Adding new service: " << data["name"].toString().toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("INSERT INTO services (name, category, price, duration_min, commission_value, description, icon_path, status) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    
    query.addBindValue(data["name"].toString());
    query.addBindValue(data["category"].toString());
    query.addBindValue(data["price"].toDouble());
    query.addBindValue(data["durationMinutes"].toInt());
    query.addBindValue(data["commissionFixed"].toDouble());
    query.addBindValue(data["description"].toString());
    query.addBindValue(data["iconPath"].toString());
    query.addBindValue(data["isActive"].toBool() ? 1 : 0);

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Service added successfully";
        response["new_id"] = query.lastInsertId().toString();

        // 广播通知
        QJsonObject notify;
        notify["module"] = "service";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[SERVICE] Failed to add service: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_ADD_SERVICE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void ServiceController::handleUpdateService(ClientHandler* client, const QJsonObject& data) {
    LOG(INFO) << "[SERVICE] Updating service ID: " << data["id"].toString().toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE services SET name=?, category=?, price=?, duration_min=?, commission_value=?, description=?, icon_path=?, status=? WHERE service_id=?");
    
    query.addBindValue(data["name"].toString());
    query.addBindValue(data["category"].toString());
    query.addBindValue(data["price"].toDouble());
    query.addBindValue(data["durationMinutes"].toInt());
    query.addBindValue(data["commissionFixed"].toDouble());
    query.addBindValue(data["description"].toString());
    query.addBindValue(data["iconPath"].toString());
    query.addBindValue(data["isActive"].toBool() ? 1 : 0);
    query.addBindValue(data["id"].toString().toInt());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Service updated successfully";

        // 广播通知
        QJsonObject notify;
        notify["module"] = "service";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[SERVICE] Failed to update service: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_UPDATE_SERVICE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void ServiceController::handleDeleteService(ClientHandler* client, const QJsonObject& data) {
    LOG(INFO) << "[SERVICE] Deleting service ID: " << data["id"].toString().toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE services SET is_deleted = 1 WHERE service_id = ?");
    query.addBindValue(data["id"].toString().toInt());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Service deleted successfully";

        // 广播通知
        QJsonObject notify;
        notify["module"] = "service";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[SERVICE] Failed to delete service: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_DELETE_SERVICE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
