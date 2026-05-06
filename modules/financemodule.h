#ifndef FINANCEMODULE_H
#define FINANCEMODULE_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>

class PerformanceModule;
class SalaryModule;

class FinanceModule : public QWidget
{
    Q_OBJECT
public:
    explicit FinanceModule(QWidget *parent = nullptr);

private slots:
    void onTabChanged(int index);

private:
    void setupUI();

    QStackedWidget *m_stack;
    PerformanceModule *m_perfModule;
    SalaryModule *m_salaryModule;

    QPushButton *m_perfTabBtn;
    QPushButton *m_salaryTabBtn;
};

#endif // FINANCEMODULE_H
