#include "productdatamanager.h"
#include <QDate>
#include <QSet>

ProductDataManager* ProductDataManager::m_instance = nullptr;

ProductDataManager* ProductDataManager::instance()
{
    if (!m_instance) {
        m_instance = new ProductDataManager();
    }
    return m_instance;
}

ProductDataManager::ProductDataManager(QObject *parent) : QObject(parent)
{
    initMockData();
}

void ProductDataManager::initMockData()
{
    // 1. 皇家基础全价猫粮 (食品类 - 深度参数)
    ProductInfo p1;
    p1.barcode = "690123456789";
    p1.name = "皇家基础全价猫粮 2kg";
    p1.brand = "Royal Canin";
    p1.origin = "法国/国产";
    p1.category = "主粮";
    p1.spec = "2kg/袋";
    p1.price = 199.00;
    p1.costPrice = 135.50;
    p1.stock = 5;
    p1.minStock = 10;
    p1.productionDate = "2026-01-10";
    p1.shelfLifeDays = 365;
    p1.supplier = "皇家宠物食品有限公司";
    p1.supplierPhone = "400-888-1234";
    p1.description = "针对成猫设计的均衡营养配方。";
    p1.ingredients = "鲜鸡肉(25%), 脱水禽肉, 糙米, 动物脂肪, 玉米蛋白粉";
    p1.storageReq = "避光、干燥、密封保存";
    p1.suitablePets = "成年猫通用，适合肠胃敏感猫咪";
    p1.pairingSuggestion = "常与：皇家主食湿粮包（成猫用）搭配购买";
    

    // 营养成分映射
    p1.nutritionMap["粗蛋白质"] = "≥30.0%";
    p1.nutritionMap["粗脂肪"] = "≥12.0%";
    p1.nutritionMap["粗纤维"] = "≤5.0%";
    p1.nutritionMap["水分"] = "≤10.0%";
    p1.nutritionMap["钙"] = "≥1.0%";
    p1.nutritionMap["总磷"] = "≥0.8%";
    p1.nutritionMap["钠(Na)"] = "0.35g/100g";
    p1.nutritionMap["碳水化合物"] = "28.5g/100g";
    p1.nutritionMap["总糖"] = "1.2g/100g";
    
    p1.images << "C:/Users/任坤/.gemini/antigravity/brain/908a4409-240b-4746-839e-aaa28ce0281c/royal_canin_cat_food_front_1777203204254.png" 
              << "C:/Users/任坤/.gemini/antigravity/brain/908a4409-240b-4746-839e-aaa28ce0281c/royal_canin_cat_food_back_1777203216188.png" 
              << "C:/Users/任坤/.gemini/antigravity/brain/908a4409-240b-4746-839e-aaa28ce0281c/royal_canin_cat_food_detail_1777203229320.png";
    m_products[p1.barcode] = p1;

    // 2. 小鲜肉混合猫砂 (用品类 - 基础参数)
    ProductInfo p2;
    p2.barcode = "690987654321";
    p2.name = "小鲜肉混合猫砂 6L";
    p2.brand = "小鲜肉";
    p2.origin = "山东";
    p2.category = "洗护";
    p2.spec = "6L/包";
    p2.price = 35.00;
    p2.costPrice = 18.00;
    p2.stock = 42;
    p2.minStock = 20;
    p2.productionDate = "2026-03-01";
    p2.shelfLifeDays = 730;
    p2.supplier = "中宠贸易实业";
    p2.supplierPhone = "010-66668888";
    p2.description = "极速吸水，长效除臭，粉尘极低。";
    p2.ingredients = "豌豆纤维, 玉米淀粉, 瓜尔胶, 钠基膨润土";
    p2.storageReq = "常温干燥保存，避免受潮";
    p2.suitablePets = "全阶段猫咪适用（特别推荐长毛猫）";
    p2.pairingSuggestion = "常与：猫砂除臭珠、一次性猫砂盆垫搭配购买";
    

    // 用品类分析数据
    p2.nutritionMap["吸水率"] = "≥400%";
    p2.nutritionMap["粉尘率"] = "≤0.5%";
    p2.nutritionMap["结团直径"] = "2-3mm";
    p2.nutritionMap["除臭率"] = "≥90%";
    
    p2.images << "C:/Users/任坤/.gemini/antigravity/brain/908a4409-240b-4746-839e-aaa28ce0281c/cat_litter_package_front_1777203241934.png"
              << "C:/Users/任坤/.gemini/antigravity/brain/908a4409-240b-4746-839e-aaa28ce0281c/cat_litter_texture_detail_1777203254848.png";
    m_products[p2.barcode] = p2;

    // 3. 渴望六种鱼猫粮 (食品类 - 高端主粮)
    ProductInfo p3;
    p3.barcode = "999888777666";
    p3.name = "渴望六种鱼猫粮 1.8kg";
    p3.brand = "Orijen";
    p3.origin = "加拿大";
    p3.category = "主粮";
    p3.spec = "1.8kg/袋";
    p3.price = 560.00;
    p3.costPrice = 280.00;
    p3.stock = 50;
    p3.minStock = 5;
    p3.productionDate = "2024-05-01";
    p3.shelfLifeDays = 540;
    p3.supplier = "渴望中国总代理";
    p3.supplierPhone = "021-12345678";
    p3.description = "含85%的高品质鱼类成分，为猫咪提供丰富的欧米茄脂肪酸。";
    p3.ingredients = "新鲜完整太平洋沙丁鱼, 新鲜完整太平洋鳕鱼, 新鲜完整太平洋鲭鱼...";
    p3.images << "C:/Users/任坤/.gemini/antigravity/brain/908a4409-240b-4746-839e-aaa28ce0281c/orijen_six_fish_front_1777384375226.png"
              << "C:/Users/任坤/.gemini/antigravity/brain/908a4409-240b-4746-839e-aaa28ce0281c/orijen_six_fish_back_1777384397331.png";
    p3.isActive = true;
    m_products[p3.barcode] = p3;
    
    // 初始化一些模拟记录 - 使用商品资料中的真实图片
    StockInRecord r1;
    r1.dateTime = QDateTime::currentDateTime().addDays(-2).toString("yyyy-MM-dd HH:mm:ss");
    r1.productName = "皇家基础全价猫粮 2kg";
    r1.barcode = "690123456789";
    r1.spec = "2kg/袋";
    r1.origin = "法国/皇家";
    r1.category = "主食";
    r1.quantity = 10;
    r1.costPrice = 85.5;
    r1.productionDate = "2024-01-01";
    r1.supplier = "皇家宠物食品有限公司";
    r1.supplierPhone = "400-888-1234";
    r1.operatorName = "店长admin";
    r1.imgPaths = p1.images; // 直接引用商品资料图片
    r1.shelfLifeDays = p1.shelfLifeDays;

    r1.isShelved = true;
    
    StockInRecord r2;
    r2.dateTime = QDateTime::currentDateTime().addDays(-5).toString("yyyy-MM-dd HH:mm:ss");
    r2.productName = "小鲜肉混合猫砂 6L";
    r2.barcode = "690987654321";
    r2.spec = "6L/袋";
    r2.origin = "江苏/小宠";
    r2.category = "洗护";
    r2.quantity = 50;
    r2.costPrice = 12.0;
    r2.productionDate = "2024-02-15";
    r2.supplier = "中宠贸易实业";
    r2.supplierPhone = "010-66668888";
    r2.operatorName = "营业员staff";
    r2.imgPaths = p2.images; 
    r2.shelfLifeDays = p2.shelfLifeDays;
    r2.isShelved = true;

    // 新增记录：相同条码，不同生产日期
    StockInRecord r3;
    r3.dateTime = QDateTime::currentDateTime().addDays(-1).toString("yyyy-MM-dd HH:mm:ss");
    r3.productName = "皇家基础全价猫粮 2kg";
    r3.barcode = "690123456789";
    r3.spec = "2kg/袋";
    r3.origin = "法国/皇家";
    r3.category = "主食";
    r3.quantity = 25;
    r3.costPrice = 88.0;
    r3.productionDate = "2024-03-20"; // 新批次
    r3.supplier = "皇家宠物食品有限公司";
    r3.supplierPhone = "400-888-1234";
    r3.operatorName = "店长admin";
    r3.imgPaths = p1.images;
    r3.shelfLifeDays = p1.shelfLifeDays;
    r3.isShelved = true;

    StockInRecord r4;
    r4.dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    r4.productName = "小鲜肉混合猫砂 6L";
    r4.barcode = "690987654321";
    r4.spec = "6L/袋";
    r4.origin = "江苏/小宠";
    r4.category = "洗护";
    r4.quantity = 100;
    r4.costPrice = 11.5;
    r4.productionDate = "2024-04-01"; 
    r4.supplier = "中宠贸易实业";
    r4.supplierPhone = "010-66668888";
    r4.operatorName = "仓库主管";
    r4.imgPaths = p2.images;
    r4.shelfLifeDays = p2.shelfLifeDays;
    r4.isShelved = true;

    // 完全未上架的新商品（用于测试上架列表）
    StockInRecord r5;
    r5.dateTime = QDateTime::currentDateTime().addDays(-1).toString("yyyy-MM-dd HH:mm:ss");
    r5.productName = "渴望六种鱼猫粮 1.8kg";
    r5.barcode = "999888777666";
    r5.spec = "1.8kg/袋";
    r5.origin = "加拿大/渴望";
    r5.category = "主食";
    r5.quantity = 20;
    r5.costPrice = 280.0;
    r5.productionDate = "2024-05-01";
    r5.supplier = "渴望中国总代理";
    r5.supplierPhone = "021-12345678";
    r5.operatorName = "店长admin";
    r5.shelfLifeDays = p3.shelfLifeDays;
    r5.isShelved = false; // 初始未上架
    r5.imgPaths << "C:/Users/任坤/.gemini/antigravity/brain/908a4409-240b-4746-839e-aaa28ce0281c/orijen_six_fish_front_1777384375226.png"
                << "C:/Users/任坤/.gemini/antigravity/brain/908a4409-240b-4746-839e-aaa28ce0281c/orijen_six_fish_back_1777384397331.png";

    StockInRecord r6;
    r6.dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    r6.productName = "渴望六种鱼猫粮 1.8kg";
    r6.barcode = "999888777666";
    r6.spec = "1.8kg/袋";
    r6.origin = "加拿大/渴望";
    r6.category = "主食";
    r6.quantity = 30;
    r6.costPrice = 285.0;
    r6.productionDate = "2024-05-15";
    r6.supplier = "渴望中国总代理";
    r6.supplierPhone = "021-12345678";
    r6.operatorName = "店长admin";
    r6.shelfLifeDays = p3.shelfLifeDays;
    r6.isShelved = false; // 初始未上架
    r6.imgPaths = r5.imgPaths;

    m_records << r1 << r2 << r3 << r4 << r5 << r6;
}

