#ifndef SALARYMODULE_H
#define SALARYMODULE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

class SalaryModule : public QWidget
{
    Q_OBJECT
public:
    explicit SalaryModule(QWidget *parent = nullptr);

private:
    void setupUI();
    void initData();
    // 数据模型
    struct SalaryRecord {
        QString empId;
        QString name;
        double baseSalary;
        double commission;
        QString status;
    };
    QList<SalaryRecord> m_salaryData;

    // 分页组件
    int m_currentPage = 1;
    int m_pageSize = 10;
    QLabel *pageLabel;
    QLineEdit *jumpEdit;
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    class QIntValidator *jumpValidator;

    void addSalaryRow(const SalaryRecord &record);
    void updatePagination();
    void updateStats();

    QTableWidget *salaryTable;
    QComboBox *employeeCombo;
    QComboBox *yearCombo;
    QComboBox *monthCombo;
    QLabel *totalPaidLabel;
    QLabel *pendingLabel;

private slots:
    void onFilter();
    void onExport();
    
    // 分页与批量事件
    void onPrevPage();
    void onNextPage();
    void onJumpPage();
    void onBatchPay();
    void onPaySingle();
    void onRecalculateSingle(); // 单个重新核算
};

#endif // SALARYMODULE_H
