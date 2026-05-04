#ifndef SERVICEDATAMANAGER_H
#define SERVICEDATAMANAGER_H

#include <QObject>
#include <QMap>
#include "../common_types.h"

class ServiceDataManager : public QObject
{
    Q_OBJECT
public:
    static ServiceDataManager* instance();
    
    QList<ServiceInfo> allServices() const;
    QList<ServiceInfo> activeServices() const;
    ServiceInfo getService(const QString &id) const;
    void addService(const ServiceInfo &info);
    void updateService(const ServiceInfo &info);
    void removeService(const QString &id);
    
    // 联想搜索接口
    QList<ServiceInfo> searchServices(const QString &keyword) const;
    
signals:
    void serviceDataChanged();

private:
    explicit ServiceDataManager(QObject *parent = nullptr);
    void initMockData();

    static ServiceDataManager *m_instance;
    QMap<QString, ServiceInfo> m_services;
};

#endif // SERVICEDATAMANAGER_H
