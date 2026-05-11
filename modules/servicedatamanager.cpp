#include "servicedatamanager.h"
#include <QDateTime>
#include <QRandomGenerator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "../utils/networkmanager.h"

ServiceDataManager* ServiceDataManager::m_instance = nullptr;

ServiceDataManager* ServiceDataManager::instance()
{
    if (!m_instance) {
        m_instance = new ServiceDataManager();
    }
    return m_instance;
}

ServiceDataManager::ServiceDataManager(QObject *parent) : QObject(parent)
{
    connect(&NetworkManager::instance(), &NetworkManager::packetReceived, 
            this, &ServiceDataManager::onPacketReceived);
            
    // 初始加载
    requestServiceList();
}

void ServiceDataManager::initMockData()
{
    // Mock data is now handled by server
}

void ServiceDataManager::requestServiceList()
{
    if (!m_services.isEmpty()) {
        emit serviceDataChanged();
        return;
    }
    if (m_isLoading) return;
    m_isLoading = true;
    qDebug() << "[SERVICE] Requesting service list from server...";
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_SERVICE_LIST, QJsonObject());
}

void ServiceDataManager::onPacketReceived(const Protocol::NetPacket &packet)
{
    if (packet.cmdId == Protocol::CMD_GET_SERVICE_LIST) {
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray arr = root["data"].toArray();
            m_services.clear();
            
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject obj = arr[i].toObject();
                ServiceInfo info;
                info.id = obj["id"].toString();
                info.name = obj["name"].toString();
                info.category = obj["category"].toString();
                info.price = obj["price"].toDouble();
                info.durationMinutes = obj["durationMinutes"].toInt();
                info.commissionFixed = obj["commissionFixed"].toDouble();
                info.description = obj["description"].toString();
                info.icon = obj["iconPath"].toString();
                info.isActive = obj["isActive"].toBool();
                
                m_services[info.id] = info;
            }
            
            qDebug() << "[SERVICE] Service list updated from server. Count: " << m_services.size();
            m_isLoading = false;
            emit serviceDataChanged();
        } else {
            m_isLoading = false;
        }
    } else if (packet.cmdId == Protocol::CMD_ADD_SERVICE || 
               packet.cmdId == Protocol::CMD_UPDATE_SERVICE || 
               packet.cmdId == Protocol::CMD_DELETE_SERVICE) {
        
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            requestServiceList();
        }
    }
}

QList<ServiceInfo> ServiceDataManager::allServices() const
{
    return m_services.values();
}

QList<ServiceInfo> ServiceDataManager::activeServices() const
{
    QList<ServiceInfo> list;
    for (const auto &info : m_services) {
        if (info.isActive) list.append(info);
    }
    return list;
}

ServiceInfo ServiceDataManager::getService(const QString &id) const
{
    return m_services.value(id);
}

void ServiceDataManager::addService(const ServiceInfo &info)
{
    QJsonObject obj;
    obj["name"] = info.name;
    obj["category"] = info.category;
    obj["price"] = info.price;
    obj["durationMinutes"] = info.durationMinutes;
    obj["commissionFixed"] = info.commissionFixed;
    obj["description"] = info.description;
    obj["iconPath"] = info.icon;
    obj["isActive"] = info.isActive;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_ADD_SERVICE, obj);
}

void ServiceDataManager::updateService(const ServiceInfo &info)
{
    QJsonObject obj;
    obj["id"] = info.id;
    obj["name"] = info.name;
    obj["category"] = info.category;
    obj["price"] = info.price;
    obj["durationMinutes"] = info.durationMinutes;
    obj["commissionFixed"] = info.commissionFixed;
    obj["description"] = info.description;
    obj["iconPath"] = info.icon;
    obj["isActive"] = info.isActive;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_SERVICE, obj);
}

void ServiceDataManager::removeService(const QString &id)
{
    QJsonObject obj;
    obj["id"] = id;
    NetworkManager::instance().sendRequest(Protocol::CMD_DELETE_SERVICE, obj);
}

QList<ServiceInfo> ServiceDataManager::searchServices(const QString &keyword) const
{
    QList<ServiceInfo> results;
    QString kw = keyword.toLower();
    for (const auto &info : m_services) {
        if (info.name.toLower().contains(kw) || info.id.toLower().contains(kw)) {
            results.append(info);
        }
    }
    return results;
}
