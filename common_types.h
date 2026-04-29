#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <QString>
#include <QMetaType>
#include <QVariant>
#include <QVariantMap>
#include <QStringList>

enum UserRole {
    ADMIN,
    STAFF
};

struct PetActivityLog {
    QString time;
    QString type;    // "投喂", "洗护", "检查", "备注", "异常"
    QString remark;
    QString icon;    // 用于渲染的图标表情
    bool isAlert = false;    // 是否为健康预警（呕吐、生病等）
    QString operatorName = ""; // 经办人
    QString operatorAvatar = ""; // 经办人头像
    QString roomNo = "";       // 发生时的房号
};

struct VaccineRecord {
    QString type;      // 疫苗种类 (如猫三联、狂犬)
    QString date;      // 接种日期
    QString expiry;    // 有效期至
};

struct FosterBatch {
    QString id;
    QString startTime;
    QString endTime; // 如果在店，则为"至今"
    bool isActive;   // 是否当前寄养中
};

struct RoomStatusPeriod {
    QString type;      // "maintenance", "cleaning"
    QString startTime; // yyyy-MM-dd HH:mm
    QString endTime;   // yyyy-MM-dd HH:mm
    QString reason;
};

struct LogisticsTask {
    QString taskId;          // 唯一任务ID
    QString petId;           // 关联的宠物ID
    QString type;            // "接宠", "送宠"
    QString status;          // "待处理", "进行中", "已送达", "已完成"
    QString driver;
    QString address;
    QString appointmentTime;
    QString relatedModule;   // "Foster", "Grooming" 等
    QString relatedRoomId;   // 寄养目标房号 (如果是寄养)
};

struct PetMedia {
    QStringList urls;
    QString type; // "image", "video"
    QString title;
    QStringList timestamps;
};

struct PetInfo {

    QString id;
    QString name;
    QString species;
    QString breed;
    QString gender;
    QString age;
    QString health;
    QString medicalHistory;
    QString vaccine;
    QString dietary;
    QString status;
    QString joinTime;
    QString ownerId;
    QString ownerName;
    QString ownerPhone;
    QString avatarPath; // 宠物头像路径
    QString roomNo;     // 当前寄养房号 (寄养中显示)
    QString fosterStartTime; // 寄养开始时间
    QString fosterEndTime;   // 寄养结束时间
    double weight = 0.0;     // 当前体重 (kg)
};

struct ProductInfo {
    QString barcode;
    QString name;
    QString brand;           // 品牌
    QString category;
    QString origin;          // 产地
    QString spec;
    double price = 0.0;
    double costPrice = 0.0;  // 成本价 (管理权限可见)
    int stock = 0;
    int minStock = 0;
    QString productionDate;
    int shelfLifeDays = 0;
    QString supplier;
    QString supplierPhone;
    QString description;
    QString ingredients;     // 原料组成 (主要成分)
    QVariantMap nutritionMap;// 营养分析表 (Key-Value)
    QString storageReq;      // 存储要求 (避光/冷藏等)
    QString suitablePets;    // 适用宠物/人群
    QString pairingSuggestion; // 搭配建议
    QStringList images;      // 多角度图片路径
    QStringList tags;        // 卖点标签 (如: 低敏, 美毛)
    double netWeightKg = 0.0; // 净重 (用于计算单日喂养成本)
    int dailyFeedingGrams = 0;// 建议每日喂食量 (g)
    bool isActive = true;    // 是否在架 (档案管理专用)
};

struct StockInRecord {
    QString dateTime;
    QString productName;
    QString barcode;
    QString spec; // 新增：规格
    QString origin; // 新增：产地/品牌
    QString category; // 新增：分类
    int quantity;
    double costPrice; // 新增：进货价
    QString productionDate; // 新增：生产日期
    QString supplier;
    QString supplierPhone; // 新增：供应商联系方式
    QString operatorName;
    QStringList imgPaths; // 修改为多图路径
    int shelfLifeDays;    // 新增：从入库单带出的保质期天数
    bool isShelved = false; // 新增：是否已完成上架入库
};

struct StockBatch {
    QString batchId;         // 批次唯一ID
    QString barcode;         // 关联商品条码
    QString productionDate;  // 生产日期
    int shelfLifeDays;       // 保质期时长 (天)
    QString expiryDate;      // 到期日期 (自动计算)
    int initialQty;          // 初始入库量
    int currentQty;          // 当前剩余量
    QString inboundDate;     // 入库日期
    QString supplier;        // 批次供应商
};

Q_DECLARE_METATYPE(StockBatch)

Q_DECLARE_METATYPE(PetInfo)
Q_DECLARE_METATYPE(ProductInfo)

#endif // COMMON_TYPES_H
