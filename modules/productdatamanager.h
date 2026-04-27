#ifndef PRODUCTDATAMANAGER_H
#define PRODUCTDATAMANAGER_H

#include <QObject>
#include <QMap>
#include "../common_types.h"

class ProductDataManager : public QObject
{
    Q_OBJECT
public:
    static ProductDataManager* instance();
    
    QList<ProductInfo> allProducts() const;
    ProductInfo getProduct(const QString &barcode) const;
    void addProduct(const ProductInfo &info);
    void updateProduct(const ProductInfo &info);
    void removeProduct(const QString &barcode);
    QList<ProductInfo> getLowStockItems() const;

signals:
    void productDataChanged();

private:
    explicit ProductDataManager(QObject *parent = nullptr);
    void initMockData();

    static ProductDataManager *m_instance;
    QMap<QString, ProductInfo> m_products;
};

#endif // PRODUCTDATAMANAGER_H
