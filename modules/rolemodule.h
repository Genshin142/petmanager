#ifndef ROLEMODULE_H
#define ROLEMODULE_H

#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>

class RoleModule : public QWidget
{
    Q_OBJECT
public:
    explicit RoleModule(QWidget *parent = nullptr);
private:
    void setupUI();
    void addEmployeeRow(const QString &id, const QString &name, const QString &role, const QString &status, 
                        const QString &gender, int age, const QString &phone, const QString &email, const QString &idCard,
                        double baseSalary, double performance, double commission);
    void updateStats();
    
private slots:
    void onAddEmployee();
    void onEditEmployee();
    void onDeleteEmployee();

private:
    QTableWidget *empTable;
    QLineEdit *searchEdit;

    // 统计卡片数值与趋势标签
    QLabel *totalEmpLabel;
    QLabel *todayAttendLabel;
    QLabel *attendRateLabel;
    QLabel *totalSalaryLabel;
    QLabel *totalSalaryTrend;
};

#endif // ROLEMODULE_H
