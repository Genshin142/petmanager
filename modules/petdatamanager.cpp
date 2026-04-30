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
    auto addDemo = [&](const QString &id, const QString &name, const QString &species, const QString &breed, const QString &gender, const QString &age, const QString &status, const QString &ownerId, const QString &ownerName, const QString &ownerPhone, 
                       const QString &vaccine = "已接种", const QString &medical = "无", const QString &diet = "常规饮食", const QString &roomNo = "", const QString &avatar = "") {
        PetInfo info;
        info.id = id; info.name = name; info.species = species; info.breed = breed; 
        info.gender = gender; info.age = age; info.status = status;
        info.ownerId = ownerId; info.ownerName = ownerName; info.ownerPhone = ownerPhone;
        info.health = "健康"; 
        info.medicalHistory = medical; 
        info.vaccine = vaccine;
        info.dietary = diet;
        info.weight = 10.0 + (QRandomGenerator::global()->bounded(150) / 10.0); // 随机 10-25kg
        info.joinTime = "2026-03-10";
        info.roomNo = roomNo;
        if (!avatar.isEmpty()) {
            info.avatarPath = ":/images/" + avatar;
        } else {
            info.avatarPath = ":/images/load_img.jpg";
        }
        if (status.contains("寄养中") || status.contains("洗护中") || status.contains("在店")) {
            info.fosterStartTime = QDate::currentDate().addDays(-10).toString("yyyy-MM-dd");
            info.fosterEndTime = QDate::currentDate().addDays(5).toString("yyyy-MM-dd");
        } else if (status.contains("待入店") || status.contains("预约") || status.contains("在家")) {
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

        if (status.contains("寄养中")) {
            addLog(today + " 09:30", "投喂", "", "早饭吃得很干净，精神头很足。", info.roomNo, "王波");
            addLog(today + " 14:20", "检查", "", "体温 38.5℃，心率正常，状态非常健康。", info.roomNo, "店员小利");
            addLog(yesterday + " 10:15", "洗护", "", "进行了深层除臭洗护，毛发亮泽。", info.roomNo, "张师傅");
            addLog(yesterday + " 18:00", "备注", "", "和其他小伙伴在操场玩得很开心。", info.roomNo, "王波");
        } else if (status.contains("洗护中")) {
            addLog(today + " 16:10", "洗护", "", "正在进行基础清洁，目前正在吹干毛发。", info.roomNo, "张师傅");
            addLog(yesterday + " 11:30", "检查", "", "入店检查：体表无寄生虫，皮肤状况良好。", info.roomNo, "店员小利");
        } else if (status.contains("在店")) {
            addLog(yesterday + " 08:30", "投喂", "", "早饭光盘行动。", "A-01", "王波");
            addLog(yesterday + " 17:00", "备注", "", "状态良好。", "A-01", "店长");
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
            medias << PetMedia{QStringList{":/images/foster_bath_new.png"}, "image", "洗澡", QStringList{"2026-04-18 14:20"}}
                   << PetMedia{QStringList{":/images/foster_bath_2.png"}, "image", "洗澡", QStringList{"2026-04-15 10:15"}}
                   << PetMedia{QStringList{":/images/foster_outdoor_new.png"}, "image", "运动", QStringList{"2026-04-20 09:30"}}
                   << PetMedia{QStringList{":/images/foster_outdoor_2.png"}, "image", "运动", QStringList{"2026-04-19 16:45"}}
                   << PetMedia{QStringList{":/images/foster_sleep_new.png"}, "image", "睡觉", QStringList{"2026-04-20 13:00"}}
                   << PetMedia{QStringList{":/images/foster_sleep_2.png"}, "image", "睡觉", QStringList{"2026-04-17 11:20"}}
                   << PetMedia{QStringList{":/images/load_img.jpg"}, "image", "喂食", QStringList{"2026-04-20 18:10"}};
        } else if (name == "豆豆" || name == "皮皮") {
            medias << PetMedia{QStringList{":/images/foster_outdoor_3.png"}, "image", "运动", QStringList{"2026-04-20 10:15"}}
                   << PetMedia{QStringList{":/images/foster_outdoor_4.png"}, "image", "运动", QStringList{"2026-04-20 10:20"}}
                   << PetMedia{QStringList{":/images/foster_outdoor_5.png"}, "image", "运动", QStringList{"2026-04-20 10:35"}}
                   << PetMedia{QStringList{":/images/foster_outdoor_6.png"}, "image", "运动", QStringList{"2026-04-20 10:45"}};
        }
        
        // 增强：为所有有寄养历史或当前在店的宠物添加“入住/离店存证”
        if (status == "寄养中" || status == "洗护中" || status == "在店" || status == "离店") {
            medias << PetMedia{QStringList{":/images/foster_bath_2.png"}, "image", "入住存证", QStringList{info.fosterStartTime + " 09:00"}};
            if (status == "离店") {
                medias << PetMedia{QStringList{":/images/foster_outdoor_3.png"}, "image", "离店存证", QStringList{QDate::currentDate().toString("yyyy-MM-dd") + " 17:30"}};
            }
            // 为往期记录添加存证 (模拟)
            medias << PetMedia{QStringList{":/images/load_img.jpg"}, "image", "入住存证", QStringList{"2026-03-01 10:15"}};
            medias << PetMedia{QStringList{":/images/load_img.jpg"}, "image", "离店存证", QStringList{"2026-03-10 16:40"}};

            // 专门为 2-10 到 2-20 这段模拟历史添加照片
            medias << PetMedia{QStringList{":/images/foster_sleep_2.png"}, "image", "睡觉", QStringList{"2026-02-12 13:00"}};
            medias << PetMedia{QStringList{":/images/foster_outdoor_4.png"}, "image", "运动", QStringList{"2026-02-15 15:00"}};
            medias << PetMedia{QStringList{":/images/foster_bath.png"}, "image", "入住存证", QStringList{"2026-02-10 10:00"}};
            medias << PetMedia{QStringList{":/images/foster_outdoor_6.png"}, "image", "离店存证", QStringList{"2026-02-20 16:00"}};
        }

        m_petMedia[info.id] = medias;

        QList<FosterBatch> batches;
        // 模拟一条活跃记录
        if (status.contains("在店") || status.contains("寄养中") || status.contains("洗护中")) {
            batches << FosterBatch{"B-CUR", info.fosterStartTime, "至今", true};
            // 仅对活跃宠物模拟历史记录，保持预约/待入宠物的“洁净”状态
            batches << FosterBatch{"B-HIS-1", "2026-03-01", "2026-03-10", false};
            batches << FosterBatch{"B-HIS-2", "2026-02-10", "2026-02-20", false};
        }
        m_historyBatches[info.id] = batches;
    };

    addDemo("P1001", "团团", "猫", "波斯猫", "母", "3岁", "在店 (寄养中)", "M001", "张三", "13800138001", "已接种(三联)", "无", "不吃禽类", "103", "persian.png");
    m_pets["P1001"].fosterStartTime = QDate::currentDate().addDays(-4).toString("yyyy-MM-dd");

    addDemo("P1002", "豆豆", "狗", "柴犬", "公", "1岁", "在店 (寄养中)", "M002", "李芳", "13911112222", "已接种", "无", "常规饮食", "101", "shiba.png");
    m_pets["P1002"].fosterStartTime = QDate::currentDate().addDays(-2).toString("yyyy-MM-dd");

    addDemo("P1003", "小雪", "猫", "布偶", "母", "2岁", "在店 (寄养中)", "M003", "王波", "13688889999", "已接种", "无", "建议喂食兔肉", "102", "ragdoll.png");
    m_pets["P1003"].fosterStartTime = QDate::currentDate().addDays(-13).toString("yyyy-MM-dd");

    addDemo("P1004", "旺财", "狗", "金毛", "公", "4岁", "在店 (寄养中)", "M004", "赵四", "13500001111", "未接种", "无", "常规饮食", "106", "golden.png");
    m_pets["P1004"].fosterStartTime = QDate::currentDate().addDays(-2).toString("yyyy-MM-dd");

    addDemo("P1005", "芝麻", "猫", "英短", "公", "1岁", "待入店 (在家)", "M005", "陈志勇", "13755556666", "已接种", "无", "喜欢吃冻干", "108", "british.png");
    
    addDemo("P1006", "包子", "狗", "法斗", "母", "2岁", "在店 (寄养中)", "M006", "孙美玲", "18822223333", "已接种", "无", "少食多餐", "107", "french.png");
    m_pets["P1006"].fosterStartTime = QDate::currentDate().addDays(-4).toString("yyyy-MM-dd");

    addDemo("P1007", "糯米", "猫", "加菲猫", "母", "2岁", "在店 (寄养中)", "M008", "吴静", "13144445555", "已接种", "无", "常规饮食", "115", "garfield.png");
    m_pets["P1007"].fosterStartTime = QDate::currentDate().addDays(-12).toString("yyyy-MM-dd");

    addDemo("P1008", "可可", "狗", "泰迪", "母", "3岁", "接送中 (在途)", "M004", "赵六", "13366667777", "已接种", "无", "常规饮食", "109", "teddy.png");
    m_pets["P1008"].fosterStartTime = QDate::currentDate().toString("yyyy-MM-dd");

    addDemo("P1009", "大黑", "狗", "拉布拉多", "公", "1岁", "已离店 (回家)", "M005", "孙七", "18900001111", "已接种", "无", "常规饮食", "", "labrador.png");
    
    addDemo("P1010", "皮皮", "狗", "柯基", "公", "2岁", "在店 (寄养中)", "M006", "周八", "13011112222", "已接种", "无", "常规饮食", "110", "corgi.png");
    m_pets["P1010"].fosterStartTime = QDate::currentDate().addDays(-4).toString("yyyy-MM-dd");

    addDemo("P1011", "球球", "猫", "英短蓝猫", "母", "2岁", "已离店 (回家)", "M007", "吴九", "13299998888", "已接种", "无", "常规饮食", "", "british.png");
    
    addDemo("P1012", "花花", "猫", "加菲猫", "母", "4岁", "离店", "M007", "吴九", "已接种", "无", "常规饮食", "", "garfield.png");
    
    addDemo("P1013", "布丁", "猫", "金渐层", "公", "2岁", "寄养中", "M008", "钱十", "已接种", "无", "仅限处方粮", "113", "british.png");
    m_pets["P1013"].fosterStartTime = QDate::currentDate().addDays(-3).toString("yyyy-MM-dd");

    addDemo("P1014", "奥利奥", "狗", "边牧", "公", "3岁", "寄养中", "M009", "陈十一", "已接种", "无", "常规饮食", "114", "labrador.png");
    m_pets["P1014"].fosterStartTime = QDate::currentDate().addDays(-10).toString("yyyy-MM-dd");

    // 模拟维护与清洁周期
    addRoomStatusPeriod(105, {"maintenance", QDate::currentDate().toString("yyyy-MM-dd") + " 08:30", QDate::currentDate().addDays(1).toString("yyyy-MM-dd") + " 18:00", "门锁更换与地板抛光"});
    addRoomStatusPeriod(108, {"cleaning", QDate::currentDate().toString("yyyy-MM-dd") + " 09:00", QDate::currentDate().toString("yyyy-MM-dd") + " 12:00", "深度消杀与紫外线杀菌"});
    addRoomStatusPeriod(120, {"maintenance", QDate::currentDate().toString("yyyy-MM-dd") + " 10:00", QDate::currentDate().addDays(2).toString("yyyy-MM-dd") + " 16:00", "墙面补漆与通风维护"});
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

QList<PetInfo> PetDataManager::getPetsByOwner(const QString &ownerId) const
{
    QList<PetInfo> result;
    for (const PetInfo &info : m_pets.values()) {
        if (info.ownerId == ownerId) {
            result.append(info);
        }
    }
    return result;
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
    // 1. 检查宠物占用
    for (const auto &pet : m_pets) {
        // 仅对“寄养中”或“已预约”的宠物进行时间重叠校验
        if (pet.roomNo == roomStr && (pet.status == "寄养中" || pet.status == "已预约")) {
            QDate stayStart = QDate::fromString(pet.fosterStartTime, "yyyy-MM-dd");
            QString endStr = pet.fosterEndTime;
            QDate stayEnd;
            
            if (endStr == "至今" || endStr.isEmpty()) {
                stayEnd = QDate::currentDate().addYears(1); 
            } else {
                stayEnd = QDate::fromString(endStr, "yyyy-MM-dd");
            }

            if (start <= stayEnd && end >= stayStart) return false;
        }
    }

    // 2. 检查维护/清洁独占周期
    if (m_roomStatusPeriods.contains(roomId)) {
        for (const auto &p : m_roomStatusPeriods[roomId]) {
            QDate s = QDate::fromString(p.startTime.split(" ").first(), "yyyy-MM-dd");
            QDate e = QDate::fromString(p.endTime.split(" ").first(), "yyyy-MM-dd");
            if (start <= e && end >= s) return false;
        }
    }
    return true;
}

void PetDataManager::addRoomStatusPeriod(int roomId, const RoomStatusPeriod &period) {
    m_roomStatusPeriods[roomId].append(period);
    emit globalDataChanged();
}

void PetDataManager::removeRoomStatusPeriod(int roomId, const QString &type) {
    if (m_roomStatusPeriods.contains(roomId)) {
        if (type.isEmpty()) {
            m_roomStatusPeriods.remove(roomId);
        } else {
            QList<RoomStatusPeriod> &list = m_roomStatusPeriods[roomId];
            for (int i = list.size() - 1; i >= 0; --i) {
                if (list[i].type == type) list.removeAt(i);
            }
            if (list.isEmpty()) m_roomStatusPeriods.remove(roomId);
        }
        emit globalDataChanged();
    }
}

QList<RoomStatusPeriod> PetDataManager::getRoomStatusPeriods(int roomId) const {
    return m_roomStatusPeriods.value(roomId);
}

void PetDataManager::executeCheckIn(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight, const QString &note) {
    Q_UNUSED(note);
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
    
    // 预约入住同样不应有寄养记录
    m_activityLogs[petId].clear();

    notifyGlobalDataChanged();
    notifyPetDataChanged(petId);
}

void PetDataManager::executeCancelBooking(int roomId, const QString &petId) {
    Q_UNUSED(roomId);
    if (!m_pets.contains(petId)) return;

    PetInfo &info = m_pets[petId];
    info.status = "待寄养";
    info.roomNo = "";
    info.fosterStartTime = "";
    info.fosterEndTime = "";

    // 添加一条取消日志
    PetActivityLog log;
    log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
    log.type = "备注";
    log.icon = "❌";
    log.remark = "客户取消了本次寄养预约。";
    log.operatorName = "系统管理员";
    addActivityLog(petId, log);

    notifyGlobalDataChanged();
    notifyPetDataChanged(petId);
}
