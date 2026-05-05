#include "servicedatamanager.h"
#include <QDateTime>
#include <QRandomGenerator>

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
    initMockData();
}

void ServiceDataManager::initMockData()
{
    auto addMock = [&](const QString &id, const QString &name, const QString &category, double price, int duration, double commFixed) {
        ServiceInfo info;
        info.id = id;
        info.name = name;
        info.category = category;
        info.price = price;
        info.durationMinutes = duration;
        info.commissionFixed = commFixed;
        info.commissionPercent = 0;
        info.salesCount = QRandomGenerator::global()->bounded(100);
        info.isActive = true;
        m_services.insert(id, info);
    };

    addMock("SVR-1001", "精油SPA洗浴 (小型犬)", "洗护", 128.0, 60, 20.0);
    addMock("SVR-1002", "精油SPA洗浴 (大型犬)", "洗护", 258.0, 90, 40.0);
    addMock("SVR-1003", "全套美容修剪 (猫咪)", "美容", 198.0, 120, 50.0);
    addMock("SVR-1004", "基础驱虫套餐", "保健", 88.0, 15, 10.0);
    addMock("SVR-1005", "普通房寄养 (单日)", "寄养", 98.0, 1440, 0.0);
    addMock("SVR-1006", "豪华套房寄养 (单日)", "寄养", 188.0, 1440, 0.0);
    addMock("SVR-1007", "同城宠物专车接送", "接送", 50.0, 30, 0.0);

    // --- 同步预约界面的附加项目 ---
    // 洗护类
    addMock("SVR-1101", "洗澡", "洗护", 50.0, 30, 10.0);
    addMock("SVR-1102", "剪指甲", "洗护", 20.0, 10, 5.0);
    addMock("SVR-1103", "清理耳道", "洗护", 15.0, 10, 5.0);
    addMock("SVR-1104", "肛门腺清理", "洗护", 20.0, 10, 5.0);
    addMock("SVR-1105", "脚底毛修剪", "洗护", 15.0, 10, 5.0);
    addMock("SVR-1106", "洁牙护理", "洗护", 30.0, 15, 10.0);

    // 美容类
    addMock("SVR-1201", "整体造型", "美容", 150.0, 90, 40.0);
    addMock("SVR-1202", "局部修剪", "美容", 60.0, 30, 20.0);
    addMock("SVR-1203", "赛级修剪", "美容", 300.0, 150, 80.0);
    addMock("SVR-1204", "香氛喷雾", "美容", 20.0, 5, 0.0);
    addMock("SVR-1205", "丝滑毛发护理", "美容", 80.0, 30, 20.0);

    // 保健类
    addMock("SVR-1301", "基础健康筛查", "保健", 100.0, 20, 30.0);
    addMock("SVR-1302", "体外驱虫", "保健", 60.0, 10, 10.0);

    // 寄养类
    addMock("SVR-1401", "自带口粮", "寄养", 0.0, 0, 0.0);
    addMock("SVR-1402", "加餐(肉罐头)", "寄养", 15.0, 5, 0.0);

    // 接送类
    addMock("SVR-1501", "含航空箱", "接送", 10.0, 0, 0.0);
    addMock("SVR-1502", "上门接宠", "接送", 20.0, 20, 5.0);
    addMock("SVR-1503", "送回入户", "接送", 20.0, 20, 5.0);
    addMock("SVR-1504", "专车专送", "接送", 50.0, 30, 10.0);
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
    m_services.insert(info.id, info);
    emit serviceDataChanged();
}

void ServiceDataManager::updateService(const ServiceInfo &info)
{
    if (m_services.contains(info.id)) {
        m_services[info.id] = info;
        emit serviceDataChanged();
    }
}

void ServiceDataManager::removeService(const QString &id)
{
    if (m_services.remove(id)) {
        emit serviceDataChanged();
    }
}

QList<ServiceInfo> ServiceDataManager::searchServices(const QString &keyword) const
{
    QList<ServiceInfo> list;
    for (const auto &info : m_services) {
        if (info.name.contains(keyword, Qt::CaseInsensitive) || 
            info.category.contains(keyword, Qt::CaseInsensitive)) {
            list.append(info);
        }
    }
    return list;
}
