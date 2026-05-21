#ifndef SALARYDATAMANAGER_H
#define SALARYDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QRecursiveMutex>
#include "../common_types.h"

namespace Protocol {
    struct NetPacket;
}

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

    void addPerformanceRecord(const PerformanceRecord &record);
    QList<PerformanceRecord> getPerformanceRecords(const QString &month, const QString &employeeId = "");
    void verifyPerformance(const QString &recordId);
    
    // 网络请求
    void requestPerformanceRecords(const QString &month, const QString &employeeId = "");
    void requestSalaries(const QString &month);
    
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
    void financeRefreshRequested(); // 👈 实时同步刷新信号

private:
    explicit SalaryDataManager(QObject *parent = nullptr);
    void initMockData();

private slots:
    void onPacketReceived(const Protocol::NetPacket &packet);

private:
    static SalaryDataManager *m_instance;
    
    QMap<QString, SalaryInfo> m_salaries; // Key: SalaryId
    QMap<QString, PerformanceRecord> m_performance; // Key: RecordId
    mutable QRecursiveMutex m_mutex;
};

#endif // SALARYDATAMANAGER_H
