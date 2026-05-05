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
    
    // 记录管理
    QList<StockInRecord> getAllRecords() const;
    void addRecord(const StockInRecord &rec);
    void updateRecord(const QString &oldDateTime, const QString &barcode, const StockInRecord &newRec);
    QList<StockInRecord> getUnlistedInboundItems() const;
    void markRecordAsShelved(const QString &barcode, const QString &productionDate);
    void removeRecord(const QString &dateTime, const QString &barcode);
    void restoreRecord(const QString &dateTime, const QString &barcode);
    void hardDeleteRecord(const QString &dateTime, const QString &barcode);
    
    // 批次管理
    void addBatch(const StockBatch &batch);
    QList<StockBatch> getBatchesForProduct(const QString &barcode) const;
    QList<StockBatch> getAllBatches() const;
    int calculateTotalStock(const QString &barcode) const;

signals:
    void productDataChanged();

private:
    explicit ProductDataManager(QObject *parent = nullptr);
    void initMockData();

    static ProductDataManager *m_instance;
    QMap<QString, ProductInfo> m_products;
    QList<StockInRecord> m_records;
    QList<StockBatch> m_batches;
};

#endif // PRODUCTDATAMANAGER_H
