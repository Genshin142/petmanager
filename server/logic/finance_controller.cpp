#include "finance_controller.h"
#include "../network/server_core.h"
#include "../database/connectionpool.h"
#include "../../protocol_codes.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "../logger_compat.h"
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
    QString startDate = data["start_date"].toString(); // yyyy-MM-dd
    QString endDate = data["end_date"].toString(); // yyyy-MM-dd
    
    LOG(INFO) << "[FINANCE] Fetching performance list. month: " << month.toStdString() 
              << ", start: " << startDate.toStdString() << ", end: " << endDate.toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    
    QString sql = "SELECT p.*, e.real_name as employee_name, s.commission_value as svc_commission, "
                  "o.member_id as real_member_id, o.payment_method as real_pay_method, o.item_details "
                  "FROM sys_performance_records p "
                  "JOIN sys_employees e ON p.emp_id = e.emp_id "
                  "LEFT JOIN services s ON p.service_name = s.name "
                  "LEFT JOIN orders o ON p.order_id = o.order_no "
                  "WHERE 1=1";
    
    if (!month.isEmpty()) {
        sql += " AND p.service_date LIKE '" + month + "%'";
    } else if (!startDate.isEmpty() && !endDate.isEmpty()) {
        sql += " AND p.service_date BETWEEN '" + startDate + "' AND '" + endDate + "'";
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
            
            int memberId = query.value("real_member_id").toInt();
            if (memberId > 0) {
                obj["customer_id"] = "M" + QString("%1").arg(memberId, 5, 10, QChar('0'));
            } else {
                obj["customer_id"] = "M" + QString("%1").arg(query.value("perf_id").toInt() % 5 + 1, 5, 10, QChar('0'));
            }
            
            QString custName = query.value("customer_name").toString();
            if (custName.isEmpty() && memberId > 0) {
                QSqlQuery memQ(db);
                memQ.prepare("SELECT name FROM members WHERE member_id = ?");
                memQ.addBindValue(memberId);
                if (memQ.exec() && memQ.next()) {
                    custName = memQ.value("name").toString();
                }
            }
            obj["customer_name"] = custName;
            
            // 支付方式中文化与字段兜底
            QString payMethod = query.value("pay_method").toString();
            if (payMethod.isEmpty() || payMethod == "MemberCard") {
                QString realPay = query.value("real_pay_method").toString();
                if (!realPay.isEmpty()) payMethod = realPay;
            }
            if (payMethod == "MemberCard") payMethod = "会员卡";
            else if (payMethod == "Cash") payMethod = "现金";
            else if (payMethod == "Alipay" || payMethod == "支付宝") payMethod = "支付宝";
            else if (payMethod == "Wechat" || payMethod == "微信") payMethod = "微信";
            obj["pay_method"] = payMethod;
            
            // 从 item_details 提取真实宠物信息
            QString itemDetailsStr = query.value("item_details").toString();
            QString realPetId = "";
            QString realPetName = "";
            QString realPetBreed = "";
            if (!itemDetailsStr.isEmpty()) {
                QJsonDocument doc = QJsonDocument::fromJson(itemDetailsStr.toUtf8());
                if (doc.isArray()) {
                    QJsonArray arr = doc.array();
                    if (arr.size() > 0) {
                        QJsonObject itemObj = arr[0].toObject();
                        realPetId = itemObj["petId"].toString();
                        realPetName = itemObj["petName"].toString();
                        realPetBreed = itemObj["petBreed"].toString();
                    }
                }
            }
            
            if (realPetId.isEmpty()) {
                realPetId = "P" + QString("%1").arg(query.value("perf_id").toInt() % 4 + 1, 5, 10, QChar('0'));
            }
            if (realPetName.isEmpty()) {
                realPetName = query.value("pet_name").toString();
            }
            if (realPetBreed.isEmpty()) {
                realPetBreed = query.value("pet_breed").toString();
            }
            
            obj["pet_id"] = realPetId;
            obj["pet_name"] = realPetName;
            obj["pet_breed"] = realPetBreed;
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
    
    // 1. 获取该月份数据库中已存在的真实薪资记录
    QMap<int, QJsonObject> existingSalaries;
    QSqlQuery query(db);
    QString sql = "SELECT s.*, e.real_name as employee_name FROM salary_records s "
                  "JOIN sys_employees e ON s.emp_id = e.emp_id";
    if (!month.isEmpty()) {
        sql += " WHERE s.salary_month = '" + month + "'";
    }
    sql += " ORDER BY s.emp_id ASC";
    
    if (query.exec(sql)) {
        while (query.next()) {
            QJsonObject obj;
            int empId = query.value("emp_id").toInt();
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
            existingSalaries[empId] = obj;
        }
    }

    // 2. 获取所有在职员工，用来补充未生成记录的员工（动态渲染）
    QJsonArray salaryArray;
    QSqlQuery empQuery(db);
    empQuery.prepare("SELECT emp_id, real_name, base_salary FROM sys_employees WHERE is_deleted = 0");
    if (empQuery.exec()) {
        while (empQuery.next()) {
            int empId = empQuery.value("emp_id").toInt();
            QString empName = empQuery.value("real_name").toString();
            double baseSalary = empQuery.value("base_salary").toDouble();
            
            if (existingSalaries.contains(empId)) {
                // 如果已经有真实的历史记录，直接使用
                salaryArray.append(existingSalaries[empId]);
            } else if (!month.isEmpty()) {
                // 否则为该员工动态构造一个“待发放”的虚拟薪资预览记录
                QJsonObject obj;
                obj["id"] = QString("TEMP_%1_%2").arg(empId).arg(month);
                obj["employee_id"] = QString::number(empId);
                obj["employee_name"] = empName;
                obj["month"] = month;
                obj["base_salary"] = baseSalary;
                
                // 动态算提成 (本月已核销和待核销明细总和)
                QSqlQuery commQuery(db);
                commQuery.prepare("SELECT SUM(commission_amt) FROM sys_performance_records WHERE emp_id = ? AND service_date LIKE ?");
                commQuery.addBindValue(empId);
                commQuery.addBindValue(month + "%");
                
                double commission = 0.0;
                if (commQuery.exec() && commQuery.next()) {
                    commission = commQuery.value(0).toDouble();
                }
                
                obj["commission"] = commission;
                obj["bonus"] = 0.0;
                obj["deduction"] = 0.0;
                
                // 扣除计算
                double totalIns = baseSalary * (0.08 + 0.02 + 0.005 + 0.07);
                double taxableIncome = baseSalary + commission - totalIns - 5000;
                double tax = (taxableIncome > 0) ? taxableIncome * 0.03 : 0.0;
                double netPay = baseSalary + commission - totalIns - tax;
                
                obj["net_pay"] = netPay;
                obj["status"] = "待发放";
                obj["pay_time"] = "";
                obj["remark"] = month + " 月份工资结算单";
                
                salaryArray.append(obj);
            }
        }
        
        QJsonObject response;
        response["status"] = Protocol::STATUS_OK;
        response["data"] = salaryArray;
        client->sendPacket(Protocol::CMD_GET_SALARY_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    } else {
        LOG(ERROR) << "[FINANCE] Failed to fetch employee list: " << empQuery.lastError().text().toStdString();
        QJsonObject response;
        response["status"] = Protocol::STATUS_ERROR;
        response["message"] = empQuery.lastError().text();
        client->sendPacket(Protocol::CMD_GET_SALARY_LIST, QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
}

void FinanceController::handleApproveSalary(ClientHandler* client, const QJsonObject& data) {
    QString salaryId = data["salary_id"].toString();
    LOG(INFO) << "[FINANCE] Approving salary ID: " << salaryId.toStdString();
    
    QSqlDatabase db = ConnectionPool::instance().openConnection();
    QSqlQuery query(db);
    bool ok = false;
    
    if (salaryId.startsWith("TEMP_")) {
        // 如果是虚拟记录，说明需要进行“首次发放”，将其持久化插入真实数据库中！
        QStringList parts = salaryId.split('_');
        if (parts.size() >= 3) {
            int empId = parts[1].toInt();
            QString month = parts[2];
            
            // 重新获取底薪和动态提成来插入
            QSqlQuery empQuery(db);
            empQuery.prepare("SELECT base_salary FROM sys_employees WHERE emp_id = ?");
            empQuery.addBindValue(empId);
            double baseSalary = 0.0;
            if (empQuery.exec() && empQuery.next()) {
                baseSalary = empQuery.value("base_salary").toDouble();
            }
            
            QSqlQuery commQuery(db);
            commQuery.prepare("SELECT SUM(commission_amt) FROM sys_performance_records WHERE emp_id = ? AND service_date LIKE ?");
            commQuery.addBindValue(empId);
            commQuery.addBindValue(month + "%");
            double commission = 0.0;
            if (commQuery.exec() && commQuery.next()) {
                commission = commQuery.value(0).toDouble();
            }
            
            double totalIns = baseSalary * (0.08 + 0.02 + 0.005 + 0.07);
            double taxableIncome = baseSalary + commission - totalIns - 5000;
            double tax = (taxableIncome > 0) ? taxableIncome * 0.03 : 0.0;
            double netPay = baseSalary + commission - totalIns - tax;
            
            query.prepare("INSERT INTO salary_records (emp_id, salary_month, base_salary, commission, bonus, deduction, net_pay, status, pay_time, remark) "
                          "VALUES (?, ?, ?, ?, 0.0, 0.0, ?, '已发放', ?, ?)");
            query.addBindValue(empId);
            query.addBindValue(month);
            query.addBindValue(baseSalary);
            query.addBindValue(commission);
            query.addBindValue(netPay);
            query.addBindValue(QDateTime::currentDateTime());
            query.addBindValue(month + " 月份工资结算单");
            ok = query.exec();
        }
    } else {
        // 如果是真实数据库记录，更新状态为“已发放”
        query.prepare("UPDATE salary_records SET status = '已发放', pay_time = ? WHERE salary_id = ?");
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(salaryId.toInt());
        ok = query.exec();
    }

    QJsonObject response;
    if (ok) {
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
