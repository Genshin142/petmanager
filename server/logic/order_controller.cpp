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

    // 1. 获取原状态、关联的会员ID、来源模块与关联单据ID
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT member_id, status, source_module, related_id FROM orders WHERE order_no = ?");
    checkQuery.addBindValue(data["order_id"].toString());
    
    int memberId = 0;
    QString oldStatus = "Unpaid";
    QString sourceModule = "";
    QString relatedId = "";
    if (checkQuery.exec() && checkQuery.next()) {
        memberId = checkQuery.value("member_id").toInt();
        oldStatus = checkQuery.value("status").toString();
        sourceModule = checkQuery.value("source_module").toString();
        relatedId = checkQuery.value("related_id").toString();
    }

    // 2. 执行订单更新
    QSqlQuery query(db);
    query.prepare("UPDATE orders SET status = ?, actual_pay = ?, payment_method = ? WHERE order_no = ?");
    query.addBindValue(data["status"].toString());
    query.addBindValue(data["final_amount"].toDouble());
    query.addBindValue(data["pay_method"].toString());
    query.addBindValue(data["order_id"].toString());

    if (!query.exec()) {
        LOG_E("[ORDER] SQL Error on Update: " << query.lastError().text().toStdString());
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    } else {
        // 3. 如果原订单不是 Paid，现在更新为 Paid
        if (oldStatus != "Paid" && data["status"].toString() == "Paid") {
            double newPay = data["final_amount"].toDouble();
            
            // A. 更新会员积分和消费额
            if (memberId > 0) {
                int pointsToGain = static_cast<int>(newPay); // 1元积1分
                QSqlQuery memberQuery(db);
                memberQuery.prepare("UPDATE members SET consume_amt = consume_amt + ?, points = points + ? WHERE member_id = ?");
                memberQuery.addBindValue(newPay);
                memberQuery.addBindValue(pointsToGain);
                memberQuery.addBindValue(memberId);
                
                if (!memberQuery.exec()) {
                    LOG_E("[ORDER] Failed to update member stats: " << memberQuery.lastError().text().toStdString());
                    response["status"] = Protocol::STATUS_ERROR;
                    response["message"] = "Member stats update failed: " + memberQuery.lastError().text();
                    client->sendPacket(Protocol::CMD_UPDATE_ORDER, QJsonDocument(response).toJson(QJsonDocument::Compact));
                    return;
                }
                LOG_I("[ORDER] Member M" << memberId << " consume_amt increased by " << newPay << ", points by " << pointsToGain);
            }
            
            // B. 👈 智能联运：若此订单来源于洗护预约，则自动为执行员工计算并创建提绩效记录
            if (sourceModule == "Appointment" && !relatedId.isEmpty()) {
                QSqlQuery apptQuery(db);
                apptQuery.prepare("SELECT staff_id, service_id, member_id, pet_id FROM appointments WHERE appt_id = ?");
                apptQuery.addBindValue(relatedId.toInt());
                if (apptQuery.exec() && apptQuery.next()) {
                    int staffId = apptQuery.value("staff_id").toInt();
                    int serviceId = apptQuery.value("service_id").toInt();
                    int memberId = apptQuery.value("member_id").toInt();
                    int petId = apptQuery.value("pet_id").toInt();
                    
                    if (staffId > 0) {
                        // 1. 查询会员姓名
                        QString customerName = "";
                        QSqlQuery memQuery(db);
                        memQuery.prepare("SELECT name FROM members WHERE member_id = ?");
                        memQuery.addBindValue(memberId);
                        if (memQuery.exec() && memQuery.next()) {
                            customerName = memQuery.value("name").toString();
                        }
                        
                        // 2. 查询宠物姓名和品种
                        QString petName = "";
                        QString petBreed = "";
                        QSqlQuery petQuery(db);
                        petQuery.prepare("SELECT pet_name, breed FROM pets WHERE pet_id = ?");
                        petQuery.addBindValue(petId);
                        if (petQuery.exec() && petQuery.next()) {
                            petName = petQuery.value("pet_name").toString();
                            petBreed = petQuery.value("breed").toString();
                        }
                        
                        // 3. 获取支付方式与实付金额
                        QString payMethod = data["pay_method"].toString();
                        double orderAmount = data["final_amount"].toDouble();
                        
                        // 4. 查询服务项目的提成比和项目名称
                        QString serviceName = "未知服务";
                        double commissionVal = 0.0;
                        QSqlQuery svcQuery(db);
                        svcQuery.prepare("SELECT name, commission_value FROM services WHERE service_id = ?");
                        svcQuery.addBindValue(serviceId);
                        if (svcQuery.exec() && svcQuery.next()) {
                            serviceName = svcQuery.value("name").toString();
                            commissionVal = svcQuery.value("commission_value").toDouble();
                        }
                        
                        double commissionAmt = commissionVal;
                        
                        // 若服务项目本身未配置固定提成，则兜底采用员工提成比例
                        if (commissionAmt <= 0) {
                            double commRate = 0.10; // 默认 10%
                            QSqlQuery rateQuery(db);
                            rateQuery.prepare("SELECT commission_rate FROM sys_employees WHERE emp_id = ?");
                            rateQuery.addBindValue(staffId);
                            if (rateQuery.exec() && rateQuery.next()) {
                                commRate = rateQuery.value("commission_rate").toDouble();
                            }
                            commissionAmt = orderAmount * commRate;
                        }
                        
                        // 5. 写入 sys_performance_records 提成明细表，自动进入提成计算模块
                        QSqlQuery perfQuery(db);
                        perfQuery.prepare("INSERT INTO sys_performance_records (emp_id, order_id, service_date, service_name, order_amount, commission_amt, status, customer_name, pay_method, pet_name, pet_breed) "
                                          "VALUES (?, ?, ?, ?, ?, ?, '待核销', ?, ?, ?, ?)");
                        perfQuery.addBindValue(staffId);
                        perfQuery.addBindValue(data["order_id"].toString());
                        perfQuery.addBindValue(QDate::currentDate().toString("yyyy-MM-dd"));
                        perfQuery.addBindValue(serviceName);
                        perfQuery.addBindValue(orderAmount);
                        perfQuery.addBindValue(commissionAmt);
                        perfQuery.addBindValue(customerName);
                        perfQuery.addBindValue(payMethod);
                        perfQuery.addBindValue(petName);
                        perfQuery.addBindValue(petBreed);
                        
                        if (perfQuery.exec()) {
                            LOG_I("[ORDER] Performance record created successfully for emp ID " << staffId << ", amt: " << commissionAmt);
                            
                            // 👈 智能向全网广播财务业绩已刷新
                            QJsonObject finNotify;
                            finNotify["module"] = "finance";
                            m_server->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, finNotify);
                        } else {
                            LOG_E("[ORDER] Failed to insert performance record: " << perfQuery.lastError().text().toStdString());
                        }
                    }
                }
            }
        }

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
