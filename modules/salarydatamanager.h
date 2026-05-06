#ifndef SALARYDATAMANAGER_H
#define SALARYDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include "../common_types.h"

class SalaryDataManager : public QObject
{
    Q_OBJECT
public:
    static SalaryDataManager* instance();

    // 薪资管理
    QList<SalaryInfo> getSalariesByMonth(const QString &month);
    SalaryInfo getSalary(const QString &id);
    void updateSalary(const SalaryInfo &info);
    void generateMonthlySalaries(const QString &month);
    void approveSalary(const QString &id);
    void paySalary(const QString &id);

    // 业绩提成管理 (后续业绩核销模块会调用)
    void addPerformanceRecord(const PerformanceRecord &record);
    QList<PerformanceRecord> getPerformanceRecords(const QString &month, const QString &employeeId = "");
    void verifyPerformance(const QString &recordId);
    
    // 汇总数据
    struct SalaryStats {
        double totalPayroll;
        int paidCount;
        int pendingCount;
    };
    SalaryStats getStats(const QString &month);

signals:
    void salaryDataChanged();
    void performanceDataChanged();

private:
    explicit SalaryDataManager(QObject *parent = nullptr);
    void initMockData();

    static SalaryDataManager *m_instance;
    
    QMap<QString, SalaryInfo> m_salaries; // Key: SalaryId
    QMap<QString, PerformanceRecord> m_performance; // Key: RecordId
};

#endif // SALARYDATAMANAGER_H
