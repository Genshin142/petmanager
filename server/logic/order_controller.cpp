#include "order_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QDateTime>
#include <QJsonDocument>
#include "../logger_compat.h"

OrderController::OrderController(ServerCore *server, QObject *parent)
    : QObject(parent), m_server(server)
{
    m_server->registerHandler(Protocol::CMD_GET_ORDER_LIST, std::bind(&OrderController::handleGetOrderList, this, std::placeholders::_1, std::placeholders::_2));
    m_server->registerHandler(Protocol::CMD_CREATE_ORDER, std::bind(&OrderController::handleCreateOrder, this, std::placeholders::_1, std::placeholders::_2));
    m_server->registerHandler(Protocol::CMD_UPDATE_ORDER, std::bind(&OrderController::handleUpdateOrder, this, std::placeholders::_1, std::placeholders::_2));
    m_server->registerHandler(Protocol::CMD_CANCEL_ORDER, std::bind(&OrderController::handleCancelOrder, this, std::placeholders::_1, std::placeholders::_2));
}

void OrderController::handleCreateOrder(ClientHandler *client, const QJsonObject &data)
{
    LOG_I("[ORDER] Received create order request: " << data["id"].toString().toStdString());
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QJsonObject response;

    if (!db.isOpen()) {
        if (!db.open()) {
            response["status"] = Protocol::STATUS_ERROR;
            response["message"] = "DB Open Error: " + db.lastError().text();
            client->sendPacket(Protocol::CMD_CREATE_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
            return;
        }
    }

    // 直接执行插入，跳过 transaction() 环节
    QSqlQuery query(db);
    query.prepare("INSERT INTO orders (order_no, source_module, related_id, member_id, operator_id, total_amount, discount, actual_pay, item_details, payment_method, status, created_at) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
    query.addBindValue(data["id"].toString()); 
    query.addBindValue(data["sourceModule"].toString());
    query.addBindValue(data["relatedId"].toString());
    
    QString mIdStr = data["memberId"].toString();
    int memberId = 0;
    if (mIdStr.startsWith('M')) memberId = mIdStr.mid(1).toInt();
    else memberId = mIdStr.toInt();
    query.addBindValue(memberId > 0 ? QVariant(memberId) : QVariant(QMetaType::fromType<int>()));
    
    int opId = data["operator_id"].toInt();
    if (opId <= 0) opId = 1; 
    query.addBindValue(opId);
    
    query.addBindValue(data["totalAmount"].toDouble());
    query.addBindValue(data.contains("discount") ? data["discount"].toDouble() : 0.0);
    query.addBindValue(data["finalAmount"].toDouble());
    query.addBindValue(data["itemDetails"].toString());
    query.addBindValue(data["payMethod"].toString());
    query.addBindValue(data["status"].toString());
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    if (!query.exec()) {
        LOG_E("[ORDER] SQL Error: " << query.lastError().text().toStdString());
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "SQL Error: " + query.lastError().text();
    } else {
        LOG_I("[ORDER] Order saved successfully: " << data["id"].toString().toStdString());
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Success";
        
        // 广播刷新通知
        QJsonObject notify;
        notify["module"] = "order";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    }

    client->sendPacket(Protocol::CMD_CREATE_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void OrderController::handleGetOrderList(ClientHandler *client, const QJsonObject &data)
{
    Q_UNUSED(data);
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("SELECT * FROM orders ORDER BY created_at DESC LIMIT 1000");
    
    QJsonObject response;
    QJsonArray orderList;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject o;
            o["id"] = query.value("order_no").toString();
            o["sourceModule"] = query.value("source_module").toString();
            o["relatedId"] = query.value("related_id").toString();
            int mid = query.value("member_id").toInt();
            o["memberId"] = mid > 0 ? QString("M%1").arg(mid, 5, 10, QChar('0')) : "";
            o["totalAmount"] = query.value("total_amount").toDouble();
            o["discount"] = query.value("discount").toDouble();
            o["finalAmount"] = query.value("actual_pay").toDouble();
            o["itemDetails"] = query.value("item_details").toString();
            o["payMethod"] = query.value("payment_method").toString();
            o["status"] = query.value("status").toString();
            o["createTime"] = query.value("created_at").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            orderList.append(o);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = orderList;
        LOG_I("[ORDER] Found " << orderList.size() << " orders for client");
    }
 else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_GET_ORDER_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void OrderController::handleUpdateOrder(ClientHandler *client, const QJsonObject &data) {
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QJsonObject response;

    if (!db.isOpen() && !db.open()) {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "DB Open Error: " + db.lastError().text();
        client->sendPacket(Protocol::CMD_UPDATE_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    db.transaction();

    // 1. 获取原状态与关联的会员ID
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT member_id, status FROM orders WHERE order_no = ?");
    checkQuery.addBindValue(data["order_id"].toString());
    
    int memberId = 0;
    QString oldStatus = "Unpaid";
    if (checkQuery.exec() && checkQuery.next()) {
        memberId = checkQuery.value("member_id").toInt();
        oldStatus = checkQuery.value("status").toString();
    }

    // 2. 执行订单更新
    QSqlQuery query(db);
    query.prepare("UPDATE orders SET status = ?, actual_pay = ?, payment_method = ? WHERE order_no = ?");
    query.addBindValue(data["status"].toString());
    query.addBindValue(data["final_amount"].toDouble());
    query.addBindValue(data["pay_method"].toString());
    query.addBindValue(data["order_id"].toString());

    if (!query.exec()) {
        db.rollback();
        LOG_E("[ORDER] SQL Error on Update: " << query.lastError().text().toStdString());
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    } else {
        // 3. 如果原订单不是 Paid，现在更新为 Paid，且存在有效的会员ID
        if (oldStatus != "Paid" && data["status"].toString() == "Paid" && memberId > 0) {
            double newPay = data["final_amount"].toDouble();
            int pointsToGain = static_cast<int>(newPay); // 1元积1分
            
            QSqlQuery memberQuery(db);
            memberQuery.prepare("UPDATE members SET consume_amt = consume_amt + ?, points = points + ? WHERE member_id = ?");
            memberQuery.addBindValue(newPay);
            memberQuery.addBindValue(pointsToGain);
            memberQuery.addBindValue(memberId);
            
            if (!memberQuery.exec()) {
                db.rollback();
                LOG_E("[ORDER] Failed to update member stats: " << memberQuery.lastError().text().toStdString());
                response["status"] = Protocol::STATUS_ERROR;
                response["message"] = "Member stats update failed: " + memberQuery.lastError().text();
                client->sendPacket(Protocol::CMD_UPDATE_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
                return;
            }
            LOG_I("[ORDER] Member M" << memberId << " consume_amt increased by " << newPay << ", points by " << pointsToGain);
        }

        db.commit();
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Success";
        
        // 广播订单和财务数据刷新通知
        QJsonObject notify;
        notify["module"] = "order";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);

        // 如果更新了会员，同时广播会员刷新通知
        if (oldStatus != "Paid" && data["status"].toString() == "Paid" && memberId > 0) {
            QJsonObject memberNotify;
            memberNotify["module"] = "member";
            m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, memberNotify);
        }
    }
    client->sendPacket(Protocol::CMD_UPDATE_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void OrderController::handleCancelOrder(ClientHandler *client, const QJsonObject &data) {
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QJsonObject response;

    if (!db.isOpen() && !db.open()) {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = "DB Open Error: " + db.lastError().text();
        client->sendPacket(Protocol::CMD_CANCEL_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
        return;
    }

    QSqlQuery query(db);
    query.prepare("UPDATE orders SET status = 'Cancelled' WHERE order_no = ?");
    query.addBindValue(data["order_id"].toString());

    if (!query.exec()) {
        LOG_E("[ORDER] SQL Error on Cancel: " << query.lastError().text().toStdString());
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    } else {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Success";
        
        QJsonObject notify;
        notify["module"] = "order";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    }
    client->sendPacket(Protocol::CMD_CANCEL_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
