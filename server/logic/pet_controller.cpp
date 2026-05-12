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
    m_core->registerHandler(Protocol::CMD_GET_VACCINES, [this](ClientHandler* client, const QJsonObject& data) {
        handleGetVaccines(client, data);
    });
    m_core->registerHandler(Protocol::CMD_UPDATE_VACCINES, [this](ClientHandler* client, const QJsonObject& data) {
        handleUpdateVaccines(client, data);
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

void PetController::handleGetVaccines(ClientHandler* client, const QJsonObject& data) {
    int petId = data["pet_id"].toInt();
    LOG_I("[VACCINE] Fetching vaccine records for pet_id: " << petId);

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT vaccine_type, DATE_FORMAT(vaccine_date, '%Y-%m-%d') as v_date, "
                  "DATE_FORMAT(expiry_date, '%Y-%m-%d') as e_date "
                  "FROM pet_vaccine_records WHERE pet_id = ? ORDER BY vaccine_date DESC");
    query.addBindValue(petId);

    QJsonObject response;
    QJsonArray records;

    if (query.exec()) {
        while (query.next()) {
            QJsonObject item;
            item["type"] = query.value("vaccine_type").toString();
            item["date"] = query.value("v_date").toString();
            item["expiry"] = query.value("e_date").toString();
            records.append(item);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = records;
        response["pet_id"] = petId;
        LOG_I("[VACCINE] Pet ID " << petId << " found " << records.size() << " records.");
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
        LOG_E("[VACCINE] Database error for pet_id " << petId << ": " << query.lastError().text().toStdString());
    }

    client->sendPacket(Protocol::CMD_GET_VACCINES, QJsonDocument(response).toJson(QJsonDocument::Compact));
}


void PetController::handleUpdateVaccines(ClientHandler* client, const QJsonObject& data) {
    int petId = data["pet_id"].toInt();
    QJsonArray records = data["records"].toArray();
    LOG_I("[VACCINE] Updating vaccine records for pet_id: " << petId << ", count: " << records.size());

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    db.transaction();
    QSqlQuery query(db);

    // 1. 先删除旧记录
    query.prepare("DELETE FROM pet_vaccine_records WHERE pet_id = ?");
    query.addBindValue(petId);
    
    if (!query.exec()) {
        db.rollback();
        QJsonObject res; res["status"] = Protocol::STATUS_ERROR; res["message"] = query.lastError().text();
        client->sendPacket(Protocol::CMD_UPDATE_VACCINES, QJsonDocument(res).toJson(QJsonDocument::Compact));
        return;
    }

    // 2. 插入新记录
    for (int i = 0; i < records.size(); ++i) {
        QJsonObject obj = records[i].toObject();
        query.prepare("INSERT INTO pet_vaccine_records (pet_id, vaccine_type, vaccine_date, expiry_date) VALUES (?, ?, ?, ?)");
        query.addBindValue(petId);
        query.addBindValue(obj["type"].toString());
        query.addBindValue(obj["date"].toString());
        query.addBindValue(obj["expiry"].toString());
        if (!query.exec()) {
            db.rollback();
            QJsonObject res; res["status"] = Protocol::STATUS_ERROR; res["message"] = query.lastError().text();
            client->sendPacket(Protocol::CMD_UPDATE_VACCINES, QJsonDocument(res).toJson(QJsonDocument::Compact));
            return;
        }
    }

    db.commit();
    QJsonObject response;
    response["status"] = Protocol::STATUS_OK;
    response["pet_id"] = petId;
    client->sendPacket(Protocol::CMD_UPDATE_VACCINES, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
