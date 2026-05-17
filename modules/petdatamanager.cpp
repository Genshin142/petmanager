#include "petdatamanager.h"
#include <QDebug>
#include <QDate>
#include <QRandomGenerator>
#include "logisticsmanager.h"
#include "memberdatamanager.h"
#include "logdatamanager.h"
#include "servicedatamanager.h"
#include "../utils/networkmanager.h"
#include <QJsonArray>
#include <QJsonDocument>

#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include "../utils/imageutils.h"
#include <QTimer>

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

    // 监听会员数据变化，一旦会员加载成功，尝试补全订单中的会员姓名 (增加防抖，避免启动时卡顿)
    connect(MemberDataManager::instance(), &MemberDataManager::dataChanged, this, [this](){
        static bool isSyncing = false;
        if (isSyncing) return;
        isSyncing = true;
        
        QTimer::singleShot(300, this, [this](){
            bool changed = false;
            QMutexLocker locker(&m_mutex);
            for (auto it = m_orders.begin(); it != m_orders.end(); ++it) {
                if (it.value().memberName.isEmpty() && !it.value().memberId.isEmpty()) {
                    MemberInfo m = MemberDataManager::instance()->getMember(it.value().memberId);
                    if (!m.id.isEmpty()) {
                        it.value().memberName = m.name;
                        changed = true;
                    }
                }
            }
            isSyncing = false;
            if (changed) emit globalDataChanged();
        });
    });
}

void PetDataManager::initMockData()
{
    // 1. 模拟数据已清空，改为纯联网模式

    // 3. 模拟订单数据已清空
}

