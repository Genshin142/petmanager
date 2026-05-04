#include "petdatamanager.h"
#include <QDebug>
#include <QDate>
#include <QRandomGenerator>
#include "logisticsmanager.h"

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
    // 1. 核心演示：宠物与客户关系 (Member & Pet)
    auto addDemo = [&](const QString &id, const QString &name, const QString &species, const QString &breed, const QString &gender, const QString &age, const QString &status, const QString &ownerId, const QString &ownerName, const QString &ownerPhone, 
                       const QString &roomNo = "", const QString &avatar = "") {
        PetInfo info;
        info.id = id; info.name = name; info.species = species; info.breed = breed; 
        info.gender = gender; info.age = age; info.status = status;
        info.ownerId = ownerId; info.ownerName = ownerName; info.ownerPhone = ownerPhone;
        info.health = "健康"; info.vaccine = "已接种"; info.medicalHistory = "无";
        info.dietary = "常规饮食"; info.roomNo = roomNo;
        info.weight = 12.5; info.joinTime = "2026-01-15";
        info.avatarPath = avatar.isEmpty() ? ":/images/load_img.jpg" : ":/images/" + avatar;
        
        if (status.contains("寄养中")) {
            info.fosterStartTime = QDate::currentDate().addDays(-3).toString("yyyy-MM-dd");
            info.fosterEndTime = QDate::currentDate().addDays(4).toString("yyyy-MM-dd");
        }
        m_pets[info.id] = info;
    };

    addDemo("P001", "布丁", "狗", "金毛犬", "公", "2岁", "待接走", "M001", "张三", "13800138000", "", "foster_outdoor_new.png");
    addDemo("P002", "芝麻", "猫", "英短蓝猫", "母", "1岁", "寄养中", "M002", "李芳", "13911112222", "A-101", "foster_sleep_new.png");
    addDemo("P003", "豆豆", "狗", "柴犬", "公", "3岁", "在家", "M002", "李芳", "13911112222", "", "foster_bath_new.png");
    addDemo("P004", "小雪", "狗", "萨摩耶", "母", "2岁", "洗护中", "M004", "赵六", "13366667777", "", "load_img.jpg");

    // 2. 模拟预约数据 (Appointment) - 建立业务来源
    auto addAppt = [&](const QString &id, const QString &petId, const QString &type, const QString &status, const QString &time) {
        AppointmentInfo a;
        a.id = id; a.petId = petId; 
        PetInfo pet = m_pets[petId];
        a.petName = pet.name;
        a.memberName = pet.ownerName; a.memberPhone = pet.ownerPhone;
        a.type = type; a.status = status; a.date = QDate::currentDate().toString("yyyy-MM-dd");
        a.hour = time; a.service = (type == "Grooming" ? "精细洗护SPA" : "基础检查");
        m_appointments[id] = a;
    };
    addAppt("APP-2026050201", "P001", "Grooming", "Completed", "09:00");
    addAppt("APP-2026050202", "P004", "Grooming", "Ongoing", "14:30");
    addAppt("APP-2026050203", "P003", "Health", "Pending", "16:00");

    // 3. 模拟订单数据 (Order) - 与预约/寄养深度勾连
    auto addOrd = [&](const QString &id, const QString &petId, const QString &details, double amt, const QString &status, const QString &source, const QString &relatedId) {
        OrderInfo o;
        o.id = id; o.petId = petId; o.petName = m_pets.value(petId).name;
        o.memberId = m_pets.value(petId).ownerId; // Fixed missing member ID
        o.memberName = m_pets.value(petId).ownerName; o.itemDetails = details;
        o.totalAmount = amt; o.finalAmount = (status == "Paid" ? amt : 0);
        o.status = status; o.sourceModule = source; o.relatedId = relatedId;
        o.createTime = QDateTime::currentDateTime().addSecs(-7200).toString("yyyy-MM-dd HH:mm:ss");
        o.payMethod = (status == "Paid" ? "会员卡余额" : "");
        m_orders[id] = o;
    };
    
    // 【布丁】洗护已完成 -> 待支付订单
    addOrd("ORD-10001", "P001", "金毛犬全套精细洗护 + 药浴", 180.0, "Unpaid", "Appointment", "APP-2026050201");
    // 【芝麻】寄养中 -> 已支付的预订金
    addOrd("ORD-10002", "P002", "猫咪寄养预付 (7天)", 350.0, "Paid", "Boarding", "F-20260502");
    // 【今日总览】再加几笔历史数据以填充统计卡片
    addOrd("ORD-10003", "P003", "常规体检", 50.0, "Paid", "Appointment", "APP-2026043001");
    // 使用商品档案中的真实商品名：皇家基础全价猫粮
    addOrd("ORD-10004", "P004", "皇家基础全价猫粮 2kg", 199.0, "Paid", "Product", "");
    // 多商品订单示例：渴望 + 猫砂
    addOrd("ORD-10005", "", "渴望六种鱼猫粮 1.8kg + 小鲜肉混合猫砂 6L", 595.0, "Unpaid", "Product", "");
    // 【新增】到店服务示例
    addOrd("ORD-10006", "P001", "常规剪指甲 (到店)", 15.0, "Paid", "Direct", "");
    addOrd("ORD-10007", "P002", "耳道清理 (到店)", 30.0, "Unpaid", "Direct", "");
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

