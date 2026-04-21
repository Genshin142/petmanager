#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <QString>
#include <QMetaType>

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
    QString avatarPath; // 宠物头像路径
    QString roomNo;     // 当前寄养房号 (寄养中显示)
};

Q_DECLARE_METATYPE(PetInfo)

#endif // COMMON_TYPES_H
