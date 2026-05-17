#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QElapsedTimer>
#include "common_types.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPushButton;
class QVBoxLayout;

class MemberModule;
class RoleModule;
class PetModule;
class FosterModule;
class AppointmentModule;
class CheckoutModule;
class StatsModule;
class FinanceModule;
class LogisticsModule;
class InboundModule;
class ServiceManagementModule;
class PersonalModule;
class ScheduleModule;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(UserRole role, QString userName, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNavClicked(int id);
    void onJumpToPetRequested(const QString &memberName, const QString &petName);
    void onJumpToPetById(const QString &petId);
    void updateBadges();

private:
    Ui::MainWindow *ui;
    QButtonGroup *navGroup;
    
    // 模块指针
    MemberModule *memberMod;
    RoleModule *roleMod;
    PetModule *petMod;
    FosterModule *fosterMod;
    AppointmentModule *apptMod;
    CheckoutModule *checkoutMod;
    StatsModule *statsMod;
    FinanceModule *financeMod;
    LogisticsModule *logisticsMod;
    InboundModule *inboundMod;
    QPushButton *navFinance;
    QPushButton *navLogistics;
    QPushButton *navInbound;
    QPushButton *navService;
    ServiceManagementModule *serviceMod;
    PersonalModule *personalMod;
    ScheduleModule *scheduleMod;
    QPushButton *navPersonal;
    QPushButton *navSchedule;
    
    
    void initSidebar();
    void initModules(UserRole role);
    QWidget* loadModule(int index, UserRole role);
    void replacePlaceholder(int index, QWidget *newWidget);
    void preloadNextModule(int index, UserRole role);
    bool isModuleLoaded(int index) const;
    QWidget* getModulePointer(int index) const;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    UserRole m_role;
    QString m_userName;
    
    QElapsedTimer m_startupTimer;
    bool m_firstPaint = true;
};
#endif // MAINWINDOW_H
