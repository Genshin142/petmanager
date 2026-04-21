#include "petdatamanager.h"
#include <QDebug>
#include <QDate>
#include <QRandomGenerator>

PetDataManager* PetDataManager::m_instance = nullptr;

PetDataManager* PetDataManager::instance()
{
    if (!m_instance) {
        m_instance = new PetDataManager();
    }
    return m_instance;
}

PetDataManager::PetDataManager(QObject *parent)
    : QObject(parent)
{
    initMockData();
}

void PetDataManager::initMockData()
{
    auto addDemo = [&](const QString &id, const QString &name, const QString &species, const QString &breed, const QString &gender, const QString &age, const QString &status, const QString &ownerId, const QString &ownerName, 
                       const QString &vaccine = "已接种", const QString &medical = "无", const QString &diet = "常规饮食", const QString &roomNo = "", const QString &avatar = "") {
        PetInfo info;
        info.id = id; info.name = name; info.species = species; info.breed = breed; 
        info.gender = gender; info.age = age; info.status = status;
        info.ownerId = ownerId; info.ownerName = ownerName;
        info.health = "健康"; 
        info.medicalHistory = medical; 
        info.vaccine = vaccine;
        info.dietary = diet;
        info.weight = 10.0 + (QRandomGenerator::global()->bounded(150) / 10.0); // 随机 10-25kg
        info.joinTime = "2026-03-10";
        info.roomNo = roomNo;
        if (!avatar.isEmpty()) {
            info.avatarPath = "images/pets/" + avatar;
        } else {
            info.avatarPath = ":/images/load_img.jpg";
        }
        if (status == "寄养中" || status == "洗护中" || status == "在店") {
            info.fosterStartTime = QDate::currentDate().addDays(-10).toString("yyyy-MM-dd");
            info.fosterEndTime = QDate::currentDate().addDays(5).toString("yyyy-MM-dd");
        } else if (status == "待寄养" || status == "已预约") {
            info.fosterStartTime = QDate::currentDate().addDays(1).toString("yyyy-MM-dd");
            info.fosterEndTime = QDate::currentDate().addDays(8).toString("yyyy-MM-dd");
        }
        m_pets[info.id] = info;

        QList<PetActivityLog> logs;
        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        QString yesterday = QDate::currentDate().addDays(-1).toString("yyyy-MM-dd");

        auto addLog = [&](const QString &time, const QString &type, const QString &icon, const QString &remark, const QString &room = "", const QString &opName = "店员小利") {
            PetActivityLog log;
            log.time = time; log.type = type; log.icon = icon; log.remark = remark;
            log.isAlert = (type == "异常");
            log.operatorName = opName;
            log.roomNo = room;
            logs.append(log);
        };

        if (status == "寄养中") {
            addLog(today + " 09:30", "投喂", "🍖", QString("[%1] 早饭吃得很干净，精神头很足。").arg(name), info.roomNo, "王波");
            addLog(today + " 14:20", "检查", "🩺", QString("[%1] 体温 38.5℃，心率正常，状态非常健康。").arg(name), info.roomNo, "店员小利");
            addLog(yesterday + " 10:15", "洗护", "🛁", QString("[%1] 进行了深层除臭洗护，毛发亮泽。").arg(name), info.roomNo, "张师傅");
            addLog(yesterday + " 18:00", "备注", "📝", QString("[%1] 和其他小伙伴在操场玩得很开心。").arg(name), info.roomNo, "王波");
        } else if (status == "洗护中") {
            addLog(today + " 16:10", "洗护", "🛁", QString("[%1] 正在进行基础清洁，目前正在吹干毛发。").arg(name), info.roomNo, "张师傅");
            addLog(yesterday + " 11:30", "检查", "🩺", QString("[%1] 入店检查：体表无寄生虫，皮肤状况良好。").arg(name), info.roomNo, "店员小利");
        } else if (status == "在店") {
            addLog(yesterday + " 08:30", "投喂", "🍖", QString("[%1] 早饭光盘行动。").arg(name), "A-01", "王波");
            addLog(yesterday + " 17:00", "备注", "👋", QString("[%1] 状态良好。").arg(name), "A-01", "店长");
        }
        m_activityLogs[info.id] = logs;
        
        // ... (疫苗与影像保持不变)

        QList<VaccineRecord> vaccines;
        if (vaccine != "未接种") {
            vaccines.append({"猫三联 (第一针)", "2026-01-10", "2027-01-10"});
            vaccines.append({"猫三联 (第二针)", "2026-02-01", "2027-02-01"});
            if (species == "狗") {
                vaccines.append({"狂犬疫苗", "2026-02-15", "2027-02-15"});
            }
        }
        m_vaccineRecords[info.id] = vaccines;

        QList<PetMedia> medias;
        if (name == "小雪" && species == "猫") {
            medias << PetMedia{QStringList{":/images/foster_bath.png"}, "image", "洗澡", QStringList{"2026-04-18 14:20"}}
                   << PetMedia{QStringList{":/images/foster_bath_2.png"}, "image", "洗澡", QStringList{"2026-04-15 10:15"}}
                   << PetMedia{QStringList{":/images/foster_outdoor.png"}, "image", "运动", QStringList{"2026-04-20 09:30"}}
                   << PetMedia{QStringList{":/images/foster_outdoor_2.png"}, "image", "运动", QStringList{"2026-04-19 16:45"}}
                   << PetMedia{QStringList{":/images/foster_sleep.png"}, "image", "睡觉", QStringList{"2026-04-20 13:00"}}
                   << PetMedia{QStringList{":/images/foster_sleep_2.png"}, "image", "睡觉", QStringList{"2026-04-17 11:20"}}
                   << PetMedia{QStringList{":/images/load_img.jpg"}, "image", "喂食", QStringList{"2026-04-20 18:10"}};
        } else if (name == "豆豆" || name == "皮皮") {
            medias << PetMedia{QStringList{":/images/foster_outdoor_3.png"}, "image", "运动", QStringList{"2026-04-20 10:15"}}
                   << PetMedia{QStringList{":/images/foster_outdoor_4.png"}, "image", "运动", QStringList{"2026-04-20 10:20"}}
                   << PetMedia{QStringList{":/images/foster_outdoor_5.png"}, "image", "运动", QStringList{"2026-04-20 10:35"}}
                   << PetMedia{QStringList{":/images/foster_outdoor_6.png"}, "image", "运动", QStringList{"2026-04-20 10:45"}};
        }
        m_petMedia[info.id] = medias;

        QList<FosterBatch> batches;
        // 模拟一条活跃记录
        if (status == "寄养中" || status == "洗护中" || status == "在店") {
            batches << FosterBatch{"B-CUR", info.fosterStartTime, "至今", true};
            // 仅对活跃宠物模拟历史记录，保持预约/待入宠物的“洁净”状态
            batches << FosterBatch{"B-HIS-1", "2026-03-01", "2026-03-10", false};
            batches << FosterBatch{"B-HIS-2", "2026-02-10", "2026-02-20", false};
        }
        m_historyBatches[info.id] = batches;
    };

    addDemo("P1001", "团团", "猫", "波斯猫", "母", "3岁", "寄养中", "M001", "张三", "已接种(三联)", "无", "不吃禽类", "103", "persian.png");
    m_pets["P1001"].fosterStartTime = QDate::currentDate().addDays(-4).toString("yyyy-MM-dd");

    addDemo("P1002", "豆豆", "狗", "柴犬", "公", "1岁", "寄养中", "M002", "李芳", "已接种", "无", "常规饮食", "101", "golden.png");
    m_pets["P1002"].fosterStartTime = QDate::currentDate().addDays(-2).toString("yyyy-MM-dd");

    addDemo("P1003", "小雪", "猫", "布偶", "母", "2岁", "寄养中", "M003", "王波", "已接种", "无", "建议喂食兔肉", "102", "snow.jpg");
    m_pets["P1003"].fosterStartTime = QDate::currentDate().addDays(-13).toString("yyyy-MM-dd");

    addDemo("P1004", "旺财", "狗", "金毛", "公", "4岁", "寄养中", "M004", "赵四", "未接种", "无", "常规饮食", "106", "golden.png");
    m_pets["P1004"].fosterStartTime = QDate::currentDate().addDays(-2).toString("yyyy-MM-dd");

    addDemo("P1005", "芝麻", "猫", "英短", "公", "1岁", "待寄养", "M005", "陈志勇", "已接种", "无", "喜欢吃冻干", "108", "persian.png");
    
    addDemo("P1006", "包子", "狗", "法斗", "母", "2岁", "寄养中", "M006", "孙美玲", "已接种", "无", "少食多餐", "107", "snow.jpg");
    m_pets["P1006"].fosterStartTime = QDate::currentDate().addDays(-4).toString("yyyy-MM-dd");

    addDemo("P1007", "糯米", "猫", "加菲猫", "母", "2岁", "寄养中", "M008", "吴静", "已接种", "无", "常规饮食", "115", "persian.png");
    m_pets["P1007"].fosterStartTime = QDate::currentDate().addDays(-12).toString("yyyy-MM-dd");

    addDemo("P1008", "可可", "狗", "泰迪", "母", "3岁", "寄养中", "M004", "赵六", "已接种", "无", "常规饮食", "104", "siamese.png");
    m_pets["P1008"].fosterStartTime = QDate::currentDate().addDays(-12).toString("yyyy-MM-dd");

    addDemo("P1009", "大黑", "狗", "拉布拉多", "公", "1岁", "离店", "M005", "孙七", "已接种", "无", "常规饮食", "", "husky.png");
    
    addDemo("P1010", "皮皮", "狗", "柯基", "公", "2岁", "寄养中", "M006", "周八", "已接种", "无", "常规饮食", "110", "golden.png");
    m_pets["P1010"].fosterStartTime = QDate::currentDate().addDays(-4).toString("yyyy-MM-dd");

    addDemo("P1011", "球球", "猫", "英短蓝猫", "母", "2岁", "离店", "M007", "吴九", "已接种", "无", "常规饮食", "", "persian.png");
    
    addDemo("P1012", "花花", "猫", "加菲猫", "母", "4岁", "离店", "M007", "吴九", "已接种", "无", "常规饮食", "", "siamese.png");
    
    addDemo("P1013", "布丁", "猫", "金渐层", "公", "2岁", "寄养中", "M008", "钱十", "已接种", "无", "仅限处方粮", "113", "load_img.jpg");
    m_pets["P1013"].fosterStartTime = QDate::currentDate().addDays(-3).toString("yyyy-MM-dd");

    addDemo("P1014", "奥利奥", "狗", "边牧", "公", "3岁", "寄养中", "M009", "陈十一", "已接种", "无", "常规饮食", "114", "load_img.jpg");
    m_pets["P1014"].fosterStartTime = QDate::currentDate().addDays(-10).toString("yyyy-MM-dd");
}

