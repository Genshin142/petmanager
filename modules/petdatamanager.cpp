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
        } else {
            addLog(yesterday + " 08:30", "投喂", "🍖", QString("[%1] 早饭光盘行动。").arg(name), "A-01", "王波");
            addLog(yesterday + " 17:00", "离店", "👋", QString("[%1] 主人已准时接走，本次寄养结束。").arg(name), "A-01", "店长");
        }
        m_activityLogs[info.id] = logs;

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
        }
        // 模拟两条历史记录
        batches << FosterBatch{"B-HIS-1", "2026-03-01", "2026-03-10", false};
        batches << FosterBatch{"B-HIS-2", "2026-02-10", "2026-02-20", false};
        m_historyBatches[info.id] = batches;
    };

    addDemo("P1001", "团团", "猫", "波斯猫", "母", "3岁", "在店", "M001", "张三", "已接种(三联)", "无", "不吃禽类，对深海鱼油成分有轻微排斥反应", "103", "persian.png");
    addDemo("P1002", "豆豆", "狗", "柴犬", "公", "1岁", "寄养中", "M002", "李芳", "已接种", "曾患严重真菌性皮肤病（已痊愈）", "常规饮食", "101", "golden.png");
    addDemo("P1003", "小雪", "猫", "布偶", "母", "2岁", "洗护中", "M003", "王波", "已接种", "无", "建议喂食兔肉或鸭肉配方", "102", "snow.jpg");
    addDemo("P1004", "可乐", "狗", "金毛", "公", "4岁", "待寄养", "M004", "赵四", "未接种", "后腿关节有轻微陈旧性劳损", "常规饮食", "105", "balu.jpg");
    addDemo("P1005", "芝麻", "猫", "英短", "公", "1岁", "待寄养", "M005", "陈志勇", "已接种", "无", "喜欢吃冻干", "108", "persian.png");
    addDemo("P1006", "包子", "狗", "法斗", "母", "2岁", "寄养中", "M006", "孙美玲", "已接种", "呼吸道较短，注意温控", "少食多餐", "106", "snow.jpg");
    addDemo("P1007", "咖啡", "狗", "泰迪", "公", "5岁", "待寄养", "M007", "周杰", "已接种", "无", "常规饮食", "112", "golden.png");
    addDemo("P1008", "糯米", "猫", "加菲猫", "母", "2岁", "在店", "M008", "吴静", "已接种", "鼻泪管易堵塞，需每日擦拭", "常规饮食", "115", "persian.png");
    addDemo("P1005", "咪咪", "猫", "银渐层", "母", "2岁", "离店", "M002", "李芳", "已接种", "幼年时期曾患猫鼻支，季节交替时易出现打喷嚏症状", "常规饮食", "", "siamese.png");
    addDemo("P1006", "旺财", "狗", "金毛犬", "公", "4岁", "洗护中", "M003", "王五", "已接种", "无", "严重鸡肉过敏，禁止喂食含鸡油、鸡肉粉的任何零食", "106", "golden.png");
    addDemo("P1007", "小雪", "狗", "萨摩耶", "母", "2岁", "寄养中", "M004", "赵六", "已接种", "无", "肠胃敏感，仅限喂食皇家处方粮，严禁乱喂路边草木", "107", "husky.png");
    addDemo("P1008", "可可", "狗", "泰迪", "母", "3岁", "离店", "M004", "赵六", "已接种", "耳道结构复杂，容易滋生耳螨，建议每周使用耳漂清洁", "常规饮食", "", "siamese.png");
    addDemo("P1009", "大黑", "狗", "拉布拉多", "公", "1岁", "离店", "M005", "孙七", "已接种", "暂无病史", "贪吃容易暴饮暴食，需配合慢食盆控制进食速度", "", "husky.png");
    addDemo("P1010", "皮皮", "狗", "柯基", "公", "2岁", "洗护中", "M006", "周八", "已接种", "脊椎压力较大，洗护时请勿长时间保持站立或倒立姿势", "常规饮食", "110", "golden.png");
    addDemo("P1011", "球球", "猫", "英短蓝猫", "母", "2岁", "离店", "M007", "吴九", "已接种", "无", "常规饮食", "", "persian.png");
    addDemo("P1012", "花花", "猫", "加菲猫", "母", "4岁", "离店", "M007", "吴九", "已接种", "鼻腔较短，运动后呼吸音较重，需注意降温避暑", "常规饮食", "", "siamese.png");
    addDemo("P1013", "布丁", "猫", "金渐层", "公", "2岁", "寄养中", "M008", "钱十", "已接种", "曾有泌尿系统结石史，需每日保证饮水量，严禁喂食高钙零食", "仅限处方粮", "113", "load_img.jpg");
    addDemo("P1014", "奥利奥", "狗", "边牧", "公", "3岁", "在店", "M009", "陈十一", "已接种", "无", "对人工色素敏感，禁止喂食彩色狗粮，建议添加煮熟的西兰花", "114", "load_img.jpg");
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
    Q_UNUSED(start); Q_UNUSED(end);
    QString roomStr = QString::number(roomId);
    for (const auto &pet : m_pets) {
        if (pet.status == "寄养中" && pet.roomNo == roomStr) {
            return false;
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

    PetActivityLog log;
    log.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
    log.type = "检查";
    log.icon = "⚖️";
    log.remark = QString("办理入住 Room %1。起始体重：%2 kg。备注：%3").arg(roomId).arg(weight).arg(note.isEmpty() ? "无" : note);
    log.operatorName = "系统管理员";
    log.roomNo = QString::number(roomId);
    addActivityLog(petId, log);

    notifyGlobalDataChanged();
    notifyPetDataChanged(petId);
}