void PetDataManager::deleteMediaPhoto(const QString &petId, const QString &title, const QString &url)
{
    if (!m_petMedia.contains(petId)) return;
    
    QList<PetMedia> &medias = m_petMedia[petId];
    for (int i = 0; i < medias.size(); ++i) {
        if (medias[i].title == title) {
            int idx = medias[i].urls.indexOf(url);
            if (idx != -1) {
                medias[i].urls.removeAt(idx);
                if (medias[i].timestamps.size() > idx) {
                    medias[i].timestamps.removeAt(idx);
                }
                
                // 如果该组照片全部删完，则移除整组记录
                if (medias[i].urls.isEmpty()) {
                    medias.removeAt(i);
                }
                
                // 同时检查是否有对应的预约单记录，保持同步
                for (auto &appt : m_appointments) {
                    if (appt.petId == petId && (appt.service + "服务记录") == title) {
                        appt.photos.removeAll(url);
                        break;
                    }
                }
                
                emit petDataChanged(petId);
                return;
            }
        }
    }
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

bool PetDataManager::isRoomAvailable(int roomId, const QDate &start, const QDate &end) const {
    if (!start.isValid() || !end.isValid() || start > end) return false;

    // 1. 检查维修/清洁状态
    if (m_roomStatusPeriods.contains(roomId)) {
        for (const auto &p : m_roomStatusPeriods[roomId]) {
            QDateTime dtStart = QDateTime::fromString(p.startTime, "yyyy-MM-dd HH:mm");
            QDateTime dtEnd = QDateTime::fromString(p.endTime, "yyyy-MM-dd HH:mm");
            QDate pStart = dtStart.isValid() ? dtStart.date() : QDate::fromString(p.startTime.left(10), "yyyy-MM-dd");
            QDate pEnd = dtEnd.isValid() ? dtEnd.date() : QDate::fromString(p.endTime.left(10), "yyyy-MM-dd");
            
            if (!(end < pStart || start > pEnd)) return false;
        }
    }

    // 2. 检查现有预约
    for (const auto &appt : m_appointments) {
        if (appt.type == "Boarding" && appt.roomNo == QString::number(roomId) && 
            appt.status != "Canceled" && appt.status != "Expired") {
            QDate apptStart = QDate::fromString(appt.date, "yyyy-MM-dd");
            QDate apptEnd = QDate::fromString(appt.boardingEndDate, "yyyy-MM-dd");
            if (!(end < apptStart || start > apptEnd)) return false;
        }
    }
    
    // 3. 检查当前在店宠物
    for (const auto &pet : m_pets) {
        if (pet.roomNo == QString::number(roomId) && 
            (pet.status.contains("寄养中") || pet.status.contains("已预约"))) {
            QDate petStart = QDate::fromString(pet.fosterStartTime, "yyyy-MM-dd");
            QDate petEnd = QDate::fromString(pet.fosterEndTime, "yyyy-MM-dd");
            if (petStart.isValid() && petEnd.isValid()) {
                if (!(end < petStart || start > petEnd)) return false;
            } else if (pet.status.contains("寄养中")) {
                return false;
            }
        }
    }

    return true;
}

QList<int> PetDataManager::getAvailableRooms(const QDate &start, const QDate &end) const {
    QList<int> available;
    for (int i = 101; i <= 120; ++i) {
        if (isRoomAvailable(i, start, end)) {
            available.append(i);
        }
    }
    return available;
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

    m_activityLogs[petId].clear();
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

std::pair<QList<AppointmentInfo>, int> PetDataManager::getAppointments(int page, int pageSize, const QString &filter, const QString &statusFilter)
{
    QList<AppointmentInfo> filtered;
    QString kw = filter.trimmed().toLower();

    for (const auto &info : m_appointments.values()) {
        bool match = true;
        if (!kw.isEmpty()) {
            if (!info.memberName.toLower().contains(kw) &&
                !info.memberPhone.toLower().contains(kw) &&
                !info.petName.toLower().contains(kw) &&
                !info.service.toLower().contains(kw)) {
                match = false;
            }
        }
        
        if (match && statusFilter != "全部") {
            QString displayStatus;
            if (info.status == "Pending" || info.status == "待处理") displayStatus = "待处理";
            else if (info.status == "Confirmed" || info.status == "已确认") displayStatus = "已确认";
            else if (info.status == "In-Service" || info.status == "服务中") displayStatus = "服务中";
            else if (info.status == "Cancelled" || info.status == "已取消") displayStatus = "已取消";
            else if (info.status == "Expired" || info.status == "已过期") displayStatus = "已过期";
            else displayStatus = "已完成";
            
            if (displayStatus != statusFilter) match = false;
        }
        
        if (match) filtered.append(info);
    }

    std::sort(filtered.begin(), filtered.end(), [](const AppointmentInfo &a, const AppointmentInfo &b) {
        if (a.date != b.date) return a.date < b.date;
        return a.hour < b.hour;
    });

    int total = filtered.size();
    int offset = (page - 1) * pageSize;
    QList<AppointmentInfo> result;
    int end = qMin(offset + pageSize, total);
    for (int i = qMax(0, offset); i < end; ++i) result.append(filtered[i]);
    return {result, total};
}

AppointmentInfo PetDataManager::getAppointment(const QString &id) const
{
    return m_appointments.value(id);
}

QList<AppointmentInfo> PetDataManager::getAppointmentsByGroupId(const QString &groupId) const
{
    QList<AppointmentInfo> result;
    if (groupId.isEmpty()) return result;
    for (const auto &info : m_appointments.values()) {
        if (info.groupId == groupId) result.append(info);
    }
    return result;
}

void PetDataManager::addAppointment(const AppointmentInfo &info)
{
    AppointmentInfo newInfo = info;
    if (newInfo.id.isEmpty()) newInfo.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    m_appointments[newInfo.id] = newInfo;
    emit globalDataChanged();
}

void PetDataManager::updateAppointment(const AppointmentInfo &info)
{
    m_appointments[info.id] = info;
    if (info.type == "Boarding" && !info.roomNo.isEmpty()) {
        int rId = info.roomNo.toInt();
        if (info.status == "Confirmed" || info.status == "已确认") {
            executeBooking(rId, info.petId, QDate::fromString(info.date, "yyyy-MM-dd"), QDate::fromString(info.boardingEndDate, "yyyy-MM-dd"), 10.0);
        } else if (info.status == "In-Service" || info.status == "服务中") {
            executeCheckIn(rId, info.petId, QDate::fromString(info.date, "yyyy-MM-dd"), QDate::fromString(info.boardingEndDate, "yyyy-MM-dd"), 10.0);
        }
    }
    emit globalDataChanged();
}

void PetDataManager::updateAppointmentPhotos(const QString &id, const QStringList &photos) {
    if (m_appointments.contains(id)) {
        m_appointments[id].photos = photos;
        QString petId = m_appointments[id].petId;
        if (!petId.isEmpty()) {
            PetMedia m;
            if (m_appointments[id].type == "Boarding") {
                if (m_appointments[id].status == "In-Service" || m_appointments[id].status == "服务中") m.title = "入住存证";
                else if (m_appointments[id].status == "Completed" || m_appointments[id].status == "已完成") m.title = "离店存证";
                else m.title = m_appointments[id].service + "记录";
            } else {
                m.title = m_appointments[id].service + "服务记录";
            }
            m.urls = photos; m.type = "image";
            for (int i = 0; i < photos.size(); ++i) m.timestamps << m_appointments[id].date + " " + QTime::currentTime().toString("HH:mm");
            
            QList<PetMedia> &medias = m_petMedia[petId];
            bool found = false;
            for (int i = 0; i < medias.size(); ++i) {
                if (medias[i].title == m.title) { medias[i] = m; found = true; break; }
            }
            if (!found) medias.append(m);
        }
        notifyGlobalDataChanged();
    }
}

AppointmentStats PetDataManager::getAppointmentStats()
{
    AppointmentStats s; s.total = m_appointments.size();
    for (const auto &a : m_appointments) {
        if (a.type == "Grooming" || a.type == "Beauty") s.grooming++;
        else if (a.type == "Transport") s.logistics++;
    }
    s.boardingLoad = 65; return s;
}

int PetDataManager::getBoardingOccupation(const QDate &start, const QDate &end) const
{
    if (!start.isValid() || !end.isValid() || start > end) return 0;
    int maxOccupancy = 0;
    for (QDate d = start; d <= end; d = d.addDays(1)) {
        int dailyCount = 0;
        for (const auto &appt : m_appointments) {
            if (appt.type == "Boarding" && appt.status != "Canceled" && appt.status != "Expired") {
                QDate apptStart = QDate::fromString(appt.date, "yyyy-MM-dd");
                QDate apptEnd = QDate::fromString(appt.boardingEndDate, "yyyy-MM-dd");
                if (d >= apptStart && d <= apptEnd) dailyCount++;
            }
        }
        maxOccupancy = qMax(maxOccupancy, dailyCount);
    }
    return maxOccupancy;
}

QList<AppointmentInfo> PetDataManager::getAppointmentsForPet(const QString &petId) const {
    QList<AppointmentInfo> list;
    for (const auto &appt : m_appointments) if (appt.petId == petId) list.append(appt);
    std::sort(list.begin(), list.end(), [](const AppointmentInfo &a, const AppointmentInfo &b) {
        return a.date > b.date || (a.date == b.date && a.hour > b.hour);
    });
    return list;
}

void PetDataManager::addOrder(const OrderInfo &info) { m_orders[info.id] = info; emit globalDataChanged(); }
void PetDataManager::updateOrder(const OrderInfo &info) { if (m_orders.contains(info.id)) { m_orders[info.id] = info; emit globalDataChanged(); } }

void PetDataManager::cancelOrder(const QString &orderId, const QString &reason)
{
    if (!m_orders.contains(orderId)) return;
    OrderInfo &order = m_orders[orderId];
    order.status = "Cancelled"; order.cancelReason = reason;
    order.operationLog += QString("[%1] 订单被作废: %2\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"), reason);
    if (order.sourceModule == "Appointment") {
        AppointmentInfo info = getAppointment(order.relatedId);
        if (!info.id.isEmpty()) { info.status = "Pending"; updateAppointment(info); }
    } else if (order.sourceModule == "Boarding") {
        PetInfo pet = getPet(order.petId);
        if (!pet.id.isEmpty()) { pet.status = "在店"; updatePet(pet); }
    } else if (order.sourceModule == "Logistics") {
        LogisticsManager::instance()->updateTaskStatus(order.relatedId, "进行中");
    }
    emit globalDataChanged();
}

QList<OrderInfo> PetDataManager::getOrders(const QDate &start, const QDate &end, const QString &filter, const QString &moduleFilter) const
{
    QList<OrderInfo> result; QString kw = filter.trimmed().toLower();
    for (const auto &order : m_orders) {
        QDate cDate = QDate::fromString(order.createTime.left(10), "yyyy-MM-dd");
        if (cDate.isValid() && (cDate < start || cDate > end)) continue;
        
        // 模块过滤
        if (moduleFilter != "全部") {
            if (order.sourceModule != moduleFilter) continue;
        }

        if (!kw.isEmpty()) {
            if (!order.id.toLower().contains(kw) && !order.memberName.toLower().contains(kw) && !order.petName.toLower().contains(kw) && !order.relatedId.toLower().contains(kw)) continue;
        }
        result.append(order);
    }
    std::sort(result.begin(), result.end(), [](const OrderInfo &a, const OrderInfo &b){ return a.createTime > b.createTime; });
    return result;
}

OrderInfo PetDataManager::getOrder(const QString &id) const { return m_orders.value(id); }

PetDataManager::OrderStats PetDataManager::getOrderStats(const QDate &start, const QDate &end)
{
    OrderStats stats = {0.0, 0, 0.0, 0.0}; int totalValid = 0; int paidCount = 0;
    for (const auto &order : m_orders) {
        QDate cDate = QDate::fromString(order.createTime.left(10), "yyyy-MM-dd");
        if (cDate.isValid() && (cDate < start || cDate > end)) continue;
        if (order.status == "Paid") { stats.totalRevenue += order.finalAmount; paidCount++; }
        else if (order.status == "Unpaid") stats.pendingCount++;
        if (order.status != "Cancelled") totalValid++;
    }
    if (paidCount > 0) stats.avgTicket = stats.totalRevenue / paidCount;
    if (totalValid > 0) stats.successRate = (double)paidCount / totalValid * 100.0;
    return stats;
}