void PetDataManager::updatePet(const PetInfo &info)
{
    m_pets[info.id] = info;
    emit petDataChanged(info.id);
    emit globalDataChanged();
}

PetInfo PetDataManager::getPet(const QString &id) const
{
    return m_pets.value(id);
}

QList<PetInfo> PetDataManager::allPets() const
{
    return m_pets.values();
}

void PetDataManager::addPet(const PetInfo &info)
{
    m_pets[info.id] = info;
    emit globalDataChanged();
}

void PetDataManager::removePet(const QString &id)
{
    m_pets.remove(id);
    m_activityLogs.remove(id);
    m_petMedia.remove(id);
    m_vaccineRecords.remove(id);
    emit globalDataChanged();
}

void PetDataManager::notifyGlobalDataChanged()
{
    emit globalDataChanged();
}

void PetDataManager::notifyPetDataChanged(const QString &petId)
{
    emit petDataChanged(petId);
}

void PetDataManager::addActivityLog(const QString &petId, const PetActivityLog &log)
{
    m_activityLogs[petId].append(log);
    emit petDataChanged(petId);
}

QList<PetActivityLog> PetDataManager::getLogs(const QString &petId) const
{
    return m_activityLogs.value(petId);
}

void PetDataManager::addMedia(const QString &petId, const PetMedia &media)
{
    m_petMedia[petId].append(media);
    emit petDataChanged(petId);
}

