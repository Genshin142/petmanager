#ifndef STAFFDATAMANAGER_H
#define STAFFDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include "addemployeedialog.h"

class StaffDataManager : public QObject
{
    Q_OBJECT
public:
    static StaffDataManager* instance();
    
    QList<EmployeeInfo> allStaff() const;
    EmployeeInfo getStaff(const QString &id) const;
    void addStaff(const EmployeeInfo &info);
    void updateStaff(const EmployeeInfo &info);
    void removeStaff(const QString &id);
    
    QStringList activeStaffNames() const;

signals:
    void staffDataChanged();

private:
    explicit StaffDataManager(QObject *parent = nullptr);
    void initMockData();
    
    static StaffDataManager* m_instance;
    QMap<QString, EmployeeInfo> m_staff;
};

#endif // STAFFDATAMANAGER_H