void PetDataManager::requestPetList()
{
    qDebug() << "[PET] Requesting pet list from server...";
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_PET_LIST, QJsonObject());
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
                    int totalMonths = obj["age_months"].toVariant().toInt();
                    if (totalMonths >= 12) {
                        int years = totalMonths / 12;
                        int months = totalMonths % 12;
                        if (months > 0)
                            info.age = QString("%1岁%2个月").arg(years).arg(months);
                        else
                            info.age = QString("%1岁").arg(years);
                    } else {
                        info.age = QString("%1个月").arg(totalMonths);
                    }
                    info.weight = obj["weight"].toVariant().toDouble();

                    info.health = obj["health_status"].toString();
                    info.medicalHistory = obj["medical_history"].toString();
                    info.vaccine = obj["vaccine_status"].toString();
                    info.dietary = obj["dietary_habit"].toString();
                    info.status = obj["current_status"].toString();
                    
                    bool isDeleted = (obj["is_deleted"].toInt() != 0) || obj["is_deleted"].toBool();
                    info.isActive = !isDeleted;
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
                    
                    // 解析寄养房号与时间信息 (从联表查询获得)
                    info.roomNo = obj["room_no"].toString();
                    info.fosterStartTime = obj["check_in_time"].toString();
                    if (info.fosterStartTime.contains("T")) info.fosterStartTime = info.fosterStartTime.split("T").first();
                    info.fosterEndTime = obj["expected_check_out_time"].toString();
                    if (info.fosterEndTime.contains("T")) info.fosterEndTime = info.fosterEndTime.split("T").first();
                    
                    m_pets[info.id] = info;
                    
                    // 核心逻辑：加载宠物后，自动请求该宠物的疫苗明细
                    requestVaccines(info.id);
                }
            }
            
            qDebug() << "[PET] Pet list updated from server. Count: " << m_pets.size();
            emit globalDataChanged();
        }
    } else if (packet.cmdId == Protocol::CMD_GET_VACCINES) {
        QJsonObject root = packet.jsonObj;
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QString petId = QString("P%1").arg(root["pet_id"].toInt(), 5, 10, QChar('0'));
            QJsonArray data = root["data"].toArray();
            QList<VaccineRecord> records;
            for (int i = 0; i < data.size(); ++i) {
                QJsonObject obj = data[i].toObject();
                VaccineRecord vr;
                vr.type = obj["type"].toString();
                vr.date = obj["date"].toString();
                vr.expiry = obj["expiry"].toString();
                records.append(vr);
            }
            m_vaccineRecords[petId] = records;
            emit petDataChanged(petId);
            qDebug() << "[VACCINE] Loaded" << records.size() << "records for pet:" << petId;
        }
    } else if (packet.cmdId == Protocol::CMD_UPDATE_VACCINES) {
        if (packet.jsonObj["status"].toInt() == Protocol::STATUS_OK) {
            qDebug() << "[VACCINE] Records saved successfully to server.";
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
            QJsonArray dataArr = root["data"].toArray();
            {
                QMutexLocker locker(&m_mutex);
                m_orders.clear();
                for (int i = 0; i < dataArr.size(); ++i) {
                    QJsonObject obj = dataArr[i].toObject();
                    OrderInfo o;
                    o.id = obj["id"].toString();
                    o.sourceModule = obj["sourceModule"].toString();
                    o.relatedId = obj["relatedId"].toString();
                    QString mid = obj["memberId"].toString();
                    if (!mid.isEmpty() && !mid.contains("M0") && mid.startsWith("M")) {
                        QString rawId = mid.mid(1);
                        mid = QString("M%1").arg(rawId.toLongLong(), 5, 10, QChar('0'));
                    }
                    o.memberId = mid;
                    o.totalAmount = obj["totalAmount"].toDouble();
                    o.discount = obj["discount"].toDouble();
                    o.finalAmount = obj["finalAmount"].toDouble();
                    o.itemDetails = obj["itemDetails"].toString();
                    o.payMethod = obj["payMethod"].toString();
                    
                    QString statusStr = obj["status"].toString();
                    if (statusStr == "已支付" || statusStr == "Paid") o.status = "Paid";
                    else if (statusStr == "待结算" || statusStr == "Unpaid") o.status = "Unpaid";
                    else o.status = statusStr;

                    o.createTime = obj["createTime"].toString();
                    MemberInfo m = MemberDataManager::instance()->getMember(o.memberId);
                    if (!m.id.isEmpty()) o.memberName = m.name;
                    m_orders[o.id] = o;
                }
            }
            qDebug() << "==========================================";
            qDebug() << "[ORDER] DATA ARRIVED. Count:" << m_orders.size();
            qDebug() << "==========================================";
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
    } else if (packet.cmdId == Protocol::CMD_GET_STATS_DASHBOARD) {
        if (packet.jsonObj["status"].toInt() == Protocol::STATUS_OK) {
            emit dashboardStatsReceived(packet.jsonObj["data"].toObject());
        }
    } else if (packet.cmdId == Protocol::CMD_GET_STATS_REVENUE) {
        if (packet.jsonObj["status"].toInt() == Protocol::STATUS_OK) {
            emit revenueTrendReceived(packet.jsonObj["data"].toArray());
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
    // Generate temporary ID if needed
    QString petId = info.id;
    if (petId.isEmpty()) {
        petId = QString("P_TEMP_%1").arg(QDateTime::currentMSecsSinceEpoch());
    }
    
    PetInfo newPet = info;
    newPet.id = petId;
    m_pets[petId] = newPet;
    
    int ageMonths = 0;
    QString ageStr = newPet.age;
    if (ageStr.contains("岁")) {
        int yearIdx = ageStr.indexOf("岁");
        ageMonths += ageStr.left(yearIdx).toInt() * 12;
        if (ageStr.contains("个月")) {
            int monthIdx = ageStr.indexOf("个月");
            ageMonths += ageStr.mid(yearIdx + 1, monthIdx - yearIdx - 1).toInt();
        }
    } else if (ageStr.contains("个月")) {
        ageMonths = ageStr.left(ageStr.indexOf("个月")).toInt();
    }
    
    QJsonObject body;
    body["owner_id"] = newPet.ownerId;
    body["pet_name"] = newPet.name;
    body["species"] = newPet.species;
    body["breed"] = newPet.breed;
    body["gender"] = newPet.gender;
    body["age_months"] = ageMonths;
    body["weight"] = newPet.weight;
    body["avatar_path"] = newPet.avatarPath;
    body["health_status"] = newPet.health;
    body["medical_history"] = newPet.medicalHistory;
    body["dietary_habit"] = newPet.dietary;
    body["status"] = newPet.status;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_ADD_PET, body);

    // 记录新增宠物日志
    QJsonObject diff;
    diff["field"] = "姓名, 品种, 性别, 年龄, 主人姓名";
    diff["old"] = "空";
    diff["new"] = QString("%1, %2, %3, %4, %5")
                  .arg(newPet.name, newPet.breed, newPet.gender, newPet.age, newPet.ownerName);
    LogDataManager::writeLog("宠物档案", "新增宠物档案: " + newPet.name, diff);
    
    emit globalDataChanged();
}

void PetDataManager::removePet(const QString &id)
{
    if (m_pets.contains(id)) {
        PetInfo info = m_pets[id];
        m_pets[id].isActive = false;
        m_pets[id].status = "已注销";
        
        QJsonObject body;
        body["pet_id"] = id.mid(1).toInt();
        NetworkManager::instance().sendRequest(Protocol::CMD_DELETE_PET, body);

        // 记录注销宠物日志
        QJsonObject diff;
        diff["field"] = "档案状态";
        diff["old"] = "正常活跃";
        diff["new"] = "已软删除注销";
        LogDataManager::writeLog("宠物档案", "软删除宠物: " + info.name, diff);
        
        notifyGlobalDataChanged();
        notifyPetDataChanged(id);
    }
}

void PetDataManager::restorePet(const QString &id)
{
    if (m_pets.contains(id)) {
        PetInfo info = m_pets[id];
        m_pets[id].isActive = true;
        m_pets[id].status = "在家"; // 恢复后默认为在家
        
        QJsonObject body;
        body["pet_id"] = id.mid(1).toInt();
        NetworkManager::instance().sendRequest(Protocol::CMD_RESTORE_PET, body);

        // 记录恢复宠物日志
        QJsonObject diff;
        diff["field"] = "档案状态";
        diff["old"] = "已软删除注销";
        diff["new"] = "正常活跃 (在家)";
        LogDataManager::writeLog("宠物档案", "恢复宠物档案: " + info.name, diff);
        
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

    // 发送网络请求更新数据库
    QJsonObject body;
    body["pet_id"] = petId.mid(1).toInt();
    QJsonArray arr;
    for (const auto &vr : records) {
        QJsonObject obj;
        obj["type"] = vr.type;
        obj["date"] = vr.date;
        obj["expiry"] = vr.expiry;
        arr.append(obj);
    }
    body["records"] = arr;
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_VACCINES, body);
}

void PetDataManager::requestVaccines(const QString &petId)
{
    QJsonObject body;
    body["pet_id"] = petId.mid(1).toInt();
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_VACCINES, body);
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

void PetDataManager::executeCheckIn(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight, const QString &note, const QString &fosterType, std::function<void(bool)> callback) {
    Q_UNUSED(note);
    if (!m_pets.contains(petId)) {
        if (callback) callback(false);
        return;
    }

    // 移除本地预更新，改为等待服务器确认后再刷新
    
    // 2. 同步到服务器
    QJsonObject body;
    body["pet_id"] = petId;
    body["status"] = "寄养中";
    body["room_no"] = QString::number(roomId);
    body["start_time"] = start.toString("yyyy-MM-dd");
    body["end_time"] = end.toString("yyyy-MM-dd");
    body["foster_type"] = fosterType;
    body["weight"] = weight;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_PET_STATUS, body, [=](const QJsonObject &resp){
        // 鲁棒性处理：兼容字符串和整数状态码
        int status = resp.contains("status") ? resp["status"].toVariant().toInt() : -1;
        bool success = (status == Protocol::STATUS_OK);
        
        if (success) {
            qDebug() << "[PET] Server confirmed check-in for pet:" << petId;
            
            // --- 核心优化：收到确认后立即本地局部更新，消除 2-3s 延迟 ---
            {
                QMutexLocker locker(&m_mutex);
                if (m_pets.contains(petId)) {
                    m_pets[petId].status = "寄养中";
                    m_pets[petId].roomNo = QString::number(roomId);
                    m_pets[petId].fosterStartTime = start.toString("yyyy-MM-dd");
                    m_pets[petId].fosterEndTime = end.toString("yyyy-MM-dd");
                    m_pets[petId].fosterType = fosterType;
                    m_pets[petId].weight = weight;
                }
                if (m_rooms.contains(roomId)) {
                    m_rooms[roomId].status = "occupied";
                }
            }
            
            // 立即触发本地刷新
            emit globalDataChanged();
            
            // 异步后台全量刷新，确保最终一致性
            requestPetList();
            requestRoomList();
            
            if (callback) callback(true);
        } else {
            QString errMsg = resp["message"].toString();
            qWarning() << "[PET] Server failed to check-in pet:" << petId << "Error:" << errMsg;
            if (callback) callback(false);
        }
    });
}