QList<PetMedia> PetDataManager::getMedia(const QString &petId) const
{
    return m_petMedia.value(petId);
}

void PetDataManager::updateVaccines(const QString &petId, const QList<VaccineRecord> &records)
{
    m_vaccineRecords[petId] = records;
    emit petDataChanged(petId);
}

QList<VaccineRecord> PetDataManager::getVaccines(const QString &petId) const
{
    return m_vaccineRecords.value(petId);
}

QList<FosterBatch> PetDataManager::getHistoryBatches(const QString &petId) const {
    return m_historyBatches.value(petId);
}

bool PetDataManager::isRoomAvailable(int roomId, const QDate &start, const QDate &end) const {
    QString roomStr = QString::number(roomId);
    
    for (const auto &pet : m_pets) {
        // 仅对“寄养中”或“已预约”的宠物进行时间重叠校验
        if (pet.roomNo == roomStr && (pet.status == "寄养中" || pet.status == "已预约")) {
            QDate stayStart = QDate::fromString(pet.fosterStartTime, "yyyy-MM-dd");
            QString endStr = pet.fosterEndTime;
            QDate stayEnd;
            
            if (endStr == "至今" || endStr.isEmpty()) {
                // 如果是“至今”，视为一个非常远的日期，或者如果是“寄养中”，至少涵盖到今天
                stayEnd = QDate::currentDate().addYears(1); 
            } else {
                stayEnd = QDate::fromString(endStr, "yyyy-MM-dd");
            }

            // 严谨的区间重叠判定：(StartA <= EndB) && (EndA >= StartB)
            if (start <= stayEnd && end >= stayStart) {
                return false; // 存在冲突
            }
        }
    }
    return true;
}

