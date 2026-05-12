#include "order_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QDateTime>
#include <QJsonDocument>
#include <glog/logging.h>

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
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // 开启事务 (如果有库存扣减逻辑)
    // db.transaction();

    query.prepare("INSERT INTO orders (order_no, source_module, related_id, member_id, operator_id, total_amount, discount, actual_pay, item_details, payment_method, status) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
    query.addBindValue(data["id"].toString()); // 使用客户端生成的 ORD 编号作为 order_no
    query.addBindValue(data["sourceModule"].toString());
    query.addBindValue(data["relatedId"].toString());
    
    // 处理散客 (memberId 为空字符串时存入 NULL 而非 0，防止外键约束失败)
    QString midStr = data["memberId"].toString();
    LOG(INFO) << "[DEBUG] Received memberId raw: " << midStr.toStdString();
    
    if (midStr.isEmpty()) {
        query.addBindValue(QVariant(QVariant::Int));
        LOG(INFO) << "[DEBUG] memberId is empty, binding NULL";
    } else {
        // 如果包含 M 前缀则剥离 (客户端格式化导致的 M00001)
        if (midStr.startsWith("M")) midStr = midStr.mid(1);
        int finalMid = midStr.toInt();
        query.addBindValue(finalMid);
        LOG(INFO) << "[DEBUG] binding memberId: " << finalMid;
    }

    // 绑定操作员 ID (经手人)
    query.addBindValue(client->userId());

    query.addBindValue(data["totalAmount"].toDouble());
    query.addBindValue(data["discount"].toDouble());
    query.addBindValue(data["finalAmount"].toDouble());
    query.addBindValue(data["itemDetails"].toString());
    query.addBindValue(data["payMethod"].toString());
    query.addBindValue(data["status"].toString());

    LOG(INFO) << "[DEBUG] Binding Order Details: " << data["itemDetails"].toString().toStdString() 
              << " Status: " << data["status"].toString().toStdString();

    QJsonObject response;
    if (query.exec()) {
        LOG(INFO) << "[ORDER] Successfully created order: " << data["id"].toString().toStdString();
        response["status"] = Protocol::STATUS_OK;
        // 广播通知刷新
        QJsonObject notify;
        notify["module"] = "order";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[ORDER] Failed to create order: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_CREATE_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void OrderController::handleGetOrderList(ClientHandler *client, const QJsonObject &data)
{
    Q_UNUSED(data);
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // 这里可以根据时间范围过滤，目前先取最近 1000 条以支持报表统计
    query.prepare("SELECT * FROM orders ORDER BY created_at DESC LIMIT 1000");
    
    QJsonObject response;
    QJsonArray orderList;
    
    if (query.exec()) {
        while (query.next()) {
            QJsonObject o;
            QSqlRecord rec = query.record();
            o["id"] = query.value("order_no").toString();
            o["sourceModule"] = query.value("source_module").toString();
            o["relatedId"] = query.value("related_id").toString();
            
            // 处理成员ID回显（添加M前缀并补零，与客户端 MemberDataManager 保持一致）
            int mid = query.value("member_id").toInt();
            o["memberId"] = mid > 0 ? QString("M%1").arg(mid, 5, 10, QChar('0')) : "";
            
            o["totalAmount"] = query.value("total_amount").toDouble();
            o["discount"] = query.value("discount").toDouble();
            o["finalAmount"] = query.value("actual_pay").toDouble();
            o["itemDetails"] = query.value("item_details").toString();
            o["payMethod"] = query.value("payment_method").toString();
            o["status"] = query.value("status").toString();
            o["createTime"] = query.value("created_at").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            o["cancelReason"] = query.value("cancel_reason").toString();
            orderList.append(o);
        }
        response["status"] = Protocol::STATUS_OK;
        response["data"] = orderList;
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_GET_ORDER_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void OrderController::handleUpdateOrder(ClientHandler *client, const QJsonObject &data)
{
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    // 更新实付金额、支付方式和订单状态
    query.prepare("UPDATE orders SET status = ?, actual_pay = ?, payment_method = ? WHERE order_no = ?");
    query.addBindValue(data["status"].toString());
    query.addBindValue(data["final_amount"].toDouble());
    query.addBindValue(data["pay_method"].toString());
    query.addBindValue(data["order_id"].toString());

    QJsonObject response;
    if (query.exec()) {
        LOG(INFO) << "[ORDER] Successfully updated order: " << data["order_id"].toString().toStdString();
        response["status"] = Protocol::STATUS_OK;
        // 广播通知刷新
        QJsonObject notify;
        notify["module"] = "order";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[ORDER] Failed to update order: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_UPDATE_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void OrderController::handleCancelOrder(ClientHandler *client, const QJsonObject &data)
{
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    query.prepare("UPDATE orders SET status = '已作废', cancel_reason = ? WHERE order_no = ?");
    query.addBindValue(data["reason"].toString());
    query.addBindValue(data["order_id"].toString());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        // 广播通知刷新
        QJsonObject notify;
        notify["module"] = "order";
        m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_CANCEL_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
