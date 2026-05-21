#include "servicedatamanager.h"
#include "logdatamanager.h"
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

void ServiceDataManager::requestServiceList(bool force)
{
    if (!force && !m_services.isEmpty()) {
        emit serviceDataChanged();
        return;
    }
    if (m_isLoading) return;
    m_isLoading = true;
    qDebug() << "[SERVICE] Requesting service list from server (force:" << force << ")...";
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
                info.salesCount = obj["salesCount"].toInt();
                
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
            requestServiceList(true);
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

    // 记录系统操作日志
    LogDataManager::writeLog("服务分类", "新增服务类别", 
        QString("服务名称: %1, 分类: %2, 价格: %3 元, 时长: %4 分钟")
        .arg(info.name, info.category, QString::number(info.price, 'f', 2), QString::number(info.durationMinutes)));
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

    // 记录系统操作日志
    LogDataManager::writeLog("服务分类", "更新服务类别", 
        QString("服务ID: %1, 名称: %2, 分类: %3, 价格: %4 元, 状态: %5")
        .arg(info.id, info.name, info.category, QString::number(info.price, 'f', 2), info.isActive ? "启用" : "禁用"));
}

void ServiceDataManager::removeService(const QString &id)
{
    ServiceInfo info = getService(id);
    QJsonObject obj;
    obj["id"] = id;
    NetworkManager::instance().sendRequest(Protocol::CMD_DELETE_SERVICE, obj);

    // 记录系统操作日志
    LogDataManager::writeLog("服务分类", "删除服务类别", 
        QString("服务ID: %1, 名称: %2, 分类: %3").arg(id, info.name, info.category));
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