QList<ProductInfo> ProductDataManager::allProducts() const
{
    return m_products.values();
}

ProductInfo ProductDataManager::getProduct(const QString &barcode) const
{
    return m_products.value(barcode);
}

void ProductDataManager::addProduct(const ProductInfo &info)
{
    m_products[info.barcode] = info;
    emit productDataChanged();
}

void ProductDataManager::updateProduct(const ProductInfo &info)
{
    m_products[info.barcode] = info;
    emit productDataChanged();
}

void ProductDataManager::removeProduct(const QString &barcode)
{
    m_products.remove(barcode);
    emit productDataChanged();
}

QList<ProductInfo> ProductDataManager::getLowStockItems() const
{
    QList<ProductInfo> list;
    for (const auto &p : m_products) {
        if (p.stock <= p.minStock) {
            list.append(p);
        }
    }
    return list;
}

QList<StockInRecord> ProductDataManager::getAllRecords() const {
    return m_records;
}

void ProductDataManager::addRecord(const StockInRecord &rec) {
    m_records.prepend(rec); // 最新的排在前面
    emit productDataChanged();
}

void ProductDataManager::updateRecord(const QString &oldDateTime, const QString &barcode, const StockInRecord &newRec) {
    for (auto &r : m_records) {
        if (r.dateTime == oldDateTime && r.barcode == barcode) {
            r = newRec;
            // 保持原有的 dateTime 还是更新？通常“编辑资料”不应该改动入库时间，除非用户明确要求。
            // 这里我们保持原有的 dateTime 以维持记录唯一性（或者使用 newRec 的，但 setEditMode 时我们保持了原有的）
            r.dateTime = oldDateTime; 
            break;
        }
    }
    emit productDataChanged();
}