void PetDataManager::executeCheckIn(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight, const QString &note) {
    if (!m_pets.contains(petId)) return;

    PetInfo &info = m_pets[petId];
    info.status = "寄养中";
    info.roomNo = QString::number(roomId);
    info.fosterStartTime = start.toString("yyyy-MM-dd");
    info.fosterEndTime = end.toString("yyyy-MM-dd");
    info.weight = weight;

    // 开启新篇章：如果是正式办理入住，清空之前的模拟动态，确保“刚入住没有记录”
    m_activityLogs[petId].clear();
    
    // 创建实时批次记录，确保它出现在列表首位
    FosterBatch batch{"B-CUR-" + QDateTime::currentDateTime().toString("yyyyMMdd"), start.toString("yyyy-MM-dd"), "至今", true};
    m_historyBatches[petId].prepend(batch);

    notifyGlobalDataChanged();
    notifyPetDataChanged(petId);
}

void PetDataManager::executeBooking(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight) {
    if (!m_pets.contains(petId)) return;

    PetInfo &info = m_pets[petId];
    info.status = "已预约";
    info.roomNo = QString::number(roomId);
    info.fosterStartTime = start.toString("yyyy-MM-dd");
    info.fosterEndTime = end.toString("yyyy-MM-dd");
    info.weight = weight;

    // 清空历史模拟动态，确保预约时时间轴是干净的
    m_activityLogs[petId].clear();

    // 创建预约批次记录
    FosterBatch batch{"B-BOOK-" + QDateTime::currentDateTime().toString("yyyyMMdd"), start.toString("yyyy-MM-dd"), end.toString("yyyy-MM-dd"), true};
    m_historyBatches[petId].prepend(batch);

    notifyGlobalDataChanged();
    notifyPetDataChanged(petId);
}
