#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MemberModule;
class RoleModule;
class PetModule;
class ProductModule;
class FosterModule;
class OrderModule;
class StatsModule;

enum UserRole {
    ADMIN,
    STAFF
};

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
    OrderModule *orderMod;
    StatsModule *statsMod;

    void initSidebar();
    void initModules();
};
#endif // MAINWINDOW_H
