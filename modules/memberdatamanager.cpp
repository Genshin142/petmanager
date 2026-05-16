#include "memberdatamanager.h"
#include "../utils/networkmanager.h"
#include "../protocol_codes.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QDate>

#include <QDir>
#include <QCoreApplication>
#include <QFile>

MemberDataManager* MemberDataManager::m_instance = nullptr;

MemberDataManager* MemberDataManager::instance()
{
    if (!m_instance) {
        m_instance = new MemberDataManager();
    }
    return m_instance;
}

MemberDataManager::MemberDataManager(QObject *parent) : QObject(parent)
{
    m_pixmapCache.setMaxCost(200); 
    m_cachePath = QCoreApplication::applicationDirPath() + "/cache/members";
    ensureCacheDir();

    connect(&NetworkManager::instance(), &NetworkManager::packetReceived,
            this, &MemberDataManager::onPacketReceived);
    requestMemberList(); 
}

void MemberDataManager::requestMemberList()
{
    bool shouldEmitLocal = false;
    {
        QMutexLocker locker(&m_mutex);
        if (m_isLoading) {
            return; // Already fetching
        }
        m_isLoading = true;
        if (!m_members.isEmpty()) {
            shouldEmitLocal = true;
        }
    }
    
    // 发送本地数据缓存更新（让UI先有数据）
    if (shouldEmitLocal) {
        emit dataChanged();
    }
    
    qDebug() << "[MEMBER] Requesting member list from server...";
    NetworkManager::instance().sendRequest(Protocol::CMD_GET_MEMBER_LIST, QJsonObject());
}

void MemberDataManager::onPacketReceived(const Protocol::NetPacket &packet)
{
    if (packet.cmdId == Protocol::CMD_GET_MEMBER_LIST) {
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        
        if (root["status"].toInt() == Protocol::STATUS_OK) {
            QJsonArray arr = root["data"].toArray();
            
            {
                QMutexLocker locker(&m_mutex);
                m_members.clear();
                for (int i = 0; i < arr.size(); ++i) {
                    QJsonObject obj = arr[i].toObject();
                    MemberInfo info;
                    // 格式化 ID：1 -> M00001
                    info.id = QString("M%1").arg(obj["member_id"].toVariant().toLongLong(), 5, 10, QChar('0'));
                    info.name = obj["name"].toString();
                    info.gender = obj["gender"].toString();
                    info.phone = obj["phone"].toString();
                    info.level = obj["level_name"].toString(); // 修正 Key 名
                    info.status = obj.contains("status") ? obj["status"].toString() : "正常"; // 容错处理
                    info.balance = obj["balance"].toDouble();
                    info.consume_amt = obj["consume_amt"].toDouble();
                    info.points = obj["points"].toInt();
                    info.birthday = obj["birthday"].toString();
                    info.pets = obj["pets"].toString();
                    info.imgData = obj["img_data"].toString();
                    info.isActive = (info.status == "正常");
                    
                    qDebug() << "[DEBUG] Member ID:" << info.id << "Name:" << info.name << "Gender:" << info.gender;
                    
                    m_members[info.id] = info;
                }
            }
            
            qDebug() << "[MEMBER] Member list updated. Count:" << m_members.size();
            m_isLoading = false;
            emit dataChanged();
        } else {
            QMutexLocker locker(&m_mutex);
            m_isLoading = false;
        }
    } else if (packet.cmdId == Protocol::CMD_NOTIFY_REFRESH) {
        QJsonDocument doc = QJsonDocument::fromJson(packet.data);
        QJsonObject root = doc.object();
        QString module = root["module"].toString();
        if (module == "member" || module.isEmpty()) {
            qDebug() << "[MEMBER] Received refresh broadcast, reloading member list...";
            // Prevent redundant loads if already loading
            if (!m_isLoading) {
                requestMemberList();
            }
        }
    }
}

void MemberDataManager::initMockData()
{
    auto addMock = [&](const QString &id, const QString &name, const QString &gender, const QString &birthday, const QString &phone, const QString &level, double balance, double consume_amt, int pts, const QString &pets = "无") {
        MemberInfo info;
        info.id = id;
        info.name = name;
        info.gender = gender;
        info.birthday = birthday;
        info.phone = phone;
        info.level = level;
        info.balance = balance;
        info.consume_amt = consume_amt;
        info.points = pts;
        info.isActive = true;
        info.status = "正常";
        info.pets = pets;
        m_members[id] = info;
    };

    /*
    addMock("M001", "张三", "男", "1990-05-20", "13800138000", "黄金会员", 500.00, 1250.00, 125, "团团（波斯猫）");
    addMock("M002", "李芳", "女", "1995-10-12", "13912345678", "普通会员", 0.00, 100.00, 10, "豆豆（柴犬）, 咪咪（银渐层）");
    ...
    addMock("M011", "林十三", "男", "1990-09-09", "13012123434", "黄金会员", 450.00, 1100.00, 110);
    */
    Q_UNUSED(addMock);
}

