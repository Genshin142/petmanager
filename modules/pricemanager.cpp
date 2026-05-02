#include "pricemanager.h"
#include <QDebug>

PriceManager* PriceManager::m_instance = nullptr;

PriceManager* PriceManager::instance()
{
    if (!m_instance) {
        m_instance = new PriceManager();
    }
    return m_instance;
}

PriceManager::PriceManager(QObject *parent) : QObject(parent)
{
    // 1. 初始化基础价格 (Base Prices)
    m_servicePrices["专业洗护"] = 0.0;
    m_servicePrices["专业美容"] = 0.0;
    m_servicePrices["医疗驱虫"] = 0.0;
    
    // 3. 具体项目价格 (原子项)
    m_servicePrices["洗澡"] = 80.0;
    m_servicePrices["整体造型"] = 120.0;
    m_servicePrices["局部修剪"] = 60.0;
    m_servicePrices["赛级修剪"] = 300.0;
    m_servicePrices["剪指甲"] = 20.0;
    m_servicePrices["清理耳道"] = 20.0;
    m_servicePrices["肛门腺清理"] = 20.0;
    m_servicePrices["脚底毛修剪"] = 20.0;
    m_servicePrices["洁牙护理"] = 20.0;
    m_servicePrices["香氛喷雾"] = 20.0;
    m_servicePrices["丝滑毛发护理"] = 20.0;
    m_servicePrices["入店接送"] = 30.0;
    m_servicePrices["单次接送"] = 40.0;
    m_servicePrices["标准间"] = 60.0;   // 寄养单价
    m_servicePrices["豪华间"] = 150.0;
    
    // 3. 单项小项价格
    m_servicePrices["单项护理"] = 0.0;
    m_servicePrices["剪指甲"] = 15.0;
    m_servicePrices["清理耳道"] = 15.0;
    m_servicePrices["肛门腺清理"] = 20.0;
    m_servicePrices["脚底毛修剪"] = 20.0;

    // 2. 初始化品种体型系数 (Size Factors)
    // Small (1.0)
    m_breedSizeFactors["英短"] = 1.0;
    m_breedSizeFactors["美短"] = 1.0;
    m_breedSizeFactors["泰迪"] = 1.0;
    m_breedSizeFactors["比熊"] = 1.0;
    m_breedSizeFactors["博美"] = 1.0;
    
    // Medium (1.2)
    m_breedSizeFactors["柯基"] = 1.2;
    m_breedSizeFactors["柴犬"] = 1.2;
    m_breedSizeFactors["法斗"] = 1.2;
    
    // Large (1.5)
    m_breedSizeFactors["金毛"] = 1.5;
    m_breedSizeFactors["拉布拉多"] = 1.5;
    m_breedSizeFactors["萨摩耶"] = 1.5;
    m_breedSizeFactors["哈士奇"] = 1.5;

    // Giant (2.0)
    m_breedSizeFactors["阿拉斯加"] = 2.0;
    m_breedSizeFactors["秋田犬"] = 2.0;
}

double PriceManager::getBasePrice(const QString &serviceName)
{
    // 支持模糊匹配
    for (auto it = m_servicePrices.begin(); it != m_servicePrices.end(); ++it) {
        if (serviceName.contains(it.key())) return it.value();
    }
    return 0.0;
}

double PriceManager::getSizeFactor(const QString &breed)
{
    if (m_breedSizeFactors.contains(breed)) {
        return m_breedSizeFactors[breed];
    }
    return 1.0; // 默认系数
}

double PriceManager::calculateFinalAmount(const QString &serviceName, const QString &breed, double addons)
{
    double base = getBasePrice(serviceName);
    double factor = getSizeFactor(breed);
    return (base * factor) + addons;
}
