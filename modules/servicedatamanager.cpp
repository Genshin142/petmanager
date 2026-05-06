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
    auto addMock = [&](const QString &id, const QString &name, const QString &category, double price, int duration, double commFixed, const QString &desc = "", int commPercent = 0) {
        ServiceInfo info;
        info.id = id;
        info.name = name;
        info.category = category;
        info.price = price;
        info.durationMinutes = duration;
        info.commissionFixed = commFixed;
        info.commissionPercent = commPercent;
        info.salesCount = QRandomGenerator::global()->bounded(100);
        info.isActive = true;
        info.description = desc;
        m_services.insert(id, info);
    };

    addMock("SVR-1001", "深度洗护", "洗护", 158.0, 60, 0, "采用高品质精油及深度清洁配方，包含精油SPA、毛发深层滋养、指甲修剪及全套基础护理。", 20);
    addMock("SVR-1101", "基础洗护", "洗护", 50.0, 30, 10.0, "包含基础清洁洗澡、吹干及基础梳毛。");
    addMock("SVR-1201", "精修造型", "美容", 150.0, 90, 0, "全身毛发设计与修剪，包含造型设计与基础打理。", 30);
    addMock("SVR-1102", "剪指甲", "洗护", 20.0, 10, 5.0);
    addMock("SVR-1103", "清理耳道", "洗护", 15.0, 10, 5.0);
    addMock("SVR-1104", "肛门腺清理", "洗护", 20.0, 10, 5.0);
    addMock("SVR-1105", "脚底毛修剪", "洗护", 15.0, 10, 5.0);
    addMock("SVR-1106", "洁牙护理", "洗护", 30.0, 15, 10.0);

    // 美容类
    addMock("SVR-1201", "整体造型", "美容", 150.0, 90, 40.0, "全身毛发设计与修剪，包含造型设计与基础打理。");
    addMock("SVR-1202", "局部修剪", "美容", 60.0, 30, 20.0, "针对脸部、脚底、生殖器周边等局部区域的精细修剪。");
    addMock("SVR-1203", "深度护理", "美容", 120.0, 45, 30.0, "深层毛发修复，改善毛发干枯打结，增加毛发光泽。");
    addMock("SVR-1204", "开结处理", "美容", 50.0, 30, 20.0, "针对长毛宠物严重打结的专业理顺处理，按打结程度收费。");
    addMock("SVR-1205", "去底绒", "美容", 80.0, 40, 30.0, "换季期间针对双层毛宠物的底绒清理，有效减少家里掉毛。");

    // 保健类
    addMock("SVR-1301", "健康筛查", "保健", 50.0, 20, 10.0, "包含称重、皮肤检查、耳道观察等基础健康状态记录。");
    addMock("SVR-1302", "体外驱虫", "保健", 60.0, 10, 10.0, "涂抹或喷洒预防跳蚤、蜱虫等外部寄生虫药物。");
    addMock("SVR-1303", "体内驱虫", "保健", 40.0, 5, 5.0, "喂食肠道寄生虫驱避药物。");
    addMock("SVR-1304", "伤口护理", "保健", 30.0, 15, 10.0, "针对轻微擦伤或剪甲出血的简单消毒与包扎。");

    // 接送类
    addMock("SVR-1501", "单程接宠", "接送", 30.0, 30, 5.0, "单程前往客户家中接回宠物。");
    addMock("SVR-1502", "单程送宠", "接送", 30.0, 30, 5.0, "单程将宠物送回客户家中。");
    addMock("SVR-1503", "往返接送", "接送", 50.0, 60, 10.0, "包含接店加送回的全流程服务。");
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
