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
    
    QTableWidget *salaryTable;
    QComboBox *employeeCombo;
    QComboBox *yearCombo;
    QComboBox *monthCombo;
    QLabel *totalPaidLabel;
    QLabel *pendingLabel;

private slots:
    void onFilter();
    void onExport();
};

#endif // SALARYMODULE_H
