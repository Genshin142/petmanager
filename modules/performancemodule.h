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
    
private slots:
    void onFilter();

private:
    QTableWidget *perfTable;
    QLabel *totalRevenueLabel;
    QLabel *serviceRevenueLabel;
    QLabel *productRevenueLabel;
    QLabel *totalCommLabel;


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
