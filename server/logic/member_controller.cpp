#include "member_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>

MemberController::MemberController(ServerCore *server, QObject *parent)
    : QObject(parent), m_server(server)
{
    m_server->registerHandler(Protocol::CMD_GET_MEMBER_LIST, std::bind(&MemberController::handleGetMemberList, this, std::placeholders::_1, std::placeholders::_2));
    m_server->registerHandler(Protocol::CMD_ADD_MEMBER, [this](ClientHandler* c, const QJsonObject& d) { handleAddMember(c, d); });
    m_server->registerHandler(Protocol::CMD_UPDATE_MEMBER, [this](ClientHandler* c, const QJsonObject& d) { handleUpdateMember(c, d); });
    m_server->registerHandler(Protocol::CMD_DELETE_MEMBER, [this](ClientHandler* c, const QJsonObject& d) { handleDeleteMember(c, d); });
}

void MemberController::handleAddMember(ClientHandler *client, const QJsonObject &data) {
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("INSERT INTO members (name, phone, gender, birthday, level_name, balance) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(data["name"].toString());
    query.addBindValue(data["phone"].toString());
    query.addBindValue(data["gender"].toString());
    query.addBindValue(data["birthday"].toString());
    query.addBindValue(data["level"].toString());
    query.addBindValue(data["balance"].toDouble());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["member_id"] = query.lastInsertId().toInt();
        // 广播
        QJsonObject notify;
        notify["module"] = "member";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_ADD_MEMBER, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void MemberController::handleUpdateMember(ClientHandler *client, const QJsonObject &data) {
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE members SET name=?, phone=?, gender=?, birthday=?, balance=?, level_name=? WHERE member_id=?");
    query.addBindValue(data["name"].toString());
    query.addBindValue(data["phone"].toString());
    query.addBindValue(data["gender"].toString());
    query.addBindValue(data["birthday"].toString());
    query.addBindValue(data["balance"].toDouble());
    query.addBindValue(data["level"].toString());
    query.addBindValue(data["member_id"].toInt());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        // 广播
        QJsonObject notify;
        notify["module"] = "member";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_UPDATE_MEMBER, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void MemberController::handleDeleteMember(ClientHandler *client, const QJsonObject &data) {
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE members SET is_deleted = 1 WHERE member_id = ?");
    query.addBindValue(data["member_id"].toInt());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        // 广播
        QJsonObject notify;
        notify["module"] = "member";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_DELETE_MEMBER, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void MemberController::handleGetMemberList(ClientHandler *client, const QJsonObject &data)
{
    Q_UNUSED(data);
    LOG_I("[MEMBER] Client requested member list.");

    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    QJsonObject response;
    QJsonArray memberList;

    // 查询所有未删除的会员
    if (query.exec("SELECT * FROM members WHERE is_deleted = 0 ORDER BY created_at DESC")) {
        while (query.next()) {
            QJsonObject m;
            QSqlRecord rec = query.record();
            
            int idIdx = rec.indexOf("member_id");
            int nameIdx = rec.indexOf("name");
            int genderIdx = rec.indexOf("gender");
            int birthdayIdx = rec.indexOf("birthday");
            int phoneIdx = rec.indexOf("phone");
            
            m["member_id"] = query.value(idIdx).toInt();
            m["name"] = query.value(nameIdx).toString();
            m["gender"] = query.value(genderIdx).toString();
            m["birthday"] = query.value(birthdayIdx).toString();
            m["phone"] = query.value(phoneIdx).toString();
            
            LOG_I("[MEMBER] ID:" << m["member_id"].toInt() << " Name:" << m["name"].toString() << " Gender:" << m["gender"].toString());
            
            m["balance"] = query.value("balance").toDouble();
            m["consume_amt"] = query.value("consume_amt").toDouble();
            m["points"] = query.value("points").toInt();
            m["level_name"] = query.value("level_name").toString();
            m["status"] = "正常"; // 目前默认返回正常，后续可扩展锁定等逻辑
            m["created_at"] = query.value("created_at").toString();
            
            memberList.append(m);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = memberList;
        LOG_I("[MEMBER] Successfully fetched " << memberList.size() << " members.");
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "Database error: " + query.lastError().text();
        LOG_E("[MEMBER] Database error: " << query.lastError().text());
    }

    client->sendPacket(Protocol::CMD_GET_MEMBER_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
