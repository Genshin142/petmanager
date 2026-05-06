#include "salarydatamanager.h"
#include "staffdatamanager.h"
#include "servicedatamanager.h"
#include <QDateTime>
#include <QRandomGenerator>

SalaryDataManager* SalaryDataManager::m_instance = nullptr;

SalaryDataManager* SalaryDataManager::instance() {
    if (!m_instance) m_instance = new SalaryDataManager();
    return m_instance;
}

SalaryDataManager::SalaryDataManager(QObject *parent) : QObject(parent) {
    initMockData();
}

void SalaryDataManager::initMockData() {
    QString currentMonth = QDate::currentDate().toString("yyyy-MM");
    QString lastMonth = QDate::currentDate().addMonths(-1).toString("yyyy-MM");
    
    auto staff = StaffDataManager::instance()->allStaff();
    
    // 生成上个月的数据 (已发放)
    for (const auto &s : staff) {
        SalaryInfo info;
        info.employeeId = s.id;
        info.employeeName = s.name;
        info.month = lastMonth;
        info.id = "SAL-" + lastMonth.mid(0, 4) + lastMonth.mid(5, 2) + "-" + s.id;
        info.baseSalary = s.baseSalary;
        info.commission = QRandomGenerator::global()->bounded(1000, 5000);
        info.bonus = QRandomGenerator::global()->bounded(200, 1000);
        info.deduction = 0;
        info.netPay = info.baseSalary + info.commission + info.bonus - info.deduction;
        info.status = "已发放";
        info.payTime = lastMonth + "-10 10:00";
        m_salaries[info.id] = info;

        // 生成一些业绩记录 (基于服务管理模块的真实配置)
        auto services = ServiceDataManager::instance()->activeServices();
        if (services.isEmpty()) {
            // 如果服务模块没初始化，则初始化之
            ServiceDataManager::instance();
            services = ServiceDataManager::instance()->activeServices();
        }

        int recordCount = QRandomGenerator::global()->bounded(3, 8);
        for (int i = 0; i < recordCount; ++i) {
            if (services.isEmpty()) break;
            const auto &svc = services[QRandomGenerator::global()->bounded(services.size())];

            PerformanceRecord pr;
            pr.id = "PERF-" + QString::number(QDateTime::currentMSecsSinceEpoch()) + QString::number(i) + s.id;
            pr.employeeId = s.id;
            pr.employeeName = s.name;
            pr.serviceDate = lastMonth + "-" + QString::number(QRandomGenerator::global()->bounded(1, 28)).rightJustified(2, '0');
            pr.serviceName = svc.name;
            pr.orderAmount = svc.price;
            pr.finalAmount = pr.orderAmount * (QRandomGenerator::global()->bounded(90, 101) / 100.0); // 随机 9-10 折
            
            // 严格对齐服务模块的设计
            if (svc.commissionFixed > 0) {
                pr.commissionType = "固定提成";
                pr.commissionRate = svc.commissionFixed;
                pr.commission = svc.commissionFixed;
            } else {
                pr.commissionType = "比例提成";
                pr.commissionRate = svc.commissionPercent / 100.0;
                pr.commission = pr.finalAmount * pr.commissionRate;
            }

            pr.orderId = "ORD202404" + QString::number(1000 + QRandomGenerator::global()->bounded(0, 999));
            pr.customerId = QString::number(2000 + QRandomGenerator::global()->bounded(1, 100));
            pr.customerName = (i % 3 == 0) ? "陈大文" : ((i % 3 == 1) ? "林小姐" : "张先生");
            pr.payMethod = (i % 2 == 0) ? "会员卡" : "微信支付";
            pr.petId = QString::number(5000 + QRandomGenerator::global()->bounded(1, 100));
            pr.petName = (i % 3 == 0) ? "小白" : ((i % 3 == 1) ? "咪咪" : "旺财");
            pr.petBreed = (i % 2 == 0) ? "布偶猫" : "金毛犬";
            pr.status = "已核销";
            pr.verifyTime = pr.serviceDate + " 18:00";
            m_performance[pr.id] = pr;
        }
    }

    // 生成本月的数据 (待审核)
    for (const auto &s : staff) {
        SalaryInfo info;
        info.employeeId = s.id;
        info.employeeName = s.name;
        info.month = currentMonth;
        info.id = "SAL-" + currentMonth.mid(0, 4) + currentMonth.mid(5, 2) + "-" + s.id;
        info.baseSalary = s.baseSalary;
        info.commission = QRandomGenerator::global()->bounded(500, 3000);
        info.bonus = 0;
        info.deduction = 0;
        info.netPay = info.baseSalary + info.commission + info.bonus - info.deduction;
        info.status = "待审核";
        m_salaries[info.id] = info;
    }
}

QList<SalaryInfo> SalaryDataManager::getSalariesByMonth(const QString &month) {
    QList<SalaryInfo> list;
    for (const auto &s : m_salaries) {
        if (s.month == month) list.append(s);
    }
    return list;
}

SalaryInfo SalaryDataManager::getSalary(const QString &id) {
    return m_salaries.value(id);
}

void SalaryDataManager::updateSalary(const SalaryInfo &info) {
    m_salaries[info.id] = info;
    emit salaryDataChanged();
}

void SalaryDataManager::approveSalary(const QString &id) {
    if (m_salaries.contains(id)) {
        m_salaries[id].status = "待发放";
        emit salaryDataChanged();
    }
}

void SalaryDataManager::paySalary(const QString &id) {
    if (m_salaries.contains(id)) {
        m_salaries[id].status = "已发放";
        m_salaries[id].payTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
        emit salaryDataChanged();
    }
}

SalaryDataManager::SalaryStats SalaryDataManager::getStats(const QString &month) {
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
    m_performance[record.id] = record;
    emit performanceDataChanged();
}

QList<PerformanceRecord> SalaryDataManager::getPerformanceRecords(const QString &month, const QString &employeeId) {
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
    if (m_performance.contains(recordId)) {
        m_performance[recordId].status = "已核销";
        m_performance[recordId].verifyTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
        emit performanceDataChanged();
    }
}

void SalaryDataManager::generateMonthlySalaries(const QString &month) {
    auto staff = StaffDataManager::instance()->allStaff();
    for (const auto &s : staff) {
        QString id = "SAL-" + month.mid(0, 4) + month.mid(5, 2) + "-" + s.id;
        if (!m_salaries.contains(id)) {
            SalaryInfo info;
            info.id = id;
            info.employeeId = s.id;
            info.employeeName = s.name;
            info.month = month;
            info.baseSalary = s.baseSalary;
            info.status = "待审核";
            // 这里以后可以加入真实的业绩汇总逻辑
            m_salaries[id] = info;
        }
    }
    emit salaryDataChanged();
}
