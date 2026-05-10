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
}

void PetController::handleGetPetList(ClientHandler* client, const QJsonObject& data) {
    LOG_I("[PET] Fetching pet list...");

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // 联表查询：获取宠物信息及主人姓名
    query.prepare("SELECT p.*, m.name as owner_name FROM pets p "
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
