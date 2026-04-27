#include "productdatamanager.h"
#include <QDate>

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
    p1.spec = "袋";
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
    p2.spec = "包";
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
