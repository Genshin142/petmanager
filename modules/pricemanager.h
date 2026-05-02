#ifndef PRICEMANAGER_H
#define PRICEMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>

class PriceManager : public QObject
{
    Q_OBJECT
public:
    static PriceManager* instance();

    // 核心接口
    double getBasePrice(const QString &serviceName);
    double getSizeFactor(const QString &breed); // 根据品种获取体型系数
    double calculateFinalAmount(const QString &serviceName, const QString &breed, double addons = 0.0);

private:
    explicit PriceManager(QObject *parent = nullptr);
    static PriceManager *m_instance;

    QMap<QString, double> m_servicePrices;
    QMap<QString, double> m_breedSizeFactors;
};

#endif // PRICEMANAGER_H
