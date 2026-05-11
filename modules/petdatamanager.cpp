#include "petdatamanager.h"
#include <QDebug>
#include <QDate>
#include <QRandomGenerator>
#include "logisticsmanager.h"
#include "memberdatamanager.h"
#include "../utils/networkmanager.h"
#include <QJsonArray>
#include <QJsonDocument>

#include <QDir>
#include <QCoreApplication>
#include <QFile>

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
    m_pixmapCache.setMaxCost(300);
    m_cachePath = QCoreApplication::applicationDirPath() + "/cache/pets";
    ensureCacheDir();

    // 连接网络管理器的包接收信号
    connect(&NetworkManager::instance(), &NetworkManager::packetReceived, 
            this, &PetDataManager::onPacketReceived);
    
    // initMockData(); // 不再使用模拟数据
    requestRoomList();
    requestOrderList();
    requestPetList();

    // 监听会员数据变化，一旦会员加载成功，尝试补全订单中的会员姓名
    connect(MemberDataManager::instance(), &MemberDataManager::dataChanged, this, [this](){
        bool changed = false;
        for (auto it = m_orders.begin(); it != m_orders.end(); ++it) {
            if (it.value().memberName.isEmpty() && !it.value().memberId.isEmpty()) {
                MemberInfo m = MemberDataManager::instance()->getMember(it.value().memberId);
                if (!m.id.isEmpty()) {
                    it.value().memberName = m.name;
                    changed = true;
                }
            }
        }
        if (changed) emit globalDataChanged();
    });
}

void PetDataManager::initMockData()
{
    // 1. 模拟数据已清空，改为纯联网模式

    // 3. 模拟订单数据已清空
}

void PetDataManager::requestPetList()
{
    bool shouldEmit = false;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_pets.isEmpty()) {
            shouldEmit = true;
        }
    }
    
    if (shouldEmit) {
        emit globalDataChanged();
        return;
    }

    qDebug() << "[PET] Requesting pet list from server...";
    NetworkManager::instance().sendRequest(2001, QJsonObject());
}