void PetDataManager::executeBooking(int roomId, const QString &petId, const QDate &start, const QDate &end, double weight, const QString &fosterType, std::function<void(bool)> callback) {
    if (!m_pets.contains(petId)) {
        if (callback) callback(false);
        return;
    }

    // 移除本地预更新
    
    // 同步到服务器
    QJsonObject body;
    body["pet_id"] = petId;
    body["status"] = "已预约";
    body["room_no"] = QString::number(roomId);
    body["start_time"] = start.toString("yyyy-MM-dd");
    body["end_time"] = end.toString("yyyy-MM-dd");
    body["foster_type"] = fosterType;
    body["weight"] = weight;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_PET_STATUS, body, [=](const QJsonObject &resp){
        int status = resp.contains("status") ? resp["status"].toVariant().toInt() : -1;
        bool success = (status == Protocol::STATUS_OK);
        
        if (success) {
            // 本地局部更新
            {
                QMutexLocker locker(&m_mutex);
                if (m_pets.contains(petId)) {
                    m_pets[petId].status = "已预约";
                    m_pets[petId].roomNo = QString::number(roomId);
                    m_pets[petId].fosterStartTime = start.toString("yyyy-MM-dd");
                    m_pets[petId].fosterEndTime = end.toString("yyyy-MM-dd");
                    m_pets[petId].weight = weight;
                }
                if (m_rooms.contains(roomId)) {
                    m_rooms[roomId].status = "booked";
                }
            }
            emit globalDataChanged();

            requestPetList();
            requestRoomList();
            if (callback) callback(true);
        } else {
            qWarning() << "[PET] Server failed to book pet:" << petId << "Error:" << resp["message"].toString();
            if (callback) callback(false);
        }
    });
}

