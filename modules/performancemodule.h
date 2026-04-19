#ifndef PERFORMANCEMODULE_H
#define PERFORMANCEMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QButtonGroup>
#include <QPushButton>

class PerformanceModule : public QWidget
{
    Q_OBJECT
public:
    explicit PerformanceModule(QWidget *parent = nullptr);
private:
    void setupUI();
    void addPerformanceRow(const QString &date, const QString &empId, const QString &empName, 
                           const QString &type, double amount, double commission);
    void updateSummary();
    void updateDayCombo(QComboBox* yearCombo, QComboBox* monthCombo, QComboBox* dayCombo);
    void updatePagination();
    
private slots:
    void onFilter();
    void onPrevPage();
    void onNextPage();
    void onJumpPage();
    void onBatchVerify();
    void onVerifySingle();

private:
    QTableWidget *perfTable;
    QLabel *totalRevenueLabel;
    QLabel *serviceRevenueLabel;
    QLabel *productRevenueLabel;
    QLabel *totalCommLabel;

    struct PerfRecord {
        QString date;
        QString empId;
        QString empName;
        QString type;
        double amount;
        double commission;
        bool isVerified; // true=已入账/核销
    };
    QList<PerfRecord> m_perfData;

    // 分页组件
    int m_currentPage = 1;
    int m_pageSize = 10;
    QLabel *pageLabel;
    QLineEdit *jumpEdit;
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    class QIntValidator *jumpValidator;
    // 筛选组件
    QLineEdit *searchEdit;
    
    // 三级联动日期选择 (开始)
    QComboBox *startYearCombo;
    QComboBox *startMonthCombo;
    QComboBox *startDayCombo;
    
    // 三级联动日期选择 (结束)
    QComboBox *endYearCombo;
    QComboBox *endMonthCombo;
    QComboBox *endDayCombo;
};

#endif // PERFORMANCEMODULE_H