void PetDataManager::onPacketReceived(const Protocol::NetPacket &packet)
{
    if (packet.cmdId == 2001) {
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray arr = root["data"].toArray();
            
            {
                QMutexLocker locker(&m_mutex);
                // 清理旧数据（或者增量更新，这里选择全量覆盖）
                m_pets.clear();
                
                for (int i = 0; i < arr.size(); ++i) {
                    QJsonObject obj = arr[i].toObject();
                    PetInfo info;
                    // ... 映射代码保持不变 ...
                    info.id = QString("P%1").arg(obj["pet_id"].toVariant().toLongLong(), 5, 10, QChar('0'));
                    info.name = obj["pet_name"].toString();
                    info.species = obj["species"].toString();
                    info.breed = obj["breed"].toString();
                    info.gender = obj["gender"].toString();
                    info.age = QString("%1个月").arg(obj["age_months"].toInt());
                    info.weight = obj["weight"].toDouble();
                    info.health = obj["health_status"].toString();
                    info.medicalHistory = obj["medical_history"].toString();
                    info.vaccine = obj["vaccine_status"].toString();
                    info.dietary = obj["dietary_habit"].toString();
                    info.status = obj["current_status"].toString();
                    info.ownerId = QString("M%1").arg(obj["member_id"].toVariant().toLongLong(), 5, 10, QChar('0'));
                    info.ownerName = obj["owner_name"].toString(); 
                    info.ownerPhone = obj["owner_phone"].toString(); 
                    info.avatarPath = obj["avatar_path"].toString();
                    info.imgData = obj["img_data"].toString();
                    info.address = obj["home_address"].toString();
                    
                    QString rawTime = obj["join_time"].toString();
                    QDateTime dt = QDateTime::fromString(rawTime, Qt::ISODate);
                    if (dt.isValid()) info.joinTime = dt.toString("yyyy-MM-dd HH:mm");
                    else info.joinTime = rawTime;
                    
                    m_pets[info.id] = info;
                }
            }
            
            qDebug() << "[PET] Pet list updated from server. Count: " << m_pets.size();
            emit globalDataChanged();
        }
    } else if (packet.cmdId == Protocol::CMD_GET_ROOM_LIST) {
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray arr = root["data"].toArray();
            m_rooms.clear();
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject obj = arr[i].toObject();
                BoardingRoom room;
                room.id = obj["id"].toInt();
                room.roomNo = obj["room_no"].toString();
                room.type = obj["room_type"].toString();
                room.status = obj["status"].toString();
                room.description = obj["description"].toString();
                room.isActive = obj["is_active"].toVariant().toBool();
                m_rooms[room.id] = room;
            }
            qDebug() << "[ROOM] Room list updated. Count: " << m_rooms.size();
            emit globalDataChanged();
        }
    } else if (packet.cmdId == Protocol::CMD_GET_APPOINTMENT_LIST) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray data = root["data"].toArray();
            for (int i = 0; i < data.size(); ++i) {
                QJsonObject obj = data[i].toObject();
                AppointmentInfo info;
                info.id = QString::number(obj["appt_id"].toInt());
                info.petName = obj["pet_name"].toString();
                info.memberName = obj["member_name"].toString();
                info.status = obj["status"].toString();
                info.address = obj["address"].toString();
                info.amount = obj["amount"].toDouble();
                info.type = obj["appt_type"].toString();
                QString apptTime = obj["appt_time"].toString();
                if (apptTime.length() >= 16) {
                    info.date = apptTime.left(10);
                    info.hour = apptTime.mid(11, 5);
                }
                m_appointments[info.id] = info;
            }
            emit globalDataChanged();
            qDebug() << "[APPT] Appointment list updated from server. Count:" << data.size();
        }
    } else if (packet.cmdId == Protocol::CMD_ADD_APPOINTMENT) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            qDebug() << "[APPT] Appointment added successfully, appt_id:" << root["appt_id"].toInt();
            requestRoomList();
        }
    } else if (packet.cmdId == Protocol::CMD_UPDATE_APPT_STATUS) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            qDebug() << "[APPT] Appointment status updated successfully";
            QDate today = QDate::currentDate();
            QDate monday = today.addDays(1 - today.dayOfWeek());
            requestAppointmentList(monday, monday.addDays(6));
        }
    } else if (packet.cmdId == Protocol::CMD_GET_ORDER_LIST) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray data = root["data"].toArray();
            m_orders.clear();
            for (int i = 0; i < data.size(); ++i) {
                QJsonObject obj = data[i].toObject();
                OrderInfo o;
                o.id = obj["id"].toString();
                o.sourceModule = obj["sourceModule"].toString();
                o.relatedId = obj["relatedId"].toString();
                // 尝试补全会员名称 (使用 5 位补全 ID: M1 -> M00001)
                QString mid = obj["memberId"].toString();
                if (!mid.isEmpty() && !mid.contains("M0")) {
                    // 如果服务器发来的是 M1，手动修正为 M00001 兼容 MemberDataManager
                    QString rawId = mid.mid(1);
                    mid = QString("M%1").arg(rawId.toLongLong(), 5, 10, QChar('0'));
                }
                o.memberId = mid;

                o.totalAmount = obj["totalAmount"].toDouble();
                o.discount = obj["discount"].toDouble();
                o.finalAmount = obj["finalAmount"].toDouble();
                o.itemDetails = obj["itemDetails"].toString();
                o.payMethod = obj["payMethod"].toString();
                o.status = obj["status"].toString();
                o.createTime = obj["createTime"].toString();
                o.cancelReason = obj["cancelReason"].toString();

                MemberInfo m = MemberDataManager::instance()->getMember(o.memberId);
                if (!m.id.isEmpty()) o.memberName = m.name;

                m_orders[o.id] = o;
            }
            qDebug() << "[ORDER] Order list updated from server. Count:" << m_orders.size();
            emit globalDataChanged();
        }
    } else if (packet.cmdId == Protocol::CMD_NOTIFY_REFRESH) {
        QJsonObject notify = packet.jsonObj;
        QString module = notify["module"].toString();
        qDebug() << "[NOTIFY] Received refresh signal for module:" << module;

        if (module == "appointment") {
            QDate today = QDate::currentDate();
            QDate monday = today.addDays(1 - today.dayOfWeek());
            requestAppointmentList(monday, monday.addDays(6));
            requestRoomList();
        } else if (module == "pet") {
            requestPetList();
        } else if (module == "staff") {
            // 如果有 StaffManager，调用它的刷新方法
            // 这里假设我们通过发送请求指令来刷新
            NetworkManager::instance().sendRequest(Protocol::CMD_GET_STAFF_LIST, QJsonObject());
        } else if (module == "member") {
            NetworkManager::instance().sendRequest(Protocol::CMD_GET_MEMBER_LIST, QJsonObject());
        } else if (module == "service") {
            NetworkManager::instance().sendRequest(Protocol::CMD_GET_SERVICE_LIST, QJsonObject());
        } else if (module == "schedule") {
            // 排班刷新通常需要日期范围，这里刷新本周
            QDate today = QDate::currentDate();
            QDate monday = today.addDays(1 - today.dayOfWeek());
            QJsonObject body;
            body["start_date"] = monday.toString("yyyy-MM-dd");
            body["end_date"] = monday.addDays(6).toString("yyyy-MM-dd");
            NetworkManager::instance().sendRequest(Protocol::CMD_GET_SCHEDULE, body);
        } else if (module == "order") {
            requestOrderList();
        } else {
            // 兜底：刷新核心数据
            requestPetList();
            requestRoomList();
            requestOrderList();
        }
    }
}