QList<StockInRecord> ProductDataManager::getUnlistedInboundItems() const {
    QList<StockInRecord> unlisted;
    QSet<QString> listedBarcodes;
    for (const auto &p : m_products) {
        if (p.isActive) listedBarcodes.insert(p.barcode);
    }
    
    QSet<QString> processedKeys; 
    for (const auto &r : m_records) {
        // 如果该记录已经上架，或者该条码已经有正式档案且我们只想要新档案列表（根据业务定）
        // 既然用户想要区分批次，那么只要 rec.isShelved 为 false，就应该是“待上架”
        if (!r.isShelved) {
            QString key = r.barcode + r.productionDate;
            if (!processedKeys.contains(key)) {
                unlisted.append(r);
                processedKeys.insert(key);
            }
        }
    }
    return unlisted;
}

void ProductDataManager::markRecordAsShelved(const QString &barcode, const QString &productionDate) {
    bool changed = false;
    for (auto &r : m_records) {
        if (r.barcode == barcode && r.productionDate == productionDate) {
            r.isShelved = true;
            changed = true;
        }
    }
    if (changed) emit productDataChanged();
}

void ProductDataManager::addBatch(const StockBatch &batch) {
    m_batches.prepend(batch);
    // 同时更新商品的汇总库存
    if (m_products.contains(batch.barcode)) {
        m_products[batch.barcode].stock = calculateTotalStock(batch.barcode);
    }
    emit productDataChanged();
}