void PetDataManager::executeCancelBooking(int roomId, const QString &petId) {
    Q_UNUSED(roomId);
    if (!m_pets.contains(petId)) return;

    {
        QMutexLocker locker(&m_mutex);
        PetInfo &info = m_pets[petId];
        info.status = "待寄养";
        info.roomNo = "";
        info.fosterStartTime = "";
        info.fosterEndTime = "";
    }
    
    emit globalDataChanged();

    // 同步到服务器
    QJsonObject body;
    body["pet_id"] = petId;
    body["status"] = "待寄养";
    body["room_no"] = "";
    body["start_time"] = "";
    body["end_time"] = "";
    
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_PET_STATUS, body, [=](const QJsonObject &resp){
        if (resp["status"].toInt() == Protocol::STATUS_OK) {
            requestPetList();
            requestRoomList();
        }
    });
}

void PetDataManager::executeCheckOut(int roomId, const QString &petId, const QDate &checkOutDate, double weight, double totalAmount, const QString &paymentMethod, std::function<void(bool, QString)> callback) {
    qDebug() << "[DEBUG] CMD_CREATE_ORDER value:" << (int)Protocol::CMD_CREATE_ORDER;
    Q_UNUSED(checkOutDate);
    PetInfo info;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_pets.contains(petId)) {
            if (callback) callback(false, "找不到宠物信息");
            return;
        }
        info = m_pets[petId];
    }

    // 1. 同步到服务器：发送宠物状态更新
    QJsonObject petStatus;
    petStatus["pet_id"] = petId;
    petStatus["status"] = "在家";
    petStatus["room_no"] = "";
    petStatus["start_time"] = "";
    petStatus["end_time"] = "";
    
    QString now = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    qDebug() << "[" << now << "] [CHECKOUT] Step 1: Sending CMD_UPDATE_PET_STATUS for pet:" << petId;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_PET_STATUS, petStatus, [=](const QJsonObject &response){
        QString t1 = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        qDebug() << "[" << t1 << "] [CHECKOUT] Step 1 Response received:" << response["status"].toInt();
        
        if (response["status"].toInt() != Protocol::STATUS_OK) {
            if (callback) callback(false, "更新宠物状态失败: " + response["message"].toString());
            return;
        }

        // 2. 同步到服务器：创建正式订单
        QString t2 = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        qDebug() << "[" << t2 << "] [CHECKOUT] Step 2: Preparing CMD_CREATE_ORDER...";
        
        // 动态构建服务名称和查找编号
        QString fType = info.fosterType.isEmpty() ? "全托" : info.fosterType;
        QString rType = "普通房间";
        {
            QMutexLocker locker(&m_mutex);
            QString rawType = m_rooms.contains(roomId) ? m_rooms[roomId].type : "标准房";
            if (rawType == "豪华房") rType = "豪华房间";
            else if (rawType == "多宠房") rType = "多宠家庭房间";
        }
        
        QString rawServiceName = fType + rType;
        QString serviceName = rawServiceName;
        if (rawServiceName == QString::fromUtf8("全托普通房间")) serviceName = QString::fromUtf8("全托寄养 (普通房间)");
        else if (rawServiceName == QString::fromUtf8("全托豪华房间")) serviceName = QString::fromUtf8("全托寄养 (豪华套房)");
        else if (rawServiceName == QString::fromUtf8("全托多宠家庭房间")) serviceName = QString::fromUtf8("全托寄养 (多宠家庭房)");
        else if (rawServiceName == QString::fromUtf8("日托普通房间")) serviceName = QString::fromUtf8("日托寄养 (普通房间)");
        else if (rawServiceName == QString::fromUtf8("日托豪华房间")) serviceName = QString::fromUtf8("日托寄养 (豪华套房)");
        else if (rawServiceName == QString::fromUtf8("日托多宠家庭房间")) serviceName = QString::fromUtf8("日托寄养 (多宠家庭房)");

        QString barcode = "S001"; // 默认兜底
        
        // 查找对应的服务编号
        QList<ServiceInfo> services = ServiceDataManager::instance()->allServices();
        for (const auto& svc : services) {
            if (svc.name == serviceName) {
                barcode = svc.id;
                break;
            }
        }

        QJsonObject orderObj;
        orderObj["id"] = "ORD-BO-" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
        orderObj["sourceModule"] = "Boarding";
        orderObj["relatedId"] = QString::number(roomId);
        orderObj["memberId"] = info.ownerId;
        orderObj["memberName"] = info.ownerName;
        orderObj["totalAmount"] = totalAmount;
        orderObj["discount"] = 0.0;
        orderObj["finalAmount"] = totalAmount;
        
        // 构建明细 JSON
        QJsonArray detailsArr;
        QJsonObject detailItem;
        detailItem["name"] = serviceName;
        detailItem["barcode"] = barcode;
        detailItem["price"] = totalAmount;
        detailItem["count"] = 1;
        
        // 注入宠物相关元数据，用于详情界面显示
        detailItem["petId"] = petId;
        detailItem["petName"] = info.name;
        detailItem["petBreed"] = info.breed;
        // 注意：不存储 base64 头像数据，避免超出数据库 TEXT 列 65KB 限制
        // 订单详情界面通过 petId 按需加载头像
        detailItem["petPhoto"] = "";
        detailItem["roomName"] = getRoom(roomId).roomNo;
        
        // 如果是寄养，增加天数显示
        int days = 1;
        QDateTime startDT = QDateTime::fromString(info.fosterStartTime, "yyyy-MM-dd HH:mm:ss");
        if (!startDT.isValid()) startDT = QDateTime::fromString(info.fosterStartTime, "yyyy-MM-dd");
        
        QDateTime actualEnd = QDateTime(checkOutDate, QTime(12, 0)); // 默认中午离店
        if (startDT.isValid()) {
            days = startDT.daysTo(actualEnd);
            if (days <= 0) days = 1;
        }
        detailItem["duration"] = days;

        detailsArr.append(detailItem);
        orderObj["itemDetails"] = QString::fromUtf8(QJsonDocument(detailsArr).toJson(QJsonDocument::Compact));
        
        orderObj["payMethod"] = paymentMethod;
        orderObj["status"] = "Paid";
        orderObj["operator_id"] = 1;

        NetworkManager::instance().sendRequest(Protocol::CMD_CREATE_ORDER, orderObj, [=](const QJsonObject &resp2){
            QString t4 = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            int status = resp2["status"].toInt();
            qDebug() << "[" << t4 << "] [CHECKOUT] Step 2 Response received:" << status;
            
            if (status == Protocol::STATUS_OK) {
                // 本地更新内存状态
                {
                    QMutexLocker locker(&m_mutex);
                    if (m_pets.contains(petId)) {
                        m_pets[petId].status = "在家";
                        m_pets[petId].roomNo = "";
                        m_pets[petId].weight = weight;
                    }
                    removeRoomStatusPeriod(roomId, "寄养");
                }
                
                notifyGlobalDataChanged();
                notifyPetDataChanged(petId);
                requestOrderList(); // 立即刷新订单列表
                
                // 延迟 500ms 再次刷新，确保数据库索引已更新并可见
                QTimer::singleShot(500, this, [this](){
                    requestOrderList();
                });

                if (callback) callback(true, "");
            } else {
                QString errMsg = resp2["message"].toString();
                if (errMsg.isEmpty()) errMsg = "数据库写入失败或连接超时";
                if (callback) callback(false, "创建订单失败: " + errMsg);
            }
        });
    });
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

    // 记录下单日志
    QJsonObject diff;
    diff["field"] = "订单号, 业务模块, 会员姓名, 总金额, 状态";
    diff["old"] = "空";
    diff["new"] = QString("%1, %2, %3, ¥%4, %5")
                  .arg(info.id, info.sourceModule == "Product" ? "商品零售" : (info.sourceModule == "Boarding" ? "宠物寄养" : "服务预约"), info.memberName)
                  .arg(QString::number(info.totalAmount, 'f', 2))
                  .arg(info.status == "Paid" ? "已支付" : "待结算");
    LogDataManager::writeLog("订单管理", "创建订单: " + info.id, diff);

    emit globalDataChanged();
}