void PetDataManager::updatePet(const PetInfo &info)
{
    m_pets[info.id] = info;
    emit petDataChanged(info.id);
    emit globalDataChanged();
}

PetInfo PetDataManager::getPet(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    return m_pets.value(id);
}

QList<PetInfo> PetDataManager::allPets() const
{
    QMutexLocker locker(&m_mutex);
    return m_pets.values();
}

QList<PetInfo> PetDataManager::getPetsByOwner(const QString &ownerId) const
{
    QMutexLocker locker(&m_mutex);
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
    if (m_pets.contains(id)) {
        m_pets[id].isActive = false;
        m_pets[id].status = "已注销";
        notifyGlobalDataChanged();
        notifyPetDataChanged(id);
    }
}

void PetDataManager::restorePet(const QString &id)
{
    if (m_pets.contains(id)) {
        m_pets[id].isActive = true;
        m_pets[id].status = "在家"; // 恢复后默认为在家
        notifyGlobalDataChanged();
        notifyPetDataChanged(id);
    }
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
    QMutexLocker locker(&m_mutex);
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
    QMutexLocker locker(&m_mutex);
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
    // 模拟数据已清空，改为纯网络请求
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

QList<BoardingRoom> PetDataManager::allRooms() const {
    QMutexLocker locker(&m_mutex);
    return m_rooms.values();
}

BoardingRoom PetDataManager::getRoom(int id) const {
    return m_rooms.value(id);
}

void PetDataManager::requestRoomList() {
    qDebug() << "[ROOM] Requesting room list...";
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_ROOM_LIST, QJsonObject());
}

QString PetDataManager::getRoomType(int roomId) const {
    if (m_rooms.contains(roomId)) return m_rooms[roomId].type;
    if (roomId >= 111 && roomId <= 115) return "豪华房";
    if (roomId >= 116 && roomId <= 120) return "多宠房";
    return "标准房";
}

