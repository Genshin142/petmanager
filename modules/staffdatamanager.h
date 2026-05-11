#ifndef STAFFDATAMANAGER_H
#define STAFFDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include "addemployeedialog.h"
#include "../protocol_codes.h"
#include <QRecursiveMutex>
#include <QPixmap>
#include <QCache>

class StaffDataManager : public QObject
{
    Q_OBJECT
public:
    static StaffDataManager* instance();
    
    QList<EmployeeInfo> allStaff() const;
    EmployeeInfo getStaff(const QString &id) const;
    void addStaff(const EmployeeInfo &info);
    void updateStaff(const EmployeeInfo &info);
    void removeStaff(const QString &id); // Logical delete: sets status to "离职"
    void restoreStaff(const QString &id);
    void hardDeleteStaff(const QString &id);
    
    QStringList activeStaffNames() const;
    
    void requestStaffList();
    
    QPixmap getStaffPixmap(const QString &id) const;

signals:
    void staffDataChanged();

private:
    explicit StaffDataManager(QObject *parent = nullptr);
    void initMockData();
    
    static StaffDataManager* m_instance;
    QMap<QString, EmployeeInfo> m_staff;
    bool m_isLoading = false;
    mutable QRecursiveMutex m_mutex;
    QString m_cachePath;
    mutable QCache<QString, QPixmap> m_pixmapCache;
    void ensureCacheDir();
    void saveToLocalCache(const QString &id, const QPixmap &pix);

private slots:
    void onPacketReceived(const Protocol::NetPacket &packet);
};

#endif // STAFFDATAMANAGER_H
