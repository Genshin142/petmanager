#include "pet_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>

PetController::PetController(ServerCore* core, QObject* parent) 
    : QObject(parent), m_core(core) 
{
    // 注册指令：2001 -> 获取宠物列表
    m_core->registerHandler(Protocol::CMD_GET_PET_LIST, std::bind(&PetController::handleGetPetList, this, std::placeholders::_1, std::placeholders::_2));
    // 注册指令：2007 -> 获取房间列表
    m_core->registerHandler(Protocol::CMD_GET_ROOM_LIST, std::bind(&PetController::handleGetRoomList, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_UPDATE_PET, [this](ClientHandler* client, const QJsonObject& data) {
        handleUpdatePet(client, data);
    });
}

void PetController::handleUpdatePet(ClientHandler* client, const QJsonObject& data) {
    int petId = data["pet_id"].toInt();
    LOG_I("[PET] Updating pet ID: " << petId);

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE pets SET pet_name = ?, weight = ?, current_status = ? WHERE pet_id = ?");
    query.addBindValue(data["pet_name"].toString());
    query.addBindValue(data["weight"].toDouble());
    query.addBindValue(data["status"].toString());
    query.addBindValue(petId);

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        // 成功后广播通知全网刷新宠物模块
        QJsonObject notify;
        notify["module"] = "pet";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_UPDATE_PET, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void PetController::handleGetPetList(ClientHandler* client, const QJsonObject& data) {
    LOG_I("[PET] Fetching pet list...");

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // 联表查询：获取宠物信息及主人姓名、电话、性别
    query.prepare("SELECT p.*, m.name as owner_name, m.phone as owner_phone, m.gender as owner_gender "
                  "FROM pets p "
                  "LEFT JOIN members m ON p.member_id = m.member_id "
                  "WHERE p.is_deleted = 0 ORDER BY p.pet_id DESC");

    QJsonObject response;
    QJsonArray petList;

    if (query.exec()) {
        while (query.next()) {
            QJsonObject pet;
            QSqlRecord rec = query.record();
            for (int i = 0; i < rec.count(); ++i) {
                QString colName = rec.fieldName(i);
                QVariant val = query.value(i);
                
                // 根据类型转换
                if (val.typeId() == QMetaType::Int || val.typeId() == QMetaType::LongLong)
                    pet[colName] = val.toLongLong();
                else if (val.typeId() == QMetaType::Double)
                    pet[colName] = val.toDouble();
                else
                    pet[colName] = val.toString();
            }
            petList.append(pet);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = petList;
        LOG_I("[PET] Successfully fetched " << petList.size() << " pets.");
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Database error: " + query.lastError().text();
        LOG_E("[PET] Database error: " << query.lastError().text().toStdString());
    }

    client->sendPacket(Protocol::CMD_GET_PET_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void PetController::handleGetRoomList(ClientHandler* client, const QJsonObject& data) {
    LOG_I("[ROOM] Fetching room list...");
    Q_UNUSED(data);

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    query.prepare("SELECT * FROM boarding_rooms WHERE is_deleted = 0 ORDER BY room_no ASC");

    QJsonObject response;
    QJsonArray roomList;

    if (query.exec()) {
        while (query.next()) {
            QJsonObject room;
            QSqlRecord rec = query.record();
            for (int i = 0; i < rec.count(); ++i) {
                QString colName = rec.fieldName(i);
                QVariant val = query.value(i);
                if (val.typeId() == QMetaType::Int || val.typeId() == QMetaType::LongLong)
                    room[colName] = val.toLongLong();
                else if (val.typeId() == QMetaType::Double)
                    room[colName] = val.toDouble();
                else
                    room[colName] = val.toString();
            }
            roomList.append(room);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = roomList;
        LOG_I("[ROOM] Successfully fetched " << roomList.size() << " rooms.");
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Database error: " + query.lastError().text();
        LOG_E("[ROOM] Database error: " << query.lastError().text().toStdString());
    }

    client->sendPacket(Protocol::CMD_GET_ROOM_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