void PetDataManager::updateOrder(const OrderInfo &info) {
    if (m_orders.contains(info.id)) {
        bool wasUnpaid = (m_orders[info.id].status == "Unpaid");
        m_orders[info.id] = info;
        
        // 1. 发送更新请求到服务器
        QJsonObject body;
        body["order_id"] = info.id;
        body["status"] = info.status; // 这里的状态是 "Paid" 或 "Unpaid"
        body["final_amount"] = info.finalAmount;
        body["pay_method"] = info.payMethod;
        NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_ORDER, body);

        // 2. 记录收款/修改日志
        if (wasUnpaid && info.status == "Paid") {
            QJsonObject diff;
            diff["field"] = "订单状态, 支付方式, 实付金额";
            diff["old"] = "待结算, 未支付, ¥0.00";
            diff["new"] = QString("已支付, %1, ¥%2")
                          .arg(info.payMethod)
                          .arg(QString::number(info.finalAmount, 'f', 2));
            LogDataManager::writeLog("订单管理", "收款结算订单: " + info.id, diff);
        } else {
            QJsonObject diff;
            diff["field"] = "订单状态, 实付金额";
            diff["old"] = "待结算";
            diff["new"] = QString("%1, ¥%2").arg(info.status == "Paid" ? "已支付" : "待结算").arg(QString::number(info.finalAmount, 'f', 2));
            LogDataManager::writeLog("订单管理", "更新订单: " + info.id, diff);
        }

        // 3. 业务状态闭环：支付成功后的后续处理
        if (wasUnpaid && info.status == "Paid") {
            // A. 寄养业务闭环：结算完成后自动退房
            if (info.sourceModule == "Boarding") {
                PetInfo pet = getPet(info.petId);
                if (!pet.id.isEmpty()) {
                    pet.status = "在家"; // 恢复为在家状态
                    pet.roomNo = "";    // 释放房位
                    updatePet(pet);
                    
                    // 同步更新寄养预约单状态
                    for (auto &appt : m_appointments) {
                        if (appt.petId == info.petId && appt.type == "Boarding" && 
                           (appt.status == "In-Service" || appt.status == "CheckedIn")) {
                            updateAppointmentStatus(appt.id, "Completed");
                            appt.status = "Completed";
                        }
                    }
                }
            }
            // B. 洗护/美容业务闭环：结算完成后将预约单设为已完成
            else if (info.sourceModule == "Appointment") {
                if (!info.relatedId.isEmpty()) {
                    updateAppointmentStatus(info.relatedId, "Completed");
                    if (m_appointments.contains(info.relatedId)) {
                        m_appointments[info.relatedId].status = "Completed";
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
    
    QJsonObject body;
    body["order_id"] = orderId;
    body["reason"] = reason;
    NetworkManager::instance().sendRequest(Protocol::CMD_CANCEL_ORDER, body);

    // 记录订单作废日志
    QJsonObject diff;
    diff["field"] = "订单状态, 作废原因";
    diff["old"] = "正常";
    diff["new"] = QString("已作废, 原因: %1").arg(reason);
    LogDataManager::writeLog("订单管理", "作废订单: " + orderId, diff);

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
        
        QJsonObject body;
        body["pet_id"] = id.mid(1).toInt();
        NetworkManager::instance().sendRequest(Protocol::CMD_HARD_DELETE_PET, body);

        // 记录彻底物理删除宠物日志
        QJsonObject diff;
        diff["field"] = "数据档案";
        diff["old"] = "存在于数据库";
        diff["new"] = "已永久物理删除";
        LogDataManager::writeLog("宠物档案", "彻底删除宠物: " + info.name, diff);
        
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
    
    // 优先使用 imgData，其次尝试 avatarPath（数据库 avatar_path 列可能存储 base64 / Data URI）
    QString source = info.imgData;
    if (source.isEmpty()) source = info.avatarPath;
    if (source.isEmpty()) return QPixmap();

    // 使用 ImageUtils 统一处理 Data URI、纯 base64、文件路径等所有格式
    QPixmap pix = ImageUtils::loadPixmap(source);
    if (!pix.isNull()) {
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

void PetDataManager::requestDashboardStats()
{
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_STATS_DASHBOARD, QJsonObject());
}

void PetDataManager::requestRevenueTrend(const QString &range, int year, int month)
{
    QJsonObject body;
    body["range"] = range;
    body["year"] = year;
    body["month"] = month;
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_STATS_REVENUE, body);
}