QList<MemberInfo> MemberDataManager::allMembers() const
{
    QMutexLocker locker(&m_mutex);
    return m_members.values();
}

QList<MemberInfo> MemberDataManager::activeMembers() const
{
    QMutexLocker locker(&m_mutex);
    QList<MemberInfo> active;
    for (const auto &info : m_members) {
        if (info.isActive) {
            active.append(info);
        }
    }
    return active;
}

MemberInfo MemberDataManager::getMember(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    return m_members.value(id);
}

void MemberDataManager::addMember(const MemberInfo &info)
{
    QJsonObject data;
    data["name"] = info.name;
    data["phone"] = info.phone;
    data["gender"] = info.gender;
    data["birthday"] = info.birthday;
    data["level"] = info.level;
    data["balance"] = info.balance;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_ADD_MEMBER, data);
}

void MemberDataManager::updateMember(const MemberInfo &info)
{
    QJsonObject data;
    // 解析 ID，M00001 -> 1
    int dbId = info.id.mid(1).toInt();
    data["member_id"] = dbId;
    data["name"] = info.name;
    data["phone"] = info.phone;
    data["gender"] = info.gender;
    data["birthday"] = info.birthday;
    data["level"] = info.level;
    data["balance"] = info.balance;
    
    NetworkManager::instance().sendRequest(Protocol::CMD_UPDATE_MEMBER, data);
}

void MemberDataManager::removeMember(const QString &id)
{
    QJsonObject data;
    data["member_id"] = id.mid(1).toInt();
    NetworkManager::instance().sendRequest(Protocol::CMD_DELETE_MEMBER, data);
}

void MemberDataManager::restoreMember(const QString &id)
{
    if (m_members.contains(id)) {
        m_members[id].isActive = true;
        m_members[id].status = "正常";
        emit dataChanged();
    }
}

void MemberDataManager::hardDeleteMember(const QString &id)
{
    if (m_members.remove(id) > 0) {
        emit dataChanged();
    }
}

void MemberDataManager::removePetFromMember(const QString &memberId, const QString &petName)
{
    if (m_members.contains(memberId)) {
        QString pets = m_members[memberId].pets;
        QStringList list = pets.split(", ", Qt::SkipEmptyParts);
        
        for (int i = 0; i < list.size(); ++i) {
            if (list[i].startsWith(petName)) {
                list.removeAt(i);
                break;
            }
        }
        
        m_members[memberId].pets = list.join(", ");
        emit dataChanged();
    }
}
QPixmap MemberDataManager::getMemberPixmap(const QString &id) const
{
    // L1: Memory
    if (m_pixmapCache.contains(id)) {
        return *m_pixmapCache.object(id);
    }

    // L2: Disk
    QString localPath = m_cachePath + "/" + id + ".png";
    if (QFile::exists(localPath)) {
        QPixmap pix(localPath);
        if (!pix.isNull()) {
            m_pixmapCache.insert(id, new QPixmap(pix));
            return pix;
        }
    }

    // L3: Data
    MemberInfo info = getMember(id);
    if (info.imgData.isEmpty()) return QPixmap();

    QPixmap pix;
    QByteArray ba = QByteArray::fromBase64(info.imgData.toUtf8());
    if (pix.loadFromData(ba)) {
        m_pixmapCache.insert(id, new QPixmap(pix));
        const_cast<MemberDataManager*>(this)->saveToLocalCache(id, pix);
    }
    return pix;
}

void MemberDataManager::ensureCacheDir()
{
    QDir dir(m_cachePath);
    if (!dir.exists()) dir.mkpath(".");
}

void MemberDataManager::saveToLocalCache(const QString &id, const QPixmap &pix)
{
    if (pix.isNull()) return;
    QString localPath = m_cachePath + "/" + id + ".png";
    if (!QFile::exists(localPath)) {
        pix.save(localPath, "PNG");
    }
}
