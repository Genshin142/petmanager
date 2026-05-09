#ifndef PRODUCTDATAMANAGER_H
#define PRODUCTDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include "../common_types.h"
#include "../protocol_codes.h"

class ProductDataManager : public QObject
{
    Q_OBJECT
public:
    static ProductDataManager* instance();
    
    void requestProductList(); // 从服务器拉取商品列表
    void requestInboundList(); // 从服务器拉取入库记录
    void shelveProduct(int inboundId); // 执行上架操作
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
    void inboundListReceived(const QList<StockInRecord> &list);
    void shelveResult(bool success, const QString &message);

private slots:
    void onPacketReceived(const Protocol::NetPacket &packet);

private:
    explicit ProductDataManager(QObject *parent = nullptr);
    void initMockData();

    static ProductDataManager *m_instance;
    QMap<QString, ProductInfo> m_products;
    QList<StockInRecord> m_records;
    QList<StockBatch> m_batches;
    mutable QMutex m_mutex;
};

#endif // PRODUCTDATAMANAGER_H
