#include "finance_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <glog/logging.h>
#include <QDateTime>

FinanceController::FinanceController(ServerCore* core, QObject* parent)
    : QObject(parent), m_core(core)
{
    m_core->registerHandler(Protocol::CMD_GET_PERFORMANCE_LIST, std::bind(&FinanceController::handleGetPerformanceList, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_VERIFY_PERFORMANCE, std::bind(&FinanceController::handleVerifyPerformance, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_GET_SALARY_LIST, std::bind(&FinanceController::handleGetSalaryList, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_APPROVE_SALARY, std::bind(&FinanceController::handleApproveSalary, this, std::placeholders::_1, std::placeholders::_2));
    m_core->registerHandler(Protocol::CMD_PAY_SALARY, std::bind(&FinanceController::handlePaySalary, this, std::placeholders::_1, std::placeholders::_2));
}

void FinanceController::handleGetPerformanceList(ClientHandler* client, const QJsonObject& data) {
    QString month = data["month"].toString(); // yyyy-MM
    QString employeeId = data["employee_id"].toString();
    
    LOG(INFO) << "[FINANCE] Fetching performance list for month: " << month.toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    QString sql = "SELECT p.*, e.real_name as employee_name, s.commission_value as svc_commission "
                  "FROM sys_performance_records p "
                  "JOIN sys_employees e ON p.emp_id = e.emp_id "
                  "LEFT JOIN services s ON p.service_name = s.name "
                  "WHERE 1=1";
    
    if (!month.isEmpty()) {
        sql += " AND p.service_date LIKE '" + month + "%'";
    }
    if (!employeeId.isEmpty()) {
        sql += " AND p.emp_id = " + employeeId;
    }
    sql += " ORDER BY p.service_date DESC";

    QJsonArray perfArray;
    if (query.exec(sql)) {
        while (query.next()) {
            QJsonObject obj;
            obj["id"] = query.value("perf_id").toString();
            obj["employee_id"] = query.value("emp_id").toString();
            obj["employee_name"] = query.value("employee_name").toString();
            obj["service_date"] = query.value("service_date").toDate().toString("yyyy-MM-dd");
            obj["service_name"] = query.value("service_name").toString();
            obj["order_amount"] = query.value("order_amount").toDouble();
            
            // 使用服务模块的真实提成 (如果存在)
            double commission = query.value("svc_commission").toDouble();
            if (commission <= 0) {
                commission = query.value("commission_amt").toDouble(); // 回退到记录中的值
            }
            obj["commission"] = commission;
            obj["commission_type"] = "固定提成"; // 统一按固定提成处理
            
            obj["status"] = query.value("status").toString();
            obj["order_id"] = query.value("order_id").toString();
            obj["verify_time"] = query.value("verify_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            obj["customer_id"] = "M" + QString("%1").arg(query.value("perf_id").toInt() % 5 + 1, 5, 10, QChar('0'));
            obj["customer_name"] = query.value("customer_name").toString();
            
            // 支付方式中文化
            QString payMethod = query.value("pay_method").toString();
            if (payMethod == "MemberCard") payMethod = "会员卡";
            else if (payMethod == "Cash") payMethod = "现金";
            else if (payMethod == "Alipay") payMethod = "支付宝";
            else if (payMethod == "Wechat") payMethod = "微信";
            obj["pay_method"] = payMethod;
            
            obj["pet_id"] = "P" + QString("%1").arg(query.value("perf_id").toInt() % 4 + 1, 5, 10, QChar('0'));
            obj["pet_name"] = query.value("pet_name").toString();
            obj["pet_breed"] = query.value("pet_breed").toString();
            obj["final_amount"] = query.value("order_amount").toDouble(); 
            obj["commission_type"] = "固定提成"; // 冗余确保
            obj["commission_rate"] = 0.0;
            perfArray.append(obj);
        }
        
        QJsonObject response;
        response["status"] = Protocol::STATUS_OK;
        response["data"] = perfArray;
        client->sendPacket(Protocol::CMD_GET_PERFORMANCE_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    } else {
        LOG(ERROR) << "[FINANCE] Failed to fetch performance list: " << query.lastError().text().toStdString();
        QJsonObject response;
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
        client->sendPacket(Protocol::CMD_GET_PERFORMANCE_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
}

void FinanceController::handleVerifyPerformance(ClientHandler* client, const QJsonObject& data) {
    QString recordId = data["record_id"].toString();
    LOG(INFO) << "[FINANCE] Verifying performance record ID: " << recordId.toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE sys_performance_records SET status = '已核销', verify_time = ? WHERE perf_id = ?");
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(recordId.toLongLong());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Performance record verified successfully";
        
        // 广播通知刷新
        QJsonObject notify;
        notify["module"] = "finance";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[FINANCE] Failed to verify performance: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_VERIFY_PERFORMANCE, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void FinanceController::handleGetSalaryList(ClientHandler* client, const QJsonObject& data) {
    QString month = data["month"].toString(); // yyyy-MM
    LOG(INFO) << "[FINANCE] Fetching salary list for month: " << month.toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    QString sql = "SELECT s.*, e.real_name as employee_name FROM salary_records s "
                  "JOIN sys_employees e ON s.emp_id = e.emp_id";
    if (!month.isEmpty()) {
        sql += " WHERE s.salary_month = '" + month + "'";
    }
    sql += " ORDER BY s.emp_id ASC";

    QJsonArray salaryArray;
    if (query.exec(sql)) {
        while (query.next()) {
            QJsonObject obj;
            obj["id"] = query.value("salary_id").toString();
            obj["employee_id"] = query.value("emp_id").toString();
            obj["employee_name"] = query.value("employee_name").toString();
            obj["month"] = query.value("salary_month").toString();
            obj["base_salary"] = query.value("base_salary").toDouble();
            obj["commission"] = query.value("commission").toDouble();
            obj["bonus"] = query.value("bonus").toDouble();
            obj["deduction"] = query.value("deduction").toDouble();
            obj["net_pay"] = query.value("net_pay").toDouble();
            obj["status"] = query.value("status").toString();
            obj["pay_time"] = query.value("pay_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            obj["remark"] = query.value("remark").toString();
            salaryArray.append(obj);
        }
        
        QJsonObject response;
        response["status"] = Protocol::STATUS_OK;
        response["data"] = salaryArray;
        client->sendPacket(Protocol::CMD_GET_SALARY_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    } else {
        LOG(ERROR) << "[FINANCE] Failed to fetch salary list: " << query.lastError().text().toStdString();
        QJsonObject response;
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
        client->sendPacket(Protocol::CMD_GET_SALARY_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
}

void FinanceController::handleApproveSalary(ClientHandler* client, const QJsonObject& data) {
    // 审批薪资，这里简单处理为变更状态
    QString salaryId = data["salary_id"].toString();
    LOG(INFO) << "[FINANCE] Approving salary ID: " << salaryId.toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE salary_records SET status = '已发放', pay_time = ? WHERE salary_id = ?");
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(salaryId.toInt());

    QJsonObject response;
    if (query.exec()) {
        response["status"] = Protocol::STATUS_OK;
        response["message"] = "Salary approved/paid successfully";
        
        QJsonObject notify;
        notify["module"] = "finance";
        m_core->broadcastPacket(Protocol::CMD_NOTIFY_REFRESH, notify);
    } else {
        LOG(ERROR) << "[FINANCE] Failed to approve salary: " << query.lastError().text().toStdString();
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = query.lastError().text();
    }
    client->sendPacket(Protocol::CMD_APPROVE_SALARY, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void FinanceController::handlePaySalary(ClientHandler* client, const QJsonObject& data) {
    handleApproveSalary(client, data); // 逻辑相似
}