QList<int> PetDataManager::getAvailableRooms(const QDate &start, const QDate &end, const QString &type) const {
    QList<int> available;
    QList<int> roomIds = m_rooms.keys();
    if (roomIds.isEmpty()) {
        // Fallback for mock if no rooms from server
        for (int i = 101; i <= 120; ++i) roomIds << i;
    }
    
    for (int rid : roomIds) {
        if (!type.isEmpty() && getRoomType(rid) != type) continue;
        if (isRoomAvailable(rid, start, end)) {
            available.append(rid);
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

    // 发送网络请求
    QJsonObject body;
    body["member_id"] = newInfo.memberName.toInt();  // 需要的话在 UI 层将 memberId 存入 info
    body["pet_id"] = newInfo.petId.mid(1).toInt();   // "P001" -> 1
    body["service_id"] = 0; // 待对接
    body["staff_id"] = 0;   // 待对接
    body["appt_time"] = newInfo.date + " " + newInfo.hour + ":00";
    body["appt_type"] = newInfo.type;
    body["notes"] = newInfo.notes;
    body["address"] = newInfo.address;
    body["amount"] = newInfo.amount;
    body["need_transport"] = !newInfo.address.isEmpty();
    
    NetworkManager::instance().sendRequest(Protocol::CMD_ADD_APPOINTMENT, body);
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

void PetDataManager::addOrder(const OrderInfo &info) {
    {
        QMutexLocker locker(&m_mutex);
        m_orders[info.id] = info;
    }
    
    // 同步到服务器
    QJsonObject body;
    body["id"] = info.id;
    body["sourceModule"] = info.sourceModule;
    body["relatedId"] = info.relatedId;
    body["memberId"] = info.memberId;
    body["totalAmount"] = info.totalAmount;
    body["discount"] = info.discount;
    body["finalAmount"] = info.finalAmount;
    body["itemDetails"] = info.itemDetails;
    body["payMethod"] = info.payMethod;
    body["status"] = info.status;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_CREATE_ORDER, body);

    emit globalDataChanged();
}

void PetDataManager::updateOrder(const OrderInfo &info) {
    if (m_orders.contains(info.id)) {
        bool wasUnpaid = (m_orders[info.id].status == "Unpaid");
        m_orders[info.id] = info;
        
        // 发送更新请求
        QJsonObject body;
        body["order_id"] = info.id;
        body["status"] = info.status;
        body["final_amount"] = info.finalAmount;
        body["pay_method"] = info.payMethod;
        NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_ORDER, body);
        // 核心释放逻辑：寄养结算完成后自动退房
        if (wasUnpaid && info.status == "Paid" && info.sourceModule == "Boarding") {
            PetInfo pet = getPet(info.petId);
            if (!pet.id.isEmpty()) {
                pet.status = "已离店";
                pet.roomNo = ""; // 物理清空房位占用
                updatePet(pet);
                
                // 同步更新预约状态为完成
                for (auto &appt : m_appointments) {
                    if (appt.petId == info.petId && appt.type == "Boarding" && 
                       (appt.status == "CheckedIn" || appt.status == "Pending")) {
                        appt.status = "Completed";
                    }
                }
            }
        }
        emit globalDataChanged();
    }
}

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
    QMutexLocker locker(&m_mutex);
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

OrderInfo PetDataManager::getOrder(const QString &id) const { 
    QMutexLocker locker(&m_mutex);
    return m_orders.value(id); 
}

OrderStats PetDataManager::getOrderStats(const QDate &start, const QDate &end)
{
    QMutexLocker locker(&m_mutex);
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

void PetDataManager::hardDeletePet(const QString &id)
{
    if (m_pets.contains(id)) {
        PetInfo info = m_pets[id];
        // 同步从会员档案中移除
        MemberDataManager::instance()->removePetFromMember(info.ownerId, info.name);
        
        m_pets.remove(id);
        m_activityLogs.remove(id);
        m_petMedia.remove(id);
        m_vaccineRecords.remove(id);
        m_historyBatches.remove(id);
        notifyGlobalDataChanged();
    }
}

// ================== 预约网络方法 ==================

void PetDataManager::requestAppointmentList(const QDate &startDate, const QDate &endDate)
{
    QJsonObject body;
    body["start_date"] = startDate.toString("yyyy-MM-dd");
    body["end_date"] = endDate.toString("yyyy-MM-dd");
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_APPOINTMENT_LIST, body);
}

void PetDataManager::updateAppointmentStatus(const QString &apptId, const QString &status)
{
    QJsonObject body;
    body["appt_id"] = apptId.toInt();
    body["status"] = status;
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_APPT_STATUS, body);
}

void PetDataManager::requestOrderList()
{
    {
        QMutexLocker locker(&m_mutex);
        if (!m_orders.isEmpty()) {
            qDebug() << "[ORDER] Order list already loaded, skipping.";
            return;
        }
    }
    qDebug() << "[ORDER] Requesting order list from server...";
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_ORDER_LIST, QJsonObject());
}

QList<OrderInfo> PetDataManager::allOrders() const
{
    QMutexLocker locker(&m_mutex);
    return m_orders.values();
}
QPixmap PetDataManager::getPetPixmap(const QString &id) const
{
    if (m_pixmapCache.contains(id)) return *m_pixmapCache.object(id);

    QString localPath = m_cachePath + "/" + id + ".png";
    if (QFile::exists(localPath)) {
        QPixmap pix(localPath);
        if (!pix.isNull()) {
            m_pixmapCache.insert(id, new QPixmap(pix));
            return pix;
        }
    }

    PetInfo info = getPet(id);
    if (info.imgData.isEmpty()) return QPixmap();

    QPixmap pix;
    QByteArray ba = QByteArray::fromBase64(info.imgData.toUtf8());
    if (pix.loadFromData(ba)) {
        m_pixmapCache.insert(id, new QPixmap(pix));
        const_cast<PetDataManager*>(this)->saveToLocalCache(id, pix);
    }
    return pix;
}

void PetDataManager::ensureCacheDir()
{
    QDir dir(m_cachePath);
    if (!dir.exists()) dir.mkpath(".");
}

void PetDataManager::saveToLocalCache(const QString &id, const QPixmap &pix)
{
    if (pix.isNull()) return;
    QString localPath = m_cachePath + "/" + id + ".png";
    if (!QFile::exists(localPath)) {
        pix.save(localPath, "PNG");
    }
}
