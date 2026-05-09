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
    // 注册指令 2002: 获取会员列表
    m_server->registerHandler(Protocol::CMD_GET_MEMBER_LIST, 
        std::bind(&MemberController::handleGetMemberList, this, std::placeholders::_1, std::placeholders::_2));
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
