#include "pet_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QtSql/QSqlDriver>
#include <QtCore/QThread>

PetController::PetController(ServerCore* core, QObject* parent) 
    : QObject(parent), m_core(core) 
{
    // 注册指令：2001 -> 获取宠物列表
    m_core->registerHandler(Protocol::CMD_GET_PET_LIST, std::bind(&PetController::handleGetPetList, this, std::placeholders::_1, std::placeholders::_2));
    // 注册指令：2007 -> 获取房间列表
    m_core->registerHandler(Protocol::CMD_GET_ROOM_LIST, std::bind(&PetController::handleGetRoomList, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_ADD_PET, [this](ClientHandler* client, const QJsonObject& data) {
        handleAddPet(client, data);
    });
    m_core->registerHandler(Protocol::CMD_UPDATE_PET, [this](ClientHandler* client, const QJsonObject& data) {
        handleUpdatePet(client, data);
    });
    m_core->registerHandler(Protocol::CMD_GET_VACCINES, [this](ClientHandler* client, const QJsonObject& data) {
        handleGetVaccines(client, data);
    });
    m_core->registerHandler(Protocol::CMD_UPDATE_VACCINES, [this](ClientHandler* client, const QJsonObject& data) {
        handleUpdateVaccines(client, data);
    });
    m_core->registerHandler(Protocol::CMD_UPDATE_PET_STATUS, [this](ClientHandler* client, const QJsonObject& data) {
        handleUpdatePetStatus(client, data);
    });
    m_core->registerHandler(Protocol::CMD_DELETE_PET, [this](ClientHandler* client, const QJsonObject& data) {
        handleDeletePet(client, data);
    });
    m_core->registerHandler(Protocol::CMD_RESTORE_PET, [this](ClientHandler* client, const QJsonObject& data) {
        handleRestorePet(client, data);
    });
    m_core->registerHandler(Protocol::CMD_HARD_DELETE_PET, [this](ClientHandler* client, const QJsonObject& data) {
        handleHardDeletePet(client, data);
    });
}

void PetController::handleUpdatePet(ClientHandler* client, const QJsonObject& data) {
    int petId = 0;
    QString rawId = data["pet_id"].toString();
    if (rawId.startsWith("P")) petId = rawId.mid(1).toInt();
    else if (data["pet_id"].isDouble()) petId = data["pet_id"].toInt();
    else petId = rawId.toInt();

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

void PetController::handleAddPet(ClientHandler* client, const QJsonObject& data) {
    LOG_I("[PET] Adding new pet: " << data["pet_name"].toString().toStdString());

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // member_id 需要根据传上来的 owner_id 解析 (比如 M00001 -> 1)
    int memberId = 0;
    QString rawOwnerId = data["owner_id"].toString();
    if (rawOwnerId.startsWith("M")) memberId = rawOwnerId.mid(1).toInt();
    else memberId = rawOwnerId.toInt();

    query.prepare("INSERT INTO pets (member_id, pet_name, species, breed, gender, age_months, weight, avatar_path, "
                  "health_status, medical_history, dietary_habit, current_status, is_deleted) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0)");
    
    query.addBindValue(memberId);
    query.addBindValue(data["pet_name"].toString());
    query.addBindValue(data["species"].toString());
    query.addBindValue(data["breed"].toString());
    query.addBindValue(data["gender"].toString());
    query.addBindValue(data["age_months"].toInt());
    query.addBindValue(data["weight"].toDouble());
    query.addBindValue(data["avatar_path"].toString());
    query.addBindValue(data["health_status"].toString());
    query.addBindValue(data["medical_history"].toString());
    query.addBindValue(data["dietary_habit"].toString());
    query.addBindValue(data["status"].toString().isEmpty() ? "在家" : data["status"].toString());

    QJsonObject response;
    if (query.exec()) {
        int newId = query.lastInsertId().toInt();
        QString newPetIdStr = QString("P%1").arg(newId, 5, 10, QChar('0'));
        response["status"] = Protocol::STATUS_OK;
        response["pet_id"] = newPetIdStr;
        
        QJsonObject notify;
        notify["module"] = "pet";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_ADD_PET, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void PetController::handleGetPetList(ClientHandler* client, const QJsonObject& data) {
    LOG_I("[PET] Fetching pet list...");

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    query.prepare("SELECT p.*, m.name as owner_name, m.phone as owner_phone, m.gender as owner_gender, "
                  "br.room_no, br.check_in_time, br.expected_check_out_time, br.status as boarding_status "
                  "FROM pets p "
                  "LEFT JOIN members m ON p.member_id = m.member_id "
                  "LEFT JOIN boarding_records br ON p.pet_id = br.pet_id AND (br.status = '入店中' OR br.status = '预约中') "
                  "ORDER BY p.pet_id DESC");

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
    int petId = 0;
    QString rawId = data["pet_id"].toString();
    if (rawId.startsWith("P")) petId = rawId.mid(1).toInt();
    else if (data["pet_id"].isDouble()) petId = data["pet_id"].toInt();
    else petId = rawId.toInt();

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
    int petId = 0;
    QString rawId = data["pet_id"].toString();
    if (rawId.startsWith("P")) petId = rawId.mid(1).toInt();
    else if (data["pet_id"].isDouble()) petId = data["pet_id"].toInt();
    else petId = rawId.toInt();

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

void PetController::handleUpdatePetStatus(ClientHandler* client, const QJsonObject& data) {
    QString now = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    int petId = 0;
    QString rawId = data["pet_id"].toString();
    if (rawId.startsWith("P")) petId = rawId.mid(1).toInt();
    else if (data["pet_id"].isDouble()) petId = data["pet_id"].toInt();
    else petId = rawId.toInt();
    
    QString status = data["status"].toString();
    QString roomNo = data["room_no"].toString();
    QString startTime = data["start_time"].toString();
    QString endTime = data["end_time"].toString();
    
    LOG(INFO) << "[PET] Updating status for Pet ID:" << petId << " Status:" << status.toStdString() << " Room:" << roomNo.toStdString();

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    if (!db.isOpen()) {
        LOG(ERROR) << "[PET] Database is NOT open for thread:" << QThread::currentThreadId();
        QJsonObject resp;
        resp["status"] = Protocol::STATUS_ERROR;
        resp["message"] = "Database connection lost";
        client->sendPacket(Protocol::CMD_UPDATE_PET_STATUS, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    bool useTransaction = db.driver()->hasFeature(QSqlDriver::Transactions);
    if (useTransaction) {
        if (!db.transaction()) {
            QString err = db.lastError().text();
            LOG(ERROR) << "[PET] Failed to start transaction: " << err.toStdString();
            QJsonObject resp;
            resp["status"] = Protocol::STATUS_ERROR;
            resp["message"] = "Failed to start transaction: " + err;
            client->sendPacket(Protocol::CMD_UPDATE_PET_STATUS, QJsonDocument(resp).toJson(QJsonDocument::Compact));
            return;
        }
    } else {
        LOG(WARNING) << "[PET] Database driver does NOT support transactions. Proceeding with auto-commit.";
    }

    QSqlQuery query(db);
    
    // 0. 先获取该宠物的 member_id (为了插入记录)
    int memberId = 0;
    query.prepare("SELECT member_id FROM pets WHERE pet_id = ?");
    query.addBindValue(petId);
    if (query.exec() && query.next()) {
        memberId = query.value(0).toInt();
    }

    // 1. 更新宠物表的基本状态
    query.prepare("UPDATE pets SET current_status = ? WHERE pet_id = ?");
    query.addBindValue(status);
    query.addBindValue(petId);
    if (!query.exec()) {
        QString err = query.lastError().text();
        if (useTransaction) db.rollback();
        LOG(ERROR) << "[PET] SQL Error (Update Pets): " << err.toStdString();
        QJsonObject resp;
        resp["status"] = Protocol::STATUS_ERROR;
        resp["message"] = "Update pets failed: " + err;
        client->sendPacket(Protocol::CMD_UPDATE_PET_STATUS, QJsonDocument(resp).toJson(QJsonDocument::Compact));
        return;
    }

    // 2. 更新或插入寄养记录表
    if (status == QString::fromUtf8("寄养中") || status == QString::fromUtf8("已预约")) {
        QString dbStatus = (status == QString::fromUtf8("寄养中") ? QString::fromUtf8("入店中") : QString::fromUtf8("预约中"));
        
        // A. 更新记录表
        query.prepare("UPDATE boarding_records SET status = ?, room_no = ?, check_in_time = ?, expected_check_out_time = ? "
                      "WHERE pet_id = ? AND (status = '入店中' OR status = '预约中')");
        query.addBindValue(dbStatus);
        query.addBindValue(roomNo);
        query.addBindValue(startTime.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : startTime);
        query.addBindValue(endTime.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : endTime);
        query.addBindValue(petId);
        
        if (!query.exec() || query.numRowsAffected() == 0) {
            query.prepare("INSERT INTO boarding_records (pet_id, member_id, room_no, status, check_in_time, expected_check_out_time) "
                          "VALUES (?, ?, ?, ?, ?, ?)");
            query.addBindValue(petId);
            query.addBindValue(memberId > 0 ? QVariant(memberId) : QVariant(QMetaType::fromType<int>()));
            query.addBindValue(roomNo);
            query.addBindValue(dbStatus);
            query.addBindValue(startTime.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : startTime);
            query.addBindValue(endTime.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : endTime);
            if (!query.exec()) {
                QString err = query.lastError().text();
                if (useTransaction) db.rollback();
                LOG(ERROR) << "[PET] SQL Error (Insert Boarding): " << err.toStdString();
                QJsonObject resp; resp["status"] = Protocol::STATUS_ERROR; resp["message"] = "Insert record failed: " + err;
                client->sendPacket(Protocol::CMD_UPDATE_PET_STATUS, QJsonDocument(resp).toJson(QJsonDocument::Compact));
                return;
            }
        }

        // B. 同步更新房间表：将宠物 ID 绑定到对应房间
        if (!roomNo.isEmpty()) {
            query.prepare("UPDATE boarding_rooms SET current_pet_id = ?, status = ? WHERE room_no = ?");
            query.addBindValue(petId);
            query.addBindValue(dbStatus);
            query.addBindValue(roomNo);
            if (!query.exec()) {
                LOG(WARNING) << "[PET] Failed to link pet to room: " << query.lastError().text().toStdString();
            }
        }
    } else if (status == QString::fromUtf8("在家") || status == QString::fromUtf8("待寄养")) {
        // A. 结清记录表
        query.prepare("UPDATE boarding_records SET status = '结清', actual_check_out_time = NOW() "
                      "WHERE pet_id = ? AND (status = '入店中' OR status = '预约中')");
        query.addBindValue(petId);
        query.exec();

        // B. 同步更新房间表：释放该宠物占用的所有房间
        query.prepare("UPDATE boarding_rooms SET current_pet_id = NULL, status = '空闲' WHERE current_pet_id = ?");
        query.addBindValue(petId);
        if (!query.exec()) {
            LOG(WARNING) << "[PET] Failed to release room: " << query.lastError().text().toStdString();
        }
    }

    QJsonObject response;
    bool success = true;
    if (useTransaction) {
        if (!db.commit()) {
            QString err = db.lastError().text();
            db.rollback();
            response["status"] = Protocol::STATUS_ERROR;
            response["message"] = "Commit failed: " + err;
            LOG(ERROR) << "[PET] Commit failed: " << err.toStdString();
            success = false;
        }
    }

    if (success) {
        response["status"] = Protocol::STATUS_OK;
        LOG(INFO) << "[PET] Status update successful for pet:" << petId;
        
        // 成功后广播通知全网刷新
        QJsonObject notify;
        notify["module"] = "pet";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    }
    client->sendPacket(Protocol::CMD_UPDATE_PET_STATUS, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void PetController::handleDeletePet(ClientHandler* client, const QJsonObject& data) {
    int petId = data["pet_id"].toInt();
    LOG_I("[PET] Soft deleting pet ID: " << petId);

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    query.prepare("UPDATE pets SET is_deleted = 1, current_status = '已注销' WHERE pet_id = :id");
    query.bindValue(":id", petId);
    
    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, QJsonObject{{"module", "pet"}});
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
        LOG_E("[PET] Soft delete failed: " << query.lastError().text().toStdString());
    }
    
    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_DELETE_PET, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void PetController::handleRestorePet(ClientHandler* client, const QJsonObject& data) {
    int petId = data["pet_id"].toInt();
    LOG_I("[PET] Restoring pet ID: " << petId);

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    query.prepare("UPDATE pets SET is_deleted = 0, current_status = '在家' WHERE pet_id = :id");
    query.bindValue(":id", petId);
    
    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, QJsonObject{{"module", "pet"}});
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
        LOG_E("[PET] Restore failed: " << query.lastError().text().toStdString());
    }
    
    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_RESTORE_PET, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void PetController::handleHardDeletePet(ClientHandler* client, const QJsonObject& data) {
    int petId = data["pet_id"].toInt();
    LOG_I("[PET] Hard deleting pet ID: " << petId);

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // Begin transaction for safe cascade deletions
    db.transaction();
    bool ok = true;
    
    query.prepare("DELETE FROM pet_activity_logs WHERE pet_id = :id");
    query.bindValue(":id", petId);
    if (!query.exec()) ok = false;
    
    if (ok) {
        query.prepare("DELETE FROM pet_vaccine_records WHERE pet_id = :id");
        query.bindValue(":id", petId);
        if (!query.exec()) ok = false;
    }
    
    if (ok) {
        query.prepare("DELETE FROM pets WHERE pet_id = :id");
        query.bindValue(":id", petId);
        if (!query.exec()) ok = false;
    }
    
    QJsonObject response;
    if (ok && db.commit()) {
        response["status"] = Protocol::STATUS_OK;
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, QJsonObject{{"module", "pet"}});
    } else {
        db.rollback();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
        LOG_E("[PET] Hard delete failed: " << query.lastError().text().toStdString());
    }
    
    ConnectionPool::instance().closeConnection(db);
    client->sendPacket(Protocol::CMD_HARD_DELETE_PET, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
