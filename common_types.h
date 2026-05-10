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

struct BoardingRoom {
    int id;
    QString roomNo;
    QString type;      // "标准房", "豪华房", "多宠房"
    QString status;    // "空闲", "入住中", "清理中", "维护中"
    QString description;
    bool isActive = true;
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
    bool isActive = true;    // 是否处于活跃状态 (逻辑删除)
};

struct ProductInfo {
    QString barcode;
    QString name;
    QString brand;           // 品牌
    QString category;
    QString origin;          // 产地
    QString spec;
    QString unit;            // 单位 (如: 袋, 瓶, 包)
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
    QString imgData;         // 新增：首图 Base64 数据 (用于服务器同步)
    QStringList tags;        // 卖点标签 (如: 低敏, 美毛)
    double netWeightKg = 0.0; // 净重 (用于计算单日喂养成本)
    int dailyFeedingGrams = 0;// 建议每日喂食量 (g)
    bool isActive = true;    // 是否在架 (档案管理专用)
    bool isWarning = false;  // 库存预警标志
};

struct ServiceInfo {
    QString id;
    QString name;
    QString category;        // 洗护, 美容, 寄养, 训练, 医疗, 其他
    double price = 0.0;
    int durationMinutes = 0; // 预计耗时
    double commissionFixed = 0.0; // 服务提成金额
    int salesCount = 0;
    bool isActive = true;
    QString description;
    QString icon;            // 图标路径
};

struct StockInRecord {
    int id;               // 数据库唯一标识
    QString inboundNo;    // 入库单号
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
    QString imgData;      // 新增：Base64 图片数据 (服务器存储)
    int shelfLifeDays;    // 新增：从入库单带出的保质期天数
    bool isShelved = false; // 新增：是否已完成上架入库
    bool isActive = true;   // 是否处于活跃状态 (逻辑删除)
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
Q_DECLARE_METATYPE(ServiceInfo)

struct AppointmentStats {
    int total = 0;
    int grooming = 0;
    int logistics = 0;
    int boardingLoad = 0;
};

Q_DECLARE_METATYPE(AppointmentInfo)
Q_DECLARE_METATYPE(AppointmentStats)

struct PerformanceRecord {
    QString id;
    QString employeeId;
    QString employeeName;
    QString serviceDate;
    QString serviceName;
    double orderAmount = 0.0;
    double commission = 0.0;
    QString status; // "待核销", "已核销"
    QString orderId;
    QString verifyTime;
    
    // 详情补充字段
    QString customerId;
    QString customerName;
    QString payMethod;
    QString petId;
    QString petName;
    QString petBreed;
    double finalAmount = 0.0;
    QString commissionType;      // "比例提成" 或 "固定提成"
    double commissionRate = 0.0; // 提成比例 (0.2) 或 固定金额 (50.0)
};

struct SalaryInfo {
    QString id;
    QString employeeId;
    QString employeeName;
    QString month; // yyyy-MM
    double baseSalary = 0.0;
    double commission = 0.0;
    double bonus = 0.0;
    double deduction = 0.0;
    double netPay = 0.0;
    QString status; // "待审核", "已发放"
    QString payTime;
    QString remark;
};

Q_DECLARE_METATYPE(PerformanceRecord)
Q_DECLARE_METATYPE(SalaryInfo)

struct SysOperationLog {
    QString id;
    QString timestamp;
    QString operatorName;
    QString module;
    QString action;
    QString details;
};

// --- 员工管理基础结构 ---
struct EmployeeInfo {
    QString id;
    QString name;
    QString role;
    QString gender;
    int age;
    QString phone;
    QString email;
    QString idCard;
    int baseSalary;
    QString status;
    QString imgPath;
    
    QString joinDate;
    QString emergencyContact;
    QString emergencyPhone;
    QString address;
    QString education;
    QString department;

    QString username;
    QString password;
};

// --- 排班管理结构 (UIUX Pro Max Design) ---
enum ShiftType {
    SHIFT_OFF = 0,      // 休息 (Off)
    SHIFT_MORNING,      // 早班 (Morning: 09:00 - 18:00)
    SHIFT_EVENING,      // 晚班 (Evening: 13:00 - 22:00)
    SHIFT_CUSTOM        // 自定义班次
};

struct ScheduleInfo {
    QString employeeId;
    QString date;       // yyyy-MM-dd
    ShiftType type;
    QString startTime;  // HH:mm
    QString endTime;    // HH:mm
    QString note;
};

Q_DECLARE_METATYPE(EmployeeInfo)
Q_DECLARE_METATYPE(ScheduleInfo)
Q_DECLARE_METATYPE(SysOperationLog)

#endif // COMMON_TYPES_H