QList<StockBatch> ProductDataManager::getBatchesForProduct(const QString &barcode) const {
    QList<StockBatch> result;
    for (const auto &b : m_batches) {
        if (b.barcode == barcode && b.currentQty > 0) {
            result.append(b);
        }
    }
    return result;
}

QList<StockBatch> ProductDataManager::getAllBatches() const {
    return m_batches;
}

int ProductDataManager::calculateTotalStock(const QString &barcode) const {
    int total = 0;
    for (const auto &b : m_batches) {
        if (b.barcode == barcode) {
            total += b.currentQty;
        }
    }
    return total;
}

void ProductDataManager::removeRecord(const QString &dateTime, const QString &barcode) {
    for (auto &r : m_records) {
        if (r.dateTime == dateTime && r.barcode == barcode) {
            r.isActive = false;
            emit productDataChanged();
            break;
        }
    }
}

void ProductDataManager::restoreRecord(const QString &dateTime, const QString &barcode) {
    for (auto &r : m_records) {
        if (r.dateTime == dateTime && r.barcode == barcode) {
            r.isActive = true;
            emit productDataChanged();
            break;
        }
    }
}

void ProductDataManager::hardDeleteRecord(const QString &dateTime, const QString &barcode) {
    for (int i = 0; i < m_records.size(); ++i) {
        if (m_records[i].dateTime == dateTime && m_records[i].barcode == barcode) {
            m_records.removeAt(i);
            // 彻底删除逻辑：如果该条码对应的商品已存在于档案中，也将其删除
            if (m_products.contains(barcode)) {
                m_products.remove(barcode);
            }
            emit productDataChanged();
            break;
        }
    }
}
