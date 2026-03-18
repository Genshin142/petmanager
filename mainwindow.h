#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include "common_types.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPushButton;

class MemberModule;
class RoleModule;
class PetModule;
class ProductModule;
class FosterModule;
class AppointmentModule;
class CheckoutModule;
class StatsModule;
class PerformanceModule;
class SalaryModule;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(UserRole role, QString userName, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNavClicked(int id);

private:
    Ui::MainWindow *ui;
    QButtonGroup *navGroup;
    
    // 模块指针
    MemberModule *memberMod;
    RoleModule *roleMod;
    PetModule *petMod;
    ProductModule *productMod;
    FosterModule *fosterMod;
    AppointmentModule *apptMod;
    CheckoutModule *checkoutMod;
    StatsModule *statsMod;
    PerformanceModule *perfMod;
    SalaryModule *salaryMod;
    QPushButton *navSalary;

    void initSidebar();
    void initModules(UserRole role);
    UserRole m_role;
};
#endif // MAINWINDOW_H
