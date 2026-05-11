#include "salarydatamanager.h"
#include "staffdatamanager.h"
#include "servicedatamanager.h"
#include <QDateTime>
#include <QRandomGenerator>
#include "../utils/networkmanager.h"
#include "../protocol_codes.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

SalaryDataManager* SalaryDataManager::m_instance = nullptr;

SalaryDataManager* SalaryDataManager::instance() {
    if (!m_instance) m_instance = new SalaryDataManager();
    return m_instance;
}

SalaryDataManager::SalaryDataManager(QObject *parent) : QObject(parent) {
    connect(&NetworkManager::instance(), &NetworkManager::packetReceived,
            this, &SalaryDataManager::onPacketReceived);
}

void SalaryDataManager::initMockData() {
    // 数据将由服务端下发
}

QList<SalaryInfo> SalaryDataManager::getSalariesByMonth(const QString &month) {
    QMutexLocker locker(&m_mutex);
    QList<SalaryInfo> list;
    for (const auto &s : m_salaries) {
        if (s.month == month) list.append(s);
    }
    return list;
}

SalaryInfo SalaryDataManager::getSalary(const QString &id) {
    QMutexLocker locker(&m_mutex);
    return m_salaries.value(id);
}

void SalaryDataManager::updateSalary(const SalaryInfo &info) {
    QMutexLocker locker(&m_mutex);
    m_salaries[info.id] = info;
    emit salaryDataChanged();
}

void SalaryDataManager::approveSalary(const QString &id) {
    QJsonObject obj;
    obj["salary_id"] = id;
    NetworkManager::instance().sendRequest(Protocol::CMD_APPROVE_SALARY, obj);
}

void SalaryDataManager::paySalary(const QString &id) {
    QJsonObject obj;
    obj["salary_id"] = id;
    NetworkManager::instance().sendRequest(Protocol::CMD_PAY_SALARY, obj);
}

SalaryDataManager::SalaryStats SalaryDataManager::getStats(const QString &month) {
    QMutexLocker locker(&m_mutex);
    SalaryStats stats = {0, 0, 0};
    for (const auto &s : m_salaries) {
        if (s.month == month) {
            stats.totalPayroll += s.netPay;
            if (s.status == "已发放") stats.paidCount++;
            else stats.pendingCount++;
        }
    }
    return stats;
}

void SalaryDataManager::addPerformanceRecord(const PerformanceRecord &record) {
    QMutexLocker locker(&m_mutex);
    m_performance[record.id] = record;
    emit performanceDataChanged();
}

QList<PerformanceRecord> SalaryDataManager::getPerformanceRecords(const QString &month, const QString &employeeId) {
    QMutexLocker locker(&m_mutex);
    QList<PerformanceRecord> list;
    for (const auto &p : m_performance) {
        if (p.serviceDate.startsWith(month)) {
            if (employeeId.isEmpty() || p.employeeId == employeeId) {
                list.append(p);
            }
        }
    }
    return list;
}

void SalaryDataManager::verifyPerformance(const QString &recordId) {
    QJsonObject obj;
    obj["record_id"] = recordId;
    NetworkManager::instance().sendRequest(Protocol::CMD_VERIFY_PERFORMANCE, obj);
}

void SalaryDataManager::requestPerformanceRecords(const QString &month, const QString &employeeId) {
    QJsonObject obj;
    obj["month"] = month;
    if (!employeeId.isEmpty()) obj["employee_id"] = employeeId;
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_PERFORMANCE_LIST, obj);
}

void SalaryDataManager::requestSalaries(const QString &month) {
    QJsonObject obj;
    obj["month"] = month;
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_SALARY_LIST, obj);
}

void SalaryDataManager::onPacketReceived(const Protocol::NetPacket &packet) {
    if (packet.cmdId == Protocol::CMD_GET_PERFORMANCE_LIST) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray data = root["data"].toArray();
            QMutexLocker locker(&m_mutex);
            m_performance.clear();
            for (int i = 0; i < data.size(); ++i) {
                QJsonObject obj = data[i].toObject();
                PerformanceRecord pr;
                pr.id = obj["id"].toString();
                pr.employeeId = obj["employee_id"].toString();
                pr.employeeName = obj["employee_name"].toString();
                pr.serviceDate = obj["service_date"].toString();
                pr.serviceName = obj["service_name"].toString();
                pr.orderAmount = obj["order_amount"].toDouble();
                pr.commission = obj["commission"].toDouble();
                pr.status = obj["status"].toString();
                pr.orderId = obj["order_id"].toString();
                pr.verifyTime = obj["verify_time"].toString();
                pr.customerId = obj["customer_id"].toString();
                pr.customerName = obj["customer_name"].toString();
                pr.payMethod = obj["pay_method"].toString();
                pr.petId = obj["pet_id"].toString();
                pr.petName = obj["pet_name"].toString();
                pr.petBreed = obj["pet_breed"].toString();
                pr.finalAmount = obj["final_amount"].toDouble();
                pr.commissionType = obj["commission_type"].toString();
                pr.commissionRate = obj["commission_rate"].toDouble();
                m_performance[pr.id] = pr;
            }
            emit performanceDataChanged();
        }
    } else if (packet.cmdId == Protocol::CMD_GET_SALARY_LIST) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray data = root["data"].toArray();
            QMutexLocker locker(&m_mutex);
            m_salaries.clear();
            for (int i = 0; i < data.size(); ++i) {
                QJsonObject obj = data[i].toObject();
                SalaryInfo info;
                info.id = obj["id"].toString();
                info.employeeId = obj["employee_id"].toString();
                info.employeeName = obj["employee_name"].toString();
                info.month = obj["month"].toString();
                info.baseSalary = obj["base_salary"].toDouble();
                info.commission = obj["commission"].toDouble();
                info.bonus = obj["bonus"].toDouble();
                info.deduction = obj["deduction"].toDouble();
                info.netPay = obj["net_pay"].toDouble();
                info.status = obj["status"].toString();
                info.payTime = obj["pay_time"].toString();
                info.remark = obj["remark"].toString();
                m_salaries[info.id] = info;
            }
            emit salaryDataChanged();
        }
    } else if (packet.cmdId == Protocol::CMD_VERIFY_PERFORMANCE || 
               packet.cmdId == Protocol::CMD_APPROVE_SALARY || 
               packet.cmdId == Protocol::CMD_PAY_SALARY) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            qDebug() << "[SALARY] Operation success for CMD:" << packet.cmdId;
        }
    }
}

void SalaryDataManager::generateMonthlySalaries(const QString &month) {
    // 该功能通常由服务端自动执行，客户端仅提供触发接口
    QJsonObject obj;
    obj["month"] = month;
    // 假设复用审批接口或新增
    qDebug() << "[SALARY] Triggering monthly salary generation for:" << month;
}
