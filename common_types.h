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
    QString relatedAppointmentId; // 关联的预约单ID (新增，用于状态同步)
    double amount = 0.0;     // 新增：接送费
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
    QString address;    // 住址/接送地址
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

struct AppointmentInfo {
    QString id;           // 唯一标识符
    QString groupId;      // 订单组标识符 (关联同时创建的子订单，如接送返程)
    QString memberName;
    QString memberPhone;
    QString petId;        // 宠物ID
    QString petName;      // 宠物名称
    QString breed;        // 品种
    QString petAvatar;    // 宠物头像路径
    QString date;
    QString hour;
    QString type;         // 业务类型 ("Transport", "Grooming", "Beauty", "Boarding")
    QString service;
    QString station;
    QString staff;
    QString status;       // "Pending", "Processing", "Completed", "Canceled"
    QString notes;        // 备注
    QStringList photos;   // 关联影像资料

    // 业务特化字段
    QString address;      // 接送地址
    QString allergy;      // 过敏史/皮肤记录
    int duration = 0;     // 寄养天数
    QString boardingEndDate; // 寄养离店日期
    QString roomNo;       // 房位编号 (用于寄养)
    double amount = 0.0;  // 新增：服务金额
};

struct OrderInfo {
    QString id;               // 订单流水号 (ORD+timestamp)
    QString sourceModule;     // 来源模块 ("Appointment", "Boarding", "Product", "Transport")
    QString relatedId;        // 关联业务单ID
    QString petId;
    QString petName;
    QString memberId;
    QString memberName;
    QString itemDetails;      // 消费项明细
    double totalAmount = 0.0; // 订单总额
    double discount = 1.0;    // 折扣率 (0.0 - 1.0)
    double finalAmount = 0.0; // 实付金额
    QString status;           // "Unpaid" (待支付), "Paid" (已支付), "Cancelled" (已作废)
    QString payMethod;        // "MemberCard", "Cash", "Alipay", "Wechat"
    QString createTime;       // 创建时间
    QString cancelReason;     // 作废原因
    QString operationLog;     // 操作日志
};

Q_DECLARE_METATYPE(OrderInfo)

struct AppointmentStats {
    int total = 0;
    int grooming = 0;
    int logistics = 0;
    int boardingLoad = 0;
};

Q_DECLARE_METATYPE(AppointmentInfo)
Q_DECLARE_METATYPE(AppointmentStats)

#endif // COMMON_TYPES_H
